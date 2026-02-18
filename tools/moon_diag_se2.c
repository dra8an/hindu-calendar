/* Compare SE Moon longitude with various flags */
#include <stdio.h>
#include <math.h>
#include "swephexp.h"

int main(void) {
    double dates[] = {
        2451545.0,   /* J2000 */
        2453097.0,
        2460000.0,
        2420000.0,
    };
    int n = sizeof(dates)/sizeof(dates[0]);
    char serr[256];
    double xx[6];

    swe_set_ephe_path(NULL);

    printf("%-14s  %-14s  %-14s  %-14s\n",
           "JD_UT", "Apparent", "TruePos+NoNut", "TruePos");
    for (int i = 0; i < n; i++) {
        double jd = dates[i];
        double apparent, truepos_nonut, truepos;

        /* Default: apparent with nutation */
        swe_calc_ut(jd, SE_MOON, SEFLG_MOSEPH, xx, serr);
        apparent = xx[0];

        /* True position, no nutation = geometric, ecliptic of date, no nut */
        swe_calc_ut(jd, SE_MOON, SEFLG_MOSEPH | SEFLG_TRUEPOS | SEFLG_NONUT, xx, serr);
        truepos_nonut = xx[0];

        /* True position with nutation */
        swe_calc_ut(jd, SE_MOON, SEFLG_MOSEPH | SEFLG_TRUEPOS, xx, serr);
        truepos = xx[0];

        printf("%.1f  %14.8f  %14.8f  %14.8f\n", jd, apparent, truepos_nonut, truepos);
    }
    swe_close();
    return 0;
}
