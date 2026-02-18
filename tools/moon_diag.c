/* Quick diagnostic: compare moshier lunar longitude at key dates */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

extern double moshier_delta_t(double jd_ut);
extern double moshier_nutation_longitude(double jd_ut);

int main(void) {
    /* Test dates: J2000, and some failure dates */
    double dates[] = {
        2451545.0,   /* J2000 = 2000-01-01 12:00 UT */
        2453097.0,   /* ~2004-04-01 */
        2460000.0,   /* ~2023-02-25 */
        2420000.0,   /* ~1913-01-25 */
    };
    int n = sizeof(dates)/sizeof(dates[0]);

    printf("%-14s  %-14s  %-14s  %-14s\n", "JD_UT", "Lunar_Lon_deg", "Nutation_deg", "Raw_lon_deg");
    for (int i = 0; i < n; i++) {
        double jd = dates[i];
        double lon = moshier_lunar_longitude(jd);
        double nut = moshier_nutation_longitude(jd);
        printf("%.1f  %14.8f  %14.8f  %14.8f\n", jd, lon, nut, lon - nut);
    }
    return 0;
}
