/*
 * bengali_weekday.c — Analyze day-of-week patterns in Bengali edge cases.
 *
 * For each of the 37 Bengali edge case entries (23 W + 14 C), compute the
 * day of week and look for patterns in (rashi, weekday) combinations.
 *
 * W = "wrong": drikpanchang treats as after midnight (month starts next day)
 * C = "correct": drikpanchang treats as before midnight (civil date = day 1)
 *
 * Build:
 *   cc -O2 -Isrc -Ilib/swisseph tools/bengali_weekday.c \
 *       build/astro.o build/date_utils.o build/tithi.o build/masa.o \
 *       build/panchang.o build/solar.o build/swe/*.o -lm -o build/bengali_weekday
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include "swephexp.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

static struct {
    int gy, gm, gd;
    int rashi;
    int wrong;  /* 1 = W (after midnight), 0 = C (before midnight) */
} entries[] = {
    /* 23 WRONG (W) entries */
    {1908,  5, 14,  2, 1}, {1976, 10, 17,  7, 1}, {2022,  3, 15, 12, 1},
    {1983,  3, 15, 12, 1}, {2047,  1, 15, 10, 1}, {1935,  8, 17,  5, 1},
    {1937, 10, 17,  7, 1}, {1944,  3, 14, 12, 1}, {2015, 10, 18,  7, 1},
    {1935,  9, 17,  6, 1}, {1981,  6, 15,  3, 1}, {1903,  6, 15,  3, 1},
    {1969,  1, 14, 10, 1}, {1930,  1, 14, 10, 1}, {1986,  5, 15,  2, 1},
    {1976, 11, 16,  8, 1}, {1924,  2, 13, 11, 1}, {2036, 12, 16,  9, 1},
    {2008,  1, 15, 10, 1}, {2044,  4, 14,  1, 1}, {2013,  8, 17,  5, 1},
    {1974,  8, 17,  5, 1}, {1937, 11, 16,  8, 1},
    /* 14 CORRECT (C) entries */
    {1963,  2, 13, 11, 0}, {1909,  7, 16,  4, 0}, {1947,  5, 15,  2, 0},
    {1927,  4, 14,  1, 0}, {1905,  3, 14, 12, 0}, {1991,  7, 17,  4, 0},
    {2005,  4, 14,  1, 0}, {2025,  5, 15,  2, 0}, {1942,  6, 15,  3, 0},
    {2030,  7, 17,  4, 0}, {1997, 12, 16,  9, 0}, {2015, 11, 17,  8, 0},
    {2020,  6, 15,  3, 0}, {1948,  7, 16,  4, 0},
};
#define N (sizeof(entries) / sizeof(entries[0]))

static const char *RASHI_NAMES[] = {
    "", "Mesha", "Vrishabha", "Mithuna", "Karka", "Simha", "Kanya",
    "Tula", "Vrishchika", "Dhanu", "Makara", "Kumbha", "Meena"
};

/* swe_day_of_week: 0=Mon, 1=Tue, ..., 5=Sat, 6=Sun */
static const char *DOW_NAMES[] = {
    "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};
static const char *DOW_SHORT[] = {
    "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

/* Ayanamsa difference model: da = 24.10 + 0.003*(year-2000) arcsec */
static double ayanamsa_diff_arcsec(int year)
{
    return 24.10 + 0.003 * (year - 2000);
}

static void jd_to_ist_hms(double jd_ut, int *h, int *m, int *s)
{
    double ist = jd_ut + 5.5 / 24.0;
    double frac = ist + 0.5 - floor(ist + 0.5);
    double secs = frac * 86400.0;
    *h = (int)(secs / 3600.0);
    *m = (int)(fmod(secs, 3600.0) / 60.0);
    *s = (int)(fmod(secs, 60.0));
}

int main(void)
{
    astro_init(NULL);

    /* Computed data for each entry */
    struct {
        int gy, gm, gd, rashi, wrong;
        int dow;           /* 0=Mon..6=Sun */
        double jd_sank;    /* our sankranti JD */
        double jd_dp;      /* drikpanchang shifted sankranti JD */
        int h_our, m_our, s_our;   /* our IST time */
        int h_dp, m_dp, s_dp;      /* dp IST time */
    } d[37];

    /* ====== Compute all data ====== */
    for (int i = 0; i < (int)N; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;

        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd = gregorian_to_jd(gy, gm, gd);
        double jd_sank = sankranti_jd(jd, target_long);

        /* Day of week for the Gregorian date */
        int dow = swe_day_of_week(jd);

        /* Sun's sidereal daily motion */
        char serr[256];
        double x[6];
        swe_calc_ut(jd_sank, SE_SUN, SEFLG_SIDEREAL | SEFLG_SPEED, x, serr);
        double daily_mot = fabs(x[3]);

        /* Shifted (dp) sankranti */
        double da = ayanamsa_diff_arcsec(gy);
        double shift_days = da / (daily_mot * 3600.0);
        double jd_dp = jd_sank + shift_days;

        d[i].gy = gy; d[i].gm = gm; d[i].gd = gd;
        d[i].rashi = rashi;
        d[i].wrong = entries[i].wrong;
        d[i].dow = dow;
        d[i].jd_sank = jd_sank;
        d[i].jd_dp = jd_dp;
        jd_to_ist_hms(jd_sank, &d[i].h_our, &d[i].m_our, &d[i].s_our);
        jd_to_ist_hms(jd_dp, &d[i].h_dp, &d[i].m_dp, &d[i].s_dp);
    }

    /* ====== SECTION 1: Full table sorted by rashi ====== */
    printf("========================================================================\n");
    printf("  SECTION 1: Full Table (sorted by rashi, then date)\n");
    printf("========================================================================\n\n");

    /* Sort indices by rashi, then by date */
    int idx[37];
    for (int i = 0; i < (int)N; i++) idx[i] = i;
    for (int i = 1; i < (int)N; i++) {
        for (int j = i; j > 0; j--) {
            int a = idx[j], b = idx[j-1];
            if (d[a].rashi < d[b].rashi ||
                (d[a].rashi == d[b].rashi && d[a].gy < d[b].gy)) {
                idx[j] = idx[j-1];
                idx[j-1] = a;
            } else break;
        }
    }

    printf("%-12s  W/C  Rashi  %-10s  %-3s  our_IST    dp_IST\n",
           "Date", "Rashi#", "DoW");
    printf("----------  ---  -----  ----------  ---  ---------  ---------\n");

    int prev_rashi = -1;
    for (int i = 0; i < (int)N; i++) {
        int k = idx[i];
        if (d[k].rashi != prev_rashi && prev_rashi != -1)
            printf("\n");
        prev_rashi = d[k].rashi;

        printf("%04d-%02d-%02d    %c   %2d    %-10s  %s  %02d:%02d:%02d   %02d:%02d:%02d\n",
               d[k].gy, d[k].gm, d[k].gd,
               d[k].wrong ? 'W' : 'C',
               d[k].rashi,
               RASHI_NAMES[d[k].rashi],
               DOW_SHORT[d[k].dow],
               d[k].h_our, d[k].m_our, d[k].s_our,
               d[k].h_dp, d[k].m_dp, d[k].s_dp);
    }

    /* ====== SECTION 2: Cross-tabulation (rashi x weekday) ====== */
    printf("\n\n========================================================================\n");
    printf("  SECTION 2: Cross-tabulation (Rashi x Weekday) — W/C counts\n");
    printf("========================================================================\n\n");

    /* Count W and C for each (rashi, dow) pair */
    int w_count[13][7]; /* [rashi 1-12][dow 0-6] */
    int c_count[13][7];
    memset(w_count, 0, sizeof(w_count));
    memset(c_count, 0, sizeof(c_count));

    for (int i = 0; i < (int)N; i++) {
        if (d[i].wrong)
            w_count[d[i].rashi][d[i].dow]++;
        else
            c_count[d[i].rashi][d[i].dow]++;
    }

    printf("%-12s  %5s %5s %5s %5s %5s %5s %5s   Total\n",
           "Rashi", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun");
    printf("----------    ----  ----  ----  ----  ----  ----  ----   -----\n");

    for (int r = 1; r <= 12; r++) {
        int total_w = 0, total_c = 0;
        int has_any = 0;
        for (int dow = 0; dow < 7; dow++) {
            if (w_count[r][dow] || c_count[r][dow]) has_any = 1;
            total_w += w_count[r][dow];
            total_c += c_count[r][dow];
        }
        if (!has_any) continue;

        printf("%-10s  ", RASHI_NAMES[r]);
        for (int dow = 0; dow < 7; dow++) {
            int w = w_count[r][dow], c = c_count[r][dow];
            if (w == 0 && c == 0)
                printf("  .   ");
            else if (w > 0 && c > 0)
                printf("%dW%dC ", w, c);
            else if (w > 0)
                printf(" %dW   ", w);
            else
                printf(" %dC   ", c);
        }
        printf("  %dW %dC\n", total_w, total_c);
    }

    /* Totals row */
    printf("\n%-10s  ", "TOTAL");
    for (int dow = 0; dow < 7; dow++) {
        int tw = 0, tc = 0;
        for (int r = 1; r <= 12; r++) {
            tw += w_count[r][dow];
            tc += c_count[r][dow];
        }
        if (tw == 0 && tc == 0)
            printf("  .   ");
        else
            printf("%dW%dC ", tw, tc);
    }
    printf("\n");

    /* ====== SECTION 3: Per-rashi pattern ====== */
    printf("\n\n========================================================================\n");
    printf("  SECTION 3: Pattern Analysis — For each rashi, W vs C weekdays\n");
    printf("========================================================================\n\n");

    for (int r = 1; r <= 12; r++) {
        int has_any = 0;
        for (int dow = 0; dow < 7; dow++)
            if (w_count[r][dow] || c_count[r][dow]) has_any = 1;
        if (!has_any) continue;

        printf("Rashi %2d (%-10s):\n", r, RASHI_NAMES[r]);

        /* W weekdays */
        printf("  W days: ");
        int w_any = 0;
        for (int dow = 0; dow < 7; dow++) {
            if (w_count[r][dow]) {
                if (w_any) printf(", ");
                printf("%s(%d)", DOW_NAMES[dow], w_count[r][dow]);
                w_any = 1;
            }
        }
        if (!w_any) printf("(none)");
        printf("\n");

        /* C weekdays */
        printf("  C days: ");
        int c_any = 0;
        for (int dow = 0; dow < 7; dow++) {
            if (c_count[r][dow]) {
                if (c_any) printf(", ");
                printf("%s(%d)", DOW_NAMES[dow], c_count[r][dow]);
                c_any = 1;
            }
        }
        if (!c_any) printf("(none)");
        printf("\n");

        /* Check if there's a clean separation */
        int mixed = 0;
        for (int dow = 0; dow < 7; dow++)
            if (w_count[r][dow] && c_count[r][dow]) mixed = 1;
        if (mixed)
            printf("  ** MIXED: same weekday appears in both W and C **\n");
        else if (w_any && c_any)
            printf("  Clean separation: W and C on different weekdays.\n");
        printf("\n");
    }

    /* ====== SECTION 4: Per-weekday pattern ====== */
    printf("\n========================================================================\n");
    printf("  SECTION 4: Pattern Analysis — For each weekday, W vs C rashis\n");
    printf("========================================================================\n\n");

    for (int dow = 0; dow < 7; dow++) {
        int has_any = 0;
        for (int r = 1; r <= 12; r++)
            if (w_count[r][dow] || c_count[r][dow]) has_any = 1;
        if (!has_any) continue;

        printf("%s:\n", DOW_NAMES[dow]);

        /* W rashis */
        printf("  W rashis: ");
        int w_any = 0;
        for (int r = 1; r <= 12; r++) {
            if (w_count[r][dow]) {
                if (w_any) printf(", ");
                printf("%s(%d)", RASHI_NAMES[r], w_count[r][dow]);
                w_any = 1;
            }
        }
        if (!w_any) printf("(none)");
        printf("\n");

        /* C rashis */
        printf("  C rashis: ");
        int c_any = 0;
        for (int r = 1; r <= 12; r++) {
            if (c_count[r][dow]) {
                if (c_any) printf(", ");
                printf("%s(%d)", RASHI_NAMES[r], c_count[r][dow]);
                c_any = 1;
            }
        }
        if (!c_any) printf("(none)");
        printf("\n");

        /* Check if there's a clean separation */
        int mixed = 0;
        for (int r = 1; r <= 12; r++)
            if (w_count[r][dow] && c_count[r][dow]) mixed = 1;
        if (mixed)
            printf("  ** MIXED: same rashi appears in both W and C for this weekday **\n");
        else if (w_any && c_any)
            printf("  Clean separation: W and C in different rashis.\n");
        printf("\n");
    }

    /* ====== SECTION 5: Summary statistics ====== */
    printf("\n========================================================================\n");
    printf("  SECTION 5: Summary Statistics\n");
    printf("========================================================================\n\n");

    /* Weekday distribution */
    printf("Weekday distribution (all entries):\n");
    for (int dow = 0; dow < 7; dow++) {
        int tw = 0, tc = 0;
        for (int r = 1; r <= 12; r++) {
            tw += w_count[r][dow];
            tc += c_count[r][dow];
        }
        if (tw + tc > 0) {
            printf("  %-10s: %2d total  (%2dW, %2dC)  W%%=%.0f%%\n",
                   DOW_NAMES[dow], tw+tc, tw, tc,
                   (tw+tc) > 0 ? 100.0*tw/(tw+tc) : 0.0);
        }
    }

    /* Rashi distribution */
    printf("\nRashi distribution (all entries):\n");
    for (int r = 1; r <= 12; r++) {
        int tw = 0, tc = 0;
        for (int dow = 0; dow < 7; dow++) {
            tw += w_count[r][dow];
            tc += c_count[r][dow];
        }
        if (tw + tc > 0) {
            printf("  %2d %-10s: %2d total  (%2dW, %2dC)  W%%=%.0f%%\n",
                   r, RASHI_NAMES[r], tw+tc, tw, tc,
                   (tw+tc) > 0 ? 100.0*tw/(tw+tc) : 0.0);
        }
    }

    /* Check: is weekday alone a predictor? */
    printf("\n--- Can weekday alone predict W/C? ---\n");
    int weekday_only_correct = 0;
    for (int dow = 0; dow < 7; dow++) {
        int tw = 0, tc = 0;
        for (int r = 1; r <= 12; r++) {
            tw += w_count[r][dow];
            tc += c_count[r][dow];
        }
        /* Best prediction for this weekday */
        if (tw >= tc)
            weekday_only_correct += tw;
        else
            weekday_only_correct += tc;
    }
    printf("  Best weekday-only prediction: %d/%d correct (%.1f%%)\n",
           weekday_only_correct, (int)N, 100.0*weekday_only_correct/N);

    /* Check: is rashi alone a predictor? */
    printf("\n--- Can rashi alone predict W/C? ---\n");
    int rashi_only_correct = 0;
    for (int r = 1; r <= 12; r++) {
        int tw = 0, tc = 0;
        for (int dow = 0; dow < 7; dow++) {
            tw += w_count[r][dow];
            tc += c_count[r][dow];
        }
        if (tw >= tc)
            rashi_only_correct += tw;
        else
            rashi_only_correct += tc;
    }
    printf("  Best rashi-only prediction: %d/%d correct (%.1f%%)\n",
           rashi_only_correct, (int)N, 100.0*rashi_only_correct/N);

    /* Check: can (rashi, weekday) pair predict W/C perfectly? */
    printf("\n--- Can (rashi, weekday) pair predict W/C? ---\n");
    int pair_correct = 0;
    int pair_ambiguous = 0;
    int pair_total_cells = 0;
    for (int r = 1; r <= 12; r++) {
        for (int dow = 0; dow < 7; dow++) {
            int w = w_count[r][dow], c = c_count[r][dow];
            if (w + c == 0) continue;
            pair_total_cells++;
            if (w > 0 && c > 0) {
                pair_ambiguous++;
                pair_correct += (w > c) ? w : c;
            } else {
                pair_correct += w + c;
            }
        }
    }
    printf("  Total (rashi,weekday) cells with data: %d\n", pair_total_cells);
    printf("  Ambiguous cells (both W and C): %d\n", pair_ambiguous);
    printf("  Best (rashi,weekday) prediction: %d/%d correct (%.1f%%)\n",
           pair_correct, (int)N, 100.0*pair_correct/N);
    if (pair_ambiguous == 0)
        printf("  ==> PERFECT SEPARATION: (rashi, weekday) fully determines W/C!\n");

    /* ====== SECTION 6: Weekday modular patterns ====== */
    printf("\n\n========================================================================\n");
    printf("  SECTION 6: Exploring (rashi + weekday) mod N patterns\n");
    printf("========================================================================\n\n");

    /* Try (rashi + dow) mod various moduli */
    for (int mod = 2; mod <= 12; mod++) {
        int score = 0;
        for (int val = 0; val < mod; val++) {
            int tw = 0, tc = 0;
            for (int i = 0; i < (int)N; i++) {
                if ((d[i].rashi + d[i].dow) % mod == val) {
                    if (d[i].wrong) tw++; else tc++;
                }
            }
            score += (tw > tc) ? tw : tc;
        }
        printf("  (rashi + dow) mod %2d => best = %d/%d (%.1f%%)\n",
               mod, score, (int)N, 100.0*score/N);
    }

    printf("\n");
    /* Also try (rashi * dow) mod various */
    for (int mod = 2; mod <= 12; mod++) {
        int score = 0;
        for (int val = 0; val < mod; val++) {
            int tw = 0, tc = 0;
            for (int i = 0; i < (int)N; i++) {
                if ((d[i].rashi * d[i].dow) % mod == val) {
                    if (d[i].wrong) tw++; else tc++;
                }
            }
            score += (tw > tc) ? tw : tc;
        }
        printf("  (rashi * dow) mod %2d => best = %d/%d (%.1f%%)\n",
               mod, score, (int)N, 100.0*score/N);
    }

    /* ====== SECTION 7: Weekday parity and rashi odd/even ====== */
    printf("\n\n========================================================================\n");
    printf("  SECTION 7: Simple binary patterns\n");
    printf("========================================================================\n\n");

    /* Weekday parity */
    {
        int w_even = 0, c_even = 0, w_odd = 0, c_odd = 0;
        for (int i = 0; i < (int)N; i++) {
            if (d[i].dow % 2 == 0) {
                if (d[i].wrong) w_even++; else c_even++;
            } else {
                if (d[i].wrong) w_odd++; else c_odd++;
            }
        }
        printf("Weekday parity (0=Mon,2=Wed,4=Fri,6=Sun are even):\n");
        printf("  Even weekday: %dW, %dC\n", w_even, c_even);
        printf("  Odd weekday:  %dW, %dC\n", w_odd, c_odd);
    }

    /* Rashi parity */
    {
        int w_even = 0, c_even = 0, w_odd = 0, c_odd = 0;
        for (int i = 0; i < (int)N; i++) {
            if (d[i].rashi % 2 == 0) {
                if (d[i].wrong) w_even++; else c_even++;
            } else {
                if (d[i].wrong) w_odd++; else c_odd++;
            }
        }
        printf("\nRashi parity:\n");
        printf("  Even rashi (2,4,6,8,10,12): %dW, %dC\n", w_even, c_even);
        printf("  Odd rashi  (1,3,5,7,9,11):  %dW, %dC\n", w_odd, c_odd);
    }

    /* Weekend vs weekday */
    {
        int w_wkend = 0, c_wkend = 0, w_wkday = 0, c_wkday = 0;
        for (int i = 0; i < (int)N; i++) {
            if (d[i].dow >= 5) {  /* Sat=5, Sun=6 */
                if (d[i].wrong) w_wkend++; else c_wkend++;
            } else {
                if (d[i].wrong) w_wkday++; else c_wkday++;
            }
        }
        printf("\nWeekend (Sat/Sun) vs Weekday:\n");
        printf("  Weekend:  %dW, %dC\n", w_wkend, c_wkend);
        printf("  Weekday:  %dW, %dC\n", w_wkday, c_wkday);
    }

    astro_close();
    return 0;
}
