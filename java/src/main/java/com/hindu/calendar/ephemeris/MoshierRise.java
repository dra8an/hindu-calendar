package com.hindu.calendar.ephemeris;

/**
 * Sunrise and sunset calculation.
 *
 * Algorithm from Meeus, "Astronomical Algorithms", 2nd ed., Ch. 15.
 * Iterative method using hour angle computation.
 *
 * Configuration: disc center with atmospheric refraction
 *   h0 = Sinclair refraction at horizon (~-0.612 deg)
 *   Sidereal time = GAST (apparent, with equation of equinoxes)
 *
 * Precision: ~2 seconds (sufficient for Hindu calendar)
 */
class MoshierRise {

    private static final double DEG2RAD = Math.PI / 180.0;
    private static final double RAD2DEG = 180.0 / Math.PI;

    private final MoshierSun sun;

    MoshierRise(MoshierSun sun) {
        this.sun = sun;
    }

    private static double normalizeDeg(double d) {
        d = d % 360.0;
        if (d < 0) d += 360.0;
        return d;
    }

    /** Sinclair refraction at apparent altitude 0 (horizon), in degrees */
    private static double sinclairRefractionHorizon(double atpress, double attemp) {
        double r = 34.46; // arcminutes at horizon
        r = ((atpress - 80.0) / 930.0 / (1.0 + 0.00008 * (r + 39.0) * (attemp - 10.0)) * r) / 60.0;
        return r;
    }

    /** Mean sidereal time at Greenwich at 0h UT, in degrees */
    private static double siderealTime0h(double jd0h) {
        double T = (jd0h - 2451545.0) / 36525.0;
        double T2 = T * T;
        double T3 = T2 * T;
        double theta = 100.46061837 + 36000.770053608 * T + 0.000387933 * T2 - T3 / 38710000.0;
        return normalizeDeg(theta);
    }

    /**
     * Compute rise or set for a specific UT date.
     * Returns JD (UT) of the event, or 0 on error (circumpolar).
     */
    private double riseSetForDate(double jd0h, double lon, double lat, double h0, boolean isRise) {
        double phi = lat * DEG2RAD;

        // Apparent sidereal time at 0h UT (GAST = GMST + eq. equinoxes)
        double theta0 = siderealTime0h(jd0h);
        double jdNoon = jd0h + 0.5;
        double dpsi = sun.nutationLongitude(jdNoon);
        double eps = sun.meanObliquityUt(jdNoon);
        theta0 += dpsi * Math.cos(eps * DEG2RAD);

        // Initial estimate using noon position
        double ra = sun.solarRa(jdNoon);
        double decl = sun.solarDeclination(jdNoon);

        // Hour angle (Meeus eq. 15.1)
        double cosH0 = (Math.sin(h0 * DEG2RAD) - Math.sin(phi) * Math.sin(decl * DEG2RAD))
                / (Math.cos(phi) * Math.cos(decl * DEG2RAD));

        if (cosH0 < -1.0 || cosH0 > 1.0) {
            return 0.0; // circumpolar
        }

        double H0deg = Math.acos(cosH0) * RAD2DEG;

        // Approximate transit time
        double m0 = (ra - lon - theta0) / 360.0;
        m0 = m0 - Math.floor(m0);

        double m;
        if (isRise)
            m = m0 - H0deg / 360.0;
        else
            m = m0 + H0deg / 360.0;

        m = m - Math.floor(m);

        // Iterate to refine
        for (int iter = 0; iter < 10; iter++) {
            double jdTrial = jd0h + m;

            double raI = sun.solarRa(jdTrial);
            double declI = sun.solarDeclination(jdTrial);

            double theta = theta0 + 360.985647 * m;

            double H = normalizeDeg(theta + lon - raI);
            if (H > 180.0) H -= 360.0;

            double sinH = Math.sin(phi) * Math.sin(declI * DEG2RAD)
                    + Math.cos(phi) * Math.cos(declI * DEG2RAD) * Math.cos(H * DEG2RAD);
            double h = Math.asin(sinH) * RAD2DEG;

            double denom = 360.0 * Math.cos(declI * DEG2RAD) * Math.cos(phi) * Math.sin(H * DEG2RAD);
            if (Math.abs(denom) < 1e-12) break;
            double dm = (h - h0) / denom;
            m += dm;

            if (Math.abs(dm) < 0.0000001) break; // ~0.009 seconds
        }

        // Midnight UT wrap-around fix
        if (isRise && m > 0.75) m -= 1.0;
        if (!isRise && m < 0.25) m += 1.0;

        return jd0h + m;
    }

    /**
     * Compute rise or set, searching forward from jdUt.
     * jdUt should be at 0h UT of the local date (approximately).
     */
    private double riseSet(double jdUt, double lon, double lat, double alt, boolean isRise) {
        double atpress = 1013.25;
        if (alt > 0)
            atpress = 1013.25 * Math.pow(1.0 - 0.0065 * alt / 288.0, 5.255);
        double h0 = -sinclairRefractionHorizon(atpress, 0.0);
        if (alt > 0)
            h0 -= 0.0353 * Math.sqrt(alt);

        int[] ymd = MoshierJulianDay.revjul(jdUt);
        double jd0h = MoshierJulianDay.julday(ymd[0], ymd[1], ymd[2], 0.0);

        double result = riseSetForDate(jd0h, lon, lat, h0, isRise);
        if (result > 0 && result >= jdUt - 0.0001) {
            return result;
        }

        result = riseSetForDate(jd0h + 1.0, lon, lat, h0, isRise);
        return result;
    }

    double sunrise(double jdUt, double lon, double lat, double alt) {
        return riseSet(jdUt, lon, lat, alt, true);
    }

    double sunset(double jdUt, double lon, double lat, double alt) {
        return riseSet(jdUt, lon, lat, alt, false);
    }
}
