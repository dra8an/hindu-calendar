/*
 * Diagnostic tool: prints each 2025 Malayalam sankranti with exact JD/IST time,
 * and the civil day under three candidate critical-time rules (noon, sunset, midnight).
 * Used to determine which rule matches drikpanchang.com.
 *
 * Build:
 *   cc -Wall -Wextra -O2 -std=c99 -Ilib/swisseph -Isrc \
 *      -o build/malayalam_diag tools/malayalam_diag.c \
 *      build/swe/*.o build/astro.o build/date_utils.o \
 *      build/tithi.o build/masa.o build/panchang.o build/solar.o -lm
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include <stdio.h>
#include <math.h>

/* Malayalam months (rashi 5=Simha through 4=Karka) */
static const char *ML_MONTHS[] = {
    "", "Chingam", "Kanni", "Thulam", "Vrishchikam", "Dhanu",
    "Makaram", "Kumbham", "Meenam", "Medam", "Edavam", "Mithunam", "Karkadakam"
};

/* Rashi names */
static const char *RASHI_NAMES[] = {
    "", "Mesha", "Vrishabha", "Mithuna", "Karka", "Simha", "Kanya",
    "Tula", "Vrishchika", "Dhanu", "Makara", "Kumbha", "Meena"
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

    printf("Malayalam 2025 Sankranti Diagnostic\n");
    printf("===================================\n\n");
    printf("%-14s %-8s %17s  %-24s  %-12s %-12s %-12s\n",
           "Month", "Rashi", "Sankranti JD(UT)", "IST Time",
           "Noon Rule", "Sunset Rule", "Midnight Rule");
    printf("%-14s %-8s %17s  %-24s  %-12s %-12s %-12s\n",
           "----------", "------", "--------------", "--------------------",
           "----------", "----------", "----------");

    /* Malayalam months for 2025: regional months 6 (Makaram) through 12 (Karkadakam)
     * of Kollam 1200, then months 1 (Chingam) through 5 (Dhanu) of Kollam 1201.
     * In rashi order: Makara(10), Kumbha(11), Meena(12), Mesha(1), Vrishabha(2),
     * Mithuna(3), Karka(4), Simha(5), Kanya(6), Tula(7), Vrishchika(8), Dhanu(9). */
    int rashis[] = {10, 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int reg_months[] = {6, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5};
    int approx_greg_months[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    for (int i = 0; i < 12; i++) {
        int rashi = rashis[i];
        double target_long = (rashi - 1) * 30.0;
        double jd_est = gregorian_to_jd(2025, approx_greg_months[i], 14);
        double jd_sank = sankranti_jd(jd_est, target_long);

        /* IST time of sankranti */
        int sy, sm, sd, sh, smin, ssec;
        jd_to_ist(jd_sank, &sy, &sm, &sd, &sh, &smin, &ssec);

        /* Civil day under three rules */
        /* Convert to local date first */
        double local_jd = jd_sank + loc.utc_offset / 24.0 + 0.5;
        int ly, lm, ld;
        jd_to_gregorian(floor(local_jd), &ly, &lm, &ld);

        /* Noon rule: midpoint of sunrise and sunset */
        double jd_day = gregorian_to_jd(ly, lm, ld);
        double sr = sunrise_jd(jd_day, &loc);
        double ss = sunset_jd(jd_day, &loc);
        double noon_crit = (sr + ss) / 2.0;
        int noon_gy = ly, noon_gm = lm, noon_gd = ld;
        if (jd_sank > noon_crit) {
            double next = jd_day + 1.0;
            jd_to_gregorian(next, &noon_gy, &noon_gm, &noon_gd);
        }

        /* Sunset rule */
        double sunset_crit = ss;
        int sunset_gy = ly, sunset_gm = lm, sunset_gd = ld;
        if (jd_sank > sunset_crit) {
            double next = jd_day + 1.0;
            jd_to_gregorian(next, &sunset_gy, &sunset_gm, &sunset_gd);
        }

        /* Midnight rule (end of civil day) */
        double midnight_crit = jd_day + 1.0 - loc.utc_offset / 24.0;
        int midnight_gy = ly, midnight_gm = lm, midnight_gd = ld;
        if (jd_sank > midnight_crit) {
            double next = jd_day + 1.0;
            jd_to_gregorian(next, &midnight_gy, &midnight_gm, &midnight_gd);
        }

        char ist_buf[32];
        snprintf(ist_buf, sizeof(ist_buf), "%04d-%02d-%02d %02d:%02d:%02d",
                 sy, sm, sd, sh, smin, ssec);

        char noon_buf[16], sunset_buf[16], mid_buf[16];
        snprintf(noon_buf, sizeof(noon_buf), "%02d-%02d", noon_gm, noon_gd);
        snprintf(sunset_buf, sizeof(sunset_buf), "%02d-%02d", sunset_gm, sunset_gd);
        snprintf(mid_buf, sizeof(mid_buf), "%02d-%02d", midnight_gm, midnight_gd);

        printf("%-14s %-8s %17.6f  %-24s  %-12s %-12s %-12s",
               ML_MONTHS[reg_months[i]], RASHI_NAMES[rashi],
               jd_sank, ist_buf,
               noon_buf, sunset_buf, mid_buf);

        /* Mark differences */
        if (noon_gd != midnight_gd || noon_gm != midnight_gm)
            printf("  <-- noon!=midnight");
        printf("\n");
    }

    astro_close();
    return 0;
}
