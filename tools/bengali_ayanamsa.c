/*
 * bengali_ayanamsa.c — Compare our SE_SIDM_LAHIRI ayanamsa with drikpanchang's
 * for all 37 Bengali edge case dates.
 *
 * For each date, computes:
 *   - Our Lahiri ayanamsa (SE_SIDM_LAHIRI)
 *   - The sankranti time in IST
 *   - The sun's sidereal daily motion (to convert ayanamsa offset to time)
 *   - The "critical ayanamsa difference" that would shift sankranti to IST midnight
 *   - What sankranti time drikpanchang would compute for various ayanamsa offsets
 *
 * The user can then provide drikpanchang's ayanamsa values from the website
 * to determine the exact offset and check if it explains the W/C pattern.
 *
 * Build:
 *   make && cc -O2 -Isrc -Ilib/swisseph tools/bengali_ayanamsa.c \
 *       build/astro.o build/date_utils.o build/tithi.o build/masa.o \
 *       build/panchang.o build/solar.o build/swe/*.o -lm -o build/bengali_ayanamsa
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include "swephexp.h"
#include <stdio.h>
#include <math.h>

static struct {
    int gy, gm, gd;
    int rashi;
    int wrong;
} entries[] = {
    /* WRONG entries */
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
    /* CORRECT entries */
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

int main(void)
{
    astro_init(NULL);

    printf("# Bengali Edge Case — Ayanamsa Comparison\n");
    printf("# For each entry: our Lahiri ayanamsa, sankranti IST time,\n");
    printf("# sun's daily motion, and critical ayanamsa offset.\n");
    printf("#\n");
    printf("# 'crit_da' = ayanamsa difference (arcsec) that would shift\n");
    printf("#             our sankranti exactly to IST midnight.\n");
    printf("#             W entries need da > crit_da, C entries need da <= crit_da.\n");
    printf("#\n");

    printf("%-12s W/C ra  sank_IST   our_ayan      daily_mot  ist_min  crit_da   year\n",
           "date");

    /* Also collect data for linear separability analysis */
    struct {
        double crit_da;  /* critical ayanamsa diff in arcsec */
        int year;
        int wrong;
        int gy, gm, gd, rashi;
        double ist_min;
        double our_ayan;
    } data[37];

    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;
        int wrong = entries[i].wrong;

        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd = gregorian_to_jd(gy, gm, gd);  /* midnight UT */
        double jd_sank = sankranti_jd(jd, target_long);

        /* IST time of sankranti (minutes after midnight) */
        double ist = jd_sank + 5.5 / 24.0;
        double ist_frac = ist + 0.5 - floor(ist + 0.5);
        double ist_min = ist_frac * 1440.0;

        /* Our Lahiri ayanamsa at the sankranti moment */
        double our_ayan = swe_get_ayanamsa_ut(jd_sank);

        /* Sun's sidereal daily motion at the sankranti (degrees/day) */
        /* Compute by finite difference: longitude at sank+0.5 minus sank-0.5 */
        char serr[256];
        double x1[6], x2[6];
        swe_calc_ut(jd_sank - 0.5, SE_SUN, SEFLG_SIDEREAL | SEFLG_SPEED, x1, serr);
        swe_calc_ut(jd_sank + 0.5, SE_SUN, SEFLG_SIDEREAL | SEFLG_SPEED, x2, serr);
        /* Use the speed from swe_calc directly (x[3] = speed in deg/day) */
        double daily_motion = x1[3];  /* degrees per day */
        if (daily_motion < 0) daily_motion = -daily_motion;

        /* Critical ayanamsa difference:
         * sankranti shifts by delta_t = delta_ayan / (daily_motion * 3600) days
         * For sankranti to shift to IST midnight, need:
         *   delta_t = ist_min / 1440 (days)
         *   delta_ayan = ist_min / 1440 * daily_motion * 3600 (arcsec)
         *   delta_ayan = ist_min * daily_motion * 3600 / 1440
         *   delta_ayan = ist_min * daily_motion * 2.5 (arcsec) */
        double crit_da = ist_min * daily_motion * 2.5;

        /* Format IST time */
        int h = (int)(ist_min / 60.0);
        int m = (int)(fmod(ist_min, 60.0));
        int s = (int)(fmod(ist_min * 60.0, 60.0));

        printf("%04d-%02d-%02d   %c  %2d  %02d:%02d:%02d  %12.6f  %8.4f   %5.1f   %6.1f   %d\n",
               gy, gm, gd,
               wrong ? 'W' : 'C',
               rashi,
               h, m, s,
               our_ayan,
               daily_motion,
               ist_min,
               crit_da,
               gy);

        data[i].crit_da = crit_da;
        data[i].year = gy;
        data[i].wrong = wrong;
        data[i].gy = gy; data[i].gm = gm; data[i].gd = gd;
        data[i].rashi = rashi;
        data[i].ist_min = ist_min;
        data[i].our_ayan = our_ayan;
    }

    /* Sort by crit_da for visualization */
    printf("\n# === SORTED BY CRITICAL AYANAMSA DIFFERENCE ===\n");
    printf("# If all W have crit_da above some line and all C below, we can separate them.\n\n");

    int sorted[37];
    for (int i = 0; i < (int)N_ENTRIES; i++) sorted[i] = i;
    for (int i = 1; i < (int)N_ENTRIES; i++) {
        for (int j = i; j > 0 && data[sorted[j]].crit_da < data[sorted[j-1]].crit_da; j--) {
            int t = sorted[j]; sorted[j] = sorted[j-1]; sorted[j-1] = t;
        }
    }

    printf("%-12s W/C ra  ist_min  crit_da  year  our_ayan\n", "date");
    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int idx = sorted[i];
        printf("%04d-%02d-%02d   %c  %2d  %5.1f   %6.1f   %4d  %12.6f\n",
               data[idx].gy, data[idx].gm, data[idx].gd,
               data[idx].wrong ? 'W' : 'C',
               data[idx].rashi,
               data[idx].ist_min,
               data[idx].crit_da,
               data[idx].year,
               data[idx].our_ayan);
    }

    /* Linear separability analysis: sweep a0 + a1*(year-2000) */
    printf("\n# === LINEAR AYANAMSA MODEL SWEEP ===\n");
    printf("# Testing da(year) = a0 + a1*(year-2000) arcsec\n");
    printf("# For correct classification:\n");
    printf("#   W entries: crit_da < da(year) [need larger shift to push past midnight]\n");
    printf("#   C entries: crit_da >= da(year) [shift not enough, stays before midnight]\n");
    printf("# Wait — direction check:\n");
    printf("#   If da > 0: our ayanamsa > drikpanchang's → our sankranti is LATER\n");
    printf("#   drikpanchang sank = our sank - delta_t → shifted EARLIER\n");
    printf("#   If shifted past midnight (earlier): assigned to previous day (= day 1)\n");
    printf("#   If shifted stays after midnight: assigned to next day (= last day)\n");
    printf("#\n");
    printf("# So: W entries (should be 'last day'): drikpanchang sank > midnight\n");
    printf("#     → our sank - delta_t > 0 → delta_t < ist_min\n");
    printf("#     → da < crit_da  (ayanamsa diff is NOT large enough to push past midnight)\n");
    printf("# And: C entries (should be 'day 1'): drikpanchang sank <= midnight\n");
    printf("#     → our sank - delta_t <= 0 → delta_t >= ist_min\n");
    printf("#     → da >= crit_da  (ayanamsa diff IS large enough)\n");
    printf("#\n");
    printf("# REVISED: W needs da < crit_da, C needs da >= crit_da\n\n");

    int best_score = 0;
    double best_a0 = 0, best_a1 = 0;

    /* Sweep a0 from 0 to 50 arcsec, a1 from -0.1 to +0.1 arcsec/year */
    for (int a0_10 = 0; a0_10 <= 500; a0_10++) {
        double a0 = a0_10 / 10.0;
        for (int a1_100 = -20; a1_100 <= 20; a1_100++) {
            double a1 = a1_100 / 100.0;
            int score = 0;

            for (int i = 0; i < (int)N_ENTRIES; i++) {
                double da = a0 + a1 * (data[i].year - 2000);
                int ok;
                if (data[i].wrong) {
                    ok = (da < data[i].crit_da);  /* W: da not enough */
                } else {
                    ok = (da >= data[i].crit_da);  /* C: da enough */
                }
                if (ok) score++;
            }
            if (score > best_score) {
                best_score = score;
                best_a0 = a0;
                best_a1 = a1;
            }
        }
    }

    printf("# Best linear model: da(year) = %.1f + %.2f*(year-2000) arcsec\n",
           best_a0, best_a1);
    printf("# Score: %d/%d\n\n", best_score, (int)N_ENTRIES);

    /* Show how this model classifies each entry */
    printf("%-12s W/C ra  ist_min  crit_da  da_model  margin   ok?\n", "date");
    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int idx = sorted[i];
        double da = best_a0 + best_a1 * (data[idx].year - 2000);
        int ok;
        if (data[idx].wrong) {
            ok = (da < data[idx].crit_da);
        } else {
            ok = (da >= data[idx].crit_da);
        }
        printf("%04d-%02d-%02d   %c  %2d  %5.1f   %6.1f   %6.1f   %+6.1f   %s\n",
               data[idx].gy, data[idx].gm, data[idx].gd,
               data[idx].wrong ? 'W' : 'C',
               data[idx].rashi,
               data[idx].ist_min,
               data[idx].crit_da,
               da,
               data[idx].wrong ? (data[idx].crit_da - da) : (da - data[idx].crit_da),
               ok ? "YES" : " NO");
    }

    /* Also show the ayanamsa values so user can compare with drikpanchang */
    printf("\n# === AYANAMSA VALUES FOR USER COMPARISON ===\n");
    printf("# Please compare our_ayan with drikpanchang's 'Lahiri Ayanamsha' field.\n");
    printf("# The difference (in arcseconds) tells us the exact offset.\n\n");

    printf("%-12s our_ayan_deg    our_ayan_dms          year\n", "date");
    for (int i = 0; i < (int)N_ENTRIES; i++) {
        int idx = sorted[i];
        double a = data[idx].our_ayan;
        int deg = (int)a;
        double minfrac = (a - deg) * 60.0;
        int min = (int)minfrac;
        double sec = (minfrac - min) * 60.0;

        printf("%04d-%02d-%02d  %12.6f   %2d° %02d' %06.3f\"   %4d\n",
               data[idx].gy, data[idx].gm, data[idx].gd,
               a, deg, min, sec,
               data[idx].year);
    }

    astro_close();
    return 0;
}
