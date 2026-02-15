/*
 * bengali_eot.c — Test equation-of-time-based apparent midnight for Bengali calendar.
 *
 * The R/D code's midnight() function computes apparent midnight using the
 * equation of time (EoT), NOT the nishita (sunset+sunrise)/2 midpoint.
 * This tool tests whether EoT-based apparent midnight at various locations
 * correctly separates the 23 wrong from 14 correct Bengali edge cases.
 *
 * Candidate rules tested:
 *   1. EoT-based apparent midnight at IST reference (82.5E)
 *   2. EoT-based apparent midnight at Ujjain (75.768E)
 *   3. EoT-based apparent midnight at Delhi (77.209E)
 *   4. EoT-based apparent midnight at Kolkata (88.364E)
 *   5. Nishita (sunset+sunrise)/2 at each location (for comparison)
 *   6. Buffer sweep on each EoT midnight
 *
 * Build:
 *   make && cc -O2 -Isrc -Ilib/swisseph tools/bengali_eot.c \
 *       build/astro.o build/date_utils.o build/tithi.o build/masa.o \
 *       build/panchang.o build/solar.o build/swe/*.o -lm -o build/bengali_eot
 *
 * Run:
 *   ./build/bengali_eot
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include "swephexp.h"
#include <stdio.h>
#include <math.h>

/* Verified entries: 0 = correct (day 1), 1 = wrong (last day of prev month) */
static struct {
    int gy, gm, gd;
    int rashi;
    int wrong;  /* 1 = drikpanchang shows last day (our code is wrong) */
} entries[] = {
    /* WRONG: drikpanchang shows last day of previous month */
    {1908,  5, 14,  2, 1},
    {1976, 10, 17,  7, 1},
    {2022,  3, 15, 12, 1},
    {1983,  3, 15, 12, 1},
    {2047,  1, 15, 10, 1},
    {1935,  8, 17,  5, 1},
    {1937, 10, 17,  7, 1},
    {1944,  3, 14, 12, 1},
    {2015, 10, 18,  7, 1},
    {1935,  9, 17,  6, 1},
    {1981,  6, 15,  3, 1},
    {1903,  6, 15,  3, 1},
    {1969,  1, 14, 10, 1},
    {1930,  1, 14, 10, 1},
    {1986,  5, 15,  2, 1},
    {1976, 11, 16,  8, 1},
    {1924,  2, 13, 11, 1},
    {2036, 12, 16,  9, 1},
    {2008,  1, 15, 10, 1},
    {2044,  4, 14,  1, 1},
    {2013,  8, 17,  5, 1},
    {1974,  8, 17,  5, 1},
    {1937, 11, 16,  8, 1},
    /* CORRECT: drikpanchang agrees — day 1 of new month */
    {1963,  2, 13, 11, 0},
    {1909,  7, 16,  4, 0},
    {1947,  5, 15,  2, 0},
    {1927,  4, 14,  1, 0},
    {1905,  3, 14, 12, 0},
    {1991,  7, 17,  4, 0},
    {2005,  4, 14,  1, 0},
    {2025,  5, 15,  2, 0},
    {1942,  6, 15,  3, 0},
    {2030,  7, 17,  4, 0},
    {1997, 12, 16,  9, 0},
    {2015, 11, 17,  8, 0},
    {2020,  6, 15,  3, 0},
    {1948,  7, 16,  4, 0},
};
#define N_ENTRIES (sizeof(entries) / sizeof(entries[0]))

/* Longitudes to test */
#define LON_IST     82.5
#define LON_UJJAIN  75.7683   /* 75deg 46' 6" */
#define LON_DELHI   77.2090
#define LON_KOLKATA 88.3639

/* Compute JD (UT) of EoT-based apparent midnight at given longitude for a date.
 * "Apparent midnight" = moment when local apparent solar time = 0 (sun at lower meridian).
 *
 * Algorithm:
 *   Mean midnight UT = JD_noon - 0.5 - geolon/360
 *   E = equation_of_time at mean_midnight (in days)
 *   Apparent midnight UT ≈ mean_midnight - E
 *   (Iterate once for better accuracy)
 */
static double eot_apparent_midnight(double jd_midnight_ut, double geolon)
{
    char serr[256];
    double E;

    /* Mean midnight UT for this longitude.
     * Our gregorian_to_jd returns JD at 0h UT (midnight-based).
     * Local mean midnight at east longitude λ = UT midnight - λ/360 days.
     * E.g. IST midnight (82.5°E) = UT midnight - 5.5h = 18:30 UT prev day. */
    double jd_mean_mid = jd_midnight_ut - geolon / 360.0;

    /* First approximation */
    swe_time_equ(jd_mean_mid, &E, serr);
    double jd_app = jd_mean_mid - E;

    /* Second iteration */
    swe_time_equ(jd_app, &E, serr);
    jd_app = jd_mean_mid - E;

    return jd_app;
}

/* Compute nishita (sunset+sunrise midpoint) at given location */
static double nishita_midnight(double jd_midnight_ut, Location *loc)
{
    double jd_prev = jd_midnight_ut - 1.0;  /* previous day for sunset */
    double ss = sunset_jd(jd_prev, loc);
    double sr = sunrise_jd(jd_midnight_ut, loc);
    return (ss + sr) / 2.0;
}

/* Format JD as IST time (HH:MM:SS) */
static void jd_to_ist_str(double jd_ut, char *buf, int buflen)
{
    double ist = jd_ut + 5.5 / 24.0 + 0.5;
    double frac = ist - floor(ist);
    double secs = frac * 86400.0;
    int h = (int)(secs / 3600.0);
    int m = (int)(fmod(secs, 3600.0) / 60.0);
    int s = (int)(fmod(secs, 60.0));
    snprintf(buf, buflen, "%02d:%02d:%02d", h, m, s);
}

/* Check a rule: returns how many entries match.
 * For WRONG: need sank > cutoff (assigns to next day)
 * For CORRECT: need sank <= cutoff (assigns to current day) */
typedef double (*cutoff_fn)(int gy, int gm, int gd, void *ctx);

static void check_rule(const char *name, cutoff_fn fn, void *ctx,
                       int *score, int *w_ok, int *c_ok)
{
    *score = 0; *w_ok = 0; *c_ok = 0;
    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;
        int wrong = entries[i].wrong;

        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd_est = gregorian_to_jd(gy, gm, gd);
        double jd_sank = sankranti_jd(jd_est, target_long);

        double jd_cutoff = fn(gy, gm, gd, ctx);

        int ok = wrong ? (jd_sank > jd_cutoff) : (jd_sank <= jd_cutoff);
        if (ok) {
            (*score)++;
            if (wrong) (*w_ok)++;
            else (*c_ok)++;
        }
    }
}

/* Cutoff functions for different rules */
static double cutoff_eot(int gy, int gm, int gd, void *ctx)
{
    double geolon = *(double *)ctx;
    double jd = gregorian_to_jd(gy, gm, gd);  /* midnight UT */
    return eot_apparent_midnight(jd, geolon);
}

static double cutoff_nishita(int gy, int gm, int gd, void *ctx)
{
    Location *loc = (Location *)ctx;
    double jd = gregorian_to_jd(gy, gm, gd);  /* midnight UT */
    return nishita_midnight(jd, loc);
}

static double cutoff_mean_midnight(int gy, int gm, int gd, void *ctx)
{
    double geolon = *(double *)ctx;
    double jd = gregorian_to_jd(gy, gm, gd);  /* midnight UT */
    return jd - geolon / 360.0;
}

int main(void)
{
    astro_init(NULL);

    Location delhi   = DEFAULT_LOCATION;
    Location kolkata = { 22.5726, 88.3639, 0.0, 5.5 };
    Location ujjain  = { 23.15,   75.7683, 0.0, 5.5 };

    double lon_ist = LON_IST;
    double lon_ujjain = LON_UJJAIN;
    double lon_delhi = LON_DELHI;
    double lon_kolkata = LON_KOLKATA;

    printf("# Bengali Edge Case Analysis — Equation of Time Midnight\n");
    printf("# Testing EoT-based apparent midnight vs nishita vs mean midnight\n");
    printf("# Total: %d entries (23 wrong + 14 correct)\n", (int)N_ENTRIES);
    printf("#\n");

    /* ====== Part 1: Score each rule ====== */
    printf("# === RULE SCORES ===\n");
    printf("# Rule                              Score   W_ok/23  C_ok/14\n");

    struct { const char *name; cutoff_fn fn; void *ctx; } rules[] = {
        { "EoT midnight IST (82.5E)",    cutoff_eot,           &lon_ist },
        { "EoT midnight Ujjain (75.8E)", cutoff_eot,           &lon_ujjain },
        { "EoT midnight Delhi (77.2E)",  cutoff_eot,           &lon_delhi },
        { "EoT midnight Kolkata (88.4E)",cutoff_eot,           &lon_kolkata },
        { "Mean midnight IST (82.5E)",   cutoff_mean_midnight, &lon_ist },
        { "Mean midnight Ujjain (75.8E)",cutoff_mean_midnight, &lon_ujjain },
        { "Mean midnight Delhi (77.2E)", cutoff_mean_midnight, &lon_delhi },
        { "Mean midnight Kolkata (88.4E)",cutoff_mean_midnight,&lon_kolkata },
        { "Nishita Delhi",               cutoff_nishita,       &delhi },
        { "Nishita Kolkata",             cutoff_nishita,       &kolkata },
        { "Nishita Ujjain",              cutoff_nishita,       &ujjain },
    };
    int n_rules = sizeof(rules) / sizeof(rules[0]);

    for (int r = 0; r < n_rules; r++) {
        int score, w_ok, c_ok;
        check_rule(rules[r].name, rules[r].fn, rules[r].ctx,
                   &score, &w_ok, &c_ok);
        printf("  %-35s %2d/%d    %2d/23    %2d/14\n",
               rules[r].name, score, (int)N_ENTRIES, w_ok, c_ok);
    }

    /* ====== Part 2: EoT buffer sweep ====== */
    printf("\n# === EoT MIDNIGHT BUFFER SWEEP ===\n");
    printf("# Testing EoT_midnight + buffer (in minutes)\n");
    printf("# Only showing scores >= 35/38\n\n");
    printf("# buf(min)  IST_score  Ujj_score  Del_score  Kol_score\n");

    for (int buf = -30; buf <= 30; buf++) {
        double buf_jd = buf / (24.0 * 60.0);
        int scores[4] = {0};
        double lons[4] = { LON_IST, LON_UJJAIN, LON_DELHI, LON_KOLKATA };

        for (int li = 0; li < 4; li++) {
            for (int i = 0; i < (int)N_ENTRIES; i++) {
                int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
                int rashi = entries[i].rashi;
                int wrong = entries[i].wrong;

                double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
                double jd_est = gregorian_to_jd(gy, gm, gd);
                double jd_sank = sankranti_jd(jd_est, target_long);

                double jd_cutoff = eot_apparent_midnight(jd_est, lons[li]) + buf_jd;
                int ok = wrong ? (jd_sank > jd_cutoff) : (jd_sank <= jd_cutoff);
                if (ok) scores[li]++;
            }
        }

        int best = 0;
        for (int li = 0; li < 4; li++)
            if (scores[li] > best) best = scores[li];

        if (best >= 35) {
            printf("  %+3d min    %2d/%d     %2d/%d     %2d/%d     %2d/%d\n",
                   buf, scores[0], (int)N_ENTRIES,
                   scores[1], (int)N_ENTRIES,
                   scores[2], (int)N_ENTRIES,
                   scores[3], (int)N_ENTRIES);
        }
    }

    /* ====== Part 3: Per-entry detail ====== */
    printf("\n# === PER-ENTRY DETAIL ===\n");
    printf("# For each entry: sankranti IST, EoT midnight (IST, Ujjain), nishita (Delhi, Kolkata)\n");
    printf("# delta = sankranti - cutoff (minutes). Positive = sank after cutoff.\n");
    printf("# For a rule to work: all W positive, all C negative or zero.\n\n");

    printf("%-12s W/C ra  sank_IST   eot_IST    eot_Ujj    nish_Del   nish_Kol   "
           "d_eIST  d_eUjj  d_nDel  d_nKol  EoT_min\n",
           "date");

    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;
        int wrong = entries[i].wrong;

        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd = gregorian_to_jd(gy, gm, gd);  /* midnight UT */
        double jd_sank = sankranti_jd(jd, target_long);

        /* EoT midnight at IST and Ujjain */
        double eot_ist = eot_apparent_midnight(jd, LON_IST);
        double eot_ujj = eot_apparent_midnight(jd, LON_UJJAIN);

        /* Nishita at Delhi and Kolkata */
        double nish_del = nishita_midnight(jd, &delhi);
        double nish_kol = nishita_midnight(jd, &kolkata);

        /* Get raw equation of time at mean midnight IST */
        char serr[256];
        double E;
        swe_time_equ(jd - LON_IST / 360.0, &E, serr);
        double eot_minutes = E * 24.0 * 60.0;

        /* Deltas in minutes */
        double d_eist = (jd_sank - eot_ist) * 24.0 * 60.0;
        double d_eujj = (jd_sank - eot_ujj) * 24.0 * 60.0;
        double d_ndel = (jd_sank - nish_del) * 24.0 * 60.0;
        double d_nkol = (jd_sank - nish_kol) * 24.0 * 60.0;

        char s_sank[16], s_eist[16], s_eujj[16], s_ndel[16], s_nkol[16];
        jd_to_ist_str(jd_sank, s_sank, sizeof(s_sank));
        jd_to_ist_str(eot_ist, s_eist, sizeof(s_eist));
        jd_to_ist_str(eot_ujj, s_eujj, sizeof(s_eujj));
        jd_to_ist_str(nish_del, s_ndel, sizeof(s_ndel));
        jd_to_ist_str(nish_kol, s_nkol, sizeof(s_nkol));

        printf("%04d-%02d-%02d   %c  %2d  %s  %s  %s  %s  %s  "
               "%+6.1f  %+6.1f  %+6.1f  %+6.1f  %+5.1f\n",
               gy, gm, gd, wrong ? 'W' : 'C', rashi,
               s_sank, s_eist, s_eujj, s_ndel, s_nkol,
               d_eist, d_eujj, d_ndel, d_nkol, eot_minutes);
    }

    /* ====== Part 4: Sorted by delta from EoT midnight IST ====== */
    printf("\n# === SORTED BY DELTA FROM EoT MIDNIGHT IST ===\n");
    printf("# If EoT midnight IST is the rule, all W should have delta > 0,\n");
    printf("# and all C should have delta <= 0.\n\n");

    struct { int idx; double delta; } sorted[38];
    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;

        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd = gregorian_to_jd(gy, gm, gd);  /* midnight UT */
        double jd_sank = sankranti_jd(jd, target_long);
        double eot_ist = eot_apparent_midnight(jd, LON_IST);

        sorted[i].idx = i;
        sorted[i].delta = (jd_sank - eot_ist) * 24.0 * 60.0;
    }

    /* Insertion sort */
    for (int i = 1; i < (int)N_ENTRIES; i++) {
        for (int j = i; j > 0 && sorted[j].delta < sorted[j-1].delta; j--) {
            int ti = sorted[j].idx; double td = sorted[j].delta;
            sorted[j].idx = sorted[j-1].idx; sorted[j].delta = sorted[j-1].delta;
            sorted[j-1].idx = ti; sorted[j-1].delta = td;
        }
    }

    printf("%-12s W/C ra  sank_IST   d_eot_IST  EoT_min  (sorted by delta from EoT midnight IST)\n", "date");
    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int idx = sorted[i].idx;
        int gy = entries[idx].gy, gm = entries[idx].gm, gd = entries[idx].gd;
        int rashi = entries[idx].rashi;
        int wrong = entries[idx].wrong;

        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd = gregorian_to_jd(gy, gm, gd);  /* midnight UT */
        double jd_sank = sankranti_jd(jd, target_long);

        char serr[256]; double E;
        swe_time_equ(jd - LON_IST / 360.0, &E, serr);

        char s_sank[16];
        jd_to_ist_str(jd_sank, s_sank, sizeof(s_sank));

        printf("%04d-%02d-%02d   %c  %2d  %s  %+7.1f    %+5.1f\n",
               gy, gm, gd, wrong ? 'W' : 'C', rashi,
               s_sank, sorted[i].delta, E * 24.0 * 60.0);
    }

    /* ====== Part 5: Compare EoT midnight vs nishita difference ====== */
    printf("\n# === EoT MIDNIGHT vs NISHITA COMPARISON ===\n");
    printf("# How much does EoT midnight differ from nishita at each location?\n\n");
    printf("%-12s ra  eot_IST    nish_Del   diff(min)  eot_Kol    nish_Kol   diff(min)\n", "date");

    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;

        double jd = gregorian_to_jd(gy, gm, gd);  /* midnight UT */
        double eot_ist = eot_apparent_midnight(jd, LON_IST);
        double eot_kol = eot_apparent_midnight(jd, LON_KOLKATA);
        double nish_del = nishita_midnight(jd, &delhi);
        double nish_kol = nishita_midnight(jd, &kolkata);

        double diff_ist_del = (eot_ist - nish_del) * 24.0 * 60.0;
        double diff_kol = (eot_kol - nish_kol) * 24.0 * 60.0;

        char s_ei[16], s_nd[16], s_ek[16], s_nk[16];
        jd_to_ist_str(eot_ist, s_ei, sizeof(s_ei));
        jd_to_ist_str(nish_del, s_nd, sizeof(s_nd));
        jd_to_ist_str(eot_kol, s_ek, sizeof(s_ek));
        jd_to_ist_str(nish_kol, s_nk, sizeof(s_nk));

        printf("%04d-%02d-%02d  %2d  %s  %s  %+6.1f     %s  %s  %+6.1f\n",
               gy, gm, gd, rashi,
               s_ei, s_nd, diff_ist_del,
               s_ek, s_nk, diff_kol);
    }

    astro_close();
    return 0;
}
