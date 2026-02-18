/* Diagnostic: measure elongation error at tithi boundary for failing dates.
 * Shows how close each failing date is to a tithi boundary using both
 * moshier and SE backends, and how much correction would be needed. */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

/* Our moshier functions */
extern double moshier_lunar_longitude(double jd_ut);
extern double moshier_solar_longitude(double jd_ut);
extern double moshier_julday(int year, int month, int day, double hour);
extern double moshier_sunrise(double jd_ut, double lon, double lat, double alt);

/* SE functions */
extern int swe_calc_ut(double tjd_ut, int ipl, int iflag, double *xx, char *serr);
extern void swe_set_ephe_path(char *path);
extern void swe_close(void);
extern int swe_rise_trans(double tjd_ut, int ipl, char *starname, int epheflag,
                          int rsmi, double *geopos, double atpress, double attemp,
                          double *tret, char *serr);
#define SE_SUN   0
#define SE_MOON  1
#define SEFLG_MOSEPH 4
#define SE_CALC_RISE     1
#define SE_BIT_DISC_CENTER 256

static double circ_diff(double a, double b) {
    double d = a - b;
    while (d > 180.0) d -= 360.0;
    while (d < -180.0) d += 360.0;
    return d;
}

/* New Delhi coordinates */
#define LAT  28.6139
#define LON  77.2090

static void check_date(int y, int m, int d, int exp_tithi) {
    char serr[256];
    double xx[6];
    double geopos[3] = {LON, LAT, 0};

    /* Our moshier sunrise (start from midnight IST, like astro.c) */
    double jd = moshier_julday(y, m, d, 0.0);
    double jd_start = jd - 5.5 / 24.0;  /* midnight IST */
    double sr = moshier_sunrise(jd_start, LON, LAT, 0.0);

    /* Our moshier elongation at sunrise */
    double our_moon = moshier_lunar_longitude(sr);
    double our_sun  = moshier_solar_longitude(sr);
    double our_elong = circ_diff(our_moon, our_sun);
    if (our_elong < 0) our_elong += 360.0;
    int our_tithi = (int)floor(our_elong / 12.0) + 1;

    /* SE sunrise (same starting point) */
    double se_sr;
    swe_rise_trans(jd_start, SE_SUN, NULL, SEFLG_MOSEPH,
                   SE_CALC_RISE | SE_BIT_DISC_CENTER, geopos, 0, 0, &se_sr, serr);

    /* SE elongation at SE sunrise */
    swe_calc_ut(se_sr, SE_MOON, SEFLG_MOSEPH, xx, serr);
    double se_moon = xx[0];
    swe_calc_ut(se_sr, SE_SUN, SEFLG_MOSEPH, xx, serr);
    double se_sun = xx[0];
    double se_elong = circ_diff(se_moon, se_sun);
    if (se_elong < 0) se_elong += 360.0;
    int se_tithi = (int)floor(se_elong / 12.0) + 1;

    /* Distance to nearest tithi boundary */
    double our_in_tithi = fmod(our_elong, 12.0);  /* 0-12 within current tithi */
    double se_in_tithi = fmod(se_elong, 12.0);

    /* Elongation difference */
    double elong_diff = (our_elong - se_elong) * 3600.0;  /* arcsec */

    /* Moon and sun differences */
    swe_calc_ut(sr, SE_MOON, SEFLG_MOSEPH, xx, serr);
    double moon_diff = circ_diff(our_moon, xx[0]) * 3600.0;
    swe_calc_ut(sr, SE_SUN, SEFLG_MOSEPH, xx, serr);
    double sun_diff = circ_diff(our_sun, xx[0]) * 3600.0;

    /* Sunrise difference */
    double sr_diff = (sr - se_sr) * 86400.0;  /* seconds */

    printf("%04d-%02d-%02d  exp=%2d our=%2d se=%2d  "
           "elong_diff=%+.3f\"  moon_diff=%+.3f\"  sun_diff=%+.3f\"  "
           "sr_diff=%+.1fs  our_in=%.3f\"  se_in=%.3f\"\n",
           y, m, d, exp_tithi, our_tithi, se_tithi,
           elong_diff, moon_diff, sun_diff, sr_diff,
           our_in_tithi * 3600.0, se_in_tithi * 3600.0);
}

int main(void) {
    swe_set_ephe_path(NULL);

    printf("# Date       exp our se   elong_diff   moon_diff    sun_diff     sr_diff   our_in_tithi  se_in_tithi\n");
    printf("# (in_tithi = distance from start of tithi in arcsec; close to 0 or 43200 = near boundary)\n\n");

    /* All 25 failing dates */
    /* 21 from test_adhika_kshaya */
    check_date(1905, 5, 19, 16);
    check_date(1906, 1,  2,  7);
    check_date(1907, 5, 20,  7);
    check_date(1908, 5, 19, 19);
    check_date(1924, 5, 19, 16);
    check_date(1928,11, 20,  7);
    check_date(1930, 5, 19, 21);
    check_date(1940, 5, 19, 13);
    check_date(1959, 5, 20, 13);
    check_date(1972, 5,  5, 21);
    check_date(1972, 5, 26, 13);
    check_date(1973, 5, 14, 12);
    check_date(1981, 5, 19, 15);
    check_date(1999, 5, 19,  5);
    check_date(2005, 5, 19, 10);
    check_date(2009, 8, 28,  8);
    check_date(2015, 8, 19,  4);
    check_date(2018, 5, 19,  5);
    check_date(2028, 5, 18, 24);
    check_date(2034, 5, 19,  2);
    check_date(2050, 5, 19, 29);

    /* 4 from test_csv_regression */
    check_date(1927, 5, 20, 19);
    check_date(1928,11, 20,  7);  /* duplicate */
    check_date(1950, 5, 19,  2);
    check_date(2009, 5, 19, 25);

    swe_close();
    return 0;
}
