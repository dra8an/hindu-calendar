/*
 *  — Lunar longitude (DE404 Moshier theory)
 *
 * Implementation of Steve Moshier's DE404-fitted analytical lunar ephemeris.
 * Only the longitude pipeline is retained; latitude, radius, and speed
 * computation are omitted.
 *
 * Based on ELP2000-85 adjusted to fit JPL DE404 on -3000 to +3000.
 * Precision: ~3-5 arcseconds (max 7") over -3000 to +3000.
 *
 * References:
 *   S. L. Moshier, "Comparison of a 7000-year lunar ephemeris with
 *   analytical theory," A&A 262, 613-616 (1992)
 *   M. Chapront-Touze & J. Chapront, ELP2000-85, A&A 190, 342 (1988)
 */
#include "moshier.h"
#include <math.h>

/* Forward declarations for helpers in moshier_sun.c */
extern double moshier_delta_t(double jd_ut);
extern double moshier_nutation_longitude(double jd_ut);

/* Arc seconds to radians */
#define STR 4.8481368110953599359e-6

#define J2000 2451545.0

/* ---- Reduce arc seconds modulo 360 degrees ---- */
static double mods3600(double x)
{
    return x - 1296000.0 * floor(x / 1296000.0);
}

/* ---- sin/cos lookup table for multiples of angles ---- */
static double sin_tbl[5][8];
static double cos_tbl[5][8];

static void sscc(int k, double arg, int n)
{
    double cu = cos(arg), su = sin(arg);
    double cv, sv, s;
    int i;
    sin_tbl[k][0] = su;
    cos_tbl[k][0] = cu;
    sv = 2.0 * su * cu;
    cv = cu * cu - su * su;
    sin_tbl[k][1] = sv;
    cos_tbl[k][1] = cv;
    for (i = 2; i < n; i++) {
        s = su * cv + cu * sv;
        cv = cu * cv - su * sv;
        sv = s;
        sin_tbl[k][i] = sv;
        cos_tbl[k][i] = cv;
    }
}

/* ---- Accumulate longitude from perturbation table ---- */
/* typflg=1: large lon/rad (4 values per line after angles)
 * typflg=2: small lon/rad (2 values per line after angles)
 * We only accumulate longitude (ans[0]), skip radius. */
static void accum_series(const short *pt, int nlines, int nangles,
                          int typflg, double *ans)
{
    int i, j, k, k1, m;
    double cu, su, cv, sv, ff;
    for (i = 0; i < nlines; i++) {
        k1 = 0;
        sv = 0.0;
        cv = 0.0;
        for (m = 0; m < nangles; m++) {
            j = *pt++;
            if (j) {
                k = j < 0 ? -j : j;
                su = sin_tbl[m][k - 1];
                cu = cos_tbl[m][k - 1];
                if (j < 0) su = -su;
                if (k1 == 0) {
                    sv = su;
                    cv = cu;
                    k1 = 1;
                } else {
                    ff = su * cv + cu * sv;
                    cv = cu * cv - su * sv;
                    sv = ff;
                }
            }
        }
        switch (typflg) {
        case 1: /* large longitude and radius */
            j = *pt++;
            k = *pt++;
            ans[0] += (10000.0 * j + k) * sv;
            pt += 2; /* skip radius */
            break;
        case 2: /* small longitude and radius */
            j = *pt++;
            ans[0] += j * sv;
            pt++; /* skip radius */
            break;
        }
    }
}

/* ================================================================
 * DE404 least-squares corrections (34,247 lunar positions)
 * Replaces corresponding terms in mean elements.
 * z[0..11]: scaled in arc seconds
 * z[12..24]: longitude perturbation terms (arc seconds × 10^5)
 * ================================================================ */
static const double z[] = {
    /* F, t^2..t^4 */
    -1.312045233711e+01,
    -1.138215912580e-03,
    -9.646018347184e-06,
    /* l, t^2..t^4 */
     3.146734198839e+01,
     4.768357585780e-02,
    -3.421689790404e-04,
    /* D, t^2..t^4 */
    -6.847070905410e+00,
    -5.834100476561e-03,
    -2.905334122698e-04,
    /* L, t^2..t^4 */
    -5.663161722088e+00,
     5.722859298199e-03,
    -8.466472828815e-05,
    /* Longitude perturbation terms (× 10^5 arcseconds) */
    -8.429817796435e+01,  /* t^2 cos(18V - 16E - l) */
    -2.072552484689e+02,  /* t^2 sin(18V - 16E - l) */
     7.876842214863e+00,  /* t^2 cos(10V - 3E - l)  */
     1.836463749022e+00,  /* t^2 sin(10V - 3E - l)  */
    -1.557471855361e+01,  /* t^2 cos(8V - 13E)      */
    -2.006969124724e+01,  /* t^2 sin(8V - 13E)      */
     2.152670284757e+01,  /* t^2 cos(4E - 8M + 3J)  */
    -6.179946916139e+00,  /* t^2 sin(4E - 8M + 3J)  */
    -9.070028191196e-01,  /* t^2 cos(18V - 16E)     */
    -1.270848233038e+01,  /* t^2 sin(18V - 16E)     */
    -2.145589319058e+00,  /* t^2 cos(2J - 5S)       */
     1.381936399935e+01,  /* t^2 sin(2J - 5S)       */
    -1.999840061168e+00,  /* t^3 sin(l')             */
};

/* ================================================================
 * Main longitude + radius perturbation table (118 terms)
 * Format: D, l', l, F, lon_1", lon_.0001", rad_1km, rad_.0001km
 * ================================================================ */
#define MOON_LR_N 118
static const short moon_lr[8 * MOON_LR_N] = {
/*               Longitude    Radius
 D  l' l  F    1"  .0001"  1km  .0001km */
 0, 0, 1, 0, 22639, 5858,-20905,-3550,
 2, 0,-1, 0,  4586, 4383, -3699,-1109,
 2, 0, 0, 0,  2369, 9139, -2955,-9676,
 0, 0, 2, 0,   769,  257,  -569,-9251,
 0, 1, 0, 0,  -666,-4171,    48, 8883,
 0, 0, 0, 2,  -411,-5957,    -3,-1483,
 2, 0,-2, 0,   211, 6556,   246, 1585,
 2,-1,-1, 0,   205, 4358,  -152,-1377,
 2, 0, 1, 0,   191, 9562,  -170,-7331,
 2,-1, 0, 0,   164, 7285,  -204,-5860,
 0, 1,-1, 0,  -147,-3213,  -129,-6201,
 1, 0, 0, 0,  -124,-9881,   108, 7427,
 0, 1, 1, 0,  -109,-3803,   104, 7552,
 2, 0, 0,-2,    55, 1771,    10, 3211,
 0, 0, 1, 2,   -45, -996,     0,    0,
 0, 0, 1,-2,    39, 5333,    79, 6606,
 4, 0,-1, 0,    38, 4298,   -34,-7825,
 0, 0, 3, 0,    36, 1238,   -23,-2104,
 4, 0,-2, 0,    30, 7726,   -21,-6363,
 2, 1,-1, 0,   -28,-3971,    24, 2085,
 2, 1, 0, 0,   -24,-3582,    30, 8238,
 1, 0,-1, 0,   -18,-5847,    -8,-3791,
 1, 1, 0, 0,    17, 9545,   -16,-6747,
 2,-1, 1, 0,    14, 5303,   -12,-8314,
 2, 0, 2, 0,    14, 3797,   -10,-4448,
 4, 0, 0, 0,    13, 8991,   -11,-6500,
 2, 0,-3, 0,    13, 1941,    14, 4027,
 0, 1,-2, 0,    -9,-6791,    -7,  -27,
 2, 0,-1, 2,    -9,-3659,     0, 7740,
 2,-1,-2, 0,     8, 6055,    10,  562,
 1, 0, 1, 0,    -8,-4531,     6, 3220,
 2,-2, 0, 0,     8,  502,    -9,-8845,
 0, 1, 2, 0,    -7,-6302,     5, 7509,
 0, 2, 0, 0,    -7,-4475,     1,  657,
 2,-2,-1, 0,     7, 3712,    -4,-9501,
 2, 0, 1,-2,    -6,-3832,     4, 1311,
 2, 0, 0, 2,    -5,-7416,     0,    0,
 4,-1,-1, 0,     4, 3740,    -3,-9580,
 0, 0, 2, 2,    -3,-9976,     0,    0,
 3, 0,-1, 0,    -3,-2097,     3, 2582,
 2, 1, 1, 0,    -2,-9145,     2, 6164,
 4,-1,-2, 0,     2, 7319,    -1,-8970,
 0, 2,-1, 0,    -2,-5679,    -2,-1171,
 2, 2,-1, 0,    -2,-5212,     2, 3536,
 2, 1,-2, 0,     2, 4889,     0, 1437,
 2,-1, 0,-2,     2, 1461,     0, 6571,
 4, 0, 1, 0,     1, 9777,    -1,-4226,
 0, 0, 4, 0,     1, 9337,    -1,-1169,
 4,-1, 0, 0,     1, 8708,    -1,-5714,
 1, 0,-2, 0,    -1,-7530,    -1,-7385,
 2, 1, 0,-2,    -1,-4372,     0,-1357,
 0, 0, 2,-2,    -1,-3726,    -4,-4212,
 1, 1, 1, 0,     1, 2618,     0,-9333,
 3, 0,-2, 0,    -1,-2241,     0, 8624,
 4, 0,-3, 0,     1, 1868,     0,-5142,
 2,-1, 2, 0,     1, 1770,     0,-8488,
 0, 2, 1, 0,    -1,-1617,     1, 1655,
 1, 1,-1, 0,     1,  777,     0, 8512,
 2, 0, 3, 0,     1,  595,     0,-6697,
 2, 0, 1, 2,     0,-9902,     0,    0,
 2, 0,-4, 0,     0, 9483,     0, 7785,
 2,-2, 1, 0,     0, 7517,     0,-6575,
 0, 1,-3, 0,     0,-6694,     0,-4224,
 4, 1,-1, 0,     0,-6352,     0, 5788,
 1, 0, 2, 0,     0,-5840,     0, 3785,
 1, 0, 0,-2,     0,-5833,     0,-7956,
 6, 0,-2, 0,     0, 5716,     0,-4225,
 2, 0,-2,-2,     0,-5606,     0, 4726,
 1,-1, 0, 0,     0,-5569,     0, 4976,
 0, 1, 3, 0,     0,-5459,     0, 3551,
 2, 0,-2, 2,     0,-5357,     0, 7740,
 2, 0,-1,-2,     0, 1790,     8, 7516,
 3, 0, 0, 0,     0, 4042,    -1,-4189,
 2,-1,-3, 0,     0, 4784,     0, 4950,
 2,-1, 3, 0,     0,  932,     0, -585,
 2, 0, 2,-2,     0,-4538,     0, 2840,
 2,-1,-1, 2,     0,-4262,     0,  373,
 0, 0, 0, 4,     0, 4203,     0,    0,
 0, 1, 0, 2,     0, 4134,     0,-1580,
 6, 0,-1, 0,     0, 3945,     0,-2866,
 2,-1, 0, 2,     0,-3821,     0,    0,
 2,-1, 1,-2,     0,-3745,     0, 2094,
 4, 1,-2, 0,     0,-3576,     0, 2370,
 1, 1,-2, 0,     0, 3497,     0, 3323,
 2,-3, 0, 0,     0, 3398,     0,-4107,
 0, 0, 3, 2,     0,-3286,     0,    0,
 4,-2,-1, 0,     0,-3087,     0,-2790,
 0, 1,-1,-2,     0, 3015,     0,    0,
 4, 0,-1,-2,     0, 3009,     0,-3218,
 2,-2,-2, 0,     0, 2942,     0, 3430,
 6, 0,-3, 0,     0, 2925,     0,-1832,
 2, 1, 2, 0,     0,-2902,     0, 2125,
 4, 1, 0, 0,     0,-2891,     0, 2445,
 4,-1, 1, 0,     0, 2825,     0,-2029,
 3, 1,-1, 0,     0, 2737,     0,-2126,
 0, 1, 1, 2,     0, 2634,     0,    0,
 1, 0, 0, 2,     0, 2543,     0,    0,
 3, 0, 0,-2,     0,-2530,     0, 2010,
 2, 2,-2, 0,     0,-2499,     0,-1089,
 2,-3,-1, 0,     0, 2469,     0,-1481,
 3,-1,-1, 0,     0,-2314,     0, 2556,
 4, 0, 2, 0,     0, 2185,     0,-1392,
 4, 0,-1, 2,     0,-2013,     0,    0,
 0, 2,-2, 0,     0,-1931,     0,    0,
 2, 2, 0, 0,     0,-1858,     0,    0,
 2, 1,-3, 0,     0, 1762,     0,    0,
 4, 0,-2, 2,     0,-1698,     0,    0,
 4,-2,-2, 0,     0, 1578,     0,-1083,
 4,-2, 0, 0,     0, 1522,     0,-1281,
 3, 1, 0, 0,     0, 1499,     0,-1077,
 1,-1,-1, 0,     0,-1364,     0, 1141,
 1,-3, 0, 0,     0,-1281,     0,    0,
 6, 0, 0, 0,     0, 1261,     0, -859,
 2, 0, 2, 2,     0,-1239,     0,    0,
 1,-1, 1, 0,     0,-1207,     0, 1100,
 0, 0, 5, 0,     0, 1110,     0, -589,
 0, 3, 0, 0,     0,-1013,     0,  213,
 4,-1,-3, 0,     0,  998,     0,    0,
};

/* T^1 longitude + radius corrections (38 terms) */
#define MOON_LR_T1_N 38
static const short moon_lr_t1[8 * MOON_LR_T1_N] = {
/*  Multiply by T
               Longitude    Radius
 D  l' l  F   .1"  .00001" .1km  .00001km */
 0, 1, 0, 0,    16, 7680,    -1,-2302,
 2,-1,-1, 0,    -5,-1642,     3, 8245,
 2,-1, 0, 0,    -4,-1383,     5, 1395,
 0, 1,-1, 0,     3, 7115,     3, 2654,
 0, 1, 1, 0,     2, 7560,    -2,-6396,
 2, 1,-1, 0,     0, 7118,     0,-6068,
 2, 1, 0, 0,     0, 6128,     0,-7754,
 1, 1, 0, 0,     0,-4516,     0, 4194,
 2,-2, 0, 0,     0,-4048,     0, 4970,
 0, 2, 0, 0,     0, 3747,     0, -540,
 2,-2,-1, 0,     0,-3707,     0, 2490,
 2,-1, 1, 0,     0,-3649,     0, 3222,
 0, 1,-2, 0,     0, 2438,     0, 1760,
 2,-1,-2, 0,     0,-2165,     0,-2530,
 0, 1, 2, 0,     0, 1923,     0,-1450,
 0, 2,-1, 0,     0, 1292,     0, 1070,
 2, 2,-1, 0,     0, 1271,     0,-6070,
 4,-1,-1, 0,     0,-1098,     0,  990,
 2, 0, 0, 0,     0, 1073,     0,-1360,
 2, 0,-1, 0,     0,  839,     0, -630,
 2, 1, 1, 0,     0,  734,     0, -660,
 4,-1,-2, 0,     0, -688,     0,  480,
 2, 1,-2, 0,     0, -630,     0,    0,
 0, 2, 1, 0,     0,  587,     0, -590,
 2,-1, 0,-2,     0, -540,     0, -170,
 4,-1, 0, 0,     0, -468,     0,  390,
 2,-2, 1, 0,     0, -378,     0,  330,
 2, 1, 0,-2,     0,  364,     0,    0,
 1, 1, 1, 0,     0, -317,     0,  240,
 2,-1, 2, 0,     0, -295,     0,  210,
 1, 1,-1, 0,     0, -270,     0, -210,
 2,-3, 0, 0,     0, -256,     0,  310,
 2,-3,-1, 0,     0, -187,     0,  110,
 0, 1,-3, 0,     0,  169,     0,  110,
 4, 1,-1, 0,     0,  158,     0, -150,
 4,-2,-1, 0,     0, -155,     0,  140,
 0, 0, 1, 0,     0,  155,     0, -250,
 2,-2,-2, 0,     0, -148,     0, -170,
};

/* T^2 longitude + radius corrections (25 terms) */
#define MOON_LR_T2_N 25
static const short moon_lr_t2[6 * MOON_LR_T2_N] = {
/*  Multiply by T^2
           Longitude    Radius
 D  l' l  F  .00001" .00001km */
 0, 1, 0, 0,  487,   -36,
 2,-1,-1, 0, -150,   111,
 2,-1, 0, 0, -120,   149,
 0, 1,-1, 0,  108,    95,
 0, 1, 1, 0,   80,   -77,
 2, 1,-1, 0,   21,   -18,
 2, 1, 0, 0,   20,   -23,
 1, 1, 0, 0,  -13,    12,
 2,-2, 0, 0,  -12,    14,
 2,-1, 1, 0,  -11,     9,
 2,-2,-1, 0,  -11,     7,
 0, 2, 0, 0,   11,     0,
 2,-1,-2, 0,   -6,    -7,
 0, 1,-2, 0,    7,     5,
 0, 1, 2, 0,    6,    -4,
 2, 2,-1, 0,    5,    -3,
 0, 2,-1, 0,    5,     3,
 4,-1,-1, 0,   -3,     3,
 2, 0, 0, 0,    3,    -4,
 4,-1,-2, 0,   -2,     0,
 2, 1,-2, 0,   -2,     0,
 2,-1, 0,-2,   -2,     0,
 2, 1, 1, 0,    2,    -2,
 2, 0,-1, 0,    2,     0,
 0, 2, 1, 0,    2,     0,
};

/* ================================================================
 * Mean element variables (arcseconds)
 * ================================================================ */
static double LP;  /* mean longitude of moon */
static double M_sun;  /* mean anomaly of sun (l') */
static double MP;     /* mean anomaly of moon (l) */
static double D;      /* mean elongation */
static double NF;     /* mean distance from ascending node (F) */
static double T, T2;

/* Planetary mean longitudes */
static double Ve, Ea, Ma, Ju, Sa;

/* (Perturbation accumulators are local to lunar_perturbations.) */

/* ---- Mean elements (DE404) ---- */
static void mean_elements(void)
{
    double fracT = fmod(T, 1.0);

    /* Mean anomaly of sun = l' (J. Laskar) */
    M_sun = mods3600(129600000.0 * fracT - 3418.961646 * T + 1287104.76154);
    M_sun += ((((((((
         1.62e-20 * T
       - 1.0390e-17) * T
       - 3.83508e-15) * T
       + 4.237343e-13) * T
       + 8.8555011e-11) * T
       - 4.77258489e-8) * T
       - 1.1297037031e-5) * T
       + 1.4732069041e-4) * T
       - 0.552891801772) * T2;

    /* Mean distance of moon from ascending node = F */
    NF = mods3600(1739232000.0 * fracT + 295263.0983 * T
                  - 2.079419901760e-01 * T + 335779.55755);

    /* Mean anomaly of moon = l */
    MP = mods3600(1717200000.0 * fracT + 715923.4728 * T
                  - 2.035946368532e-01 * T + 485868.28096);

    /* Mean elongation of moon = D */
    D = mods3600(1601856000.0 * fracT + 1105601.4603 * T
                 + 3.962893294503e-01 * T + 1072260.73512);

    /* Mean longitude of moon */
    LP = mods3600(1731456000.0 * fracT + 1108372.83264 * T
                     - 6.784914260953e-01 * T + 785939.95571);

    /* Higher degree secular terms (DE404 corrections) */
    NF   += ((z[2] * T + z[1]) * T + z[0]) * T2;
    MP   += ((z[5] * T + z[4]) * T + z[3]) * T2;
    D    += ((z[8] * T + z[7]) * T + z[6]) * T2;
    LP += ((z[11] * T + z[10]) * T + z[9]) * T2;
}

/* ---- Planetary mean longitudes (Laskar, Bretagnon) ---- */
static void mean_elements_pl(void)
{
    Ve = mods3600(210664136.4335482 * T + 655127.283046);
    Ve += ((((((((
        -9.36e-023 * T
        - 1.95e-20) * T
        + 6.097e-18) * T
        + 4.43201e-15) * T
        + 2.509418e-13) * T
        - 3.0622898e-10) * T
        - 2.26602516e-9) * T
        - 1.4244812531e-5) * T
        + 0.005871373088) * T2;

    Ea = mods3600(129597742.26669231 * T + 361679.214649);
    Ea += ((((((((-1.16e-22 * T
        + 2.976e-19) * T
        + 2.8460e-17) * T
        - 1.08402e-14) * T
        - 1.226182e-12) * T
        + 1.7228268e-10) * T
        + 1.515912254e-7) * T
        + 8.863982531e-6) * T
        - 2.0199859001e-2) * T2;

    Ma = mods3600(68905077.59284 * T + 1279559.78866);
    Ma += (-1.043e-5 * T + 9.38012e-3) * T2;

    Ju = mods3600(10925660.428608 * T + 123665.342120);
    Ju += (1.543273e-5 * T - 3.06037836351e-1) * T2;

    Sa = mods3600(4399609.65932 * T + 180278.89694);
    Sa += ((4.475946e-8 * T - 6.874806E-5) * T + 7.56161437443E-1) * T2;
}

/* ================================================================
 * Lunar longitude perturbations (DE404-fitted)
 *
 * Computes all perturbation corrections to the mean lunar longitude:
 *   1. Initialize sin/cos multiples for D, l', l, F
 *   2. T^2 table corrections (LRT2, 25 terms)
 *   3. Explicit planetary perturbations (Venus, Jupiter, Earth, Mars, Saturn)
 *   4. T^1 table corrections (LRT, 38 terms)
 *   5. Additional DE404-fitted explicit terms
 *   6. Main perturbation table (LR, 118 terms)
 *   7. Polynomial assembly and conversion to radians
 *
 * Returns lunar longitude in radians (arcseconds-based, needs degree
 * conversion by caller).
 * ================================================================ */
static double lunar_perturbations(void)
{
    double moonpol0;     /* table accumulator */
    double l_acc;        /* explicit perturbation longitude (arcsec) */
    double l1, l2, l3, l4;  /* polynomial coefficients for T^1..T^4 */
    double f_ve;         /* 18V - 16E */
    double cg, sg;       /* cos/sin of current argument */
    double a, g_arg;
    int i, j;

    /* Zero ss/cc arrays (Bhanu Pinnamaneni fix) */
    for (i = 0; i < 5; i++)
        for (j = 0; j < 8; j++) {
            sin_tbl[i][j] = 0;
            cos_tbl[i][j] = 0;
        }

    sscc(0, STR * D, 6);
    sscc(1, STR * M_sun, 4);
    sscc(2, STR * MP, 4);
    sscc(3, STR * NF, 4);

    /* ---- T^2 table corrections ---- */
    moonpol0 = 0.0;
    accum_series(moon_lr_t2, MOON_LR_T2_N, 4, 2, &moonpol0);

    /* ---- Explicit planetary perturbations ---- */
    f_ve = 18.0 * Ve - 16.0 * Ea;

    g_arg = STR * (f_ve - MP);       /* 18V - 16E - l */
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc = 6.367278 * cg + 12.747036 * sg;      /* t^0 */
    l1 = 23123.70 * cg - 10570.02 * sg;          /* t^1 */
    l2 = z[12] * cg + z[13] * sg;                /* t^2 */

    g_arg = STR * (10.0 * Ve - 3.0 * Ea - MP);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.253102 * cg + 0.503359 * sg;
    l1 += 1258.46 * cg + 707.29 * sg;
    l2 += z[14] * cg + z[15] * sg;

    g_arg = STR * (8.0 * Ve - 13.0 * Ea);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.187231 * cg - 0.127481 * sg;
    l1 += -319.87 * cg - 18.34 * sg;
    l2 += z[16] * cg + z[17] * sg;

    a = 4.0 * Ea - 8.0 * Ma + 3.0 * Ju;
    g_arg = STR * a;
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.866287 * cg + 0.248192 * sg;
    l1 += 41.87 * cg + 1053.97 * sg;
    l2 += z[18] * cg + z[19] * sg;

    g_arg = STR * (a - MP);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.165009 * cg + 0.044176 * sg;
    l1 += 4.67 * cg + 201.55 * sg;

    g_arg = STR * f_ve;               /* 18V - 16E */
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += 0.330401 * cg + 0.661362 * sg;
    l1 += 1202.67 * cg - 555.59 * sg;
    l2 += z[20] * cg + z[21] * sg;

    g_arg = STR * (f_ve - 2.0 * MP);  /* 18V - 16E - 2l */
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += 0.352185 * cg + 0.705041 * sg;
    l1 += 1283.59 * cg - 586.43 * sg;

    g_arg = STR * (2.0 * Ju - 5.0 * Sa);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.034700 * cg + 0.160041 * sg;
    l2 += z[22] * cg + z[23] * sg;

    g_arg = STR * (LP - NF);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += 0.000116 * cg + 7.063040 * sg;
    l1 += 298.8 * sg;

    /* T^3 terms */
    sg = sin(STR * M_sun);
    l3 = z[24] * sg;
    l4 = 0;

    l2 += moonpol0;

    /* ---- T^1 table corrections ---- */
    moonpol0 = 0.0;
    accum_series(moon_lr_t1, MOON_LR_T1_N, 4, 1, &moonpol0);

    g_arg = STR * (2.0 * Ve - 3.0 * Ea);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.343550 * cg - 0.000276 * sg;
    l1 += 105.90 * cg + 336.53 * sg;

    g_arg = STR * (f_ve - 2.0 * D);   /* 18V - 16E - 2D */
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += 0.074668 * cg + 0.149501 * sg;
    l1 += 271.77 * cg - 124.20 * sg;

    g_arg = STR * (f_ve - 2.0 * D - MP);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += 0.073444 * cg + 0.147094 * sg;
    l1 += 265.24 * cg - 121.16 * sg;

    g_arg = STR * (f_ve + 2.0 * D - MP);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += 0.072844 * cg + 0.145829 * sg;
    l1 += 265.18 * cg - 121.29 * sg;

    g_arg = STR * (f_ve + 2.0 * (D - MP));
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += 0.070201 * cg + 0.140542 * sg;
    l1 += 255.36 * cg - 116.79 * sg;

    g_arg = STR * (Ea + D - NF);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += 0.288209 * cg - 0.025901 * sg;
    l1 += -63.51 * cg - 240.14 * sg;

    g_arg = STR * (2.0 * Ea - 3.0 * Ju + 2.0 * D - MP);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += 0.077865 * cg + 0.438460 * sg;
    l1 += 210.57 * cg + 124.84 * sg;

    g_arg = STR * (Ea - 2.0 * Ma);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.216579 * cg + 0.241702 * sg;
    l1 += 197.67 * cg + 125.23 * sg;

    g_arg = STR * (a + MP);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.165009 * cg + 0.044176 * sg;
    l1 += 4.67 * cg + 201.55 * sg;

    g_arg = STR * (a + 2.0 * D - MP);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.133533 * cg + 0.041116 * sg;
    l1 += 6.95 * cg + 187.07 * sg;

    g_arg = STR * (a - 2.0 * D + MP);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.133430 * cg + 0.041079 * sg;
    l1 += 6.28 * cg + 169.08 * sg;

    g_arg = STR * (3.0 * Ve - 4.0 * Ea);
    cg = cos(g_arg);
    sg = sin(g_arg);
    l_acc += -0.175074 * cg + 0.003035 * sg;
    l1 += 49.17 * cg + 150.57 * sg;

    g_arg = STR * (2.0 * (Ea + D - MP) - 3.0 * Ju + 213534.0);
    l1 += 158.4 * sin(g_arg);

    l1 += moonpol0;

    /* ---- Additional DE404-fitted explicit terms ---- */
    g_arg = STR * (2.0 * (Ea - Ju + D) - MP + 648431.172);
    l_acc += 1.14307 * sin(g_arg);

    g_arg = STR * (Ve - Ea + 648035.568);
    l_acc += 0.82155 * sin(g_arg);

    g_arg = STR * (3.0 * (Ve - Ea) + 2.0 * D - MP + 647933.184);
    l_acc += 0.64371 * sin(g_arg);

    g_arg = STR * (Ea - Ju + 4424.04);
    l_acc += 0.63880 * sin(g_arg);

    g_arg = STR * (LP + MP - NF + 4.68);
    l_acc += 0.49331 * sin(g_arg);

    g_arg = STR * (LP - MP - NF + 4.68);
    l_acc += 0.4914 * sin(g_arg);

    g_arg = STR * (LP + NF + 2.52);
    l_acc += 0.36061 * sin(g_arg);

    g_arg = STR * (2.0 * Ve - 2.0 * Ea + 736.2);
    l_acc += 0.30154 * sin(g_arg);

    g_arg = STR * (2.0 * Ea - 3.0 * Ju + 2.0 * D - 2.0 * MP + 36138.2);
    l_acc += 0.28282 * sin(g_arg);

    g_arg = STR * (2.0 * Ea - 2.0 * Ju + 2.0 * D - 2.0 * MP + 311.0);
    l_acc += 0.24516 * sin(g_arg);

    g_arg = STR * (Ea - Ju - 2.0 * D + MP + 6275.88);
    l_acc += 0.21117 * sin(g_arg);

    g_arg = STR * (2.0 * (Ea - Ma) - 846.36);
    l_acc += 0.19444 * sin(g_arg);

    g_arg = STR * (2.0 * (Ea - Ju) + 1569.96);
    l_acc -= 0.18457 * sin(g_arg);

    g_arg = STR * (2.0 * (Ea - Ju) - MP - 55.8);
    l_acc += 0.18256 * sin(g_arg);

    g_arg = STR * (Ea - Ju - 2.0 * D + 6490.08);
    l_acc += 0.16499 * sin(g_arg);

    g_arg = STR * (Ea - 2.0 * Ju - 212378.4);
    l_acc += 0.16427 * sin(g_arg);

    g_arg = STR * (2.0 * (Ve - Ea - D) + MP + 1122.48);
    l_acc += 0.16088 * sin(g_arg);

    g_arg = STR * (Ve - Ea - MP + 32.04);
    l_acc -= 0.15350 * sin(g_arg);

    g_arg = STR * (Ea - Ju - MP + 4488.88);
    l_acc += 0.14346 * sin(g_arg);

    g_arg = STR * (2.0 * (Ve - Ea + D) - MP - 8.64);
    l_acc += 0.13594 * sin(g_arg);

    g_arg = STR * (2.0 * (Ve - Ea - D) + 1319.76);
    l_acc += 0.13432 * sin(g_arg);

    g_arg = STR * (Ve - Ea - 2.0 * D + MP - 56.16);
    l_acc -= 0.13122 * sin(g_arg);

    g_arg = STR * (Ve - Ea + MP + 54.36);
    l_acc -= 0.12722 * sin(g_arg);

    g_arg = STR * (3.0 * (Ve - Ea) - MP + 433.8);
    l_acc += 0.12539 * sin(g_arg);

    g_arg = STR * (Ea - Ju + MP + 4002.12);
    l_acc += 0.10994 * sin(g_arg);

    g_arg = STR * (20.0 * Ve - 21.0 * Ea - 2.0 * D + MP - 317511.72);
    l_acc += 0.10652 * sin(g_arg);

    g_arg = STR * (26.0 * Ve - 29.0 * Ea - MP + 270002.52);
    l_acc += 0.10490 * sin(g_arg);

    g_arg = STR * (3.0 * Ve - 4.0 * Ea + D - MP - 322765.56);
    l_acc += 0.10386 * sin(g_arg);

    /* ---- Main perturbation table (118 terms) + final assembly ---- */
    moonpol0 = 0.0;
    accum_series(moon_lr, MOON_LR_N, 4, 1, &moonpol0);
    l_acc += (((l4 * T + l3) * T + l2) * T + l1) * T * 1.0e-5;
    moonpol0 = LP + l_acc + 1.0e-4 * moonpol0;

    return STR * mods3600(moonpol0);
}

/* ================================================================
 * Public interface: tropical lunar longitude in degrees [0, 360)
 * ================================================================ */
double moshier_lunar_longitude(double jd_ut)
{
    double lon_rad, lon_deg;

    double jd_tt = jd_ut + moshier_delta_t(jd_ut);
    T = (jd_tt - J2000) / 36525.0;
    T2 = T * T;

    mean_elements();
    mean_elements_pl();
    lon_rad = lunar_perturbations();

    /* Convert to degrees */
    lon_deg = lon_rad * (180.0 / M_PI);

    /* Distance-dependent light-time correction for the Moon.
     *
     * Light-time retardation uses actual distance and velocity.
     * For the Moon-Sun elongation (tithi), annual aberration cancels between
     * Moon and Sun; the residual is the Moon's geocentric light-time shift.
     *
     * By Kepler's 2nd law (r²ω = h = const), the angular shift is:
     *   Δlon = -(r/c) × (h/r²) = -h/(c×r)  ∝  1/r
     *
     * Mean correction: r_mean/c × ω_mean = 0.000196° = 0.706"
     * We scale by (r_mean / r_actual) to account for eccentricity (±5.5%).
     *
     * Top 5 radius perturbation terms from DE404 LR table (km, cosine): */
    {
        double cos_l    = cos(STR * MP);
        double cos_2d_l = cos(STR * (2.0 * D - MP));
        double cos_2d   = cos(STR * (2.0 * D));
        double cos_2l   = cos(STR * (2.0 * MP));
        double cos_lp   = cos(STR * M_sun);

        double r_mean = 385000.529;  /* km */
        double delta_r = -20905.355 * cos_l
                       -  3699.111 * cos_2d_l
                       -  2955.968 * cos_2d
                       -   569.925 * cos_2l
                       +    48.888 * cos_lp;

        lon_deg -= 0.000196 * (r_mean / (r_mean + delta_r));
    }

    /* Apply nutation */
    lon_deg += moshier_nutation_longitude(jd_ut);

    /* Normalize to [0, 360) */
    lon_deg = fmod(lon_deg, 360.0);
    if (lon_deg < 0) lon_deg += 360.0;

    return lon_deg;
}
