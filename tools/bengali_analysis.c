/*
 * bengali_analysis.c — Systematic analysis of Bengali solar calendar edge cases.
 *
 * For each of the 37 negative-delta entries (23 wrong + 14 correct),
 * computes candidate critical times and checks which rule cleanly
 * separates correct from wrong.
 *
 * Key candidates:
 *   - Apparent midnight (nishita) at Delhi and Kolkata
 *   - IST midnight (00:00)
 *   - Various fixed IST cutoffs
 *   - Kolkata local mean solar midnight
 *
 * Build:
 *   make && cc -O2 -Isrc -Ilib/swisseph tools/bengali_analysis.c \
 *       build/astro.o build/date_utils.o build/tithi.o build/masa.o \
 *       build/panchang.o build/solar.o build/swe/*.o -lm -o build/bengali_analysis
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
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

/* Convert JD (UT) to IST minutes after midnight (can be negative = before midnight) */
static double jd_to_ist_minutes(double jd_ut)
{
    double ist = jd_ut + 5.5 / 24.0;
    /* Get day fraction */
    double frac = ist + 0.5 - floor(ist + 0.5);
    return frac * 24.0 * 60.0;
}

/* Format IST minutes as HH:MM:SS */
static void fmt_ist(double mins, char *buf, int buflen)
{
    int neg = mins < 0;
    if (neg) mins = -mins;
    int h = (int)(mins / 60.0);
    int m = (int)(fmod(mins, 60.0));
    int s = (int)(fmod(mins * 60.0, 60.0));
    snprintf(buf, buflen, "%s%02d:%02d:%02d", neg ? "-" : "", h, m, s);
}

int main(void)
{
    astro_init(NULL);

    Location delhi = DEFAULT_LOCATION;
    Location kolkata = { 22.5726, 88.3639, 0.0, 5.5 };

    printf("# Bengali Edge Case Analysis\n");
    printf("# For each entry: sankranti IST, apparent midnight (Delhi & Kolkata), and rule check\n");
    printf("#\n");
    printf("# W = wrong (our code disagrees with drikpanchang)\n");
    printf("# C = correct\n");
    printf("# For a rule to work: all W entries need sank > cutoff, all C need sank <= cutoff\n");
    printf("#\n");

    printf("%-12s W/C ra  sank_IST   "
           "app_mid_del  app_mid_kol  "
           "del_ok  kol_ok  "
           "sank-amd  sank-amk\n",
           "date");

    int delhi_ok_count = 0, kolkata_ok_count = 0;
    int delhi_wrong_ok = 0, delhi_correct_ok = 0;
    int kolkata_wrong_ok = 0, kolkata_correct_ok = 0;

    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;
        int wrong = entries[i].wrong;

        /* Find sankranti */
        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd_est = gregorian_to_jd(gy, gm, gd);
        double jd_sank = sankranti_jd(jd_est, target_long);
        double sank_ist_min = jd_to_ist_minutes(jd_sank);

        /* Previous day's midnight UT (for sunset computation) */
        double jd_prev = gregorian_to_jd(gy, gm, gd) - 1.0;

        /* Apparent midnight Delhi = midpoint(sunset, next sunrise) */
        double ss_del = sunset_jd(jd_prev, &delhi);
        double sr_del = sunrise_jd(jd_est, &delhi);
        double am_del = (ss_del + sr_del) / 2.0;
        double am_del_min = jd_to_ist_minutes(am_del);

        /* Apparent midnight Kolkata */
        double ss_kol = sunset_jd(jd_prev, &kolkata);
        double sr_kol = sunrise_jd(jd_est, &kolkata);
        double am_kol = (ss_kol + sr_kol) / 2.0;
        double am_kol_min = jd_to_ist_minutes(am_kol);

        /* Check: apparent midnight rule
         * For WRONG entries: need sank > apparent_midnight (assign to next day)
         * For CORRECT entries: need sank <= apparent_midnight (assign to current day) */
        int del_ok = wrong ? (jd_sank > am_del) : (jd_sank <= am_del);
        int kol_ok = wrong ? (jd_sank > am_kol) : (jd_sank <= am_kol);

        if (del_ok) delhi_ok_count++;
        if (kol_ok) kolkata_ok_count++;
        if (wrong && del_ok) delhi_wrong_ok++;
        if (!wrong && del_ok) delhi_correct_ok++;
        if (wrong && kol_ok) kolkata_wrong_ok++;
        if (!wrong && kol_ok) kolkata_correct_ok++;

        double delta_del = (jd_sank - am_del) * 24.0 * 60.0;
        double delta_kol = (jd_sank - am_kol) * 24.0 * 60.0;

        char s_sank[16], s_amd[16], s_amk[16];
        fmt_ist(sank_ist_min, s_sank, sizeof(s_sank));
        fmt_ist(am_del_min, s_amd, sizeof(s_amd));
        fmt_ist(am_kol_min, s_amk, sizeof(s_amk));

        printf("%04d-%02d-%02d   %c  %2d  %s  %s     %s     "
               "  %s      %s    %+6.1f    %+6.1f\n",
               gy, gm, gd,
               wrong ? 'W' : 'C',
               rashi,
               s_sank, s_amd, s_amk,
               del_ok ? "YES" : " NO",
               kol_ok ? "YES" : " NO",
               delta_del, delta_kol);
    }

    printf("\n# Summary:\n");
    printf("#   Delhi apparent midnight:  %d/%d correct (%d/%d wrong, %d/%d correct)\n",
           delhi_ok_count, (int)N_ENTRIES, delhi_wrong_ok, 23, delhi_correct_ok, 14);
    printf("#   Kolkata apparent midnight: %d/%d correct (%d/%d wrong, %d/%d correct)\n",
           kolkata_ok_count, (int)N_ENTRIES, kolkata_wrong_ok, 23, kolkata_correct_ok, 14);

    /* Also try: apparent midnight + various buffers */
    printf("\n# === Buffer sweep: apparent_midnight + buffer ===\n");
    printf("# Checking buffers from -30 to +30 minutes\n\n");
    printf("# buffer_min  del_score  kol_score  del_wrong_ok  del_correct_ok  kol_wrong_ok  kol_correct_ok\n");

    for (int buf = -30; buf <= 30; buf++) {
        int d_score = 0, k_score = 0;
        int d_w = 0, d_c = 0, k_w = 0, k_c = 0;
        double buf_jd = buf / (24.0 * 60.0);

        for (int i = 0; i < (int)N_ENTRIES; i++) {
            int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
            int rashi = entries[i].rashi;
            int wrong = entries[i].wrong;

            double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
            double jd_est = gregorian_to_jd(gy, gm, gd);
            double jd_sank = sankranti_jd(jd_est, target_long);

            double jd_prev = gregorian_to_jd(gy, gm, gd) - 1.0;

            double ss_del = sunset_jd(jd_prev, &delhi);
            double sr_del = sunrise_jd(jd_est, &delhi);
            double am_del = (ss_del + sr_del) / 2.0 + buf_jd;

            double ss_kol = sunset_jd(jd_prev, &kolkata);
            double sr_kol = sunrise_jd(jd_est, &kolkata);
            double am_kol = (ss_kol + sr_kol) / 2.0 + buf_jd;

            int d_ok = wrong ? (jd_sank > am_del) : (jd_sank <= am_del);
            int k_ok = wrong ? (jd_sank > am_kol) : (jd_sank <= am_kol);

            if (d_ok) { d_score++; if (wrong) d_w++; else d_c++; }
            if (k_ok) { k_score++; if (wrong) k_w++; else k_c++; }
        }
        if (d_score >= 35 || k_score >= 35) {
            printf("  %+3d min     %2d/37      %2d/37        %2d/23           %2d/14           %2d/23           %2d/14\n",
                   buf, d_score, k_score, d_w, d_c, k_w, k_c);
        }
    }

    /* Also sweep fixed IST cutoffs from 23:30 to 00:30 */
    printf("\n# === Fixed IST cutoff sweep ===\n");
    printf("# Checking cutoffs from 23:30 to 00:30 IST\n\n");
    printf("# cutoff_IST  score  wrong_ok  correct_ok\n");

    for (int m = -30; m <= 30; m++) {
        /* Cutoff in IST minutes after midnight: m minutes */
        /* Negative = before midnight (e.g. -30 = 23:30) */
        double cutoff_jd_offset = m / (24.0 * 60.0);  /* offset from IST midnight */
        int score = 0, w_ok = 0, c_ok = 0;

        for (int i = 0; i < (int)N_ENTRIES; i++) {
            int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
            int rashi = entries[i].rashi;
            int wrong = entries[i].wrong;

            double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
            double jd_est = gregorian_to_jd(gy, gm, gd);
            double jd_sank = sankranti_jd(jd_est, target_long);

            /* IST midnight = JD of local midnight UT - 5.5/24 */
            double jd_ist_mid = jd_est - 5.5 / 24.0;
            double jd_cutoff = jd_ist_mid + cutoff_jd_offset;

            int ok = wrong ? (jd_sank > jd_cutoff) : (jd_sank <= jd_cutoff);
            if (ok) { score++; if (wrong) w_ok++; else c_ok++; }
        }
        if (score >= 35) {
            int h = (m >= 0) ? (m / 60) : (23 + (60 + m) / 60);
            int mm = (m >= 0) ? (m % 60) : ((60 + m) % 60);
            printf("  %02d:%02d IST    %2d/37    %2d/23       %2d/14\n",
                   h, mm, score, w_ok, c_ok);
        }
    }

    /* Per-entry: compute the EXACT apparent midnight and show delta */
    printf("\n# === Per-entry detail sorted by (sank - app_mid_kolkata) ===\n");
    printf("# This shows how far the sankranti is from Kolkata apparent midnight.\n");
    printf("# If the rule is app_mid_kolkata, all W should be positive, all C negative.\n\n");

    /* Sort entries by delta from Kolkata apparent midnight */
    struct {
        int idx;
        double delta_kol;
    } sorted[37];

    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;

        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd_est = gregorian_to_jd(gy, gm, gd);
        double jd_sank = sankranti_jd(jd_est, target_long);

        double jd_prev = gregorian_to_jd(gy, gm, gd) - 1.0;
        double ss_kol = sunset_jd(jd_prev, &kolkata);
        double sr_kol = sunrise_jd(jd_est, &kolkata);
        double am_kol = (ss_kol + sr_kol) / 2.0;

        sorted[i].idx = i;
        sorted[i].delta_kol = (jd_sank - am_kol) * 24.0 * 60.0;
    }

    /* Simple insertion sort */
    for (int i = 1; i < (int)N_ENTRIES; i++) {
        for (int j = i; j > 0 && sorted[j].delta_kol < sorted[j-1].delta_kol; j--) {
            int ti = sorted[j].idx;
            double td = sorted[j].delta_kol;
            sorted[j].idx = sorted[j-1].idx;
            sorted[j].delta_kol = sorted[j-1].delta_kol;
            sorted[j-1].idx = ti;
            sorted[j-1].delta_kol = td;
        }
    }

    printf("%-12s W/C ra  sank_IST   delta_kol  (sorted by delta from Kolkata apparent midnight)\n", "date");
    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int idx = sorted[i].idx;
        int gy = entries[idx].gy, gm = entries[idx].gm, gd = entries[idx].gd;
        int rashi = entries[idx].rashi;
        int wrong = entries[idx].wrong;

        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd_est = gregorian_to_jd(gy, gm, gd);
        double jd_sank = sankranti_jd(jd_est, target_long);
        double sank_min = jd_to_ist_minutes(jd_sank);

        char s_sank[16];
        fmt_ist(sank_min, s_sank, sizeof(s_sank));

        printf("%04d-%02d-%02d   %c  %2d  %s  %+7.1f\n",
               gy, gm, gd, wrong ? 'W' : 'C', rashi,
               s_sank, sorted[i].delta_kol);
    }

    astro_close();
    return 0;
}
