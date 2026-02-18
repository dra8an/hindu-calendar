/* Compare delta-T between our moshier library and SE */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

extern double moshier_delta_t(double jd_ut);

/* SE functions */
extern double swe_deltat_ex(double tjd, int iflag, char *serr);
extern void swe_set_ephe_path(char *path);
extern void swe_close(void);
#define SEFLG_MOSEPH 4

int main(void) {
    swe_set_ephe_path(NULL);
    char serr[256];

    printf("# Year    JD            Our_dT_sec    SE_dT_sec     Diff_sec   Moon_shift_arcsec\n");

    for (int year = 1900; year <= 2055; year += 5) {
        double jd = 2451545.0 + (year - 2000) * 365.25;
        double our_dt = moshier_delta_t(jd);  /* days */
        double se_dt = swe_deltat_ex(jd, SEFLG_MOSEPH, serr);  /* days */

        double our_sec = our_dt * 86400.0;
        double se_sec = se_dt * 86400.0;
        double diff_sec = our_sec - se_sec;
        /* Moon moves ~0.549 arcsec/second */
        double moon_shift = fabs(diff_sec) * 0.549;

        printf("%4d  %.1f  %10.3f  %10.3f  %10.3f  %10.3f\n",
               year, jd, our_sec, se_sec, diff_sec, moon_shift);
    }

    swe_close();
    return 0;
}
