/*
 * moshier_rise.c — Sunrise and sunset calculation
 *
 * Algorithm from Meeus, "Astronomical Algorithms", 2nd ed., Ch. 15.
 * Iterative method using hour angle computation.
 *
 * Configuration: disc center with atmospheric refraction (SE_BIT_DISC_CENTER)
 *   h₀ = -0.5667° (center of disc at geometric horizon, standard refraction)
 *
 * Precision: ~1 minute (sufficient for Hindu calendar)
 */
#include "moshier.h"
#include <math.h>

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

/* Forward declarations for helpers in moshier_sun.c */
extern double moshier_solar_declination(double jd_ut);
extern double moshier_solar_ra(double jd_ut);

static double normalize_deg(double d)
{
    d = fmod(d, 360.0);
    if (d < 0) d += 360.0;
    return d;
}

/* Depression angle for disc center with standard atmospheric refraction */
static const double H0_CENTER = -0.5667;  /* degrees */

/* Apparent sidereal time at Greenwich at 0h UT, in degrees (Meeus Ch. 12) */
static double sidereal_time_0h(double jd_0h)
{
    double T = (jd_0h - 2451545.0) / 36525.0;
    double T2 = T * T;
    double T3 = T2 * T;
    /* Meeus eq. 12.4 — θ₀ in degrees */
    double theta = 100.46061837 + 36000.770053608*T + 0.000387933*T2 - T3/38710000.0;
    return normalize_deg(theta);
}

/* Compute rise or set for a specific UT date (jd_0h = JD at 0h UT).
 * Returns JD (UT) of the event, or 0 on error (circumpolar). */
static double rise_set_for_date(double jd_0h, double lon, double lat, double h0, int is_rise)
{
    double phi = lat * DEG2RAD;

    /* Compute apparent sidereal time at 0h UT */
    double theta0 = sidereal_time_0h(jd_0h);

    /* Initial estimate using noon position */
    double jd_noon = jd_0h + 0.5;
    double ra = moshier_solar_ra(jd_noon);
    double decl = moshier_solar_declination(jd_noon);

    /* Hour angle (Meeus eq. 15.1) */
    double cos_H0 = (sin(h0 * DEG2RAD) - sin(phi) * sin(decl * DEG2RAD))
                  / (cos(phi) * cos(decl * DEG2RAD));

    /* Check for circumpolar conditions */
    if (cos_H0 < -1.0 || cos_H0 > 1.0) {
        return 0.0;
    }

    double H0_deg = acos(cos_H0) * RAD2DEG;

    /* Approximate transit time as fraction of day.
     * Transit: local sidereal time = RA, so GMST + 360.985647*m0 = RA - lon_east
     * m0 ≈ (RA - lon_east - GMST_0h) / 360 */
    double m0 = (ra - lon - theta0) / 360.0;
    m0 = m0 - floor(m0);

    /* Rise and set times */
    double m;
    if (is_rise)
        m = m0 - H0_deg / 360.0;
    else
        m = m0 + H0_deg / 360.0;

    /* Normalize to [0, 1) */
    m = m - floor(m);

    /* Iterate to refine (Meeus p. 103) */
    for (int iter = 0; iter < 5; iter++) {
        double jd_trial = jd_0h + m;

        /* Recompute solar position at trial time */
        double ra_i = moshier_solar_ra(jd_trial);
        double decl_i = moshier_solar_declination(jd_trial);

        /* Local sidereal time */
        double theta = theta0 + 360.985647 * m;

        /* Local hour angle */
        double H = normalize_deg(theta + lon - ra_i);
        if (H > 180.0) H -= 360.0;

        /* Altitude of sun */
        double sin_h = sin(phi) * sin(decl_i * DEG2RAD)
                     + cos(phi) * cos(decl_i * DEG2RAD) * cos(H * DEG2RAD);
        double h = asin(sin_h) * RAD2DEG;

        /* Correction */
        double denom = 360.0 * cos(decl_i * DEG2RAD) * cos(phi) * sin(H * DEG2RAD);
        if (fabs(denom) < 1e-12) break;
        double dm = (h - h0) / denom;
        m += dm;

        if (fabs(dm) < 0.00001) break;  /* ~0.9 seconds */
    }

    /* Clamp to reasonable range */
    if (m < 0) m += 1.0;
    if (m > 1) m -= 1.0;

    return jd_0h + m;
}

/* Compute rise or set, searching forward from jd_ut.
 * Matches SE behavior: finds the next event after the given JD. */
static double rise_set(double jd_ut, double lon, double lat, double alt, int is_rise)
{
    /* Adjust h0 for altitude above sea level (Meeus p. 102) */
    double h0 = H0_CENTER;
    if (alt > 0) {
        h0 -= 0.0353 * sqrt(alt);
    }

    /* Get 0h UT of the UT date containing jd_ut */
    int yr, mo, dy;
    double hr;
    moshier_revjul(jd_ut, &yr, &mo, &dy, &hr);
    double jd_0h = moshier_julday(yr, mo, dy, 0.0);

    /* Try current date first */
    double result = rise_set_for_date(jd_0h, lon, lat, h0, is_rise);
    if (result > 0 && result >= jd_ut - 0.0001) {
        return result;
    }

    /* Event already passed — try next day */
    result = rise_set_for_date(jd_0h + 1.0, lon, lat, h0, is_rise);
    return result;
}

double moshier_sunrise(double jd_ut, double lon, double lat, double alt)
{
    return rise_set(jd_ut, lon, lat, alt, 1);
}

double moshier_sunset(double jd_ut, double lon, double lat, double alt)
{
    return rise_set(jd_ut, lon, lat, alt, 0);
}
