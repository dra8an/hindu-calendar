package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.LunisolarScheme;
import com.hindu.calendar.model.MasaInfo;
import com.hindu.calendar.model.MasaName;

import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

public class Masa {

    private final Ephemeris ephemeris;
    private final Tithi tithi;

    // Simple cache for lunisolar month start/length
    private record CacheKey(MasaName masa, int sakaYear, boolean isAdhika, LunisolarScheme scheme) {}
    private record CacheEntry(double jdStart, int length) {}
    private final Map<CacheKey, CacheEntry> monthCache = new HashMap<>();

    public Masa(Ephemeris ephemeris, Tithi tithi) {
        this.ephemeris = ephemeris;
        this.tithi = tithi;
    }

    /**
     * Inverse Lagrange interpolation.
     * Given n data points (x[i], y[i]), find x value where y = ya.
     */
    private static double inverseLagrange(double[] x, double[] y, int n, double ya) {
        double total = 0.0;
        for (int i = 0; i < n; i++) {
            double numer = 1.0;
            double denom = 1.0;
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    numer *= (ya - y[j]);
                    denom *= (y[i] - y[j]);
                }
            }
            total += numer * x[i] / denom;
        }
        return total;
    }

    /** Add 360 to elements so they are monotonically increasing */
    private static void unwrapAngles(double[] angles, int n) {
        for (int i = 1; i < n; i++) {
            if (angles[i] < angles[i - 1])
                angles[i] += 360.0;
        }
    }

    public double newMoonBefore(double jdUt, int tithiHint) {
        double start = jdUt - tithiHint;

        double[] x = new double[9];
        double[] y = new double[9];
        for (int i = 0; i < 9; i++) {
            x[i] = -2.0 + i * 0.5;
            y[i] = tithi.lunarPhase(start + x[i]);
        }
        unwrapAngles(y, 9);

        double y0 = inverseLagrange(x, y, 9, 360.0);
        return start + y0;
    }

    public double newMoonAfter(double jdUt, int tithiHint) {
        double start = jdUt + (30 - tithiHint);

        double[] x = new double[9];
        double[] y = new double[9];
        for (int i = 0; i < 9; i++) {
            x[i] = -2.0 + i * 0.5;
            y[i] = tithi.lunarPhase(start + x[i]);
        }
        unwrapAngles(y, 9);

        double y0 = inverseLagrange(x, y, 9, 360.0);
        return start + y0;
    }

    public double fullMoonNear(double jdUt) {
        double[] x = new double[9];
        double[] y = new double[9];
        for (int i = 0; i < 9; i++) {
            x[i] = -2.0 + i * 0.5;
            y[i] = tithi.lunarPhase(jdUt + x[i]);
        }
        unwrapAngles(y, 9);

        double y0 = inverseLagrange(x, y, 9, 180.0);
        return jdUt + y0;
    }

    public int solarRashi(double jdUt) {
        double nirayana = ephemeris.solarLongitudeSidereal(jdUt);
        int rashi = (int) Math.ceil(nirayana / 30.0);
        if (rashi <= 0) rashi = 12;
        if (rashi > 12) rashi = rashi % 12;
        if (rashi == 0) rashi = 12;
        return rashi;
    }

    public MasaInfo masaForDate(int year, int month, int day, Location loc) {
        double jd = ephemeris.gregorianToJd(year, month, day);
        double jdRise = ephemeris.sunriseJd(jd, loc);
        if (jdRise <= 0) {
            jdRise = jd + 0.5 - loc.utcOffset() / 24.0;
        }

        int t = tithi.tithiAtMoment(jdRise);

        double lastNm = newMoonBefore(jdRise, t);
        double nextNm = newMoonAfter(jdRise, t);

        int rashiLast = solarRashi(lastNm);
        int rashiNext = solarRashi(nextNm);

        boolean isAdhika = (rashiLast == rashiNext);

        int masaNum = rashiLast + 1;
        if (masaNum > 12) masaNum -= 12;
        MasaName name = MasaName.fromNumber(masaNum);

        int yearSaka = hinduYearSaka(jdRise, masaNum);
        int yearVikram = hinduYearVikram(yearSaka);

        return new MasaInfo(name, isAdhika, yearSaka, yearVikram, lastNm, nextNm);
    }

    public int hinduYearSaka(double jdUt, int masaNum) {
        double siderealYear = 365.25636;
        double ahar = jdUt - 588465.5;
        int kali = (int) ((ahar + (4 - masaNum) * 30) / siderealYear);
        return kali - 3179;
    }

    public int hinduYearVikram(int sakaYear) {
        return sakaYear + 135;
    }

    // ===== Lunisolar month start/length =====

    private double amantaMonthStart(MasaName masa, int sakaYear, boolean isAdhika, Location loc) {
        // Step 1: Estimate approximate Gregorian date in the target month
        int gy = sakaYear + 78;
        int approxGm = masa.number() + 3;
        if (approxGm > 12) {
            approxGm -= 12;
            gy++;
        }

        // Start at the 15th of the estimated month
        int estY = gy, estM = approxGm, estD = 15;
        MasaInfo mi = masaForDate(estY, estM, estD, loc);

        // Step 2-3: Navigate using new moon boundaries
        for (int attempt = 0; attempt < 14; attempt++) {
            if (mi.name() == masa && mi.isAdhika() == isAdhika &&
                mi.yearSaka() == sakaYear) {
                break;
            }

            int isAdhikaInt = isAdhika ? 0 : 1;
            int miAdhikaInt = mi.isAdhika() ? 0 : 1;
            int targetOrd = sakaYear * 13 + masa.number() + isAdhikaInt;
            int curOrd = mi.yearSaka() * 13 + mi.name().number() + miAdhikaInt;

            double jdNav;
            if (targetOrd > curOrd) {
                jdNav = mi.jdEnd() + 1.0;
            } else {
                jdNav = mi.jdStart() - 1.0;
            }
            int[] ymd = ephemeris.jdToGregorian(jdNav);
            estY = ymd[0]; estM = ymd[1]; estD = ymd[2];
            mi = masaForDate(estY, estM, estD, loc);
        }

        if (mi.name() != masa || mi.isAdhika() != isAdhika ||
            mi.yearSaka() != sakaYear) {
            return 0;
        }

        // Step 4: Find the first civil day of this month
        int[] nmYmd = ephemeris.jdToGregorian(mi.jdStart());

        MasaInfo check = masaForDate(nmYmd[0], nmYmd[1], nmYmd[2], loc);
        if (check.name() == masa && check.isAdhika() == isAdhika &&
            check.yearSaka() == sakaYear) {
            return ephemeris.gregorianToJd(nmYmd[0], nmYmd[1], nmYmd[2]);
        }

        double jdNext = ephemeris.gregorianToJd(nmYmd[0], nmYmd[1], nmYmd[2]) + 1;
        int[] next = ephemeris.jdToGregorian(jdNext);
        check = masaForDate(next[0], next[1], next[2], loc);
        if (check.name() == masa && check.isAdhika() == isAdhika &&
            check.yearSaka() == sakaYear) {
            return jdNext;
        }

        jdNext += 1;
        next = ephemeris.jdToGregorian(jdNext);
        check = masaForDate(next[0], next[1], next[2], loc);
        if (check.name() == masa && check.isAdhika() == isAdhika &&
            check.yearSaka() == sakaYear) {
            return jdNext;
        }

        return 0;
    }

    public double lunisolarMonthStart(MasaName masa, int sakaYear, boolean isAdhika,
                                       LunisolarScheme scheme, Location loc) {
        // Check cache
        CacheKey key = new CacheKey(masa, sakaYear, isAdhika, scheme);
        CacheEntry cached = monthCache.get(key);
        if (cached != null && cached.jdStart > 0)
            return cached.jdStart;

        double result;

        if (scheme == LunisolarScheme.PURNIMANTA) {
            double amantaStart = amantaMonthStart(masa, sakaYear, isAdhika, loc);
            if (amantaStart == 0) return 0;

            // Get the actual new moon JD
            double jdRise = ephemeris.sunriseJd(amantaStart, loc);
            if (jdRise <= 0) jdRise = amantaStart + 0.5 - loc.utcOffset() / 24.0;
            MasaInfo mi = masaForDate(
                    ephemeris.jdToGregorian(amantaStart)[0],
                    ephemeris.jdToGregorian(amantaStart)[1],
                    ephemeris.jdToGregorian(amantaStart)[2], loc);

            // Find full moon ~15 days before this new moon
            double jdFull = fullMoonNear(mi.jdStart() - 15.0);

            // Find first civil day on/after full moon with Krishna paksha tithi
            int[] fmYmd = ephemeris.jdToGregorian(jdFull);
            for (int offset = 0; offset <= 2; offset++) {
                double jdTry = ephemeris.gregorianToJd(fmYmd[0], fmYmd[1], fmYmd[2]) + offset;
                int[] tryYmd = ephemeris.jdToGregorian(jdTry);
                double jr = ephemeris.sunriseJd(jdTry, loc);
                if (jr <= 0) jr = jdTry + 0.5 - loc.utcOffset() / 24.0;
                int t = tithi.tithiAtMoment(jr);
                if (t >= 16) {
                    result = jdTry;
                    monthCache.put(key, new CacheEntry(result, 0));
                    return result;
                }
            }
            return 0;
        } else {
            result = amantaMonthStart(masa, sakaYear, isAdhika, loc);
            if (result == 0) return 0;
        }

        monthCache.put(key, new CacheEntry(result, 0));
        return result;
    }

    public int lunisolarMonthLength(MasaName masa, int sakaYear, boolean isAdhika,
                                     LunisolarScheme scheme, Location loc) {
        // Check cache
        CacheKey key = new CacheKey(masa, sakaYear, isAdhika, scheme);
        CacheEntry cached = monthCache.get(key);
        if (cached != null && cached.length > 0)
            return cached.length;

        double jdStart = lunisolarMonthStart(masa, sakaYear, isAdhika, scheme, loc);
        if (jdStart == 0) return 0;

        int length = 0;

        if (scheme == LunisolarScheme.PURNIMANTA) {
            MasaName nextMasa = (masa == MasaName.PHALGUNA) ? MasaName.CHAITRA :
                    MasaName.fromNumber(masa.number() + 1);
            int nextSaka = (masa == MasaName.PHALGUNA) ? sakaYear + 1 : sakaYear;
            boolean nextAdhika = false;
            if (isAdhika) {
                nextMasa = masa;
                nextSaka = sakaYear;
            }
            double jdNext = lunisolarMonthStart(nextMasa, nextSaka, nextAdhika, scheme, loc);
            if (jdNext > 0)
                length = (int) (jdNext - jdStart);
        } else {
            // Amanta: scan days 28-31 from start
            for (int d = 28; d <= 31; d++) {
                double jd = jdStart + d;
                int[] ymd = ephemeris.jdToGregorian(jd);
                MasaInfo mi = masaForDate(ymd[0], ymd[1], ymd[2], loc);
                if (mi.name() != masa || mi.isAdhika() != isAdhika) {
                    length = d;
                    break;
                }
            }
        }

        // Store length in cache
        if (length > 0) {
            CacheEntry existing = monthCache.get(key);
            if (existing != null) {
                monthCache.put(key, new CacheEntry(existing.jdStart, length));
            }
        }

        return length;
    }
}
