/*
 * moshier_ayanamsa.c — Lahiri ayanamsa
 *
 * Replicates SE_SIDM_LAHIRI using the same algorithm as Swiss Ephemeris:
 *   1. Take the vernal point at the target date as Cartesian (1, 0, 0)
 *   2. Precess from target date → J2000 using current precession model
 *   3. Precess from J2000 → t0 (reference epoch) using IAU 1976 model
 *   4. Convert to ecliptic of t0, then to polar coordinates
 *   5. Ayanamsa = -longitude + ayan_t0
 *   6. Add correction for precession model difference
 *   7. Add nutation in longitude
 *
 * Constants (from sweph.h ayanamsa[] table, index 1 = SE_SIDM_LAHIRI):
 *   t0 = 2435553.5 JD (1956 September 22.0 TT)
 *   ayan_t0 = 23.250182778 - 0.004658035 = 23.245524743°
 *   prec_offset = SEMOD_PREC_IAU_1976
 */
#include "moshier.h"
#include <math.h>

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)
#define J2000   2451545.0

/* Lahiri reference epoch and ayanamsa */
#define LAHIRI_T0      2435553.5       /* JD of reference epoch (TT) */
#define LAHIRI_AYAN_T0 23.245524743    /* ayanamsa at t0 in degrees */

/* Forward declarations for helpers in moshier_sun.c */
extern double moshier_delta_t(double jd_ut);
extern double moshier_nutation_longitude(double jd_ut);
extern double moshier_mean_obliquity(double jd_ut);

/* IAU 1976 precession angles Z, z, theta in radians
 * T = Julian centuries from J2000.0 */
static void iau1976_precession_angles(double T, double *Z, double *z, double *theta)
{
    /* Lieske et al. (1977) — same constants as SE's precess_1() */
    *Z     = ((0.017998*T + 0.30188)*T + 2306.2181)*T * DEG2RAD / 3600.0;
    *z     = ((0.018203*T + 1.09468)*T + 2306.2181)*T * DEG2RAD / 3600.0;
    *theta = ((-0.041833*T - 0.42665)*T + 2004.3109)*T * DEG2RAD / 3600.0;
}

/* Precess Cartesian equatorial coordinates.
 * direction = +1: from J to J2000 (forward)
 * direction = -1: from J2000 to J (backward) */
static void precess_equatorial(double *x, double J, int direction)
{
    if (J == J2000) return;

    double T = (J - J2000) / 36525.0;
    double Z, z, theta;
    iau1976_precession_angles(T, &Z, &z, &theta);

    double costh = cos(theta), sinth = sin(theta);
    double cosZ = cos(Z), sinZ = sin(Z);
    double cosz = cos(z), sinz = sin(z);
    double A = cosZ * costh;
    double B = sinZ * costh;

    double r[3];
    if (direction > 0) {
        /* From J to J2000 */
        r[0] =  (A*cosz - sinZ*sinz)*x[0] + (A*sinz + sinZ*cosz)*x[1] + cosZ*sinth*x[2];
        r[1] = -(B*cosz + cosZ*sinz)*x[0] - (B*sinz - cosZ*cosz)*x[1] - sinZ*sinth*x[2];
        r[2] =          -sinth*cosz *x[0]           - sinth*sinz *x[1] +       costh*x[2];
    } else {
        /* From J2000 to J */
        r[0] =  (A*cosz - sinZ*sinz)*x[0] - (B*cosz + cosZ*sinz)*x[1] - sinth*cosz*x[2];
        r[1] =  (A*sinz + sinZ*cosz)*x[0] - (B*sinz - cosZ*cosz)*x[1] - sinth*sinz*x[2];
        r[2] =            cosZ*sinth*x[0]           - sinZ*sinth *x[1] +       costh*x[2];
    }
    x[0] = r[0]; x[1] = r[1]; x[2] = r[2];
}

/* Obliquity of the ecliptic at a given JD (TT), in radians.
 * IAU 1976 formula (Lieske 1979), used by SE for this purpose. */
static double obliquity_iau1976(double jd_tt)
{
    double T = (jd_tt - J2000) / 36525.0;
    /* Laskar's polynomial */
    double U = T / 100.0;
    double eps = 23.0 + 26.0/60.0 + 21.448/3600.0
        + (-4680.93*U - 1.55*U*U + 1999.25*U*U*U
           - 51.38*U*U*U*U - 249.67*U*U*U*U*U
           - 39.05*U*U*U*U*U*U + 7.12*U*U*U*U*U*U*U
           + 27.87*U*U*U*U*U*U*U*U + 5.79*U*U*U*U*U*U*U*U*U
           + 2.45*U*U*U*U*U*U*U*U*U*U) / 3600.0;
    return eps * DEG2RAD;
}

/* Rotate equatorial → ecliptic (by obliquity eps in radians) */
static void equatorial_to_ecliptic(double *x, double eps)
{
    double c = cos(eps), s = sin(eps);
    double y1 = c * x[1] + s * x[2];
    double z1 = -s * x[1] + c * x[2];
    x[1] = y1;
    x[2] = z1;
}

/* Cartesian → polar longitude (radians) */
static double cart_to_lon(double *x)
{
    return atan2(x[1], x[0]);
}

/* Default precession model correction.
 * SE uses Vondrak 2011 as default but Lahiri was defined with IAU 1976.
 * Since we only implement IAU 1976, no correction needed.
 * This function is a no-op but kept for documentation. */

double moshier_ayanamsa(double jd_ut)
{
    /* Convert UT to TT (SE uses TT internally for ayanamsa) */
    double jd_tt = jd_ut + moshier_delta_t(jd_ut);

    /* Step 1: Vernal point at target date = (1, 0, 0) in equatorial */
    double x[3] = {1.0, 0.0, 0.0};

    /* Step 2: Precess from target date to J2000 */
    precess_equatorial(x, jd_tt, +1);   /* J → J2000 */

    /* Step 3: Precess from J2000 to t0 (Lahiri reference epoch) */
    precess_equatorial(x, LAHIRI_T0, -1);  /* J2000 → t0 */

    /* Step 4: Convert to ecliptic of t0 */
    double eps_t0 = obliquity_iau1976(LAHIRI_T0);
    equatorial_to_ecliptic(x, eps_t0);

    /* Step 5: Get polar longitude */
    double lon = cart_to_lon(x) * RAD2DEG;

    /* Step 6: Ayanamsa = -longitude + initial value */
    double ayan = -lon + LAHIRI_AYAN_T0;

    /* Step 7: Add nutation in longitude */
    double dpsi = moshier_nutation_longitude(jd_ut);
    ayan += dpsi;

    /* Normalize to [0, 360) */
    ayan = fmod(ayan, 360.0);
    if (ayan < 0) ayan += 360.0;

    return ayan;
}
