package com.hindu.calendar.ephemeris;

/**
 * Lahiri (Chitrapaksha) ayanamsa using IAU 1976 precession.
 *
 * Algorithm:
 *   1. Take the vernal point at the target date as Cartesian (1, 0, 0)
 *   2. Precess from target date -> J2000 using current precession model
 *   3. Precess from J2000 -> t0 (reference epoch) using IAU 1976 model
 *   4. Convert to ecliptic of t0, then to polar coordinates
 *   5. Ayanamsa = -longitude + ayan_t0
 */
class MoshierAyanamsa {

    private static final double DEG2RAD = Math.PI / 180.0;
    private static final double RAD2DEG = 180.0 / Math.PI;
    private static final double J2000 = 2451545.0;

    private static final double LAHIRI_T0 = 2435553.5;
    private static final double LAHIRI_AYAN_T0 = 23.245524743;

    private final MoshierSun sun;

    MoshierAyanamsa(MoshierSun sun) {
        this.sun = sun;
    }

    /** IAU 1976 precession angles Z, z, theta in radians */
    private static double[] iau1976PrecessionAngles(double T) {
        double Z = ((0.017998 * T + 0.30188) * T + 2306.2181) * T * DEG2RAD / 3600.0;
        double z = ((0.018203 * T + 1.09468) * T + 2306.2181) * T * DEG2RAD / 3600.0;
        double theta = ((-0.041833 * T - 0.42665) * T + 2004.3109) * T * DEG2RAD / 3600.0;
        return new double[]{Z, z, theta};
    }

    /**
     * Precess Cartesian equatorial coordinates.
     * direction = +1: from J to J2000 (forward)
     * direction = -1: from J2000 to J (backward)
     */
    private static void precessEquatorial(double[] x, double J, int direction) {
        if (J == J2000) return;

        double T = (J - J2000) / 36525.0;
        double[] angles = iau1976PrecessionAngles(T);
        double Z = angles[0], z = angles[1], theta = angles[2];

        double costh = Math.cos(theta), sinth = Math.sin(theta);
        double cosZ = Math.cos(Z), sinZ = Math.sin(Z);
        double cosz = Math.cos(z), sinz = Math.sin(z);
        double A = cosZ * costh;
        double B = sinZ * costh;

        double[] r = new double[3];
        if (direction > 0) {
            r[0] = (A * cosz - sinZ * sinz) * x[0] + (A * sinz + sinZ * cosz) * x[1] + cosZ * sinth * x[2];
            r[1] = -(B * cosz + cosZ * sinz) * x[0] - (B * sinz - cosZ * cosz) * x[1] - sinZ * sinth * x[2];
            r[2] = -sinth * cosz * x[0] - sinth * sinz * x[1] + costh * x[2];
        } else {
            r[0] = (A * cosz - sinZ * sinz) * x[0] - (B * cosz + cosZ * sinz) * x[1] - sinth * cosz * x[2];
            r[1] = (A * sinz + sinZ * cosz) * x[0] - (B * sinz - cosZ * cosz) * x[1] - sinth * sinz * x[2];
            r[2] = cosZ * sinth * x[0] - sinZ * sinth * x[1] + costh * x[2];
        }
        x[0] = r[0];
        x[1] = r[1];
        x[2] = r[2];
    }

    /** IAU 1976 obliquity of ecliptic at JD (TT), in radians */
    private static double obliquityIau1976(double jdTt) {
        double T = (jdTt - J2000) / 36525.0;
        double U = T / 100.0;
        double eps = 23.0 + 26.0 / 60.0 + 21.448 / 3600.0
                + (-4680.93 * U - 1.55 * U * U + 1999.25 * U * U * U
                - 51.38 * U * U * U * U - 249.67 * U * U * U * U * U
                - 39.05 * U * U * U * U * U * U + 7.12 * U * U * U * U * U * U * U
                + 27.87 * U * U * U * U * U * U * U * U + 5.79 * U * U * U * U * U * U * U * U * U
                + 2.45 * U * U * U * U * U * U * U * U * U * U) / 3600.0;
        return eps * DEG2RAD;
    }

    /** Rotate equatorial -> ecliptic */
    private static void equatorialToEcliptic(double[] x, double eps) {
        double c = Math.cos(eps), s = Math.sin(eps);
        double y1 = c * x[1] + s * x[2];
        double z1 = -s * x[1] + c * x[2];
        x[1] = y1;
        x[2] = z1;
    }

    /** Lahiri ayanamsa in degrees (MEAN, without nutation) */
    double ayanamsa(double jdUt) {
        double jdTt = jdUt + sun.deltaT(jdUt);

        // Step 1: Vernal point at target date
        double[] x = {1.0, 0.0, 0.0};

        // Step 2: Precess from target date to J2000
        precessEquatorial(x, jdTt, +1);

        // Step 3: Precess from J2000 to t0
        precessEquatorial(x, LAHIRI_T0, -1);

        // Step 4: Convert to ecliptic of t0
        double epsT0 = obliquityIau1976(LAHIRI_T0);
        equatorialToEcliptic(x, epsT0);

        // Step 5: Get polar longitude
        double lon = Math.atan2(x[1], x[0]) * RAD2DEG;

        // Step 6: Ayanamsa = -longitude + initial value
        double ayan = -lon + LAHIRI_AYAN_T0;

        // Normalize to [0, 360)
        ayan = ayan % 360.0;
        if (ayan < 0) ayan += 360.0;

        return ayan;
    }
}
