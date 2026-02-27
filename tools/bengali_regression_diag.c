/*
 * bengali_regression_diag.c -- Diagnostic tool for 7 regression dates
 * caused by the pre-midnight zone (23:36-00:00 IST) check in the
 * Bengali solar calendar code.
 *
 * The change added a pre-midnight zone that pushed month starts an
 * extra day when the tithi didn't extend past the sankranti.  But it
 * broke 7 previously correct cases.
 *
 * Key question: are all 7 in the pre-midnight zone (23:36-00:00 IST)?
 * If so, the fix would be to NOT apply the pre-midnight zone rule for
 * Makara (rashi 10), since Makara already has the "always after midnight"
 * override in the post-midnight zone, but in the pre-midnight zone
 * Makara should behave normally.
 *
 * For each regression date, prints:
 *   1. Exact sankranti time in IST
 *   2. Whether it falls in the pre-midnight zone (23:36-00:00 IST)
 *   3. Tithi at sunrise of the sankranti date, and whether it extends
 *      past the sankranti
 *   4. What the old code assigns vs what drikpanchang shows
 *
 * Build:
 *   cc -O2 -Isrc -Ilib/moshier tools/bengali_regression_diag.c \
 *      src/panchang.c src/solar.c src/astro.c src/date_utils.c \
 *      src/tithi.c src/masa.c lib/moshier/moshier_*.c \
 *      -lm -o tools/bengali_regression_diag
 *
 * Run:
 *   ./tools/bengali_regression_diag
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "tithi.h"
#include "types.h"
#include <stdio.h>
#include <math.h>

/* Rashi names (1-12) */
static const char *RASHI_NAMES[] = {
    "", "Mesha", "Vrisha", "Mithun", "Karka", "Simha", "Kanya",
    "Tula", "Vrisch", "Dhanu", "Makara", "Kumbha", "Meena"
};

/* Bengali month names corresponding to rashis 1-12 */
static const char *BENGALI_MONTH[] = {
    "", "Boishakh", "Joishtho", "Asharh", "Srabon", "Bhadro", "Ashshin",
    "Kartik", "Ogrohaeon", "Poush", "Magh", "Falgun", "Choitro"
};

/* Tithi names */
static const char *TITHI_NAME[] = {
    "",
    "S-Pratipada", "S-Dwitiya", "S-Tritiya", "S-Chaturthi", "S-Panchami",
    "S-Shashthi", "S-Saptami", "S-Ashtami", "S-Navami", "S-Dashami",
    "S-Ekadashi", "S-Dwadashi", "S-Trayodashi", "S-Chaturdashi", "Purnima",
    "K-Pratipada", "K-Dwitiya", "K-Tritiya", "K-Chaturthi", "K-Panchami",
    "K-Shashthi", "K-Saptami", "K-Ashtami", "K-Navami", "K-Dashami",
    "K-Ekadashi", "K-Dwadashi", "K-Trayodashi", "K-Chaturdashi", "Amavasya"
};

/*
 * The 7 regression dates.
 * old_date = what old code (correctly) assigned (matches drikpanchang)
 * new_date = what new code (incorrectly) assigns after the pre-midnight zone change
 *
 * In all cases, the new code pushed the month start 1 day later than it should be.
 */
typedef struct {
    int old_y, old_m, old_d;     /* Old code = drikpanchang (CORRECT) */
    int new_y, new_m, new_d;     /* New code (REGRESSION -- pushed 1 day) */
    int rashi;
    const char *bengali_month;
} Regression;

static Regression CASES[] = {
    { 1928, 1,15,  1928, 1,16,  10, "Magh"     },
    { 1967, 1,15,  1967, 1,16,  10, "Magh"     },
    { 2003, 4,15,  2003, 4,16,   1, "Boishakh" },
    { 2006, 1,15,  2006, 1,16,  10, "Magh"     },
    { 2013,10,18,  2013,10,19,   7, "Kartik"   },
    { 2034,12,17,  2034,12,18,   9, "Poush"    },
    { 2045, 1,15,  2045, 1,16,  10, "Magh"     },
};
#define NUM_CASES (sizeof(CASES) / sizeof(CASES[0]))

static void jd_to_ist_hms(double jd_ut, int *h, int *m, int *s)
{
    double ist = jd_ut + 5.5 / 24.0;
    double frac = ist + 0.5 - floor(ist + 0.5);
    double secs = frac * 86400.0;
    *h = (int)(secs / 3600.0);
    *m = (int)(fmod(secs, 3600.0) / 60.0);
    *s = (int)(fmod(secs, 60.0));
}

static void jd_to_ist_str(double jd_ut, char *buf, int bufsize)
{
    double local_jd = jd_ut + 5.5 / 24.0 + 0.5;
    int y, m, d;
    jd_to_gregorian(floor(local_jd), &y, &m, &d);
    double frac = local_jd - floor(local_jd);
    double secs = frac * 86400.0;
    int hh = (int)(secs / 3600.0);
    int mm = (int)(fmod(secs, 3600.0) / 60.0);
    int ss = (int)(fmod(secs, 60.0));
    snprintf(buf, bufsize, "%04d-%02d-%02d %02d:%02d:%02d IST", y, m, d, hh, mm, ss);
}

/* Get next/previous day */
static void next_day_g(int y, int m, int d, int *ny, int *nm, int *nd)
{
    double jd = gregorian_to_jd(y, m, d) + 1.0;
    jd_to_gregorian(jd, ny, nm, nd);
}

static void prev_day_g(int y, int m, int d, int *py, int *pm, int *pd)
{
    double jd = gregorian_to_jd(y, m, d) - 1.0;
    jd_to_gregorian(jd, py, pm, pd);
}

int main(void)
{
    astro_init(NULL);
    Location delhi = DEFAULT_LOCATION;

    printf("================================================================\n");
    printf("  Bengali Solar Calendar: 7 Regression Diagnostic\n");
    printf("  Pre-midnight zone (23:36-00:00 IST) change analysis\n");
    printf("  Location: Delhi (28.6139N, 77.2090E, UTC+5:30)\n");
    printf("================================================================\n");
    printf("\n  Question: Are all 7 regressions in the pre-midnight zone?\n");
    printf("  If so: Makara should NOT use pre-midnight zone rule.\n\n");

    int in_premidnight_count = 0;
    int makara_in_premidnight = 0;

    for (int i = 0; i < (int)NUM_CASES; i++) {
        Regression *c = &CASES[i];

        printf("=== Case %d: %s (rashi %d = %s) ===\n",
               i + 1, c->bengali_month, c->rashi, RASHI_NAMES[c->rashi]);
        printf("  Correct (drikpanchang/old): %04d-%02d-%02d\n",
               c->old_y, c->old_m, c->old_d);
        printf("  Regression (new code):      %04d-%02d-%02d  (pushed 1 day)\n",
               c->new_y, c->new_m, c->new_d);

        /* ---- 1. Find the exact sankranti ---- */
        double target_long = (c->rashi == 1) ? 0.0 : (double)(c->rashi - 1) * 30.0;
        /* Search near the correct (earlier) date */
        double jd_search = gregorian_to_jd(c->old_y, c->old_m, c->old_d);
        double jd_sank = sankranti_jd(jd_search, target_long);

        char sank_str[64];
        jd_to_ist_str(jd_sank, sank_str, sizeof(sank_str));
        printf("\n  1. SANKRANTI TIME: %s  (JD_UT=%.6f)\n", sank_str, jd_sank);

        /* IST date of the sankranti */
        double local_jd = jd_sank + 5.5 / 24.0 + 0.5;
        int sank_y, sank_m, sank_d;
        jd_to_gregorian(floor(local_jd), &sank_y, &sank_m, &sank_d);

        /* IST hour/min/sec of the sankranti */
        int sh, smn, ss;
        jd_to_ist_hms(jd_sank, &sh, &smn, &ss);
        double sank_ist_hours = sh + smn / 60.0 + ss / 3600.0;

        /* ---- 2. Pre-midnight zone check (23:36-00:00 IST) ---- */
        /*
         * Pre-midnight zone: 23:36 <= IST_time < 00:00
         * (i.e., the sankranti is on the date shown, between 23:36 and midnight)
         *
         * Post-midnight zone: 00:00 <= IST_time < 00:24
         * (i.e., the sankranti is just after midnight of the NEXT IST date)
         */
        int in_premidnight = (sank_ist_hours >= 23.6);  /* 23:36 = 23.6 hours */
        int in_postmidnight = (sank_ist_hours < 0.4);   /* 00:24 = 0.4 hours */

        printf("\n  2. ZONE CHECK:\n");
        printf("     Sankranti IST time:  %02d:%02d:%02d (%.4f hours)\n",
               sh, smn, ss, sank_ist_hours);
        printf("     Pre-midnight zone  (23:36-00:00)?  %s\n",
               in_premidnight ? "*** YES ***" : "no");
        printf("     Post-midnight zone (00:00-00:24)?  %s\n",
               in_postmidnight ? "*** YES ***" : "no");

        if (in_premidnight) {
            in_premidnight_count++;
            if (c->rashi == 10) makara_in_premidnight++;
        }

        /* The civil date the sankranti falls on (IST) */
        printf("     Sankranti falls on IST date: %04d-%02d-%02d\n",
               sank_y, sank_m, sank_d);

        /* ---- Compute Bengali critical time (midnight + 24 min of the NEXT day) ---- */
        /* The sankranti_to_civil_day function first converts to local date,
         * then checks against crit = that date's midnight + 24 min.
         * If sank <= crit, the Bengali rule applies. */
        double jd_sank_day = gregorian_to_jd(sank_y, sank_m, sank_d);
        double jd_crit = jd_sank_day - delhi.utc_offset / 24.0 + 24.0 / (24.0 * 60.0);
        char crit_str[64];
        jd_to_ist_str(jd_crit, crit_str, sizeof(crit_str));
        printf("     Bengali crit time:   %s\n", crit_str);
        printf("     Sank <= crit?        %s\n",
               (jd_sank <= jd_crit) ? "YES (Bengali tithi rule zone)" : "NO (push to next day)");

        /* ---- 3. Tithi at sunrise ---- */
        /* For the post-midnight zone (sank <= crit), the Bengali rule uses
         * tithi at PREVIOUS day's sunrise. For the pre-midnight zone,
         * the hypothetical new code would use tithi at the sankranti
         * date's sunrise (i.e., the current day). */

        /* Tithi at sankranti date's sunrise (pre-midnight zone context) */
        TithiInfo ti_curr = tithi_at_sunrise(sank_y, sank_m, sank_d, &delhi);
        char ti_curr_end_str[64];
        jd_to_ist_str(ti_curr.jd_end, ti_curr_end_str, sizeof(ti_curr_end_str));
        int tn_curr = ti_curr.tithi_num;
        const char *tname_curr = (tn_curr >= 1 && tn_curr <= 30) ? TITHI_NAME[tn_curr] : "???";

        /* Tithi at previous day's sunrise (post-midnight zone context) */
        int prev_y, prev_m, prev_d;
        prev_day_g(sank_y, sank_m, sank_d, &prev_y, &prev_m, &prev_d);
        TithiInfo ti_prev = tithi_at_sunrise(prev_y, prev_m, prev_d, &delhi);
        char ti_prev_end_str[64];
        jd_to_ist_str(ti_prev.jd_end, ti_prev_end_str, sizeof(ti_prev_end_str));
        int tn_prev = ti_prev.tithi_num;
        const char *tname_prev = (tn_prev >= 1 && tn_prev <= 30) ? TITHI_NAME[tn_prev] : "???";

        double margin_curr = (ti_curr.jd_end - jd_sank) * 1440.0;
        double margin_prev = (ti_prev.jd_end - jd_sank) * 1440.0;

        printf("\n  3. TITHI ANALYSIS:\n");
        printf("     --- Tithi at sank-date sunrise (%04d-%02d-%02d) ---\n",
               sank_y, sank_m, sank_d);
        printf("       Tithi #%d (%s)\n", tn_curr, tname_curr);
        printf("       Tithi ends: %s\n", ti_curr_end_str);
        printf("       Extends past sank? %+.1f min -> %s\n",
               margin_curr, (margin_curr > 0) ? "YES" : "NO");

        printf("     --- Tithi at prev-day sunrise (%04d-%02d-%02d) ---\n",
               prev_y, prev_m, prev_d);
        printf("       Tithi #%d (%s)\n", tn_prev, tname_prev);
        printf("       Tithi ends: %s\n", ti_prev_end_str);
        printf("       Extends past sank? %+.1f min -> %s\n",
               margin_prev, (margin_prev > 0) ? "YES" : "NO");

        /* ---- 4. Old vs new code behavior ---- */
        printf("\n  4. CODE BEHAVIOR:\n");

        /* Old code (no pre-midnight zone):
         * a) Convert sank to IST date
         * b) crit = that date's midnight + 24 min (= 00:24 IST)
         * c) If sank <= crit: Bengali rule (Karka=keep, Makara=push, others=tithi)
         * d) If sank > crit: push to next day unconditionally
         *
         * For a sankranti at 23:36-23:59 IST, sank > crit (00:24 IST),
         * so old code pushes to next day unconditionally. */

        int old_sank_le_crit = (jd_sank <= jd_crit);
        int nx_y, nx_m, nx_d;
        next_day_g(sank_y, sank_m, sank_d, &nx_y, &nx_m, &nx_d);

        printf("     OLD CODE (no pre-midnight zone):\n");
        if (old_sank_le_crit) {
            /* Sank is in post-midnight zone (<= 00:24 IST) */
            if (c->rashi == 4) {
                printf("       Sank <= crit, Karka: keep this day -> %04d-%02d-%02d\n",
                       sank_y, sank_m, sank_d);
            } else if (c->rashi == 10) {
                printf("       Sank <= crit, Makara: push -> %04d-%02d-%02d\n",
                       nx_y, nx_m, nx_d);
            } else {
                int extends = (ti_prev.jd_end > jd_sank);
                if (extends) {
                    printf("       Sank <= crit, tithi extends: keep -> %04d-%02d-%02d\n",
                           sank_y, sank_m, sank_d);
                } else {
                    printf("       Sank <= crit, tithi ends before: push -> %04d-%02d-%02d\n",
                           nx_y, nx_m, nx_d);
                }
            }
        } else {
            /* Sank > crit: push to next day unconditionally */
            printf("       Sank > crit (%.2f IST > 00:24 IST): push -> %04d-%02d-%02d\n",
                   sank_ist_hours, nx_y, nx_m, nx_d);
        }

        printf("     NEW CODE (with pre-midnight zone 23:36-00:00):\n");
        if (in_premidnight) {
            /* Pre-midnight zone: the new code applies Bengali tithi-like rule here too */
            printf("       IN PRE-MIDNIGHT ZONE (%02d:%02d IST)\n", sh, smn);
            if (c->rashi == 4) {
                printf("       Karka override: keep this day -> %04d-%02d-%02d\n",
                       sank_y, sank_m, sank_d);
            } else if (c->rashi == 10) {
                printf("       Makara override: push EXTRA day -> %04d-%02d-%02d\n",
                       c->new_y, c->new_m, c->new_d);
                printf("       ** THIS IS THE BUG: Makara in pre-midnight zone\n");
                printf("          should NOT get extra push, just normal push to next day **\n");
            } else {
                int extends = (ti_curr.jd_end > jd_sank);
                if (!extends) {
                    printf("       Tithi does NOT extend past sank: push EXTRA -> %04d-%02d-%02d\n",
                           c->new_y, c->new_m, c->new_d);
                } else {
                    printf("       Tithi extends past sank: keep next day -> %04d-%02d-%02d\n",
                           nx_y, nx_m, nx_d);
                }
            }
        } else if (old_sank_le_crit) {
            /* Post-midnight zone: same as old code */
            printf("       In post-midnight zone, same as old code\n");
        } else {
            printf("       Sank > crit, not in pre-midnight zone either\n");
            printf("       This shouldn't happen if it's a regression!\n");
        }

        printf("     CORRECT (drikpanchang): %04d-%02d-%02d\n",
               c->old_y, c->old_m, c->old_d);

        /* ---- Verify: does the old code match drikpanchang? ---- */
        printf("\n     Old code result matches drikpanchang? ");
        /* Old code: if sank > crit, push to next day = sank_date + 1 */
        if (!old_sank_le_crit) {
            /* Push to next day */
            if (nx_y == c->old_y && nx_m == c->old_m && nx_d == c->old_d) {
                printf("YES (both = %04d-%02d-%02d)\n", nx_y, nx_m, nx_d);
            } else {
                printf("NO (old=%04d-%02d-%02d, drik=%04d-%02d-%02d)\n",
                       nx_y, nx_m, nx_d, c->old_y, c->old_m, c->old_d);
            }
        } else {
            /* Post-midnight zone: depends on rule */
            printf("(post-midnight zone, check above)\n");
        }

        printf("\n");
    }

    /* ============================================================ */
    printf("================================================================\n");
    printf("  SUMMARY\n");
    printf("================================================================\n\n");

    printf("%-4s  %-10s %-7s %-8s  %-19s  %-12s  %-12s  %-8s  %-8s\n",
           "#", "Month", "Rashi", "R-name",
           "Sank IST", "Pre-mid?", "Post-mid?",
           "Correct", "Regress");

    for (int i = 0; i < (int)NUM_CASES; i++) {
        Regression *c = &CASES[i];
        double target_long = (c->rashi == 1) ? 0.0 : (double)(c->rashi - 1) * 30.0;
        double jd_sank = sankranti_jd(gregorian_to_jd(c->old_y, c->old_m, c->old_d),
                                       target_long);

        int sh, smn, ss;
        jd_to_ist_hms(jd_sank, &sh, &smn, &ss);
        double sank_ist_hours = sh + smn / 60.0 + ss / 3600.0;
        int in_pre = (sank_ist_hours >= 23.6);
        int in_post = (sank_ist_hours < 0.4);

        /* Get the IST date */
        double loc_jd = jd_sank + 5.5 / 24.0 + 0.5;
        int sy, sm, sd;
        jd_to_gregorian(floor(loc_jd), &sy, &sm, &sd);

        printf("%-4d  %-10s %3d     %-8s  %04d-%02d-%02d %02d:%02d:%02d  %-12s  %-12s  %02d-%02d    %02d-%02d\n",
               i + 1, c->bengali_month, c->rashi, RASHI_NAMES[c->rashi],
               sy, sm, sd, sh, smn, ss,
               in_pre ? "YES (23:36+)" : "no",
               in_post ? "YES (<00:24)" : "no",
               c->old_m, c->old_d,
               c->new_m, c->new_d);
    }

    printf("\n  Total in pre-midnight zone (23:36-00:00): %d / %d\n",
           in_premidnight_count, (int)NUM_CASES);
    printf("  Of which Makara (rashi 10):                %d\n", makara_in_premidnight);
    printf("\n");

    /* ---- Breakdown by rashi ---- */
    printf("  Rashi breakdown of regressions:\n");
    int rashi_counts[13] = {0};
    for (int i = 0; i < (int)NUM_CASES; i++)
        rashi_counts[CASES[i].rashi]++;
    for (int r = 1; r <= 12; r++) {
        if (rashi_counts[r] > 0)
            printf("    %s (rashi %d): %d cases\n",
                   RASHI_NAMES[r], r, rashi_counts[r]);
    }

    printf("\n  CONCLUSION:\n");
    if (in_premidnight_count == (int)NUM_CASES) {
        printf("    ALL 7 regressions are in the pre-midnight zone (23:36-00:00 IST).\n");
        if (makara_in_premidnight > 0) {
            printf("    %d of %d are Makara cases -- the Makara 'always push' override\n",
                   makara_in_premidnight, (int)NUM_CASES);
            printf("    should NOT apply in the pre-midnight zone.\n");
            printf("    FIX: In the pre-midnight zone, do NOT apply Makara/Karka overrides.\n");
            printf("    The pre-midnight zone rule should ONLY use the tithi-based check\n");
            printf("    for non-Makara/non-Karka rashis, or skip the pre-midnight zone\n");
            printf("    entirely for Makara.\n");
        }
    } else {
        printf("    NOT all regressions are in the pre-midnight zone.\n");
        printf("    %d in pre-midnight, %d elsewhere.\n",
               in_premidnight_count, (int)NUM_CASES - in_premidnight_count);
        printf("    Need further investigation.\n");
    }

    astro_close();
    return 0;
}
