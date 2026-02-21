package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.MasaInfo;
import com.hindu.calendar.model.MasaName;

public class Masa {

    private final Ephemeris ephemeris;
    private final Tithi tithi;

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

        double[] x = new double[17];
        double[] y = new double[17];
        for (int i = 0; i < 17; i++) {
            x[i] = -2.0 + i * 0.25;
            y[i] = tithi.lunarPhase(start + x[i]);
        }
        unwrapAngles(y, 17);

        double y0 = inverseLagrange(x, y, 17, 360.0);
        return start + y0;
    }

    public double newMoonAfter(double jdUt, int tithiHint) {
        double start = jdUt + (30 - tithiHint);

        double[] x = new double[17];
        double[] y = new double[17];
        for (int i = 0; i < 17; i++) {
            x[i] = -2.0 + i * 0.25;
            y[i] = tithi.lunarPhase(start + x[i]);
        }
        unwrapAngles(y, 17);

        double y0 = inverseLagrange(x, y, 17, 360.0);
        return start + y0;
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
}
