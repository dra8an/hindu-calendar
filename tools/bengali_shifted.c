/*
 * bengali_shifted.c — Test drikpanchang's actual sankranti times against cutoff rules.
 *
 * Uses the measured ayanamsa difference (drikpanchang - SE_SIDM_LAHIRI) to compute
 * the exact sankranti time drikpanchang would produce, then tests those shifted
 * times against every candidate cutoff rule.
 *
 * Ayanamsa model from 5 measured data points:
 *   da(year) = 24.10 + 0.003 * (year - 2000) arcseconds
 *   (drikpanchang's ayanamsa is larger → their sankranti is later)
 *
 * Build:
 *   make && cc -O2 -Isrc -Ilib/swisseph tools/bengali_shifted.c \
 *       build/astro.o build/date_utils.o build/tithi.o build/masa.o \
 *       build/panchang.o build/solar.o build/swe/*.o -lm -o build/bengali_shifted
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
    {1908,  5, 14,  2, 1}, {1976, 10, 17,  7, 1}, {2022,  3, 15, 12, 1},
    {1983,  3, 15, 12, 1}, {2047,  1, 15, 10, 1}, {1935,  8, 17,  5, 1},
    {1937, 10, 17,  7, 1}, {1944,  3, 14, 12, 1}, {2015, 10, 18,  7, 1},
    {1935,  9, 17,  6, 1}, {1981,  6, 15,  3, 1}, {1903,  6, 15,  3, 1},
    {1969,  1, 14, 10, 1}, {1930,  1, 14, 10, 1}, {1986,  5, 15,  2, 1},
    {1976, 11, 16,  8, 1}, {1924,  2, 13, 11, 1}, {2036, 12, 16,  9, 1},
    {2008,  1, 15, 10, 1}, {2044,  4, 14,  1, 1}, {2013,  8, 17,  5, 1},
    {1974,  8, 17,  5, 1}, {1937, 11, 16,  8, 1},
    {1963,  2, 13, 11, 0}, {1909,  7, 16,  4, 0}, {1947,  5, 15,  2, 0},
    {1927,  4, 14,  1, 0}, {1905,  3, 14, 12, 0}, {1991,  7, 17,  4, 0},
    {2005,  4, 14,  1, 0}, {2025,  5, 15,  2, 0}, {1942,  6, 15,  3, 0},
    {2030,  7, 17,  4, 0}, {1997, 12, 16,  9, 0}, {2015, 11, 17,  8, 0},
    {2020,  6, 15,  3, 0}, {1948,  7, 16,  4, 0},
};
#define N (sizeof(entries) / sizeof(entries[0]))

/* Ayanamsa difference model: da = 24.10 + 0.003*(year-2000) arcsec */
static double ayanamsa_diff_arcsec(int year)
{
    return 24.10 + 0.003 * (year - 2000);
}

/* Compute drikpanchang's shifted sankranti JD */
static double shifted_sankranti(double jd_our_sank, double daily_motion, int year)
{
    double da = ayanamsa_diff_arcsec(year);
    /* Time shift = da_arcsec / (daily_motion_deg/day * 3600 arcsec/deg) days */
    double shift_days = da / (daily_motion * 3600.0);
    return jd_our_sank + shift_days;  /* dp's sankranti is later */
}

static void jd_to_ist_hms(double jd_ut, int *h, int *m, int *s, double *min_after_midnight)
{
    double ist = jd_ut + 5.5 / 24.0;
    double frac = ist + 0.5 - floor(ist + 0.5);
    double secs = frac * 86400.0;
    *h = (int)(secs / 3600.0);
    *m = (int)(fmod(secs, 3600.0) / 60.0);
    *s = (int)(fmod(secs, 60.0));
    *min_after_midnight = frac * 1440.0;
}

int main(void)
{
    astro_init(NULL);

    Location delhi   = DEFAULT_LOCATION;
    Location kolkata = { 22.5726, 88.3639, 0.0, 5.5 };
    Location ujjain  = { 23.15,   75.7683, 0.0, 5.5 };

    printf("# Bengali Shifted Sankranti Analysis\n");
    printf("# Using drikpanchang ayanamsa (our + ~24 arcsec) for exact sankranti times\n");
    printf("# Then testing against all candidate cutoff rules\n\n");

    /* Compute all data */
    struct {
        double jd_sank_our, jd_sank_dp;
        double ist_our, ist_dp;  /* minutes after midnight */
        double nish_del, nish_kol, nish_ujj;
        double eot_ist;  /* EoT midnight at IST */
        double daily_mot;
        int gy, gm, gd, rashi, wrong;
    } d[37];

    for (int i = 0; i < (int)N; i++) {
        int gy = entries[i].gy, gm = entries[i].gm, gd = entries[i].gd;
        int rashi = entries[i].rashi;

        double target_long = (rashi == 1) ? 0.0 : (rashi - 1) * 30.0;
        double jd = gregorian_to_jd(gy, gm, gd);
        double jd_sank = sankranti_jd(jd, target_long);

        /* Sun's sidereal daily motion */
        char serr[256];
        double x[6];
        swe_calc_ut(jd_sank, SE_SUN, SEFLG_SIDEREAL | SEFLG_SPEED, x, serr);
        double daily_mot = fabs(x[3]);

        /* Shifted sankranti */
        double jd_dp = shifted_sankranti(jd_sank, daily_mot, gy);

        /* IST times */
        int h, m, s;
        jd_to_ist_hms(jd_sank, &h, &m, &s, &d[i].ist_our);
        jd_to_ist_hms(jd_dp, &h, &m, &s, &d[i].ist_dp);

        /* Nishita at various locations */
        double jd_prev = jd - 1.0;
        double ss_del = sunset_jd(jd_prev, &delhi);
        double sr_del = sunrise_jd(jd, &delhi);
        d[i].nish_del = (ss_del + sr_del) / 2.0;

        double ss_kol = sunset_jd(jd_prev, &kolkata);
        double sr_kol = sunrise_jd(jd, &kolkata);
        d[i].nish_kol = (ss_kol + sr_kol) / 2.0;

        double ss_ujj = sunset_jd(jd_prev, &ujjain);
        double sr_ujj = sunrise_jd(jd, &ujjain);
        d[i].nish_ujj = (ss_ujj + sr_ujj) / 2.0;

        /* EoT midnight IST */
        double E;
        double jd_mean_ist = jd - 82.5 / 360.0;
        swe_time_equ(jd_mean_ist, &E, serr);
        d[i].eot_ist = jd_mean_ist - E;

        d[i].jd_sank_our = jd_sank;
        d[i].jd_sank_dp = jd_dp;
        d[i].daily_mot = daily_mot;
        d[i].gy = gy; d[i].gm = gm; d[i].gd = gd;
        d[i].rashi = rashi; d[i].wrong = entries[i].wrong;
    }

    /* ====== Test all rules with SHIFTED times ====== */
    printf("# === RULE SCORES WITH SHIFTED SANKRANTI TIMES ===\n");
    printf("# For W: need dp_sank > cutoff. For C: need dp_sank <= cutoff.\n\n");
    printf("# %-40s Score   W/23   C/14\n", "Rule");

    /* Test each rule */
    struct { const char *name; int score, w_ok, c_ok; } results[20];
    int nr = 0;

    /* 1. Fixed IST cutoffs */
    for (int cutoff_min = -10; cutoff_min <= 40; cutoff_min++) {
        double cutoff_jd_offset = cutoff_min / 1440.0;
        int score = 0, w_ok = 0, c_ok = 0;

        for (int i = 0; i < (int)N; i++) {
            double jd = gregorian_to_jd(d[i].gy, d[i].gm, d[i].gd);
            double jd_cutoff = jd - 82.5 / 360.0 + cutoff_jd_offset;  /* IST midnight + offset */
            int ok = d[i].wrong ? (d[i].jd_sank_dp > jd_cutoff) : (d[i].jd_sank_dp <= jd_cutoff);
            if (ok) { score++; if (d[i].wrong) w_ok++; else c_ok++; }
        }

        if (score >= 22) {
            char name[64];
            snprintf(name, sizeof(name), "IST midnight %+d min (shifted)", cutoff_min);
            printf("  %-40s %2d/37   %2d/23  %2d/14\n", name, score, w_ok, c_ok);
        }
    }

    /* 2. Nishita at various locations */
    {
        const char *names[] = {"Nishita Delhi (shifted)", "Nishita Kolkata (shifted)", "Nishita Ujjain (shifted)"};
        for (int loc = 0; loc < 3; loc++) {
            int score = 0, w_ok = 0, c_ok = 0;
            for (int i = 0; i < (int)N; i++) {
                double cutoff = (loc == 0) ? d[i].nish_del : (loc == 1) ? d[i].nish_kol : d[i].nish_ujj;
                int ok = d[i].wrong ? (d[i].jd_sank_dp > cutoff) : (d[i].jd_sank_dp <= cutoff);
                if (ok) { score++; if (d[i].wrong) w_ok++; else c_ok++; }
            }
            printf("  %-40s %2d/37   %2d/23  %2d/14\n", names[loc], score, w_ok, c_ok);
        }
    }

    /* 3. EoT midnight IST */
    {
        int score = 0, w_ok = 0, c_ok = 0;
        for (int i = 0; i < (int)N; i++) {
            int ok = d[i].wrong ? (d[i].jd_sank_dp > d[i].eot_ist) : (d[i].jd_sank_dp <= d[i].eot_ist);
            if (ok) { score++; if (d[i].wrong) w_ok++; else c_ok++; }
        }
        printf("  %-40s %2d/37   %2d/23  %2d/14\n", "EoT midnight IST (shifted)", score, w_ok, c_ok);
    }

    /* 4. Nishita + buffer sweep */
    printf("\n# === NISHITA + BUFFER SWEEP (shifted times) ===\n");
    printf("# Testing nishita(location) + buffer as cutoff\n\n");

    for (int buf = -30; buf <= 30; buf++) {
        double buf_jd = buf / 1440.0;
        int scores[3] = {0}, w_oks[3] = {0}, c_oks[3] = {0};

        for (int i = 0; i < (int)N; i++) {
            double cutoffs[3] = { d[i].nish_del + buf_jd, d[i].nish_kol + buf_jd, d[i].nish_ujj + buf_jd };
            for (int loc = 0; loc < 3; loc++) {
                int ok = d[i].wrong ? (d[i].jd_sank_dp > cutoffs[loc]) : (d[i].jd_sank_dp <= cutoffs[loc]);
                if (ok) { scores[loc]++; if (d[i].wrong) w_oks[loc]++; else c_oks[loc]++; }
            }
        }

        int best = 0;
        for (int loc = 0; loc < 3; loc++) if (scores[loc] > best) best = scores[loc];
        if (best >= 22) {
            printf("  buf=%+3d  Delhi %2d/37(%2dW,%2dC)  Kol %2d/37(%2dW,%2dC)  Ujj %2d/37(%2dW,%2dC)\n",
                   buf, scores[0], w_oks[0], c_oks[0],
                   scores[1], w_oks[1], c_oks[1],
                   scores[2], w_oks[2], c_oks[2]);
        }
    }

    /* ====== Per-entry detail sorted by shifted IST ====== */
    printf("\n# === PER-ENTRY DETAIL (sorted by shifted IST time) ===\n");
    printf("# Shows shifted sankranti time and various cutoff values\n\n");

    int idx[37];
    for (int i = 0; i < (int)N; i++) idx[i] = i;
    for (int i = 1; i < (int)N; i++) {
        for (int j = i; j > 0 && d[idx[j]].ist_dp < d[idx[j-1]].ist_dp; j--) {
            int t = idx[j]; idx[j] = idx[j-1]; idx[j-1] = t;
        }
    }

    printf("%-12s W/C ra  our_IST   dp_IST    shift  nish_Del  nish_Kol  nish_Ujj  dp>nDel dp>nKol dp>nUjj\n", "date");
    for (int i = 0; i < (int)N; i++) {
        int k = idx[i];
        int h1, m1, s1, h2, m2, s2;
        double dummy;
        jd_to_ist_hms(d[k].jd_sank_our, &h1, &m1, &s1, &dummy);
        jd_to_ist_hms(d[k].jd_sank_dp, &h2, &m2, &s2, &dummy);

        int hn, mn, sn;
        jd_to_ist_hms(d[k].nish_del, &hn, &mn, &sn, &dummy);
        char s_nd[16]; snprintf(s_nd, 16, "%02d:%02d:%02d", hn, mn, sn);

        jd_to_ist_hms(d[k].nish_kol, &hn, &mn, &sn, &dummy);
        char s_nk[16]; snprintf(s_nk, 16, "%02d:%02d:%02d", hn, mn, sn);

        jd_to_ist_hms(d[k].nish_ujj, &hn, &mn, &sn, &dummy);
        char s_nu[16]; snprintf(s_nu, 16, "%02d:%02d:%02d", hn, mn, sn);

        double shift = (d[k].jd_sank_dp - d[k].jd_sank_our) * 1440.0;
        int dp_gt_ndel = d[k].jd_sank_dp > d[k].nish_del;
        int dp_gt_nkol = d[k].jd_sank_dp > d[k].nish_kol;
        int dp_gt_nujj = d[k].jd_sank_dp > d[k].nish_ujj;

        /* For W: dp > nishita is CORRECT. For C: dp <= nishita is CORRECT. */
        int ok_del = d[k].wrong ? dp_gt_ndel : !dp_gt_ndel;
        int ok_kol = d[k].wrong ? dp_gt_nkol : !dp_gt_nkol;
        int ok_ujj = d[k].wrong ? dp_gt_nujj : !dp_gt_nujj;

        printf("%04d-%02d-%02d   %c  %2d  %02d:%02d:%02d  %02d:%02d:%02d  %+5.1f  %s  %s  %s   %s     %s     %s\n",
               d[k].gy, d[k].gm, d[k].gd,
               d[k].wrong ? 'W' : 'C',
               d[k].rashi,
               h1, m1, s1, h2, m2, s2, shift,
               s_nd, s_nk, s_nu,
               ok_del ? " OK" : "BAD",
               ok_kol ? " OK" : "BAD",
               ok_ujj ? " OK" : "BAD");
    }

    /* ====== Check: which entries FAIL with nishita Delhi + shifted ====== */
    printf("\n# === FAILURES WITH NISHITA DELHI + SHIFTED ===\n");
    int fail_count = 0;
    for (int i = 0; i < (int)N; i++) {
        int ok = d[i].wrong ? (d[i].jd_sank_dp > d[i].nish_del) : (d[i].jd_sank_dp <= d[i].nish_del);
        if (!ok) {
            fail_count++;
            double delta = (d[i].jd_sank_dp - d[i].nish_del) * 1440.0;
            int h, m, s; double dummy;
            jd_to_ist_hms(d[i].jd_sank_dp, &h, &m, &s, &dummy);
            int hn, mn, sn;
            jd_to_ist_hms(d[i].nish_del, &hn, &mn, &sn, &dummy);
            printf("  %04d-%02d-%02d %c ra=%2d  dp=%02d:%02d:%02d  nish=%02d:%02d:%02d  delta=%+.1f min  need_extra=%.1f arcsec\n",
                   d[i].gy, d[i].gm, d[i].gd,
                   d[i].wrong ? 'W' : 'C',
                   d[i].rashi,
                   h, m, s, hn, mn, sn, delta,
                   /* How much more ayanamsa diff would fix this entry */
                   fabs(delta) / 1440.0 * d[i].daily_mot * 3600.0);
        }
    }
    printf("# Total failures: %d/%d\n", fail_count, (int)N);

    astro_close();
    return 0;
}
