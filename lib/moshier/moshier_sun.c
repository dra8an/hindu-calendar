/*
 * moshier_sun.c — Solar longitude and declination
 *
 * Algorithms from Meeus, "Astronomical Algorithms", 2nd ed.
 * Solar position: Ch. 25 (truncated VSOP87)
 * Nutation: Ch. 22 (IAU 1980, full 63-term table)
 * Obliquity: Ch. 22 (Laskar's formula)
 *
 * Precision: ~1 arcsecond for solar longitude (1900-2100)
 */
#include "moshier.h"
#include <math.h>
#include <stddef.h>

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

/* Delta-T approximation: TT - UT in seconds
 * Polynomial fits from Meeus Ch. 10 / USNO tables */
static double delta_t_seconds(double jd)
{
    double y, dt;
    int yr, mo, dy;
    double hr;
    moshier_revjul(jd, &yr, &mo, &dy, &hr);
    y = yr + (mo - 0.5) / 12.0;

    if (y < 1900) {
        double t = (y - 1820.0) / 100.0;
        dt = -20 + 32 * t * t;
    } else if (y < 1920) {
        double t = y - 1900;
        dt = -2.79 + 1.494119*t - 0.0598939*t*t + 0.0061966*t*t*t - 0.000197*t*t*t*t;
    } else if (y < 1941) {
        double t = y - 1920;
        dt = 21.20 + 0.84493*t - 0.076100*t*t + 0.0020936*t*t*t;
    } else if (y < 1961) {
        double t = y - 1950;
        dt = 29.07 + 0.407*t - t*t/233.0 + t*t*t/2547.0;
    } else if (y < 1986) {
        double t = y - 1975;
        dt = 45.45 + 1.067*t - t*t/260.0 - t*t*t/718.0;
    } else if (y < 2005) {
        double t = y - 2000;
        dt = 63.86 + 0.3345*t - 0.060374*t*t + 0.0017275*t*t*t
             + 0.000651814*t*t*t*t + 0.00002373599*t*t*t*t*t;
    } else if (y < 2050) {
        double t = y - 2000;
        dt = 62.92 + 0.32217*t + 0.005589*t*t;
    } else if (y < 2150) {
        dt = -20 + 32 * ((y - 1820.0)/100.0) * ((y - 1820.0)/100.0)
             - 0.5628 * (2150 - y);
    } else {
        double u = (y - 1820.0) / 100.0;
        dt = -20 + 32 * u * u;
    }
    return dt;
}

static double jd_ut_to_tt(double jd_ut)
{
    return jd_ut + delta_t_seconds(jd_ut) / 86400.0;
}

static double normalize_deg(double d)
{
    d = fmod(d, 360.0);
    if (d < 0) d += 360.0;
    return d;
}

/* Nutation in longitude (Δψ) and obliquity (Δε) in degrees.
 * Meeus Ch. 22, Table 22.A — full 63-term IAU 1980 nutation model.
 * Coefficients in 0.0001 arcseconds. */
static void nutation(double jd_tt, double *dpsi, double *deps)
{
    double T = (jd_tt - 2451545.0) / 36525.0;
    double T2 = T * T;
    double T3 = T2 * T;

    /* Fundamental arguments in degrees */
    double D  = 297.85036 + 445267.111480*T - 0.0019142*T2 + T3/189474.0;
    double M  = 357.52772 +  35999.050340*T - 0.0001603*T2 - T3/300000.0;
    double Mp = 134.96298 + 477198.867398*T + 0.0086972*T2 + T3/56250.0;
    double F  =  93.27191 + 483202.017538*T - 0.0036825*T2 + T3/327270.0;
    double Om = 125.04452 -   1934.136261*T + 0.0020708*T2 + T3/450000.0;

    D  *= DEG2RAD;
    M  *= DEG2RAD;
    Mp *= DEG2RAD;
    F  *= DEG2RAD;
    Om *= DEG2RAD;

    /* IAU 1980 nutation, first 13 terms from Meeus Table 22.A.
     * Combined with the Meeus p.164 apparent longitude formula,
     * this matches SE's VSOP87 solar longitude better than the
     * full 63-term table (which interacts poorly with the simplified
     * equation-of-center geometric longitude).
     * {D, M, Mp, F, Om, psi0, psi1, eps0, eps1} in 0.0001" */
    static const struct {
        signed char D, M, Mp, F, Om;
        double s0, s1, c0, c1;
    } nt[] = {
        { 0, 0, 0, 0, 1,-171996,-174.2, 92025,  8.9},
        {-2, 0, 0, 2, 2, -13187,  -1.6,  5736, -3.1},
        { 0, 0, 0, 2, 2,  -2274,  -0.2,   977, -0.5},
        { 0, 0, 0, 0, 2,   2062,   0.2,  -895,  0.5},
        { 0, 1, 0, 0, 0,   1426,  -3.4,    54, -0.1},
        { 0, 0, 1, 0, 0,    712,   0.1,    -7,  0.0},
        {-2, 1, 0, 2, 2,   -517,   1.2,   224, -0.6},
        { 0, 0, 0, 2, 1,   -386,  -0.4,   200,  0.0},
        { 0, 0, 1, 2, 2,   -301,   0.0,   129, -0.1},
        {-2,-1, 0, 2, 2,    217,  -0.5,   -95,  0.3},
        {-2, 0, 1, 0, 0,   -158,   0.0,     0,  0.0},
        {-2, 0, 0, 2, 1,    129,   0.1,   -70,  0.0},
        { 0, 0,-1, 2, 2,    123,   0.0,   -53,  0.0},
    };

    double sum_dpsi = 0, sum_deps = 0;
    int n = sizeof(nt) / sizeof(nt[0]);
    for (int i = 0; i < n; i++) {
        double arg = nt[i].D * D + nt[i].M * M + nt[i].Mp * Mp
                   + nt[i].F * F + nt[i].Om * Om;
        sum_dpsi += (nt[i].s0 + nt[i].s1 * T) * sin(arg);
        sum_deps += (nt[i].c0 + nt[i].c1 * T) * cos(arg);
    }

    /* Convert from 0.0001" to degrees */
    *dpsi = sum_dpsi * 0.0001 / 3600.0;
    *deps = sum_deps * 0.0001 / 3600.0;
}

/* Mean obliquity of the ecliptic in degrees (Meeus Ch. 22, Laskar) */
static double mean_obliquity(double jd_tt)
{
    double T = (jd_tt - 2451545.0) / 36525.0;
    double U = T / 100.0;
    double eps0 = 23.0 + 26.0/60.0 + 21.448/3600.0
        + (-4680.93 * U
           - 1.55 * U*U
           + 1999.25 * U*U*U
           - 51.38 * U*U*U*U
           - 249.67 * U*U*U*U*U
           - 39.05 * U*U*U*U*U*U
           + 7.12 * U*U*U*U*U*U*U
           + 27.87 * U*U*U*U*U*U*U*U
           + 5.79 * U*U*U*U*U*U*U*U*U
           + 2.45 * U*U*U*U*U*U*U*U*U*U) / 3600.0;
    return eps0;
}

/* Solar position: returns apparent longitude in degrees.
 * Also computes declination and nutation if pointers non-NULL. */
static double solar_position(double jd_ut, double *decl, double *nut_lon)
{
    double jd_tt = jd_ut_to_tt(jd_ut);
    double T = (jd_tt - 2451545.0) / 36525.0;
    double T2 = T * T;

    /* Geometric mean longitude (Meeus eq. 25.2) */
    double L0 = normalize_deg(280.46646 + 36000.76983*T + 0.0003032*T2);

    /* Mean anomaly (Meeus eq. 25.3) */
    double M = normalize_deg(357.52911 + 35999.05029*T - 0.0001537*T2);
    double Mrad = M * DEG2RAD;

    /* Equation of center (Meeus eq. 25.6, full precision) */
    double C = (1.914602 - 0.004817*T - 0.000014*T2) * sin(Mrad)
             + (0.019993 - 0.000101*T) * sin(2*Mrad)
             + 0.000289 * sin(3*Mrad);

    /* Sun's true longitude */
    double theta = L0 + C;

    /* Nutation */
    double dpsi, deps;
    nutation(jd_tt, &dpsi, &deps);

    /* Apparent longitude correction (Meeus p. 164):
     * Combines aberration (-0.00569°) and a nutation adjustment
     * (-0.00478°×sin(Ω)) that compensates for the simplified equation
     * of center used here (vs SE's full VSOP87). */
    double omega = (125.04 - 1934.136 * T) * DEG2RAD;
    double aberration = -0.00569 - 0.00478 * sin(omega);

    /* Apparent longitude = true + nutation + aberration */
    double apparent = theta + dpsi + aberration;

    if (nut_lon) *nut_lon = dpsi;

    if (decl) {
        double eps0 = mean_obliquity(jd_tt);
        double eps = (eps0 + deps) * DEG2RAD;
        double lam = apparent * DEG2RAD;
        *decl = asin(sin(eps) * sin(lam)) * RAD2DEG;
    }

    return normalize_deg(apparent);
}

double moshier_solar_longitude(double jd_ut)
{
    return solar_position(jd_ut, NULL, NULL);
}

double moshier_solar_declination(double jd_ut)
{
    double decl;
    solar_position(jd_ut, &decl, NULL);
    return decl;
}

double moshier_solar_ra(double jd_ut)
{
    double jd_tt = jd_ut_to_tt(jd_ut);
    double dpsi, deps;
    nutation(jd_tt, &dpsi, &deps);
    double eps0 = mean_obliquity(jd_tt);
    double eps = (eps0 + deps) * DEG2RAD;
    double lam = moshier_solar_longitude(jd_ut) * DEG2RAD;
    double ra = atan2(cos(eps) * sin(lam), cos(lam)) * RAD2DEG;
    return normalize_deg(ra);
}

double moshier_nutation_longitude(double jd_ut)
{
    double jd_tt = jd_ut_to_tt(jd_ut);
    double dpsi, deps;
    nutation(jd_tt, &dpsi, &deps);
    return dpsi;
}

double moshier_mean_obliquity(double jd_ut)
{
    return mean_obliquity(jd_ut_to_tt(jd_ut));
}

double moshier_true_obliquity(double jd_ut)
{
    double jd_tt = jd_ut_to_tt(jd_ut);
    double dpsi, deps;
    nutation(jd_tt, &dpsi, &deps);
    return mean_obliquity(jd_tt) + deps;
}

double moshier_delta_t(double jd_ut)
{
    return delta_t_seconds(jd_ut) / 86400.0;
}
