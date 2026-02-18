/* Measure apparent lunar longitude difference between moshier and SE
 * using proper circular difference */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

extern int swe_calc_ut(double tjd_ut, int ipl, int iflag, double *xx, char *serr);
extern void swe_set_ephe_path(char *path);
extern void swe_close(void);
#define SE_MOON 1
#define SEFLG_MOSEPH 4

static double circ_diff(double a, double b) {
    double d = a - b;
    while (d > 180.0) d -= 360.0;
    while (d < -180.0) d += 360.0;
    return d;
}

int main(void) {
    swe_set_ephe_path(NULL);
    char serr[256];
    double xx[6];

    printf("# JD          Year    Our_lon       SE_lon        Diff_arcsec\n");

    /* Sample every 30 days from 1900 to 2050 */
    double jd_start = 2415020.5;  /* ~1900-01-01 */
    double jd_end   = 2469807.5;  /* ~2050-01-01 */

    double max_diff = 0, sum_sq = 0, sum = 0;
    int count = 0, pos_count = 0, neg_count = 0;

    for (double jd = jd_start; jd <= jd_end; jd += 30.0) {
        double our = moshier_lunar_longitude(jd);

        swe_calc_ut(jd, SE_MOON, SEFLG_MOSEPH, xx, serr);
        double se = xx[0];

        double diff = circ_diff(our, se) * 3600.0;  /* arcsec */

        int year = (int)((jd - 2451545.0) / 365.25 + 2000.0);

        if (fabs(diff) > 5.0 || count % 50 == 0) {
            printf("%.1f  %d  %12.7f  %12.7f  %8.3f\n", jd, year, our, se, diff);
        }

        if (fabs(diff) > fabs(max_diff)) max_diff = diff;
        sum_sq += diff * diff;
        sum += diff;
        if (diff > 0) pos_count++; else neg_count++;
        count++;
    }

    printf("\n# Summary (%d samples, 1900-2050):\n", count);
    printf("#   Max |diff|: %.3f arcsec\n", fabs(max_diff));
    printf("#   RMS diff:   %.3f arcsec\n", sqrt(sum_sq / count));
    printf("#   Mean diff:  %.4f arcsec (positive = our > SE)\n", sum / count);
    printf("#   Positive:   %d, Negative: %d\n", pos_count, neg_count);
    swe_close();
    return 0;
}
