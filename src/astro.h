#ifndef ASTRO_H
#define ASTRO_H

#include "types.h"

/* Initialize ephemeris backend (Lahiri ayanamsa, optional ephemeris path) */
void astro_init(const char *ephe_path);

/* Close ephemeris backend */
void astro_close(void);

/* Tropical (Sayana) solar longitude (degrees, 0-360) at Julian Day UT.
 * Used for tithi calculation (ayanamsa cancels in moon-sun difference). */
double solar_longitude(double jd_ut);

/* Tropical (Sayana) lunar longitude (degrees, 0-360) at Julian Day UT. */
double lunar_longitude(double jd_ut);

/* Sidereal (Nirayana) solar longitude (degrees, 0-360) at Julian Day UT.
 * Used for rashi/month determination. */
double solar_longitude_sidereal(double jd_ut);

/* Ayanamsa value (degrees) at a given Julian Day (UT) */
double get_ayanamsa(double jd_ut);

/* Julian Day of sunrise for a given JD and location.
 * jd_ut should be Julian Day at 0h UT of the local date.
 * Returns 0 on error. */
double sunrise_jd(double jd_ut, const Location *loc);

/* Julian Day of sunset for a given JD and location.
 * Returns 0 on error. */
double sunset_jd(double jd_ut, const Location *loc);

#endif /* ASTRO_H */
