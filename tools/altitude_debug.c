/* Debug: compute solar altitude at SE's sunrise time using our functions.
 * This reveals whether the ~13s offset is from position error or algorithm error. */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

extern double moshier_solar_declination(double jd_ut);
extern double moshier_solar_ra(double jd_ut);
extern double moshier_julday(int year, int month, int day, double hour);
extern double moshier_sunrise(double jd_ut, double lon, double lat, double alt);
extern double moshier_nutation_longitude(double jd_ut);
extern double moshier_mean_obliquity(double jd_ut);

extern int swe_calc_ut(double tjd_ut, int ipl, int iflag, double *xx, char *serr);
extern double swe_sidtime(double tjd_ut);
extern int swe_rise_trans(double tjd_ut, int ipl, char *starname, int epheflag,
                          int rsmi, double *geopos, double atpress, double attemp,
                          double *tret, char *serr);
extern void swe_set_ephe_path(char *path);
extern void swe_close(void);
#define SE_SUN 0
#define SEFLG_MOSEPH 4
#define SE_CALC_RISE 1
#define SE_BIT_DISC_CENTER 256

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

#define LAT  28.6139
#define LON  77.2090

static double normalize_deg(double d) {
    d = fmod(d, 360.0);
    if (d < 0) d += 360.0;
    return d;
}

/* Compute MEAN sidereal time at any JD (Meeus eq. 12.3) */
static double mean_sidereal_time(double jd_ut) {
    double D = jd_ut - 2451545.0;
    double T = D / 36525.0;
    double T2 = T * T, T3 = T2 * T;
    /* Meeus eq. 12.3 (for any instant, not just 0h UT) */
    double theta = 280.46061837 + 360.98564736629*D + 0.000387933*T2 - T3/38710000.0;
    return normalize_deg(theta);
}

/* Compute solar altitude using our functions */
static double solar_altitude_our(double jd_ut, double lon, double lat) {
    double phi = lat * DEG2RAD;
    double ra = moshier_solar_ra(jd_ut);
    double decl = moshier_solar_declination(jd_ut);

    /* Mean sidereal time at this JD (our current approach) */
    double gmst = mean_sidereal_time(jd_ut);
    double H = normalize_deg(gmst + lon - ra);
    if (H > 180.0) H -= 360.0;

    double sin_h = sin(phi) * sin(decl * DEG2RAD)
                 + cos(phi) * cos(decl * DEG2RAD) * cos(H * DEG2RAD);
    return asin(sin_h) * RAD2DEG;
}

/* Compute solar altitude using apparent sidereal time */
static double solar_altitude_our_gast(double jd_ut, double lon, double lat) {
    double phi = lat * DEG2RAD;
    double ra = moshier_solar_ra(jd_ut);
    double decl = moshier_solar_declination(jd_ut);

    /* Apparent sidereal time = GMST + equation of equinoxes */
    double gmst = mean_sidereal_time(jd_ut);
    double dpsi = moshier_nutation_longitude(jd_ut);  /* degrees */
    double eps = moshier_mean_obliquity(jd_ut);  /* degrees */
    double eqeq = dpsi * cos(eps * DEG2RAD);  /* equation of equinoxes */
    double gast = gmst + eqeq;

    double H = normalize_deg(gast + lon - ra);
    if (H > 180.0) H -= 360.0;

    double sin_h = sin(phi) * sin(decl * DEG2RAD)
                 + cos(phi) * cos(decl * DEG2RAD) * cos(H * DEG2RAD);
    return asin(sin_h) * RAD2DEG;
}

/* Compute solar altitude using SE's sidereal time */
static double solar_altitude_se_st(double jd_ut, double lon, double lat) {
    double phi = lat * DEG2RAD;
    double ra = moshier_solar_ra(jd_ut);
    double decl = moshier_solar_declination(jd_ut);

    /* SE's sidereal time (degrees) */
    double gast_se = swe_sidtime(jd_ut) * 15.0;

    double H = normalize_deg(gast_se + lon - ra);
    if (H > 180.0) H -= 360.0;

    double sin_h = sin(phi) * sin(decl * DEG2RAD)
                 + cos(phi) * cos(decl * DEG2RAD) * cos(H * DEG2RAD);
    return asin(sin_h) * RAD2DEG;
}

static void check_date(int y, int m, int d) {
    char serr[256];
    double geopos[3] = {LON, LAT, 0};

    double jd = moshier_julday(y, m, d, 0.0);
    double jd_start = jd - 5.5 / 24.0;

    /* SE sunrise */
    double se_sr;
    swe_rise_trans(jd_start, SE_SUN, NULL, SEFLG_MOSEPH,
                   SE_CALC_RISE | SE_BIT_DISC_CENTER, geopos, 0, 0, &se_sr, serr);

    /* Our sunrise */
    double our_sr = moshier_sunrise(jd_start, LON, LAT, 0.0);

    double sr_diff = (our_sr - se_sr) * 86400.0;

    /* Compute altitude at SE's sunrise using our functions */
    double alt_our_at_se = solar_altitude_our(se_sr, LON, LAT);
    double alt_gast_at_se = solar_altitude_our_gast(se_sr, LON, LAT);
    double alt_se_st_at_se = solar_altitude_se_st(se_sr, LON, LAT);

    /* Compute altitude at our sunrise using our functions */
    double alt_our_at_our = solar_altitude_our(our_sr, LON, LAT);
    double alt_gast_at_our = solar_altitude_our_gast(our_sr, LON, LAT);

    printf("%04d-%02d-%02d  sr_diff=%+.1fs\n", y, m, d, sr_diff);
    printf("  At SE sunrise (JD %.6f):\n", se_sr);
    printf("    Our GMST alt: %+.5f°  (should be -0.5667°)\n", alt_our_at_se);
    printf("    Our GAST alt: %+.5f°  (with eq. equinoxes)\n", alt_gast_at_se);
    printf("    SE-ST alt:    %+.5f°  (using swe_sidtime)\n", alt_se_st_at_se);
    printf("  At our sunrise (JD %.6f):\n", our_sr);
    printf("    Our GMST alt: %+.5f°  (converged to h0?)\n", alt_our_at_our);
    printf("    Our GAST alt: %+.5f°\n", alt_gast_at_our);
    printf("\n");
}

int main(void) {
    swe_set_ephe_path(NULL);

    check_date(1906, 1, 2);
    check_date(1928, 11, 20);
    check_date(1972, 5, 5);
    check_date(1973, 5, 14);
    check_date(2009, 8, 28);
    check_date(2015, 8, 19);

    swe_close();
    return 0;
}
