/*
 * moshier.h — Self-contained astronomical ephemeris library
 *
 * Replacement for Swiss Ephemeris using Meeus/VSOP87 algorithms.
 * Provides the 8 SE functions needed by src/astro.c and src/date_utils.c.
 *
 * Precision (1900-2100):
 *   Solar longitude: ~1 arcsecond
 *   Lunar longitude: ~10 arcseconds
 *   Ayanamsa: matches SE_SIDM_LAHIRI (IAU 1976 precession)
 *   Sunrise/sunset: ~1 minute
 *   JD conversion: exact
 */
#ifndef MOSHIER_H
#define MOSHIER_H

/* JD <-> Gregorian (Meeus Ch.7) */
double moshier_julday(int year, int month, int day, double hour);
void   moshier_revjul(double jd, int *year, int *month, int *day, double *hour);
int    moshier_day_of_week(double jd);  /* 0=Mon..6=Sun (SE convention) */

/* Tropical solar longitude in degrees [0,360) */
double moshier_solar_longitude(double jd_ut);

/* Tropical lunar longitude in degrees [0,360) */
double moshier_lunar_longitude(double jd_ut);

/* Lahiri ayanamsa in degrees (matches SE_SIDM_LAHIRI) */
double moshier_ayanamsa(double jd_ut);

/* Sunrise/sunset — returns JD (UT), disc center with refraction */
double moshier_sunrise(double jd_ut, double lon, double lat, double alt);
double moshier_sunset(double jd_ut, double lon, double lat, double alt);

#endif /* MOSHIER_H */
