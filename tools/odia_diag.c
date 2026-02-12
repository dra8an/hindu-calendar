/*
 * Diagnostic tool: prints the Karka (Cancer) sankranti for 2026 with exact
 * JD/IST time and civil day under different critical-time rules.
 * Used to investigate the Odia Shravana 1 discrepancy (Jul 16 vs Jul 17).
 *
 * Build:
 *   cc -Wall -Wextra -O2 -std=c99 -Ilib/swisseph -Isrc \
 *      -o build/odia_diag tools/odia_diag.c \
 *      build/swe/*.o build/astro.o build/date_utils.o \
 *      build/tithi.o build/masa.o build/panchang.o build/solar.o -lm
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include <stdio.h>
#include <math.h>

static const char *RASHI_NAMES[] = {
    "", "Mesha", "Vrishabha", "Mithuna", "Karka", "Simha", "Kanya",
    "Tula", "Vrishchika", "Dhanu", "Makara", "Kumbha", "Meena"
};

static const char *ODIA_MONTHS[] = {
    "", "Baisakha", "Jyeshtha", "Ashadha", "Shravana", "Bhadrapada",
    "Ashvina", "Kartika", "Margashirsha", "Pausha", "Magha", "Phalguna", "Chaitra"
};

static void jd_to_ist(double jd_ut, int *y, int *m, int *d, int *hh, int *mm, int *ss)
{
    double local = jd_ut + 5.5 / 24.0 + 0.5;  /* UT->IST, noon-based->midnight-based */
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

    printf("Odia 2026 Sankranti Diagnostic\n");
    printf("==============================\n\n");

    /* Print all 12 sankrantis for Odia year 2026 */
    printf("%-14s %-8s %17s  %-24s  %-12s %-12s %-12s\n",
           "Month", "Rashi", "Sankranti JD(UT)", "IST Time",
           "Midnight", "Midnight+24m", "NextMid");
    printf("%-14s %-8s %17s  %-24s  %-12s %-12s %-12s\n",
           "----------", "------", "--------------", "--------------------",
           "----------", "----------", "----------");

    for (int rashi = 1; rashi <= 12; rashi++) {
        double target_long = (rashi - 1) * 30.0;
        int approx_month = 3 + rashi;
        int approx_year = 2026;
        if (approx_month > 12) { approx_month -= 12; approx_year = 2027; }
        double jd_est = gregorian_to_jd(approx_year, approx_month, 14);
        double jd_sank = sankranti_jd(jd_est, target_long);

        /* IST time */
        int sy, sm, sd, sh, smin, ssec;
        jd_to_ist(jd_sank, &sy, &sm, &sd, &sh, &smin, &ssec);

        /* Local date of sankranti */
        double local_jd = jd_sank + loc.utc_offset / 24.0 + 0.5;
        int ly, lm, ld;
        jd_to_gregorian(floor(local_jd), &ly, &lm, &ld);

        double jd_day = gregorian_to_jd(ly, lm, ld);

        /* Rule 1: End of civil day (current Odia rule) */
        double midnight_crit = jd_day + 1.0 - loc.utc_offset / 24.0;
        int mid_gy = ly, mid_gm = lm, mid_gd = ld;
        if (jd_sank > midnight_crit) {
            double next = jd_day + 1.0;
            jd_to_gregorian(next, &mid_gy, &mid_gm, &mid_gd);
        }

        /* Rule 2: Midnight + 24 min (Bengali-style) */
        double mid24_crit = jd_day - loc.utc_offset / 24.0 + 24.0 / (24.0 * 60.0);
        int mid24_gy = ly, mid24_gm = lm, mid24_gd = ld;
        if (jd_sank > mid24_crit) {
            double next = jd_day + 1.0;
            jd_to_gregorian(next, &mid24_gy, &mid24_gm, &mid24_gd);
        }

        /* Rule 3: Next midnight (start of next day) = start of this civil day */
        double nextmid_crit = jd_day - loc.utc_offset / 24.0;
        int nm_gy = ly, nm_gm = lm, nm_gd = ld;
        if (jd_sank > nextmid_crit) {
            /* Sankranti happened during this day - belongs to this day */
        } else {
            /* Sankranti happened before this day started */
        }
        /* Actually let me reframe: test if sankranti < start of this day */
        /* Start of local day = midnight IST = jd_day - utc_offset/24 */
        double start_of_day = jd_day - loc.utc_offset / 24.0;
        double end_of_day = start_of_day + 1.0;
        nm_gy = ly; nm_gm = lm; nm_gd = ld;
        if (jd_sank > end_of_day) {
            double next = jd_day + 1.0;
            jd_to_gregorian(next, &nm_gy, &nm_gm, &nm_gd);
        }

        char ist_buf[32];
        snprintf(ist_buf, sizeof(ist_buf), "%04d-%02d-%02d %02d:%02d:%02d",
                 sy, sm, sd, sh, smin, ssec);

        char mid_buf[16], mid24_buf[16], nm_buf[16];
        snprintf(mid_buf, sizeof(mid_buf), "%02d-%02d", mid_gm, mid_gd);
        snprintf(mid24_buf, sizeof(mid24_buf), "%02d-%02d", mid24_gm, mid24_gd);
        snprintf(nm_buf, sizeof(nm_buf), "%02d-%02d", nm_gm, nm_gd);

        int reg_month = rashi; /* For Odia, first_rashi=1 so month=rashi */

        printf("%-14s %-8s %17.6f  %-24s  %-12s %-12s %-12s",
               ODIA_MONTHS[reg_month], RASHI_NAMES[rashi],
               jd_sank, ist_buf,
               mid_buf, mid24_buf, nm_buf);
        printf("\n");
    }

    /* Detailed view of Karka sankranti */
    printf("\n\n=== Detailed: Karka Sankranti (Shravana start) ===\n\n");
    double target = 90.0; /* Karka = rashi 4, target = 90 degrees */
    double jd_est = gregorian_to_jd(2026, 7, 14);
    double jd_sank = sankranti_jd(jd_est, target);

    int sy, sm, sd, sh, smin, ssec;
    jd_to_ist(jd_sank, &sy, &sm, &sd, &sh, &smin, &ssec);
    printf("Sankranti JD (UT):  %.10f\n", jd_sank);
    printf("IST:                %04d-%02d-%02d %02d:%02d:%02d\n", sy, sm, sd, sh, smin, ssec);

    /* Show exact fractional hours */
    double ist_frac = (jd_sank + 5.5/24.0 + 0.5);
    double time_frac = (ist_frac - floor(ist_frac)) * 24.0;
    printf("IST hours:          %.6f (%.4f minutes past midnight)\n",
           time_frac, time_frac * 60.0);

    /* Local date */
    double local_jd = jd_sank + loc.utc_offset / 24.0 + 0.5;
    int ly, lm, ld;
    jd_to_gregorian(floor(local_jd), &ly, &lm, &ld);
    printf("Local date:         %04d-%02d-%02d\n", ly, lm, ld);

    double jd_day = gregorian_to_jd(ly, lm, ld);
    printf("\nCritical times (JD UT):\n");

    /* Start of local day (midnight IST) */
    double start = jd_day - loc.utc_offset / 24.0;
    printf("  Start of day (midnight IST %02d-%02d): %.10f\n", lm, ld, start);

    /* End of local day (midnight IST next day = current Odia rule) */
    double end = jd_day + 1.0 - loc.utc_offset / 24.0;
    printf("  End of day   (midnight IST %02d-%02d): %.10f\n", lm, ld+1, end);

    printf("  Sankranti:                           %.10f\n", jd_sank);
    printf("  Difference (sank - end):             %.10f days = %.2f seconds\n",
           jd_sank - end, (jd_sank - end) * 86400.0);
    printf("  Difference (sank - start):           %.10f days = %.2f hours\n",
           jd_sank - start, (jd_sank - start) * 24.0);

    printf("\nVerdict: sankranti %s end of day (midnight next day)\n",
           jd_sank <= end ? "<=" : ">");

    /* Also check: what if the rule is "midnight start of this day"? */
    printf("\nAlternate check: is sankranti before midnight IST of %02d-%02d?\n", lm, ld);
    printf("  start_of_%02d-%02d = %.10f\n", lm, ld, start);
    printf("  sankranti = %.10f\n", jd_sank);
    printf("  sank %s start\n", jd_sank <= start ? "<=" : ">");

    astro_close();
    return 0;
}
