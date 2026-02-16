/*
 * moshier_moon.c — Lunar longitude
 *
 * ELP-2000/82 theory, 60 longitude terms from Meeus Ch. 47, Table 47.A.
 * Arguments: D (mean elongation), M (sun mean anomaly),
 *            Mp (moon mean anomaly), F (argument of latitude).
 * Coefficients in 0.000001 degrees.
 *
 * Precision: ~1 arcsecond (1900-2100).
 */
#include "moshier.h"
#include <math.h>

#define DEG2RAD (M_PI / 180.0)

/* Forward declarations for helpers in moshier_sun.c */
extern double moshier_delta_t(double jd_ut);
extern double moshier_nutation_longitude(double jd_ut);

static double normalize_deg(double d)
{
    d = fmod(d, 360.0);
    if (d < 0) d += 360.0;
    return d;
}

/* 60 longitude terms from Meeus Table 47.A (ELP-2000/82)
 * {D, M, Mp, F, coefficient_in_1e-6_degrees} */
static const struct { int D, M, Mp, F; long coeff; } lr_terms[] = {
    { 0, 0, 1, 0,  6288774},
    { 2, 0,-1, 0,  1274027},
    { 2, 0, 0, 0,   658314},
    { 0, 0, 2, 0,   213618},
    { 0, 1, 0, 0,  -185116},
    { 0, 0, 0, 2,  -114332},
    { 2, 0,-2, 0,    58793},
    { 2,-1,-1, 0,    57066},
    { 2, 0, 1, 0,    53322},
    { 2,-1, 0, 0,    45758},
    { 0, 1,-1, 0,   -40923},
    { 1, 0, 0, 0,   -34720},
    { 0, 1, 1, 0,   -30383},
    { 2, 0, 0,-2,    15327},
    { 0, 0, 1, 2,   -12528},
    { 0, 0, 1,-2,    10980},
    { 4, 0,-1, 0,    10675},
    { 0, 0, 3, 0,    10034},
    { 4, 0,-2, 0,     8548},
    { 2, 1,-1, 0,    -7888},
    { 2, 1, 0, 0,    -6766},
    { 1, 0,-1, 0,    -5163},
    { 1, 1, 0, 0,     4987},
    { 2,-1, 1, 0,     4036},
    { 2, 0, 2, 0,     3994},
    { 4, 0, 0, 0,     3861},
    { 2, 0,-3, 0,     3665},
    { 0, 1,-2, 0,    -2689},
    { 2, 0,-1, 2,    -2602},
    { 2,-1,-2, 0,     2390},
    { 1, 0, 1, 0,    -2348},
    { 2,-2, 0, 0,     2236},
    { 0, 1, 2, 0,    -2120},
    { 0, 2, 0, 0,    -2069},
    { 2,-2,-1, 0,     2048},
    { 2, 0, 1,-2,    -1773},
    { 2, 0, 0, 2,    -1595},
    { 4,-1,-1, 0,     1215},
    { 0, 0, 2, 2,    -1110},
    { 3, 0,-1, 0,     -892},
    { 2, 1, 1, 0,     -810},
    { 4,-1,-2, 0,      759},
    { 0, 2,-1, 0,     -713},
    { 2, 2,-1, 0,     -700},
    { 2, 1,-2, 0,      691},
    { 2,-1, 0,-2,      596},
    { 4, 0, 1, 0,      549},
    { 0, 0, 4, 0,      537},
    { 4,-1, 0, 0,      520},
    { 1, 0,-2, 0,     -487},
    { 2, 1, 0,-2,     -399},
    { 0, 0, 2,-2,     -381},
    { 1, 1, 1, 0,      351},
    { 3, 0,-2, 0,     -340},
    { 4, 0,-3, 0,      330},
    { 2,-1, 2, 0,      327},
    { 0, 2, 1, 0,     -323},
    { 1, 1,-1, 0,      299},
    { 2, 0, 3, 0,      294},
    { 2, 0,-1,-2,      0},
};
#define N_LR_TERMS 60

double moshier_lunar_longitude(double jd_ut)
{
    double jd_tt = jd_ut + moshier_delta_t(jd_ut);
    double T = (jd_tt - 2451545.0) / 36525.0;
    double T2 = T * T;
    double T3 = T2 * T;
    double T4 = T3 * T;

    /* Fundamental arguments (Meeus Ch. 47) */
    double Lp = normalize_deg(218.3164477 + 481267.88123421*T
                              - 0.0015786*T2 + T3/538841.0 - T4/65194000.0);
    double D = normalize_deg(297.8501921 + 445267.1114034*T
                             - 0.0018819*T2 + T3/545868.0 - T4/113065000.0);
    double M = normalize_deg(357.5291092 + 35999.0502909*T
                             - 0.0001536*T2 + T3/24490000.0);
    double Mp = normalize_deg(134.9633964 + 477198.8675055*T
                              + 0.0087414*T2 + T3/69699.0 - T4/14712000.0);
    double F = normalize_deg(93.2720950 + 483202.0175233*T
                             - 0.0036539*T2 - T3/3526000.0 + T4/863310000.0);

    /* Eccentricity correction (Meeus p. 338) */
    double E = 1.0 - 0.002516*T - 0.0000074*T2;
    double E2 = E * E;

    double Dr = D * DEG2RAD, Mr = M * DEG2RAD;
    double Mpr = Mp * DEG2RAD, Fr = F * DEG2RAD;

    /* Sum longitude terms (Meeus Table 47.A, 60 terms, 10^-6 degrees) */
    double sum_l = 0;
    for (int i = 0; i < N_LR_TERMS; i++) {
        double arg = lr_terms[i].D*Dr + lr_terms[i].M*Mr
                   + lr_terms[i].Mp*Mpr + lr_terms[i].F*Fr;
        double coeff = (double)lr_terms[i].coeff;
        int m_abs = lr_terms[i].M < 0 ? -lr_terms[i].M : lr_terms[i].M;
        if (m_abs == 1) coeff *= E;
        else if (m_abs == 2) coeff *= E2;
        sum_l += coeff * sin(arg);
    }

    /* Additional corrections (Meeus p. 338) */
    double A1 = normalize_deg(119.75 + 131.849*T);
    double A2 = normalize_deg(53.09 + 479264.290*T);
    sum_l += 3958 * sin(A1 * DEG2RAD);         /* Venus */
    sum_l += 1962 * sin((Lp - F) * DEG2RAD);   /* Flattening */
    sum_l += 318 * sin(A2 * DEG2RAD);           /* Jupiter */

    /* Convert from 10^-6 degrees to degrees */
    double lon = Lp + sum_l / 1000000.0;

    /* Apply nutation */
    lon += moshier_nutation_longitude(jd_ut);

    return normalize_deg(lon);
}
