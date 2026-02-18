/* Measure raw lunar longitude drift: our moshier vs SE geometric (TRUEPOS+NONUT) */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

extern double moshier_delta_t(double jd_ut);
extern double moshier_nutation_longitude(double jd_ut);

/* Minimal SE interface for comparison */
extern int swe_calc_ut(double tjd_ut, int ipl, int iflag, double *xx, char *serr);
extern void swe_set_ephe_path(char *path);
extern void swe_close(void);
#define SE_MOON 1
#define SEFLG_MOSEPH 4
#define SEFLG_TRUEPOS 256
#define SEFLG_NONUT (1024)

int main(void) {
    swe_set_ephe_path(NULL);
    char serr[256];
    double xx[6];

    printf("# Year  T_centuries  raw_diff_arcsec  apparent_diff_arcsec\n");

    /* Scan from 1900 to 2050 in steps of 10 years */
    for (int year = 1900; year <= 2050; year += 5) {
        /* Approximate JD for Jan 1 of each year */
        double jd = 2451545.0 + (year - 2000) * 365.25;
        double T = (jd + moshier_delta_t(jd) - 2451545.0) / 36525.0;

        /* Our raw lunar longitude (no nutation) */
        double our_lon = moshier_lunar_longitude(jd);
        double our_nut = moshier_nutation_longitude(jd);
        double our_raw = our_lon - our_nut;

        /* SE geometric (TRUEPOS+NONUT) */
        swe_calc_ut(jd, SE_MOON, SEFLG_MOSEPH | SEFLG_TRUEPOS | SEFLG_NONUT, xx, serr);
        double se_raw = xx[0];

        /* SE apparent */
        swe_calc_ut(jd, SE_MOON, SEFLG_MOSEPH, xx, serr);
        double se_app = xx[0];

        double raw_diff = (our_raw - se_raw) * 3600.0;  /* arcsec */
        double app_diff = (our_lon - se_app) * 3600.0;  /* arcsec */

        printf("%4d  %8.4f  %8.3f  %8.3f\n", year, T, raw_diff, app_diff);
    }

    swe_close();
    return 0;
}
