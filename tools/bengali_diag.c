/*
 * bengali_diag.c — Diagnostic tool for Bengali solar calendar edge cases.
 *
 * For each of the 100 closest-to-critical-time Bengali sankrantis, prints
 * multiple candidate critical time values to help identify which rule
 * drikpanchang.com actually uses.
 *
 * Candidate rules tested:
 *   1. IST midnight + 24 min (current rule: 00:24 IST)
 *   2. IST midnight (00:00 IST)
 *   3. Local mean solar midnight for Kolkata (88.3639°E)
 *   4. Local mean solar midnight for Kolkata + 24 min
 *   5. Various fixed IST cutoffs (00:00, 00:10, 00:20, 00:30)
 *
 * Build:
 *   make && cc -O2 -Isrc -Ilib/swisseph tools/bengali_diag.c \
 *       build/astro.o build/date_utils.o build/tithi.o build/masa.o \
 *       build/panchang.o build/solar.o build/swe/*.o -lm -o build/bengali_diag
 *
 * Run:
 *   ./build/bengali_diag
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define TOP_N      100
#define YEAR_START 1900
#define YEAR_END   2050

/* Kolkata coordinates */
#define KOLKATA_LAT  22.5726
#define KOLKATA_LON  88.3639
#define KOLKATA_ALT  0.0

typedef struct {
    int gy, gm, gd;          /* Gregorian date of sankranti's local day */
    int sank_hh, sank_mm, sank_ss;
    double jd_sankranti;      /* Exact JD (UT) of sankranti */
    double delta_cur;         /* Delta from current rule (IST midnight+24min) */
    int rashi;
    int our_month, our_year;
    /* Deltas from candidate rules (minutes) */
    double delta_ist_midnight;      /* IST 00:00 */
    double delta_ist_midnight_24;   /* IST 00:24 (current rule) */
    double delta_kolkata_midnight;  /* Local mean midnight at Kolkata */
    double delta_kolkata_mid_24;    /* Kolkata midnight + 24 min */
    double delta_kolkata_sunrise;   /* Kolkata sunrise */
    /* Assignment under each rule: 0 = current day (day 1), 1 = next day (last day) */
    int assign_ist_mid;
    int assign_ist_mid24;
    int assign_kol_mid;
    int assign_kol_mid24;
    int assign_kol_sunrise;
} Entry;

static void jd_to_ist(double jd_ut, int *y, int *m, int *d,
                       int *hh, int *mm, int *ss)
{
    double local = jd_ut + 5.5 / 24.0 + 0.5;
    double day_frac = local - floor(local);
    jd_to_gregorian(floor(local), y, m, d);
    double secs = day_frac * 86400.0;
    *hh = (int)(secs / 3600.0);
    *mm = (int)(fmod(secs, 3600.0) / 60.0);
    *ss = (int)(fmod(secs, 60.0));
}

static int cmp_abs_delta(const void *a, const void *b)
{
    double da = fabs(((const Entry *)a)->delta_cur);
    double db = fabs(((const Entry *)b)->delta_cur);
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

int main(void)
{
    astro_init(NULL);

    Location delhi = DEFAULT_LOCATION;
    Location kolkata = { KOLKATA_LAT, KOLKATA_LON, KOLKATA_ALT, 5.5 };

    Entry *all = malloc(((YEAR_END - YEAR_START + 1) * 12) * sizeof(Entry));
    int count = 0;

    for (int year = YEAR_START; year <= YEAR_END; year++) {
        for (int rashi = 1; rashi <= 12; rashi++) {
            double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;

            /* Estimate sankranti date */
            int est_month = 3 + rashi;
            int est_year = year;
            if (est_month > 12) { est_month -= 12; est_year++; }
            double jd_est = gregorian_to_jd(est_year, est_month, 14);
            double jd_sank = sankranti_jd(jd_est, target_long);

            /* IST date of sankranti */
            Entry e;
            memset(&e, 0, sizeof(e));
            e.jd_sankranti = jd_sank;
            e.rashi = rashi;
            jd_to_ist(jd_sank, &e.gy, &e.gm, &e.gd,
                       &e.sank_hh, &e.sank_mm, &e.sank_ss);

            /* Our current prediction (using Delhi) */
            SolarDate sd = gregorian_to_solar(e.gy, e.gm, e.gd, &delhi,
                                              SOLAR_CAL_BENGALI);
            e.our_month = sd.month;
            e.our_year = sd.year;

            /* JD of local midnight UT for this date in Delhi */
            double jd_mid_ut = gregorian_to_jd(e.gy, e.gm, e.gd);

            /* Candidate 1: IST midnight = 18:30 UTC of previous day */
            double jd_ist_midnight = jd_mid_ut - 5.5 / 24.0;

            /* Candidate 2: IST midnight + 24 min (current rule) */
            double jd_ist_mid24 = jd_ist_midnight + 24.0 / (24.0 * 60.0);

            /* Candidate 3: Kolkata local mean solar midnight
             * Kolkata is at 88.3639°E. Local mean solar midnight =
             * UTC midnight - (longitude / 15) hours east = 0h UT - 5.89h
             * i.e. local midnight at Kolkata is at 18:06:32 UTC prev day
             * vs IST midnight at 18:30:00 UTC prev day.
             * Difference: Kolkata local midnight is ~23.5 min EARLIER than IST midnight */
            double kolkata_offset_h = KOLKATA_LON / 15.0;  /* ~5.891 hours */
            double jd_kol_midnight = jd_mid_ut - kolkata_offset_h / 24.0;

            /* Candidate 4: Kolkata local midnight + 24 min */
            double jd_kol_mid24 = jd_kol_midnight + 24.0 / (24.0 * 60.0);

            /* Candidate 5: Kolkata sunrise */
            double jd_kol_sunrise = sunrise_jd(jd_mid_ut, &kolkata);

            /* Compute deltas (minutes) */
            e.delta_ist_midnight = (jd_sank - jd_ist_midnight) * 24.0 * 60.0;
            e.delta_ist_midnight_24 = (jd_sank - jd_ist_mid24) * 24.0 * 60.0;
            e.delta_cur = e.delta_ist_midnight_24;  /* for sorting */
            e.delta_kolkata_midnight = (jd_sank - jd_kol_midnight) * 24.0 * 60.0;
            e.delta_kolkata_mid_24 = (jd_sank - jd_kol_mid24) * 24.0 * 60.0;
            e.delta_kolkata_sunrise = (jd_sank - jd_kol_sunrise) * 24.0 * 60.0;

            /* Assignment under each rule:
             * sankranti <= critical → current day (assign=0)
             * sankranti > critical  → next day (assign=1) */
            e.assign_ist_mid = (jd_sank <= jd_ist_midnight) ? 0 : 1;
            e.assign_ist_mid24 = (jd_sank <= jd_ist_mid24) ? 0 : 1;
            e.assign_kol_mid = (jd_sank <= jd_kol_midnight) ? 0 : 1;
            e.assign_kol_mid24 = (jd_sank <= jd_kol_mid24) ? 0 : 1;
            e.assign_kol_sunrise = (jd_sank <= jd_kol_sunrise) ? 0 : 1;

            all[count++] = e;
        }
    }

    /* Sort by |delta from current rule| and take top 100 */
    qsort(all, count, sizeof(Entry), cmp_abs_delta);

    printf("# Bengali Solar Calendar Edge Case Diagnostic\n");
    printf("# 100 closest sankrantis to IST midnight+24min (current rule)\n");
    printf("# Delhi location for current code, Kolkata for candidate rules\n");
    printf("# Kolkata: %.4fN, %.4fE (local midnight ~23.5 min before IST midnight)\n",
           KOLKATA_LAT, KOLKATA_LON);
    printf("#\n");
    printf("# Columns:\n");
    printf("#   greg_date: Gregorian date (IST)\n");
    printf("#   sank_ist: Sankranti time in IST\n");
    printf("#   rashi: Sun entering rashi 1-12\n");
    printf("#   delta_ist24: minutes from IST midnight+24min (current rule)\n");
    printf("#   delta_ist00: minutes from IST midnight\n");
    printf("#   delta_kol00: minutes from Kolkata local midnight\n");
    printf("#   delta_kol24: minutes from Kolkata midnight+24min\n");
    printf("#   delta_kol_sr: minutes from Kolkata sunrise\n");
    printf("#   our_m/y: our month/year prediction\n");
    printf("#   a_ist00/a_ist24/a_kol00/a_kol24/a_ksr: assignment (0=cur,1=nxt)\n");
    printf("#   exp: BLANK — fill in from drikpanchang (0=day1, 1=lastday)\n");
    printf("#\n");

    /* Header */
    printf("%-12s %-10s %2s %8s %8s %8s %8s %8s  %2s %4s  "
           "a_ist00 a_ist24 a_kol00 a_kol24 a_ksr  exp\n",
           "greg_date", "sank_ist", "ra",
           "d_ist24", "d_ist00", "d_kol00", "d_kol24", "d_kolsr",
           "om", "oy");

    int n = count < TOP_N ? count : TOP_N;
    for (int i = 0; i < n; i++) {
        Entry *e = &all[i];
        printf("%04d-%02d-%02d  %02d:%02d:%02d  %2d  %+7.1f  %+7.1f  %+7.1f  %+7.1f  %+7.1f  %2d %4d  "
               "  %d       %d       %d       %d     %d\n",
               e->gy, e->gm, e->gd,
               e->sank_hh, e->sank_mm, e->sank_ss,
               e->rashi,
               e->delta_ist_midnight_24,
               e->delta_ist_midnight,
               e->delta_kolkata_midnight,
               e->delta_kolkata_mid_24,
               e->delta_kolkata_sunrise,
               e->our_month, e->our_year,
               e->assign_ist_mid,
               e->assign_ist_mid24,
               e->assign_kol_mid,
               e->assign_kol_mid24,
               e->assign_kol_sunrise);
    }

    /* Also print a summary sorted by rashi for pattern analysis */
    printf("\n\n# === SORTED BY RASHI (for pattern analysis) ===\n");
    printf("# Same 100 entries, grouped by rashi\n\n");

    for (int r = 1; r <= 12; r++) {
        int first = 1;
        for (int i = 0; i < n; i++) {
            if (all[i].rashi != r) continue;
            if (first) {
                printf("# Rashi %d:\n", r);
                first = 0;
            }
            Entry *e = &all[i];
            printf("  %04d-%02d-%02d  %02d:%02d:%02d  d_ist24=%+7.1f  d_kol24=%+7.1f  "
                   "a_ist24=%d a_kol24=%d  m=%2d y=%4d\n",
                   e->gy, e->gm, e->gd,
                   e->sank_hh, e->sank_mm, e->sank_ss,
                   e->delta_ist_midnight_24,
                   e->delta_kolkata_mid_24,
                   e->assign_ist_mid24,
                   e->assign_kol_mid24,
                   e->our_month, e->our_year);
        }
    }

    free(all);
    astro_close();
    return 0;
}
