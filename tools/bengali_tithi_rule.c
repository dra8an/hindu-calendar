/*
 * bengali_tithi_rule.c — Test the traditional Bengali tithi-based rule
 * for the 37 edge case entries.
 *
 * Rule (from Sewell & Dikshit, "The Indian Calendar", pp. 12-13):
 *
 * When sankranti falls in the buffer zone (24 min before to 24 min after midnight):
 *   - Karkata (rashi 4, Cancer): always treat as "before midnight" → day 1
 *   - Makara (rashi 10, Capricorn): always treat as "after midnight" → last day
 *   - All others: check tithi at sunrise of the Hindu day:
 *     - If the tithi extends past the sankranti moment → day 1
 *     - If the tithi ends before the sankranti → last day
 *
 * "day 1" = C (correct, our code agrees)
 * "last day" = W (wrong, our code says day 1 but drikpanchang says last day)
 *
 * Build:
 *   make && cc -O2 -Isrc -Ilib/swisseph tools/bengali_tithi_rule.c \
 *       build/astro.o build/date_utils.o build/tithi.o build/masa.o \
 *       build/panchang.o build/solar.o build/swe/*.o -lm -o build/bengali_tithi_rule
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "tithi.h"
#include "types.h"
#include "swephexp.h"
#include <stdio.h>
#include <math.h>

static struct {
    int gy, gm, gd;
    int rashi;
    int wrong;  /* 1 = W (drikpanchang says last day), 0 = C (agrees day 1) */
} entries[] = {
    /* W entries */
    {1908,  5, 14,  2, 1}, {1976, 10, 17,  7, 1}, {2022,  3, 15, 12, 1},
    {1983,  3, 15, 12, 1}, {2047,  1, 15, 10, 1}, {1935,  8, 17,  5, 1},
    {1937, 10, 17,  7, 1}, {1944,  3, 14, 12, 1}, {2015, 10, 18,  7, 1},
    {1935,  9, 17,  6, 1}, {1981,  6, 15,  3, 1}, {1903,  6, 15,  3, 1},
    {1969,  1, 14, 10, 1}, {1930,  1, 14, 10, 1}, {1986,  5, 15,  2, 1},
    {1976, 11, 16,  8, 1}, {1924,  2, 13, 11, 1}, {2036, 12, 16,  9, 1},
    {2008,  1, 15, 10, 1}, {2044,  4, 14,  1, 1}, {2013,  8, 17,  5, 1},
    {1974,  8, 17,  5, 1}, {1937, 11, 16,  8, 1},
    /* C entries */
    {1963,  2, 13, 11, 0}, {1909,  7, 16,  4, 0}, {1947,  5, 15,  2, 0},
    {1927,  4, 14,  1, 0}, {1905,  3, 14, 12, 0}, {1991,  7, 17,  4, 0},
    {2005,  4, 14,  1, 0}, {2025,  5, 15,  2, 0}, {1942,  6, 15,  3, 0},
    {2030,  7, 17,  4, 0}, {1997, 12, 16,  9, 0}, {2015, 11, 17,  8, 0},
    {2020,  6, 15,  3, 0}, {1948,  7, 16,  4, 0},
};
#define N (sizeof(entries) / sizeof(entries[0]))

static const char *rashi_names[] = {
    "", "Mesha", "Vrisha", "Mithun", "Karka", "Simha", "Kanya",
    "Tula", "Vrisch", "Dhanu", "Makara", "Kumbha", "Meena"
};

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

    Location delhi   = DEFAULT_LOCATION;
    Location kolkata = { 22.5726, 88.3639, 0.0, 5.5 };

    printf("# Bengali Tithi-Based Rule Test\n");
    printf("# Rule: Karkata→C, Makara→W, others: tithi_end > sankranti → C, else → W\n\n");

    /* Test with different configurations */
    struct {
        const char *label;
        Location *loc;
        int use_prev_day;  /* 0 = sunrise of civil date, 1 = sunrise of prev day */
        int use_shifted;   /* 0 = our sankranti, 1 = drikpanchang shifted */
    } configs[] = {
        {"Delhi sunrise(prev_day), our sank",      &delhi,   1, 0},
        {"Delhi sunrise(civil_date), our sank",    &delhi,   0, 0},
        {"Kolkata sunrise(prev_day), our sank",    &kolkata, 1, 0},
        {"Kolkata sunrise(civil_date), our sank",  &kolkata, 0, 0},
        {"Delhi sunrise(prev_day), dp sank",       &delhi,   1, 1},
        {"Delhi sunrise(civil_date), dp sank",     &delhi,   0, 1},
        {"Kolkata sunrise(prev_day), dp sank",     &kolkata, 1, 1},
        {"Kolkata sunrise(civil_date), dp sank",   &kolkata, 0, 1},
    };
    int nconfigs = sizeof(configs) / sizeof(configs[0]);

    /* Precompute sankranti times */
    double jd_sank[37], jd_sank_dp[37];
    for (int i = 0; i < (int)N; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;
        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd = gregorian_to_jd(gy, gm, gd);
        jd_sank[i] = sankranti_jd(jd, target_long);

        /* Drikpanchang's shifted sankranti (ayanamsa model) */
        char serr[256]; double x[6];
        swe_calc_ut(jd_sank[i], SE_SUN, SEFLG_SIDEREAL | SEFLG_SPEED, x, serr);
        double daily_mot = fabs(x[3]);
        double da = 24.10 + 0.003 * (gy - 2000);  /* arcsec */
        double shift_days = da / (daily_mot * 3600.0);
        jd_sank_dp[i] = jd_sank[i] + shift_days;
    }

    printf("%-45s  Score  W/23  C/14\n", "Configuration");
    int best_config = -1, best_score = 0;

    for (int c = 0; c < nconfigs; c++) {
        int score = 0, w_ok = 0, c_ok = 0;

        for (int i = 0; i < (int)N; i++) {
            int rashi = entries[i].rashi;
            int expected_w = entries[i].wrong;
            int predicted_w;

            if (rashi == 4) {
                predicted_w = 0;  /* Karkata: always C (day 1) */
            } else if (rashi == 10) {
                predicted_w = 1;  /* Makara: always W (last day) */
            } else {
                /* Tithi at sunrise: check if tithi extends past sankranti */
                int sr_gy = entries[i].gy, sr_gm = entries[i].gm, sr_gd = entries[i].gd;
                if (configs[c].use_prev_day) {
                    /* Previous day's sunrise (Hindu day start) */
                    sr_gd--;
                    if (sr_gd == 0) {
                        sr_gm--;
                        if (sr_gm == 0) { sr_gm = 12; sr_gy--; }
                        /* Simple: just use day 28 for all months (close enough for tithi) */
                        int days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
                        if (sr_gm == 2 && ((sr_gy%4==0 && sr_gy%100!=0) || sr_gy%400==0))
                            sr_gd = 29;
                        else
                            sr_gd = days_in_month[sr_gm];
                    }
                }

                TithiInfo ti = tithi_at_sunrise(sr_gy, sr_gm, sr_gd, configs[c].loc);
                double sank = configs[c].use_shifted ? jd_sank_dp[i] : jd_sank[i];

                /* If tithi ends AFTER sankranti → C (day 1), else → W (last day) */
                predicted_w = (ti.jd_end > sank) ? 0 : 1;
            }

            int ok = (predicted_w == expected_w);
            if (ok) {
                score++;
                if (expected_w) w_ok++;
                else c_ok++;
            }
        }

        printf("  %-43s  %2d/37  %2d/23  %2d/14\n",
               configs[c].label, score, w_ok, c_ok);
        if (score > best_score) {
            best_score = score;
            best_config = c;
        }
    }

    /* Detailed output for best configuration */
    printf("\n# === BEST CONFIG: %s (%d/37) ===\n\n",
           configs[best_config].label, best_score);

    printf("%-12s W/C ra  %-7s sank_IST  ti_sr ti_end_IST  tithi_ext  rule    pred  ok?\n",
           "date", "rashi");

    for (int i = 0; i < (int)N; i++) {
        int rashi = entries[i].rashi;
        int expected_w = entries[i].wrong;

        /* Sankranti IST time */
        int sh, sm, ss;
        double sank = configs[best_config].use_shifted ? jd_sank_dp[i] : jd_sank[i];
        jd_to_ist_hms(sank, &sh, &sm, &ss);

        /* Tithi at sunrise */
        int sr_gy = entries[i].gy, sr_gm = entries[i].gm, sr_gd = entries[i].gd;
        if (configs[best_config].use_prev_day) {
            sr_gd--;
            if (sr_gd == 0) {
                sr_gm--;
                if (sr_gm == 0) { sr_gm = 12; sr_gy--; }
                int days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
                if (sr_gm == 2 && ((sr_gy%4==0 && sr_gy%100!=0) || sr_gy%400==0))
                    sr_gd = 29;
                else
                    sr_gd = days_in_month[sr_gm];
            }
        }

        TithiInfo ti = tithi_at_sunrise(sr_gy, sr_gm, sr_gd, configs[best_config].loc);

        int th, tm, ts;
        jd_to_ist_hms(ti.jd_end, &th, &tm, &ts);

        int predicted_w;
        const char *rule;
        if (rashi == 4) {
            predicted_w = 0;
            rule = "KARKAT";
        } else if (rashi == 10) {
            predicted_w = 1;
            rule = "MAKAR ";
        } else {
            predicted_w = (ti.jd_end > sank) ? 0 : 1;
            rule = "TITHI ";
        }

        int ok = (predicted_w == expected_w);

        /* Delta: tithi_end - sankranti in minutes */
        double delta_min = (ti.jd_end - sank) * 1440.0;

        printf("%04d-%02d-%02d   %c  %2d  %-7s %02d:%02d:%02d  %2d  %02d:%02d:%02d  %+8.1f  %s  %s    %s\n",
               entries[i].gy, entries[i].gm, entries[i].gd,
               expected_w ? 'W' : 'C',
               rashi,
               rashi_names[rashi],
               sh, sm, ss,
               ti.tithi_num,
               th, tm, ts,
               delta_min,
               rule,
               predicted_w ? "W" : "C",
               ok ? " OK" : "FAIL");
    }

    /* Also show failures with detail */
    printf("\n# === FAILURES ===\n");
    int fail_count = 0;
    for (int i = 0; i < (int)N; i++) {
        int rashi = entries[i].rashi;
        int expected_w = entries[i].wrong;
        double sank = configs[best_config].use_shifted ? jd_sank_dp[i] : jd_sank[i];

        int sr_gy = entries[i].gy, sr_gm = entries[i].gm, sr_gd = entries[i].gd;
        if (configs[best_config].use_prev_day) {
            sr_gd--;
            if (sr_gd == 0) {
                sr_gm--;
                if (sr_gm == 0) { sr_gm = 12; sr_gy--; }
                int days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
                if (sr_gm == 2 && ((sr_gy%4==0 && sr_gy%100!=0) || sr_gy%400==0))
                    sr_gd = 29;
                else
                    sr_gd = days_in_month[sr_gm];
            }
        }

        TithiInfo ti = tithi_at_sunrise(sr_gy, sr_gm, sr_gd, configs[best_config].loc);
        int predicted_w;
        if (rashi == 4) predicted_w = 0;
        else if (rashi == 10) predicted_w = 1;
        else predicted_w = (ti.jd_end > sank) ? 0 : 1;

        if (predicted_w != expected_w) {
            fail_count++;
            int sh, sm, ss; jd_to_ist_hms(sank, &sh, &sm, &ss);
            int th, tm, ts; jd_to_ist_hms(ti.jd_end, &th, &tm, &ts);
            double delta = (ti.jd_end - sank) * 1440.0;
            printf("  %04d-%02d-%02d %c ra=%2d %-7s  sank=%02d:%02d  ti=%2d end=%02d:%02d  delta=%+.1fmin\n",
                   entries[i].gy, entries[i].gm, entries[i].gd,
                   expected_w ? 'W' : 'C',
                   rashi, rashi_names[rashi],
                   sh, sm, ti.tithi_num, th, tm, delta);
        }
    }
    printf("# Total failures: %d/37\n", fail_count);

    astro_close();
    return 0;
}
