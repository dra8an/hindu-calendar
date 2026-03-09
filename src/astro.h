/*
 * astro.h - Astronomical computation backend
 *
 * Low-level astronomical functions: solar/lunar positions, ayanamsa,
 * and sunrise/sunset.  All functions work with Julian Day in UT.
 *
 * Two backends are supported (selected at compile time):
 *   - Moshier (default): self-contained, ~2,000 lines, VSOP87 sun + DE404 moon
 *   - Swiss Ephemeris:   compile with USE_SWISSEPH=1, requires ephemeris files
 *
 * Both backends produce identical results for calendar purposes.
 * Precision vs Swiss Ephemeris: solar +/-1", lunar +/-0.07", sunrise +/-2s.
 *
 * Must call astro_init() before any other function, and astro_close()
 * when done.
 */
#ifndef ASTRO_H
#define ASTRO_H

#include "types.h"

/*
 * astro_init - Initialize the ephemeris backend.
 *
 *   ephe_path: Path to Swiss Ephemeris data files, or NULL.
 *              Ignored when using the Moshier backend.
 *
 * Sets Lahiri (Chitrapaksha) ayanamsa.  Must be called once before
 * using any other function in this library.
 */
void astro_init(const char *ephe_path);

/*
 * astro_close - Release ephemeris backend resources.
 *
 * Call once when done with all calendar computations.
 */
void astro_close(void);

/*
 * solar_longitude - Tropical (Sayana) solar longitude.
 *
 *   jd_ut: Julian Day in Universal Time.
 *   Returns: degrees [0, 360).
 *
 * Tropical longitude is used for tithi calculation because the ayanamsa
 * cancels in the moon-sun difference.
 */
double solar_longitude(double jd_ut);

/*
 * lunar_longitude - Tropical (Sayana) lunar longitude.
 *
 *   jd_ut: Julian Day in Universal Time.
 *   Returns: degrees [0, 360).
 */
double lunar_longitude(double jd_ut);

/*
 * solar_longitude_sidereal - Sidereal (Nirayana) solar longitude.
 *
 *   jd_ut: Julian Day in Universal Time.
 *   Returns: degrees [0, 360).
 *
 * Sidereal longitude = tropical - ayanamsa.  Used for rashi (zodiac
 * sign) determination and solar calendar sankranti finding.
 */
double solar_longitude_sidereal(double jd_ut);

/*
 * get_ayanamsa - Lahiri ayanamsa at a given moment.
 *
 *   jd_ut: Julian Day in Universal Time.
 *   Returns: degrees (typically ~23-25 for modern dates).
 *
 * This is the MEAN ayanamsa (without nutation).  Nutation cancels
 * in the sidereal position: sid = (trop + dpsi) - (ayan + dpsi).
 */
double get_ayanamsa(double jd_ut);

/*
 * sunrise_jd - Julian Day of sunrise.
 *
 *   jd_ut: JD at 0h UT of the desired local date.
 *          For a Gregorian date, use gregorian_to_jd().
 *   loc:   Observer location (lat, lon, alt, utc_offset).
 *   Returns: JD (UT) of upper-limb sunrise, or 0 on error.
 *
 * Uses Sinclair refraction formula and accounts for solar semi-diameter
 * (upper limb), matching drikpanchang.com sunrise times to +/-45 seconds.
 */
double sunrise_jd(double jd_ut, const Location *loc);

/*
 * sunset_jd - Julian Day of sunset.
 *
 *   jd_ut: JD at 0h UT of the desired local date.
 *   loc:   Observer location.
 *   Returns: JD (UT) of upper-limb sunset, or 0 on error.
 */
double sunset_jd(double jd_ut, const Location *loc);

#endif /* ASTRO_H */
