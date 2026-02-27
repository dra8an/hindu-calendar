/* Analyze all Bengali sankrantis in the midnight zone (23:26-00:34 IST).
   For each: show delta from midnight, rashi, whether it triggers the
   Karkata/Makara exception, and whether it matches drikpanchang. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "date_utils.h"
#include "solar.h"
#include "astro.h"

typedef struct {
    int gy, gm, gd;          /* Gregorian date of sankranti (IST) */
    int rashi;
    double delta_min;         /* Minutes from midnight (negative = before) */
    double sank_hours;        /* IST hour of sankranti */
    int our_gy, our_gm, our_gd;  /* Civil day our code assigns (month start) */
    int is_mismatch;          /* 1 if known mismatch with drikpanchang */
    int drik_diff;            /* +1 or -1 */
} Entry;

/* Known mismatches: sankranti IST date, rashi, our start date, drik diff */
static const struct { int sgy,sgm,sgd; int rashi; int ogy,ogm,ogd; int diff; } MISMATCHES[] = {
    {1933,10,16, 7, 1933,10,17, +1},   /* Kartik: sank 23:47, our=17, drik=18 */
    {1952, 7,16, 4, 1952, 7,17, -1},   /* Srabon: sank 00:31, our=17, drik=16 */
    {1958,12,15, 9, 1958,12,16, +1},   /* Poush:  sank 23:49, our=16, drik=17 */
    {1972,10,16, 7, 1972,10,17, +1},   /* Kartik: sank 23:40, our=17, drik=18 */
    {1974, 9,16, 6, 1974, 9,17, +1},   /* Ashshin: sank 23:56, our=17, drik=18 */
    {1976,10,17, 7, 1976,10,17, +1},   /* Kartik: sank 00:23, our=17, drik=18 */
    {2011,10,17, 7, 2011,10,18, +1},   /* Kartik: sank 23:40, our=18, drik=19 */
    {2013, 9,16, 6, 2013, 9,17, +1},   /* Ashshin: sank 23:58, our=17, drik=18 */
};
#define N_MISMATCHES 8

static const char *RASHI_NAMES[] = {
    "", "Mesha", "Vrishabha", "Mithuna", "Karkata", "Simha", "Kanya",
    "Tula", "Vrischika", "Dhanu", "Makara", "Kumbha", "Meena"
};

static const char *MONTH_NAMES[] = {
    "", "Boishakh", "Jyoishtho", "Asharh", "Srabon", "Bhadro", "Ashshin",
    "Kartik", "Ogrohayon", "Poush", "Magh", "Falgun", "Choitro"
};

static int cmp_delta(const void *a, const void *b)
{
    double da = ((const Entry *)a)->delta_min;
    double db = ((const Entry *)b)->delta_min;
    return (da > db) - (da < db);
}

/* Match by sankranti IST date + rashi */
static int is_known_mismatch(int sgy, int sgm, int sgd, int rashi, int *diff)
{
    for (int i = 0; i < N_MISMATCHES; i++) {
        if (MISMATCHES[i].sgy == sgy && MISMATCHES[i].sgm == sgm &&
            MISMATCHES[i].sgd == sgd && MISMATCHES[i].rashi == rashi) {
            *diff = MISMATCHES[i].diff;
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    Location loc = DEFAULT_LOCATION;
    Entry entries[200];
    int count = 0;

    for (int gy = 1900; gy <= 2050; gy++) {
        for (int rashi = 1; rashi <= 12; rashi++) {
            double target_long = (rashi - 1) * 30.0;

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

            double frac = ist_jd - floor(ist_jd);
            double hours = frac * 24.0;

            /* Expanded zone: 23:26 to 00:34 (±34 min from midnight) */
            double delta_from_midnight;
            int in_zone = 0;
            if (hours >= (23.0 + 26.0/60.0)) {  /* >= 23:26 */
                delta_from_midnight = (hours - 24.0) * 60.0;
                in_zone = 1;
            } else if (hours < (34.0/60.0)) {    /* < 00:34 */
                delta_from_midnight = hours * 60.0;
                in_zone = 1;
            }

            /* Also check Srabon 1952 even if outside zone */
            if (rashi == 4 && approx_year == 1952 && !in_zone) {
                int hh = (int)hours;
                int mm = (int)((hours - hh) * 60.0);
                int ss = (int)(((hours - hh) * 60.0 - mm) * 60.0);
                fprintf(stderr, "Note: Srabon 1952 (R4 Karkata) sankranti at "
                       "%04d-%02d-%02d %02d:%02d:%02d IST — OUTSIDE midnight zone\n",
                       sy, sm, sd, hh, mm, ss);
            }

            if (!in_zone) continue;

            /* Get our code's civil day assignment via gregorian_to_solar.
               The month start is the day where solar day = 1 for this rashi.
               We check the IST date and the next day. */
            SolarDate sd1 = gregorian_to_solar(sy, sm, sd, &loc, SOLAR_CAL_BENGALI);
            int cy, cm, cd;
            if (sd1.rashi == rashi && sd1.day == 1) {
                cy = sy; cm = sm; cd = sd;
            } else {
                /* Try next day */
                int ny, nm, nd;
                double next_jd = gregorian_to_jd(sy, sm, sd) + 1;
                jd_to_gregorian((int)next_jd, &ny, &nm, &nd);
                SolarDate sd2 = gregorian_to_solar(ny, nm, nd, &loc, SOLAR_CAL_BENGALI);
                if (sd2.rashi == rashi && sd2.day == 1) {
                    cy = ny; cm = nm; cd = nd;
                } else {
                    /* Try previous day */
                    double prev_jd = gregorian_to_jd(sy, sm, sd) - 1;
                    jd_to_gregorian((int)prev_jd, &ny, &nm, &nd);
                    SolarDate sd3 = gregorian_to_solar(ny, nm, nd, &loc, SOLAR_CAL_BENGALI);
                    if (sd3.rashi == rashi && sd3.day == 1) {
                        cy = ny; cm = nm; cd = nd;
                    } else {
                        cy = sy; cm = sm; cd = sd; /* fallback */
                    }
                }
            }

            Entry *e = &entries[count++];
            e->gy = sy; e->gm = sm; e->gd = sd;
            e->rashi = rashi;
            e->delta_min = delta_from_midnight;
            e->sank_hours = hours;
            e->our_gy = cy; e->our_gm = cm; e->our_gd = cd;

            int diff = 0;
            e->is_mismatch = is_known_mismatch(sy, sm, sd, rashi, &diff);
            e->drik_diff = diff;
        }
    }

    /* Sort by delta from midnight */
    qsort(entries, count, sizeof(Entry), cmp_delta);

    printf("Bengali midnight zone sankrantis (23:26-00:34 IST), sorted by delta from midnight\n");
    printf("==================================================================================\n\n");
    printf(" #  Delta     Sank. IST      R#  Rashi       Month      Start date   Rule        Match\n");
    printf("--- -------   ------------   --  ----------  ---------  -----------  ----------  --------\n");

    int n_correct = 0, n_wrong = 0;
    int n_karkata = 0, n_makara = 0, n_tithi = 0;
    int n_pre = 0, n_post = 0;

    for (int i = 0; i < count; i++) {
        Entry *e = &entries[i];

        const char *match_str;
        if (e->is_mismatch) {
            match_str = "WRONG";
            n_wrong++;
        } else {
            match_str = "ok";
            n_correct++;
        }

        const char *rule_str;
        if (e->rashi == 4) { rule_str = "Karkata(C)"; n_karkata++; }
        else if (e->rashi == 10) { rule_str = "Makara(W)"; n_makara++; }
        else { rule_str = "tithi"; n_tithi++; }

        if (e->delta_min < 0) n_pre++; else n_post++;

        /* Format IST time */
        int hh = (int)e->sank_hours;
        int mm = (int)((e->sank_hours - hh) * 60.0);
        int ss = (int)(((e->sank_hours - hh) * 60.0 - mm) * 60.0);

        printf("%2d  %+6.1fm   %02d:%02d:%02d      R%-2d %-10s  %-9s  %04d-%02d-%02d  %-10s  %s",
               i + 1, e->delta_min,
               hh, mm, ss,
               e->rashi, RASHI_NAMES[e->rashi],
               MONTH_NAMES[e->rashi],
               e->our_gy, e->our_gm, e->our_gd,
               rule_str, match_str);

        if (e->is_mismatch)
            printf("  (drik: %+d day)", e->drik_diff);

        printf("\n");
    }

    printf("\n==================================================================================\n");
    printf("Total in midnight zone: %d\n", count);
    printf("  Pre-midnight (23:36-00:00): %d\n", n_pre);
    printf("  Post-midnight (00:00-00:24): %d\n", n_post);
    printf("\nRule breakdown:\n");
    printf("  Karkata exception (always current day): %d\n", n_karkata);
    printf("  Makara exception (always next day): %d\n", n_makara);
    printf("  Tithi rule applied: %d\n", n_tithi);
    printf("\nAccuracy:\n");
    printf("  Correct: %d / %d (%.1f%%)\n", n_correct, count, 100.0 * n_correct / count);
    printf("  Wrong:   %d / %d (%.1f%%)\n", n_wrong, count, 100.0 * n_wrong / count);

    return 0;
}
