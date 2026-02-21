package com.hindu.calendar.ephemeris;

/**
 * Solar longitude and declination using VSOP87 planetary theory.
 *
 * Pipeline: VSOP87 EMB J2000 -> precession -> EMB->Earth -> geocentric -> nutation -> aberration
 *
 * Nutation: IAU 1980, 13 terms (Meeus Ch. 22, Table 22.A)
 * Obliquity: Laskar's formula (Meeus Ch. 22)
 *
 * Precision: ~1 arcsecond for solar longitude (1900-2100)
 */
class MoshierSun {

    private static final double DEG2RAD = Math.PI / 180.0;
    private static final double RAD2DEG = 180.0 / Math.PI;
    private static final double STR = 4.8481368110953599359e-6; // radians per arcsecond
    private static final double TIMESCALE = 3652500.0; // 10000 Julian years in days
    private static final double J2000_JD = 2451545.0;
    private static final double J1900_JD = 2415020.0;
    private static final double EARTH_MOON_MRAT = 1.0 / 0.0123000383;

    // VSOP87 sin/cos lookup tables
    private final double[][] ssTbl = new double[9][24];
    private final double[][] ccTbl = new double[9][24];

    private static double mods3600(double x) {
        return x - 1.296e6 * Math.floor(x / 1.296e6);
    }

    private static double normalizeDeg(double d) {
        d = d % 360.0;
        if (d < 0) d += 360.0;
        return d;
    }

    // ===== VSOP87 data tables for Earth =====

    private static final double[] FREQS = {
            53810162868.8982, 21066413643.3548, 12959774228.3429,
            6890507749.3988, 1092566037.7991, 439960985.5372,
            154248119.3933, 78655032.0744, 52272245.1795
    };

    private static final double[] PHASES = {
            252.25090552 * 3600.0, 181.97980085 * 3600.0, 100.46645683 * 3600.0,
            355.43299958 * 3600.0, 34.35151874 * 3600.0, 50.07744430 * 3600.0,
            314.05500511 * 3600.0, 304.34866548 * 3600.0, 860492.1546
    };

    private static final int[] EAR_MAX_HARMONIC = {1, 9, 14, 17, 5, 5, 2, 1, 0};

    private static final double[] EARTABL = {
            -65.54655, -232.74963, 12959774227.57587, 361678.59587,
            2.52679, -4.93511, 2.46852, -8.88928,
            6.66257, -1.94502, 0.66887, -0.06141,
            0.08893, 0.18971, 0.00068, -0.00307,
            0.03092, 0.03214, -0.14321, 0.22548,
            0.00314, -0.00221, 8.98017, 7.25747,
            -1.06655, 1.19671, -2.42276, 0.29621,
            1.55635, 0.99167, -0.00026, 0.00187,
            0.00189, 0.02742, 0.00158, 0.01475,
            0.00353, -0.02048, -0.01775, -0.01023,
            0.01927, -0.03122, -1.55440, -4.97423,
            2.14765, -2.77045, 1.02707, 0.55507,
            -0.08066, 0.18479, 0.00750, 0.00583,
            -0.16977, 0.35555, 0.32036, 0.01309,
            0.54625, 0.08167, 0.10681, 0.17231,
            -0.02287, 0.01631, -0.00866, -0.00190,
            0.00016, -0.01514, -0.00073, 0.04205,
            -0.00072, 0.01490, -0.38831, 0.41043,
            -1.11857, -0.84329, 1.15123, -1.34167,
            0.01026, -0.00432, -0.02833, -0.00705,
            -0.00285, 0.01645, -0.01234, 0.05609,
            -0.01893, -0.00171, -0.30527, 0.45390,
            0.56713, 0.70030, 1.27125, -0.76481,
            0.34857, -2.60318, -0.00160, 0.00643,
            0.28492, -0.37998, 0.23347, 0.00540,
            0.00342, 0.04406, 0.00037, -0.02449,
            0.01469, 1.59358, 0.24956, 0.71066,
            0.25477, -0.98371, -0.69412, 0.19687,
            -0.44423, -0.83331, 0.49647, -0.31021,
            0.05696, -0.00802, -0.14423, -0.04719,
            0.16762, -0.01234, 0.02481, 0.03465,
            0.01091, 0.02123, 0.08212, -0.07375,
            0.01524, -0.07388, 0.06673, -0.22486,
            0.10026, -0.00559, 0.14711, -0.11680,
            0.05460, 0.02749, -1.04467, 0.34273,
            -0.67582, -2.15117, 2.47372, -0.04332,
            0.05016, -0.03991, 0.01908, 0.00943,
            0.07321, -0.23637, 0.10564, -0.00446,
            -0.09523, -0.30710, 0.17400, -0.10681,
            0.05104, -0.14078, 0.01390, 0.07288,
            -0.26308, -0.20717, 0.20773, -0.37096,
            -0.00205, -0.27274, -0.00792, -0.00183,
            0.02985, 0.04895, 0.03785, -0.14731,
            0.02976, -0.02495, -0.02644, -0.04085,
            -0.00843, 0.00027, 0.00090, 0.00611,
            0.00040, 4.83425, 0.01692, -0.01335,
            0.04482, -0.03602, 0.01672, 0.00838,
            0.03682, -0.11206, 0.05163, -0.00219,
            -0.08381, -0.20911, 0.16400, -0.13325,
            -0.05945, 0.02114, -0.00710, -0.04695,
            -0.01657, -0.00513, -0.06999, -0.23054,
            0.13128, -0.07975, 0.00054, -0.00699,
            -0.01253, -0.04007, 0.00658, -0.00607,
            -0.48696, 0.31859, -0.84292, -0.87950,
            1.30507, -0.94042, -0.00234, 0.00339,
            -0.30647, -0.24605, 0.24948, -0.43369,
            -0.64033, 0.20754, -0.43829, -1.31801,
            1.55412, -0.02893, -0.02323, 0.02181,
            -0.00398, -0.01548, -0.08005, -0.01537,
            -0.00362, -0.02033, 0.00028, -0.03732,
            -0.14083, -7.21175, -0.07430, 0.01886,
            -0.00223, 0.01915, -0.02270, -0.03702,
            0.10167, -0.02917, 0.00879, -2.04198,
            -0.00433, -0.41764, 0.00671, -0.00030,
            0.00070, -0.01066, 0.01144, -0.03190,
            -0.29653, 0.38638, -0.16611, -0.07661,
            0.22071, 0.14665, 0.02487, 0.13524,
            -275.60942, -335.52251, -413.89009, 359.65390,
            1396.49813, 1118.56095, 2559.41622, -3393.39088,
            -6717.66079, -1543.17403, -1.90405, -0.22958,
            -0.57989, -0.36584, -0.04547, -0.14164,
            0.00749, -0.03973, 0.00033, 0.01842,
            -0.08301, -0.03523, -0.00408, -0.02008,
            0.00008, 0.00778, -0.00046, 0.02760,
            -0.03135, 0.07710, 0.06130, 0.04003,
            -0.04703, 0.00671, -0.00754, -0.01000,
            -0.01902, -0.00125, -0.00264, -0.00903,
            -0.02672, 0.12765, -0.03872, 0.03532,
            -0.01534, -0.00710, -0.01087, 0.01124,
            -0.01664, 0.06304, -0.02779, 0.00214,
            -0.01279, -5.51814, 0.05847, -0.02093,
            0.03950, 0.06696, -0.04064, 0.02687,
            0.01478, -0.02169, 0.05821, 0.03301,
            -0.03861, 0.07535, 0.00290, -0.00644,
            0.00631, 0.12905, 0.02400, 0.13194,
            -0.14339, 0.00529, 0.00343, 0.00819,
            0.02692, -0.03332, -0.07284, -0.02064,
            0.07038, 0.03999, 0.02759, 0.07599,
            0.00033, 0.00641, 0.00128, 0.02032,
            -0.00852, 0.00680, 0.23019, 0.17100,
            0.09861, 0.55013, -0.00192, 0.00953,
            -0.00943, 0.01783, 0.05975, 0.01486,
            0.00160, 0.01558, -0.01629, -0.02035,
            0.01533, 2.73176, 0.05858, -0.01327,
            0.00209, -0.01506, 0.00755, 0.03300,
            -0.00796, -0.65270, 0.02305, 0.00165,
            -0.02512, 0.06560, 0.16108, -0.02087,
            0.00016, 0.10729, 0.04175, 0.00559,
            0.01176, 0.00110, 15.15730, -0.52460,
            -37.16535, -25.85564, -60.94577, 4.29961,
            57.11617, 67.96463, 31.41414, -64.75731,
            0.00848, 0.02971, -0.03690, -0.00010,
            -0.03568, 0.06325, 0.11311, 0.02431,
            -0.00383, 0.00421, -0.00140, 0.00680,
            0.00069, -0.21036, 0.00386, 0.04210,
            -0.01324, 0.16454, -0.01398, -0.00109,
            0.02548, -0.03842, -0.06504, -0.02204,
            0.01359, 0.00232, 0.07634, -1.64648,
            -1.73103, 0.89176, 0.81398, 0.65209,
            0.00021, -0.08441, -0.00012, 0.01262,
            -0.00666, -0.00050, -0.00130, 0.01596,
            -0.00485, -0.00213, 0.00009, -0.03941,
            -0.02266, -0.04421, -0.01341, 0.01083,
            -0.00011, 0.00004, 0.00003, -0.02017,
            0.00003, -0.01096, 0.00002, -0.00623,
    };

    private static final byte[] EARARGS = {
            0, 3,
            3, 4, 3, -8, 4, 3, 5, 2,
            2, 2, 5, -5, 6, 1,
            3, 2, 2, 1, 3, -8, 4, 0,
            3, 3, 2, -7, 3, 4, 4, 1,
            3, 7, 3, -13, 4, -1, 5, 0,
            2, 8, 2, -13, 3, 3,
            3, 1, 2, -8, 3, 12, 4, 0,
            1, 1, 8, 0,
            1, 1, 7, 0,
            2, 1, 5, -2, 6, 0,
            3, 3, 3, -6, 4, 2, 5, 1,
            2, 8, 3, -15, 4, 3,
            2, 2, 5, -4, 6, 0,
            1, 1, 6, 1,
            2, 9, 3, -17, 4, 2,
            3, 3, 2, -5, 3, 1, 5, 0,
            3, 2, 3, -4, 4, 2, 5, 0,
            3, 3, 2, -5, 3, 2, 5, 0,
            2, 1, 5, -1, 6, 0,
            2, 1, 3, -2, 4, 2,
            2, 2, 5, -3, 6, 0,
            1, 2, 6, 1,
            2, 3, 5, -5, 6, 1,
            1, 1, 5, 3,
            2, 1, 5, -5, 6, 0,
            2, 7, 3, -13, 4, 2,
            2, 2, 5, -2, 6, 0,
            2, 3, 2, -5, 3, 2,
            2, 2, 3, -4, 4, 2,
            2, 5, 2, -8, 3, 1,
            2, 6, 3, -11, 4, 1,
            2, 1, 1, -4, 3, 0,
            1, 2, 5, 1,
            2, 3, 3, -6, 4, 1,
            2, 5, 3, -9, 4, 1,
            2, 2, 2, -3, 3, 2,
            2, 4, 3, -8, 4, 1,
            2, 4, 3, -7, 4, 1,
            2, 3, 3, -5, 4, 1,
            2, 1, 2, -2, 3, 1,
            2, 2, 3, -3, 4, 1,
            2, 1, 3, -1, 4, 0,
            2, 4, 2, -7, 3, 0,
            2, 4, 2, -6, 3, 1,
            1, 1, 4, 1,
            2, 1, 3, -3, 4, 0,
            2, 7, 3, -12, 4, 0,
            2, 1, 2, -1, 3, 0,
            2, 1, 3, -4, 5, 0,
            2, 6, 3, -10, 4, 1,
            2, 5, 3, -8, 4, 1,
            2, 1, 3, -3, 5, 1,
            2, 2, 2, -4, 3, 1,
            2, 6, 2, -9, 3, 0,
            2, 4, 3, -6, 4, 1,
            3, 1, 3, -3, 5, 2, 6, 0,
            2, 1, 3, -5, 6, 1,
            2, 1, 3, -2, 5, 2,
            3, 1, 3, -4, 5, 5, 6, 0,
            2, 3, 3, -4, 4, 1,
            2, 3, 2, -4, 3, 2,
            2, 1, 3, -3, 6, 1,
            3, 1, 3, 1, 5, -5, 6, 1,
            2, 1, 3, -1, 5, 1,
            3, 1, 3, -3, 5, 5, 6, 1,
            2, 1, 3, -2, 6, 1,
            2, 2, 3, -2, 4, 0,
            2, 1, 3, -1, 6, 0,
            2, 1, 3, -2, 7, 0,
            2, 1, 3, -1, 7, 0,
            2, 8, 2, -14, 3, 0,
            3, 1, 3, 2, 5, -5, 6, 1,
            3, 5, 3, -8, 4, 3, 5, 1,
            1, 1, 3, 4,
            3, 3, 3, -8, 4, 3, 5, 2,
            2, 8, 2, -12, 3, 0,
            3, 1, 3, 1, 5, -2, 6, 0,
            2, 9, 3, -15, 4, 1,
            2, 1, 3, 1, 6, 0,
            1, 2, 4, 0,
            2, 1, 3, 1, 5, 1,
            2, 8, 3, -13, 4, 1,
            2, 3, 2, -6, 3, 0,
            2, 1, 3, -4, 4, 0,
            2, 5, 2, -7, 3, 0,
            2, 7, 3, -11, 4, 1,
            2, 1, 1, -3, 3, 0,
            2, 6, 3, -9, 4, 1,
            2, 2, 2, -2, 3, 0,
            2, 5, 3, -7, 4, 2,
            2, 4, 3, -5, 4, 2,
            2, 1, 2, -3, 3, 0,
            2, 3, 3, -3, 4, 0,
            2, 4, 2, -5, 3, 1,
            2, 2, 3, -5, 5, 0,
            1, 1, 2, 1,
            2, 2, 3, -4, 5, 1,
            3, 2, 3, -4, 5, 2, 6, 0,
            2, 6, 3, -8, 4, 1,
            2, 2, 3, -3, 5, 1,
            2, 6, 2, -8, 3, 0,
            2, 5, 3, -6, 4, 0,
            2, 2, 3, -5, 6, 1,
            2, 2, 3, -2, 5, 1,
            3, 2, 3, -4, 5, 5, 6, 1,
            2, 4, 3, -4, 4, 0,
            2, 3, 2, -3, 3, 0,
            2, 2, 3, -3, 6, 0,
            2, 2, 3, -1, 5, 1,
            2, 2, 3, -2, 6, 0,
            2, 3, 3, -2, 4, 0,
            2, 2, 3, -1, 6, 0,
            1, 2, 3, 4,
            2, 5, 2, -6, 3, 1,
            2, 2, 2, -1, 3, 1,
            2, 6, 3, -7, 4, 0,
            2, 5, 3, -5, 4, 0,
            2, 4, 2, -4, 3, 0,
            2, 3, 3, -4, 5, 0,
            2, 3, 3, -3, 5, 0,
            2, 6, 2, -7, 3, 0,
            2, 3, 3, -2, 5, 1,
            2, 3, 2, -2, 3, 0,
            1, 3, 3, 2,
            2, 5, 2, -5, 3, 0,
            2, 1, 1, -1, 3, 0,
            2, 7, 2, -8, 3, 0,
            2, 4, 3, -4, 5, 0,
            2, 4, 3, -3, 5, 0,
            2, 6, 2, -6, 3, 0,
            1, 4, 3, 1,
            2, 7, 2, -7, 3, 1,
            2, 8, 2, -8, 3, 0,
            2, 9, 2, -9, 3, 0,
            -1
    };

    // ===== Delta-T lookup table (seconds), yearly from 1900.0 to 2050.0 =====
    private static final int DT_TAB_START = 1900;
    private static final int DT_TAB_END = 2050;
    private static final double[] DT_TAB = {
            -2.053, -0.820, 0.549, 1.992, 3.450, 4.862, 6.182, 7.431, 8.642, 9.851,
            11.092, 12.387, 13.709, 15.018, 16.275, 17.440, 18.482, 19.405, 20.222, 20.945,
            21.588, 22.159, 22.662, 23.097, 23.466, 23.767, 24.003, 24.178, 24.299, 24.372,
            24.403, 24.397, 24.363, 24.306, 24.234, 24.154, 24.076, 24.030, 24.049, 24.168,
            24.421, 24.825, 25.343, 25.922, 26.508, 27.048, 27.503, 27.890, 28.237, 28.574,
            28.931, 29.321, 29.699, 30.180, 30.623, 31.070, 31.350, 31.681, 32.181, 32.681,
            33.151, 33.591, 34.001, 34.471, 35.031, 35.731, 36.541, 37.431, 38.291, 39.201,
            40.181, 41.171, 42.232, 43.372, 44.486, 45.477, 46.458, 47.523, 48.536, 49.588,
            50.540, 51.382, 52.168, 52.958, 53.789, 54.343, 54.872, 55.323, 55.820, 56.301,
            56.856, 57.566, 58.310, 59.123, 59.986, 60.787, 61.630, 62.296, 62.967, 63.468,
            63.829, 64.091, 64.300, 64.474, 64.574, 64.688, 64.846, 65.147, 65.458, 65.777,
            66.070, 66.325, 66.603, 66.907, 67.281, 67.644, 68.103, 68.593, 68.968, 69.220,
            69.361, 69.359, 69.294, 69.183, 69.100, 69.000, 68.900, 68.800, 68.800, 69.037,
            69.276, 69.518, 69.762, 70.008, 70.257, 70.508, 70.761, 71.017, 71.276, 71.537,
            71.800, 72.066, 72.335, 72.606, 72.880, 73.157, 73.436, 73.718, 74.003, 74.290,
            74.581,
    };

    // ===== Nutation coefficients (IAU 1980, 13 terms) =====
    private static final byte[][] NT_ARGS = {
            {0, 0, 0, 0, 1}, {-2, 0, 0, 2, 2}, {0, 0, 0, 2, 2}, {0, 0, 0, 0, 2},
            {0, 1, 0, 0, 0}, {0, 0, 1, 0, 0}, {-2, 1, 0, 2, 2}, {0, 0, 0, 2, 1},
            {0, 0, 1, 2, 2}, {-2, -1, 0, 2, 2}, {-2, 0, 1, 0, 0}, {-2, 0, 0, 2, 1},
            {0, 0, -1, 2, 2},
    };
    private static final double[] NT_S0 = {
            -171996, -13187, -2274, 2062, 1426, 712, -517, -386, -301, 217, -158, 129, 123
    };
    private static final double[] NT_S1 = {
            -174.2, -1.6, -0.2, 0.2, -3.4, 0.1, 1.2, -0.4, 0.0, -0.5, 0.0, 0.1, 0.0
    };
    private static final double[] NT_C0 = {
            92025, 5736, 977, -895, 54, -7, 224, 200, 129, -95, 0, -70, -53
    };
    private static final double[] NT_C1 = {
            8.9, -3.1, -0.5, 0.5, -0.1, 0.0, -0.6, 0.0, -0.1, 0.3, 0.0, 0.0, 0.0
    };

    // Precompute sin(k*arg) and cos(k*arg) for k=1..n using recurrence
    private void sscc(int k, double arg, int n) {
        double su = Math.sin(arg), cu = Math.cos(arg);
        ssTbl[k][0] = su;
        ccTbl[k][0] = cu;
        double sv = 2.0 * su * cu;
        double cv = cu * cu - su * su;
        ssTbl[k][1] = sv;
        ccTbl[k][1] = cv;
        for (int i = 2; i < n; i++) {
            double s = su * cv + cu * sv;
            cv = cu * cv - su * sv;
            sv = s;
            ssTbl[k][i] = sv;
            ccTbl[k][i] = cv;
        }
    }

    /** VSOP87 heliocentric ecliptic J2000 longitude of EMB in arcseconds. */
    private double vsop87EarthLongitude(double jdTt) {
        double T = (jdTt - J2000_JD) / TIMESCALE;

        // Precompute sin/cos of fundamental planetary arguments
        for (int i = 0; i < 9; i++) {
            if (EAR_MAX_HARMONIC[i] > 0) {
                double sr = (mods3600(FREQS[i] * T) + PHASES[i]) * STR;
                sscc(i, sr, EAR_MAX_HARMONIC[i]);
            }
        }

        int pIdx = 0;
        int plIdx = 0;
        double sl = 0.0;

        for (; ; ) {
            int np = EARARGS[pIdx++];
            if (np < 0) break;

            if (np == 0) {
                // Polynomial term
                int nt = EARARGS[pIdx++];
                double cu = EARTABL[plIdx++];
                for (int ip = 0; ip < nt; ip++)
                    cu = cu * T + EARTABL[plIdx++];
                sl += mods3600(cu);
                continue;
            }

            // Periodic term: combine angle from fundamental arguments
            int k1 = 0;
            double cv = 0.0, sv = 0.0;
            for (int ip = 0; ip < np; ip++) {
                int j = EARARGS[pIdx++];
                int m = EARARGS[pIdx++] - 1;
                if (j != 0) {
                    int k = (j < 0) ? -j : j;
                    k--;
                    double su = ssTbl[m][k];
                    if (j < 0) su = -su;
                    double cu = ccTbl[m][k];
                    if (k1 == 0) {
                        sv = su;
                        cv = cu;
                        k1 = 1;
                    } else {
                        double t = su * cv + cu * sv;
                        cv = cu * cv - su * sv;
                        sv = t;
                    }
                }
            }

            // Evaluate Chebyshev polynomial in T for this term
            int nt = EARARGS[pIdx++];
            double cu = EARTABL[plIdx++];
            double su = EARTABL[plIdx++];
            for (int ip = 0; ip < nt; ip++) {
                cu = cu * T + EARTABL[plIdx++];
                su = su * T + EARTABL[plIdx++];
            }
            sl += cu * cv + su * sv;
        }

        return sl;
    }

    /** Simplified Moon series for EMB->Earth correction. */
    private double embEarthCorrection(double jdTt, double L_emb_rad) {
        double T = (jdTt - J1900_JD) / 36525.0;

        // Moon mean anomaly (M')
        double a = ((1.44e-5 * T + 0.009192) * T + 477198.8491) * T + 296.104608;
        a = a % 360.0;
        if (a < 0) a += 360.0;
        a *= DEG2RAD;
        double smp = Math.sin(a), cmp = Math.cos(a);
        double s2mp = 2.0 * smp * cmp;

        // Moon mean elongation (D) - doubled before sin/cos
        a = ((1.9e-6 * T - 0.001436) * T + 445267.1142) * T + 350.737486;
        a = a % 360.0;
        if (a < 0) a += 360.0;
        a = 2.0 * DEG2RAD * a;
        double s2d = Math.sin(a), c2d = Math.cos(a);

        // Moon argument of latitude (F)
        a = ((-3.e-7 * T - 0.003211) * T + 483202.0251) * T + 11.250889;
        a = a % 360.0;
        if (a < 0) a += 360.0;
        a *= DEG2RAD;
        double sf = Math.sin(a), cf = Math.cos(a);

        double sx = s2d * cmp - c2d * smp; // sin(2D - M')

        // Sun mean anomaly (M)
        double M = ((-3.3e-6 * T - 1.50e-4) * T + 35999.0498) * T + 358.475833;
        M = M % 360.0;
        if (M < 0) M += 360.0;

        // Moon ecliptic longitude
        double L = ((1.9e-6 * T - 0.001133) * T + 481267.8831) * T + 270.434164;
        L += 6.288750 * smp
                + 1.274018 * sx
                + 0.658309 * s2d
                + 0.213616 * s2mp
                - 0.185596 * Math.sin(DEG2RAD * M)
                - 0.114336 * (2.0 * sf * cf); // sin(2F)

        // Moon ecliptic latitude (radians)
        double aTmp = smp * cf;
        double sxTmp = cmp * sf;
        double B = 5.128189 * sf
                + 0.280606 * (aTmp + sxTmp)  // sin(M'+F)
                + 0.277693 * (aTmp - sxTmp)  // sin(M'-F)
                + 0.173238 * (s2d * cf - c2d * sf); // sin(2D-F)
        B *= DEG2RAD;

        // Moon parallax -> distance in AU
        double cx = c2d * cmp + s2d * smp; // cos(2D - M')
        double p = 0.950724
                + 0.051818 * cmp
                + 0.009531 * cx
                + 0.007843 * c2d
                + 0.002824 * (cmp * cmp - smp * smp); // cos(2M')
        p *= DEG2RAD;
        double rMoon = 4.263523e-5 / Math.sin(p);

        L = L % 360.0;
        if (L < 0) L += 360.0;
        double L_moon_rad = L * DEG2RAD;

        return -rMoon * Math.cos(B) * Math.sin(L_moon_rad - L_emb_rad) / (EARTH_MOON_MRAT + 1.0);
    }

    /** Delta-T (TT - UT) in days */
    double deltaT(double jdUt) {
        return deltaTSeconds(jdUt) / 86400.0;
    }

    private double deltaTSeconds(double jdUt) {
        int[] ymd = MoshierJulianDay.revjul(jdUt);
        double y = ymd[0] + (ymd[1] - 0.5) / 12.0;

        if (y >= DT_TAB_START && y < DT_TAB_END + 1) {
            double idx = y - DT_TAB_START;
            int i = (int) idx;
            if (i >= DT_TAB.length - 1) i = DT_TAB.length - 2;
            double frac = idx - i;
            return DT_TAB[i] + frac * (DT_TAB[i + 1] - DT_TAB[i]);
        } else if (y < DT_TAB_START) {
            double t = (y - 1820.0) / 100.0;
            return -20 + 32 * t * t;
        } else if (y < 2150) {
            return -20 + 32 * ((y - 1820.0) / 100.0) * ((y - 1820.0) / 100.0)
                    - 0.5628 * (2150 - y);
        } else {
            double u = (y - 1820.0) / 100.0;
            return -20 + 32 * u * u;
        }
    }

    private double jdUtToTt(double jdUt) {
        return jdUt + deltaTSeconds(jdUt) / 86400.0;
    }

    /** Nutation in longitude (dpsi) and obliquity (deps), returned as double[2] in degrees. */
    double[] nutation(double jdTt) {
        double T = (jdTt - 2451545.0) / 36525.0;
        double T2 = T * T;
        double T3 = T2 * T;

        double D = 297.85036 + 445267.111480 * T - 0.0019142 * T2 + T3 / 189474.0;
        double M = 357.52772 + 35999.050340 * T - 0.0001603 * T2 - T3 / 300000.0;
        double Mp = 134.96298 + 477198.867398 * T + 0.0086972 * T2 + T3 / 56250.0;
        double F = 93.27191 + 483202.017538 * T - 0.0036825 * T2 + T3 / 327270.0;
        double Om = 125.04452 - 1934.136261 * T + 0.0020708 * T2 + T3 / 450000.0;

        D *= DEG2RAD;
        M *= DEG2RAD;
        Mp *= DEG2RAD;
        F *= DEG2RAD;
        Om *= DEG2RAD;

        double sumDpsi = 0, sumDeps = 0;
        for (int i = 0; i < 13; i++) {
            double arg = NT_ARGS[i][0] * D + NT_ARGS[i][1] * M + NT_ARGS[i][2] * Mp
                    + NT_ARGS[i][3] * F + NT_ARGS[i][4] * Om;
            sumDpsi += (NT_S0[i] + NT_S1[i] * T) * Math.sin(arg);
            sumDeps += (NT_C0[i] + NT_C1[i] * T) * Math.cos(arg);
        }

        // Convert from 0.0001" to degrees
        return new double[]{sumDpsi * 0.0001 / 3600.0, sumDeps * 0.0001 / 3600.0};
    }

    /** Mean obliquity of the ecliptic in degrees (Laskar) */
    double meanObliquity(double jdTt) {
        double T = (jdTt - 2451545.0) / 36525.0;
        double U = T / 100.0;
        return 23.0 + 26.0 / 60.0 + 21.448 / 3600.0
                + (-4680.93 * U
                - 1.55 * U * U
                + 1999.25 * U * U * U
                - 51.38 * U * U * U * U
                - 249.67 * U * U * U * U * U
                - 39.05 * U * U * U * U * U * U
                + 7.12 * U * U * U * U * U * U * U
                + 27.87 * U * U * U * U * U * U * U * U
                + 5.79 * U * U * U * U * U * U * U * U * U
                + 2.45 * U * U * U * U * U * U * U * U * U * U) / 3600.0;
    }

    // ===== Solar position pipeline =====

    /**
     * Returns apparent solar longitude in degrees.
     * Also sets decl[0] if decl != null, nutLon[0] if nutLon != null.
     */
    private double solarPosition(double jdUt, double[] decl, double[] nutLon) {
        double jdTt = jdUtToTt(jdUt);
        double T = (jdTt - J2000_JD) / 36525.0;

        // Step 1: VSOP87 heliocentric ecliptic J2000 longitude of EMB
        double sl = vsop87EarthLongitude(jdTt); // arcseconds
        double L_emb_j2000 = sl * STR; // radians, J2000 ecliptic

        // Step 2: General precession in longitude (IAU 1976)
        double pA = (5029.0966 + 1.11113 * T - 0.000006 * T * T) * T;
        double L_emb_date = L_emb_j2000 + pA * STR;

        // Step 3: EMB->Earth correction
        double dL = embEarthCorrection(jdTt, L_emb_date);
        double L_earth_date = L_emb_date + dL;

        // Step 4: Geocentric flip
        double L_sun_date = L_earth_date + Math.PI;

        // Step 5: Nutation
        double[] nut = nutation(jdTt);
        double dpsi = nut[0], deps = nut[1];
        double L_apparent = L_sun_date + dpsi * DEG2RAD;

        // Step 6: Standard aberration
        L_apparent -= 20.496 * STR;

        // Convert to degrees and normalize
        double apparent = normalizeDeg(L_apparent * RAD2DEG);

        if (nutLon != null) nutLon[0] = dpsi;

        if (decl != null) {
            double eps0 = meanObliquity(jdTt);
            double eps = (eps0 + deps) * DEG2RAD;
            double lam = apparent * DEG2RAD;
            decl[0] = Math.asin(Math.sin(eps) * Math.sin(lam)) * RAD2DEG;
        }

        return apparent;
    }

    // ===== Public API =====

    double solarLongitude(double jdUt) {
        return solarPosition(jdUt, null, null);
    }

    double solarDeclination(double jdUt) {
        double[] decl = new double[1];
        solarPosition(jdUt, decl, null);
        return decl[0];
    }

    double solarRa(double jdUt) {
        double jdTt = jdUtToTt(jdUt);
        double[] nut = nutation(jdTt);
        double dpsi = nut[0], deps = nut[1];
        double eps0 = meanObliquity(jdTt);
        double eps = (eps0 + deps) * DEG2RAD;
        double lam = solarLongitude(jdUt) * DEG2RAD;
        double ra = Math.atan2(Math.cos(eps) * Math.sin(lam), Math.cos(lam)) * RAD2DEG;
        return normalizeDeg(ra);
    }

    double nutationLongitude(double jdUt) {
        double jdTt = jdUtToTt(jdUt);
        return nutation(jdTt)[0];
    }

    double meanObliquityUt(double jdUt) {
        return meanObliquity(jdUtToTt(jdUt));
    }

    double trueObliquity(double jdUt) {
        double jdTt = jdUtToTt(jdUt);
        return meanObliquity(jdTt) + nutation(jdTt)[1];
    }
}
