/* Count how many Bengali sankrantis fall in the midnight zone (23:36-00:24 IST)
   across all 1,811 month boundaries from 1900-2050. */

#include <stdio.h>
#include <math.h>
#include "date_utils.h"
#include "solar.h"

int main(void)
{
    Location loc = DEFAULT_LOCATION;
    int total = 0, in_zone = 0;

    /* Midnight zone in IST: 23:36 to 00:24 next day
       In fractional hours from midnight: -0.4h to +0.4h (±24 min) */

    printf("Bengali sankrantis in midnight zone (23:36-00:24 IST), 1900-2050\n");
    printf("================================================================\n\n");
    printf("%-12s  %-6s  %-20s  %-6s\n", "Date", "Rashi", "Sankranti IST", "Delta");
    printf("------------------------------------------------------------\n");

    for (int gy = 1900; gy <= 2050; gy++) {
        for (int rashi = 1; rashi <= 12; rashi++) {
            double target_long = (rashi - 1) * 30.0;

            /* Approximate Gregorian month for this rashi */
            int approx_month = 3 + rashi;
            int approx_year = gy;
            if (approx_month > 12) {
                approx_month -= 12;
                approx_year++;
            }

            double jd_est = gregorian_to_jd(approx_year, approx_month, 14);
            double jd_sank = sankranti_jd(jd_est, target_long);

            /* Convert to IST */
            double ist_jd = jd_sank + 5.5 / 24.0 + 0.5;
            int sy, sm, sd;
            jd_to_gregorian((int)floor(ist_jd), &sy, &sm, &sd);

            /* Time of day in IST (hours from midnight) */
            double frac = ist_jd - floor(ist_jd);
            double hours = frac * 24.0;
            int hh = (int)hours;
            int mm = (int)((hours - hh) * 60.0);
            int ss = (int)(((hours - hh) * 60.0 - mm) * 60.0);

            /* Midnight zone: 23:36 (23.6h) to 24:24 (0.4h next day)
               Normalize: if hours >= 23.6 or hours < 0.4 */
            double delta_from_midnight;  /* minutes from midnight */
            if (hours >= 23.6) {
                delta_from_midnight = (hours - 24.0) * 60.0;  /* negative */
            } else if (hours < 0.4) {
                delta_from_midnight = hours * 60.0;  /* positive */
            } else {
                total++;
                continue;
            }

            in_zone++;
            total++;

            printf("%04d-%02d-%02d    R%-4d  %02d:%02d:%02d IST          %+.1f min\n",
                   sy, sm, sd, rashi, hh, mm, ss, delta_from_midnight);
        }
    }

    printf("\n================================================================\n");
    printf("Total month boundaries: %d\n", total);
    printf("In midnight zone (23:36-00:24): %d (%.1f%%)\n",
           in_zone, 100.0 * in_zone / total);
    printf("Outside midnight zone: %d\n", total - in_zone);

    return 0;
}
