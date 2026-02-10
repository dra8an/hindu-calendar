#include "astro.h"
#include "swephexp.h"
#include <math.h>
#include <stdio.h>

void astro_init(const char *ephe_path)
{
    if (ephe_path)
        swe_set_ephe_path((char *)ephe_path);
    /* Set Lahiri ayanamsa (same as drikpanchang.com) */
    swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
}

void astro_close(void)
{
    swe_close();
}

/* Internal: compute tropical longitude for a planet */
static double tropical_longitude(double jd_ut, int planet)
{
    double xx[6];
    char serr[256];
    int flags = SEFLG_MOSEPH;
    int ret = swe_calc_ut(jd_ut, planet, flags, xx, serr);
    if (ret < 0) {
        fprintf(stderr, "swe_calc_ut error (planet %d): %s\n", planet, serr);
        return -1.0;
    }
    double lon = fmod(xx[0], 360.0);
    if (lon < 0) lon += 360.0;
    return lon;
}

double solar_longitude(double jd_ut)
{
    return tropical_longitude(jd_ut, SE_SUN);
}

double lunar_longitude(double jd_ut)
{
    return tropical_longitude(jd_ut, SE_MOON);
}

double solar_longitude_sidereal(double jd_ut)
{
    double sayana = solar_longitude(jd_ut);
    if (sayana < 0) return sayana;
    double ayan = swe_get_ayanamsa_ut(jd_ut);
    double nirayana = fmod(sayana - ayan, 360.0);
    if (nirayana < 0) nirayana += 360.0;
    return nirayana;
}

double get_ayanamsa(double jd_ut)
{
    return swe_get_ayanamsa_ut(jd_ut);
}

double sunrise_jd(double jd_ut, const Location *loc)
{
    double geopos[3];
    double trise;
    char serr[256];

    geopos[0] = loc->longitude;
    geopos[1] = loc->latitude;
    geopos[2] = loc->altitude;

    /* Hindu sunrise: center of disc at horizon.
     * Matches the Python drik-panchanga reference. */
    int rsmi = SE_CALC_RISE | SE_BIT_DISC_CENTER;
    int ret = swe_rise_trans(jd_ut - loc->utc_offset / 24.0,
                             SE_SUN, NULL,
                             SEFLG_MOSEPH, rsmi,
                             geopos, 0.0, 0.0,
                             &trise, serr);
    if (ret < 0) {
        fprintf(stderr, "swe_rise_trans (sunrise) error: %s\n", serr);
        return 0.0;
    }
    return trise;
}

double sunset_jd(double jd_ut, const Location *loc)
{
    double geopos[3];
    double tset;
    char serr[256];

    geopos[0] = loc->longitude;
    geopos[1] = loc->latitude;
    geopos[2] = loc->altitude;

    int rsmi = SE_CALC_SET | SE_BIT_DISC_CENTER;
    int ret = swe_rise_trans(jd_ut - loc->utc_offset / 24.0,
                             SE_SUN, NULL,
                             SEFLG_MOSEPH, rsmi,
                             geopos, 0.0, 0.0,
                             &tset, serr);
    if (ret < 0) {
        fprintf(stderr, "swe_rise_trans (sunset) error: %s\n", serr);
        return 0.0;
    }
    return tset;
}
