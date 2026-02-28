/* Diagnose Tamil edge cases with upper limb sunset */
#include "astro.h"
#include "date_utils.h"
#include "solar.h"
#include <stdio.h>
#include <math.h>

static void diag(int y, int m, int d)
{
    Location delhi = DEFAULT_LOCATION;
    double jd = gregorian_to_jd(y, m, d);

    double sr = sunrise_jd(jd, &delhi);
    double ss = sunset_jd(jd, &delhi);

    /* Convert to IST hours */
    double sr_ist = (sr + 5.5/24.0 + 0.5);
    sr_ist = (sr_ist - floor(sr_ist)) * 24.0;
    double ss_ist = (ss + 5.5/24.0 + 0.5);
    ss_ist = (ss_ist - floor(ss_ist)) * 24.0;

    /* Sidereal solar longitude */
    double lon = solar_longitude_sidereal(ss);
    int rashi = (int)floor(lon / 30.0) + 1;
    double target = (rashi - 1) * 30.0;

    /* Find sankranti near this date */
    double sank = sankranti_jd(jd, target);
    double sank_ist = (sank + 5.5/24.0 + 0.5);
    sank_ist = (sank_ist - floor(sank_ist)) * 24.0;

    /* Critical time = sunset - buffer */
    double crit6 = ss - 6.0 / (24.0*60.0);
    double crit_ist = (crit6 + 5.5/24.0 + 0.5);
    crit_ist = (crit_ist - floor(crit_ist)) * 24.0;

    double margin_min = (sank - crit6) * 24.0 * 60.0;

    printf("%04d-%02d-%02d  rashi=%d  sunrise=%.4fh  sunset=%.4fh  "
           "sankranti=%.4fh  crit(ss-6m)=%.4fh  margin=%.2f min\n",
           y, m, d, rashi, sr_ist, ss_ist, sank_ist, crit_ist, margin_min);

    /* Try different buffers */
    for (double buf = 5.0; buf <= 9.0; buf += 0.5) {
        double c = ss - buf / (24.0*60.0);
        double mg = (sank - c) * 24.0 * 60.0;
        const char *assign = (sank <= c) ? "THIS day" : "NEXT day";
        printf("  buf=%.1f min: margin=%.2f min -> %s\n", buf, mg, assign);
    }
}

int main(void)
{
    astro_init(NULL);
    printf("=== Tamil edge cases ===\n\n");
    diag(1923, 2, 12);
    printf("\n");
    diag(1964, 1, 14);
    astro_close();
    return 0;
}
