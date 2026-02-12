/*
 * Scan all Odia sankrantis 1900-2050 that fall within 2 hours of midnight IST.
 * Shows exact time + which day each rule assigns them to.
 *
 * Build:
 *   cc -Wall -Wextra -O2 -std=c99 -Ilib/swisseph -Isrc \
 *      -o build/odia_midnight_scan tools/odia_midnight_scan.c \
 *      build/swe/*.o build/astro.o build/date_utils.o \
 *      build/tithi.o build/masa.o build/panchang.o build/solar.o -lm
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include <stdio.h>
#include <math.h>

static void jd_to_ist_time(double jd_ut, int *y, int *m, int *d, int *hh, int *mm, int *ss)
{
    double local = jd_ut + 5.5 / 24.0 + 0.5;
    double day_frac = local - floor(local);
    jd_to_gregorian(floor(local), y, m, d);
    double secs = day_frac * 86400.0;
    *hh = (int)(secs / 3600.0);
    *mm = (int)(fmod(secs, 3600.0) / 60.0);
    *ss = (int)(fmod(secs, 60.0));
}

int main(void)
{
    astro_init(NULL);
    Location loc = DEFAULT_LOCATION;

    printf("Odia sankrantis near midnight IST (22:00-02:00), 1900-2050\n");
    printf("===========================================================\n\n");
    printf("%-12s %-8s %-20s  %-8s  %-10s  %-10s  %s\n",
           "Greg Date", "Rashi", "IST Time", "HH:MM", "Midnight", "Sunset", "Notes");
    printf("%-12s %-8s %-20s  %-8s  %-10s  %-10s  %s\n",
           "----------", "------", "--------------------", "------", "--------", "--------", "-----");

    int count = 0;
    for (int gy = 1900; gy <= 2050; gy++) {
        for (int rashi = 1; rashi <= 12; rashi++) {
            double target_long = (rashi - 1) * 30.0;
            int approx_month = 3 + rashi;
            int approx_year = gy;
            if (approx_month > 12) { approx_month -= 12; approx_year++; }
            double jd_est = gregorian_to_jd(approx_year, approx_month, 14);
            double jd_sank = sankranti_jd(jd_est, target_long);

            /* IST time */
            int sy, sm, sd, sh, smin, ssec;
            jd_to_ist_time(jd_sank, &sy, &sm, &sd, &sh, &smin, &ssec);

            /* Check if near midnight (22:00-23:59 or 00:00-02:00) */
            if (sh < 22 && sh >= 2) continue;

            /* Local date */
            double local_jd = jd_sank + loc.utc_offset / 24.0 + 0.5;
            int ly, lm, ld;
            jd_to_gregorian(floor(local_jd), &ly, &lm, &ld);

            double jd_day = gregorian_to_jd(ly, lm, ld);

            /* Midnight rule (end of day) */
            double midnight_crit = jd_day + 1.0 - loc.utc_offset / 24.0;
            int mid_gy, mid_gm, mid_gd;
            if (jd_sank <= midnight_crit) {
                mid_gy = ly; mid_gm = lm; mid_gd = ld;
            } else {
                double next = jd_day + 1.0;
                jd_to_gregorian(next, &mid_gy, &mid_gm, &mid_gd);
            }

            /* Sunset rule */
            double ss_jd = sunset_jd(jd_day, &loc);
            int sun_gy, sun_gm, sun_gd;
            if (jd_sank <= ss_jd) {
                sun_gy = ly; sun_gm = lm; sun_gd = ld;
            } else {
                double next = jd_day + 1.0;
                jd_to_gregorian(next, &sun_gy, &sun_gm, &sun_gd);
            }

            char diff_note[64] = "";
            if (mid_gd != sun_gd || mid_gm != sun_gm)
                snprintf(diff_note, sizeof(diff_note), "mid!=sunset");

            printf("%04d-%02d-%02d   %-8d %-20s  %02d:%02d     %02d-%02d       %02d-%02d       %s\n",
                   sy, sm, sd, rashi,
                   "",
                   sh, smin,
                   mid_gm, mid_gd,
                   sun_gm, sun_gd,
                   diff_note);
            count++;
        }
    }

    printf("\nTotal: %d sankrantis near midnight\n", count);

    astro_close();
    return 0;
}
