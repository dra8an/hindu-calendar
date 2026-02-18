/* Compare moshier vs SE sunrise times across the year for Delhi.
 * Shows sr_diff pattern to identify systematic offsets. */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

extern double moshier_sunrise(double jd_ut, double lon, double lat, double alt);
extern double moshier_julday(int year, int month, int day, double hour);

extern int swe_rise_trans(double tjd_ut, int ipl, char *starname, int epheflag,
                          int rsmi, double *geopos, double atpress, double attemp,
                          double *tret, char *serr);
extern void swe_set_ephe_path(char *path);
extern void swe_close(void);
#define SE_SUN   0
#define SEFLG_MOSEPH 4
#define SE_CALC_RISE     1
#define SE_BIT_DISC_CENTER 256

#define LAT  28.6139
#define LON  77.2090

int main(void) {
    swe_set_ephe_path(NULL);
    char serr[256];
    double geopos[3] = {LON, LAT, 0};

    printf("# Date        sr_diff(s)  our_IST      se_IST\n");

    /* Check every 15 days across 1965 */
    for (int m = 1; m <= 12; m++) {
        for (int d = 1; d <= 28; d += 14) {
            double jd = moshier_julday(1965, m, d, 0.0);
            double jd_start = jd - 5.5 / 24.0;

            double sr = moshier_sunrise(jd_start, LON, LAT, 0.0);
            double se_sr;
            swe_rise_trans(jd_start, SE_SUN, NULL, SEFLG_MOSEPH,
                           SE_CALC_RISE | SE_BIT_DISC_CENTER, geopos, 0, 0, &se_sr, serr);

            double sr_diff = (sr - se_sr) * 86400.0;

            /* Convert to IST hours */
            double our_ist = fmod((sr - floor(sr)) * 24.0 + 5.5, 24.0);
            double se_ist = fmod((se_sr - floor(se_sr)) * 24.0 + 5.5, 24.0);

            printf("1965-%02d-%02d  %+7.1fs  %6.3fh  %6.3fh\n",
                   m, d, sr_diff, our_ist, se_ist);
        }
    }

    /* Also check failing date specifically */
    printf("\n# Failing date:\n");
    double jd = moshier_julday(1965, 5, 30, 0.0);
    double jd_start = jd - 5.5 / 24.0;
    double sr = moshier_sunrise(jd_start, LON, LAT, 0.0);
    double se_sr;
    swe_rise_trans(jd_start, SE_SUN, NULL, SEFLG_MOSEPH,
                   SE_CALC_RISE | SE_BIT_DISC_CENTER, geopos, 0, 0, &se_sr, serr);
    printf("1965-05-30  %+7.1fs\n", (sr - se_sr) * 86400.0);

    swe_close();
    return 0;
}
