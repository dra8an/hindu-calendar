package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.SolarCalendarType;
import com.hindu.calendar.model.SolarDate;
import com.hindu.calendar.model.TithiInfo;

public class Solar {

    private final Ephemeris ephemeris;
    private final Tithi tithi;

    public Solar(Ephemeris ephemeris) {
        this.ephemeris = ephemeris;
        this.tithi = new Tithi(ephemeris);
    }

    // ===== Critical time computation =====

    private double criticalTimeJd(double jdMidnightUt, Location loc, SolarCalendarType type) {
        return switch (type) {
            case TAMIL ->
                    ephemeris.sunsetJd(jdMidnightUt, loc) - 8.0 / (24.0 * 60.0);
            case BENGALI ->
                    jdMidnightUt - loc.utcOffset() / 24.0 + 24.0 / (24.0 * 60.0);
            case ODIA ->
                    jdMidnightUt + 16.7 / 24.0;
            case MALAYALAM -> {
                double sr = ephemeris.sunriseJd(jdMidnightUt, loc);
                double ss = ephemeris.sunsetJd(jdMidnightUt, loc);
                yield sr + 0.6 * (ss - sr) - 9.5 / (24.0 * 60.0);
            }
        };
    }

    // ===== Sankranti finding =====

    public double sankrantiJd(double jdApprox, double targetLongitude) {
        double lo = jdApprox - 20.0;
        double hi = jdApprox + 20.0;

        double lonLo = ephemeris.solarLongitudeSidereal(lo);
        double diffLo = lonLo - targetLongitude;
        if (diffLo > 180.0) diffLo -= 360.0;
        if (diffLo < -180.0) diffLo += 360.0;

        if (diffLo >= 0) lo -= 30.0;

        for (int i = 0; i < 50; i++) {
            double mid = (lo + hi) / 2.0;
            double lon = ephemeris.solarLongitudeSidereal(mid);

            double diff = lon - targetLongitude;
            if (diff > 180.0) diff -= 360.0;
            if (diff < -180.0) diff += 360.0;

            if (diff >= 0)
                hi = mid;
            else
                lo = mid;
        }

        return (lo + hi) / 2.0;
    }

    public double sankrantiBefore(double jdUt) {
        double lon = ephemeris.solarLongitudeSidereal(jdUt);
        int rashi = (int) Math.floor(lon / 30.0) + 1;
        if (rashi > 12) rashi = 12;
        if (rashi < 1) rashi = 1;

        double target = (rashi - 1) * 30.0;
        double degreesPast = lon - target;
        if (degreesPast < 0) degreesPast += 360.0;
        double jdEst = jdUt - degreesPast;

        return sankrantiJd(jdEst, target);
    }

    // ===== Sankranti to civil day =====

    private int[] sankrantiToCivilDay(double jdSankranti, Location loc,
                                      SolarCalendarType type, int rashi) {
        double localJd = jdSankranti + loc.utcOffset() / 24.0 + 0.5;
        int[] ymd = ephemeris.jdToGregorian(Math.floor(localJd));
        int sy = ymd[0], sm = ymd[1], sd = ymd[2];

        double jdDay = ephemeris.gregorianToJd(sy, sm, sd);
        double crit = criticalTimeJd(jdDay, loc, type);

        if (jdSankranti <= crit) {
            // Bengali tithi-based override
            if (type == SolarCalendarType.BENGALI && rashi != 4) {
                boolean pushNext = false;
                if (rashi == 10) {
                    pushNext = true;
                } else {
                    int[] prevYmd = ephemeris.jdToGregorian(jdDay - 1.0);
                    TithiInfo ti = tithi.tithiAtSunrise(prevYmd[0], prevYmd[1], prevYmd[2], loc);
                    pushNext = (ti.jdEnd() <= jdSankranti);
                }
                if (pushNext) {
                    return ephemeris.jdToGregorian(jdDay + 1.0);
                }
            }
            return new int[]{sy, sm, sd};
        } else {
            return ephemeris.jdToGregorian(jdDay + 1.0);
        }
    }

    // ===== Rashi to regional month number =====

    private static int rashiToRegionalMonth(int rashi, SolarCalendarType type) {
        int m = rashi - type.firstRashi() + 1;
        if (m <= 0) m += 12;
        return m;
    }

    // ===== Solar year =====

    private int solarYear(double jdUt, Location loc, double jdGregDate, SolarCalendarType type) {
        int[] ymd = ephemeris.jdToGregorian(jdUt);
        int gy = ymd[0];

        double targetLong = (double) (type.firstRashi() - 1) * 30.0;
        int approxGregMonth = 3 + type.firstRashi();
        if (approxGregMonth > 12) approxGregMonth -= 12;

        double jdYearStartEst = ephemeris.gregorianToJd(gy, approxGregMonth, 14);
        double jdYearStart = sankrantiJd(jdYearStartEst, targetLong);

        int[] ysYmd = sankrantiToCivilDay(jdYearStart, loc, type, type.firstRashi());
        double jdYearCivil = ephemeris.gregorianToJd(ysYmd[0], ysYmd[1], ysYmd[2]);

        if (jdGregDate >= jdYearCivil) {
            return gy - type.gyOffsetOn();
        } else {
            return gy - type.gyOffsetBefore();
        }
    }

    // ===== Public API =====

    public SolarDate gregorianToSolar(int year, int month, int day,
                                       Location loc, SolarCalendarType type) {
        double jd = ephemeris.gregorianToJd(year, month, day);
        double jdCrit = criticalTimeJd(jd, loc, type);

        double lon = ephemeris.solarLongitudeSidereal(jdCrit);

        int rashi = (int) Math.floor(lon / 30.0) + 1;
        if (rashi > 12) rashi = 12;
        if (rashi < 1) rashi = 1;

        double target = (rashi - 1) * 30.0;
        double degreesPast = lon - target;
        if (degreesPast < 0) degreesPast += 360.0;
        double jdEst = jdCrit - degreesPast;
        double jdSankranti = sankrantiJd(jdEst, target);

        int[] civilDay = sankrantiToCivilDay(jdSankranti, loc, type, rashi);
        double jdMonthStart = ephemeris.gregorianToJd(civilDay[0], civilDay[1], civilDay[2]);
        int solarDay = (int) (jd - jdMonthStart) + 1;

        // Bengali tithi-based rule may push month start past our date
        if (solarDay <= 0) {
            rashi = (rashi == 1) ? 12 : rashi - 1;
            double prevTarget = (double) (rashi - 1) * 30.0;
            jdSankranti = sankrantiJd(jdSankranti - 28.0, prevTarget);
            civilDay = sankrantiToCivilDay(jdSankranti, loc, type, rashi);
            jdMonthStart = ephemeris.gregorianToJd(civilDay[0], civilDay[1], civilDay[2]);
            solarDay = (int) (jd - jdMonthStart) + 1;
        }

        int regionalMonth = rashiToRegionalMonth(rashi, type);
        int solarYr = solarYear(jdCrit, loc, jd, type);

        return new SolarDate(solarYr, regionalMonth, solarDay, rashi, jdSankranti);
    }

    public int[] solarToGregorian(SolarDate sd, SolarCalendarType type, Location loc) {
        int rashi = sd.month() + type.firstRashi() - 1;
        if (rashi > 12) rashi -= 12;

        int gy = sd.year() + type.gyOffsetOn();

        int rashiGregMonth = 3 + rashi;
        int startGregMonth = 3 + type.firstRashi();

        if (rashiGregMonth > 12 && startGregMonth <= 12) {
            gy++;
        } else if (rashiGregMonth <= 12 && startGregMonth <= 12 &&
                rashiGregMonth < startGregMonth) {
            gy++;
        }

        double targetLong = (rashi - 1) * 30.0;
        int estMonth = rashiGregMonth;
        if (estMonth > 12) estMonth -= 12;
        double jdEst = ephemeris.gregorianToJd(gy, estMonth, 14);
        double jdSank = sankrantiJd(jdEst, targetLong);

        int[] civilDay = sankrantiToCivilDay(jdSank, loc, type, rashi);
        double jdResult = ephemeris.gregorianToJd(civilDay[0], civilDay[1], civilDay[2]) + (sd.day() - 1);
        return ephemeris.jdToGregorian(jdResult);
    }

    public static final String[] RASHI_NAMES = {
            "", "Mesha", "Vrishabha", "Mithuna", "Karka", "Simha", "Kanya",
            "Tula", "Vrishchika", "Dhanu", "Makara", "Kumbha", "Meena"
    };
}
