/* Debug sunrise computation for a single date to understand 1-day offset */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

extern double moshier_sunrise(double jd_ut, double lon, double lat, double alt);
extern double moshier_julday(int year, int month, int day, double hour);
extern void moshier_revjul(double jd, int *year, int *month, int *day, double *hour);

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

    int y = 1906, m = 1, d = 2;

    double jd = moshier_julday(y, m, d, 0.0);
    double jd_start = jd - 5.5 / 24.0;

    printf("Date: %04d-%02d-%02d\n", y, m, d);
    printf("JD at 0h UT:    %.6f\n", jd);
    printf("JD at start:    %.6f (midnight IST)\n", jd_start);

    int yr2, mo2, dy2; double hr2;
    moshier_revjul(jd_start, &yr2, &mo2, &dy2, &hr2);
    printf("Start date/time: %04d-%02d-%02d %.4fh UT\n", yr2, mo2, dy2, hr2);

    /* Moshier sunrise */
    double sr = moshier_sunrise(jd_start, LON, LAT, 0.0);
    moshier_revjul(sr, &yr2, &mo2, &dy2, &hr2);
    printf("\nMoshier sunrise JD: %.6f\n", sr);
    printf("Moshier sunrise:    %04d-%02d-%02d %.4fh UT (%.4fh IST)\n",
           yr2, mo2, dy2, hr2, hr2 + 5.5);

    /* SE sunrise */
    double se_sr;
    swe_rise_trans(jd_start, SE_SUN, NULL, SEFLG_MOSEPH,
                   SE_CALC_RISE | SE_BIT_DISC_CENTER, geopos, 0, 0, &se_sr, serr);
    moshier_revjul(se_sr, &yr2, &mo2, &dy2, &hr2);
    printf("\nSE sunrise JD:      %.6f\n", se_sr);
    printf("SE sunrise:         %04d-%02d-%02d %.4fh UT (%.4fh IST)\n",
           yr2, mo2, dy2, hr2, hr2 + 5.5);

    printf("\nDifference: %.1f seconds\n", (sr - se_sr) * 86400.0);

    /* Also try one of the May dates */
    printf("\n=== May 19 date ===\n");
    y = 2018; m = 5; d = 19;
    jd = moshier_julday(y, m, d, 0.0);
    jd_start = jd - 5.5 / 24.0;
    printf("Date: %04d-%02d-%02d\n", y, m, d);
    printf("JD at 0h UT:    %.6f\n", jd);
    printf("JD at start:    %.6f (midnight IST)\n", jd_start);

    moshier_revjul(jd_start, &yr2, &mo2, &dy2, &hr2);
    printf("Start date/time: %04d-%02d-%02d %.4fh UT\n", yr2, mo2, dy2, hr2);

    sr = moshier_sunrise(jd_start, LON, LAT, 0.0);
    moshier_revjul(sr, &yr2, &mo2, &dy2, &hr2);
    printf("\nMoshier sunrise JD: %.6f\n", sr);
    printf("Moshier sunrise:    %04d-%02d-%02d %.4fh UT (%.4fh IST)\n",
           yr2, mo2, dy2, hr2, hr2 + 5.5);

    swe_rise_trans(jd_start, SE_SUN, NULL, SEFLG_MOSEPH,
                   SE_CALC_RISE | SE_BIT_DISC_CENTER, geopos, 0, 0, &se_sr, serr);
    moshier_revjul(se_sr, &yr2, &mo2, &dy2, &hr2);
    printf("\nSE sunrise JD:      %.6f\n", se_sr);
    printf("SE sunrise:         %04d-%02d-%02d %.4fh UT (%.4fh IST)\n",
           yr2, mo2, dy2, hr2, hr2 + 5.5);

    printf("\nDifference: %.1f seconds\n", (sr - se_sr) * 86400.0);

    swe_close();
    return 0;
}
