/* Quick diagnostic: SE lunar longitude at key dates */
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

    printf("%-14s  %-14s\n", "JD_UT", "SE_Lunar_Lon");
    for (int i = 0; i < n; i++) {
        double jd = dates[i];
        /* SEFLG_MOSEPH = Moshier ephemeris (no data files needed) */
        int ret = swe_calc_ut(jd, SE_MOON, SEFLG_MOSEPH, xx, serr);
        if (ret < 0) {
            printf("Error: %s\n", serr);
            continue;
        }
        printf("%.1f  %14.8f\n", jd, xx[0]);
    }
    swe_close();
    return 0;
}
