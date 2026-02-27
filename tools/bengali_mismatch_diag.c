/*
 * bengali_mismatch_diag.c -- Diagnostic tool for the 8 Bengali solar calendar
 * mismatches against drikpanchang.com.
 *
 * For each mismatch, prints:
 *   1. Exact sankranti time (IST) for the relevant rashi transition
 *   2. Midnight IST time (both start-of-day and next-day midnight)
 *   3. Difference (sankranti - midnight) in minutes
 *   4. Sunrise time for the previous day
 *   5. Tithi at that sunrise
 *   6. Whether the tithi extends past the sankranti time
 *   7. What our code assigns (which civil day)
 *   8. What drikpanchang assigns (which civil day)
 *
 * Also computes a "drikpanchang-shifted" sankranti using the ~24 arcsecond
 * ayanamsa difference to see if that explains the discrepancy.
 *
 * Build:
 *   cc -O2 -Isrc -Ilib/moshier tools/bengali_mismatch_diag.c \
 *      src/panchang.c src/solar.c src/astro.c src/date_utils.c \
 *      src/tithi.c src/masa.c lib/moshier/moshier_*.c \
 *      -lm -o tools/bengali_mismatch_diag
 *
 * Run:
 *   ./tools/bengali_mismatch_diag
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
 * The 8 mismatched month starts.
 * ref = our code, drik = drikpanchang.com, diff = drik - ref (days).
 */
typedef struct {
    int ref_y, ref_m, ref_d;
    int drik_y, drik_m, drik_d;
    int diff;
    int rashi;
    const char *bengali_month;
} Mismatch;

static Mismatch CASES[] = {
    { 1933,10,17,  1933,10,18,  +1,  7, "Kartik"   },
    { 1952, 7,17,  1952, 7,16,  -1,  4, "Srabon"   },
    { 1958,12,16,  1958,12,17,  +1,  9, "Poush"    },
    { 1972,10,17,  1972,10,18,  +1,  7, "Kartik"   },
    { 1974, 9,17,  1974, 9,18,  +1,  6, "Ashshin"  },
    { 1976,10,17,  1976,10,18,  +1,  7, "Kartik"   },
    { 2011,10,18,  2011,10,19,  +1,  7, "Kartik"   },
    { 2013, 9,17,  2013, 9,18,  +1,  6, "Ashshin"  },
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

/* Get next day */
static void next_day_g(int y, int m, int d, int *ny, int *nm, int *nd)
{
    double jd = gregorian_to_jd(y, m, d) + 1.0;
    jd_to_gregorian(jd, ny, nm, nd);
}

/* Get previous day */
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
    printf("  Bengali Solar Calendar: 8 Mismatch Diagnostic\n");
    printf("  Location: Delhi (28.6139N, 77.2090E, UTC+5:30)\n");
    printf("  Bengali rule: crit = midnight+24min IST; Karka->C, Makara->W,\n");
    printf("    others: tithi at prev sunrise extends past sank -> C, else W\n");
    printf("================================================================\n\n");

    for (int i = 0; i < (int)NUM_CASES; i++) {
        Mismatch *c = &CASES[i];

        printf("=== Case %d: %s (rashi %d, %s), diff=%+d ===\n",
               i + 1, c->bengali_month, c->rashi, RASHI_NAMES[c->rashi], c->diff);
        printf("  Our month start:  %04d-%02d-%02d\n",
               c->ref_y, c->ref_m, c->ref_d);
        printf("  Drik month start: %04d-%02d-%02d\n",
               c->drik_y, c->drik_m, c->drik_d);

        /* Find the sankranti */
        double target_long = (c->rashi == 1) ? 0.0 : (double)(c->rashi - 1) * 30.0;

        /* Search near the earlier date (sankranti is always before the month start) */
        int search_y, search_m, search_d;
        if (c->diff > 0) {
            search_y = c->ref_y; search_m = c->ref_m; search_d = c->ref_d;
        } else {
            search_y = c->drik_y; search_m = c->drik_m; search_d = c->drik_d;
        }
        double jd_search = gregorian_to_jd(search_y, search_m, search_d);
        double jd_sank = sankranti_jd(jd_search, target_long);

        /* --- 1. Exact sankranti time (IST) --- */
        char sank_str[64];
        jd_to_ist_str(jd_sank, sank_str, sizeof(sank_str));
        printf("\n  1. SANKRANTI TIME:     %s  (JD_UT=%.6f)\n", sank_str, jd_sank);

        /* IST date of sankranti */
        double local_jd = jd_sank + 5.5 / 24.0 + 0.5;
        int sank_y, sank_m, sank_d;
        jd_to_gregorian(floor(local_jd), &sank_y, &sank_m, &sank_d);

        /* Compute shifted sankranti (drikpanchang ayanamsa model).
         * Drikpanchang uses a Lahiri ayanamsa ~24 arcsec larger than ours.
         * This shifts the sankranti later by da/(daily_motion) days. */
        double sid_long_at_sank = solar_longitude_sidereal(jd_sank);
        /* Solar daily motion ~0.96-1.02 deg/day, estimate from nearby points */
        double dt = 0.5;
        double sid_plus = solar_longitude_sidereal(jd_sank + dt);
        double sid_minus = solar_longitude_sidereal(jd_sank - dt);
        double daily_motion = (sid_plus - sid_minus);
        if (daily_motion < 0) daily_motion += 360.0; /* handle wrap */
        /* daily_motion is per 1 day (dt=0.5 each side) */

        double da_arcsec = 24.10 + 0.003 * (sank_y - 2000);
        double shift_days = (da_arcsec / 3600.0) / daily_motion;
        double jd_sank_dp = jd_sank + shift_days;
        char sank_dp_str[64];
        jd_to_ist_str(jd_sank_dp, sank_dp_str, sizeof(sank_dp_str));
        printf("     Shifted sankranti:  %s  (shift=+%.1f min)\n",
               sank_dp_str, shift_days * 1440.0);

        /* --- 2. Midnight IST --- */
        /* This-day midnight (start of the IST date containing sankranti) */
        double jd_this_midnight_ut = gregorian_to_jd(sank_y, sank_m, sank_d) - 5.5 / 24.0;
        /* Next-day midnight (start of the next IST day) */
        int nx_y, nx_m, nx_d;
        next_day_g(sank_y, sank_m, sank_d, &nx_y, &nx_m, &nx_d);
        double jd_next_midnight_ut = gregorian_to_jd(nx_y, nx_m, nx_d) - 5.5 / 24.0;

        printf("\n  2. MIDNIGHT IST:\n");
        printf("     This-day midnight:  %04d-%02d-%02d 00:00:00 IST\n",
               sank_y, sank_m, sank_d);
        printf("     Next-day midnight:  %04d-%02d-%02d 00:00:00 IST\n",
               nx_y, nx_m, nx_d);

        /* Bengali critical time for this-day and next-day */
        double crit_this = jd_this_midnight_ut + 24.0 / (24.0 * 60.0);
        double crit_next = jd_next_midnight_ut + 24.0 / (24.0 * 60.0);
        printf("     Crit this-day:      %04d-%02d-%02d 00:24:00 IST\n",
               sank_y, sank_m, sank_d);
        printf("     Crit next-day:      %04d-%02d-%02d 00:24:00 IST\n",
               nx_y, nx_m, nx_d);

        /* --- 3. Difference (sankranti - midnight) in minutes --- */
        double delta_this_mid = (jd_sank - jd_this_midnight_ut) * 1440.0;
        double delta_next_mid = (jd_sank - jd_next_midnight_ut) * 1440.0;
        double delta_this_crit = (jd_sank - crit_this) * 1440.0;
        double delta_next_crit = (jd_sank - crit_next) * 1440.0;
        /* Same for shifted */
        double delta_next_mid_dp = (jd_sank_dp - jd_next_midnight_ut) * 1440.0;
        double delta_next_crit_dp = (jd_sank_dp - crit_next) * 1440.0;

        printf("\n  3. SANKRANTI vs MIDNIGHT (minutes):\n");
        printf("     Sank - this midnight:  %+.2f min (%.1f hours)\n",
               delta_this_mid, delta_this_mid / 60.0);
        printf("     Sank - next midnight:  %+.2f min\n", delta_next_mid);
        printf("     Sank - this crit:      %+.2f min\n", delta_this_crit);
        printf("     Sank - next crit:      %+.2f min\n", delta_next_crit);
        printf("     Shifted sank - next midnight: %+.2f min\n", delta_next_mid_dp);
        printf("     Shifted sank - next crit:     %+.2f min\n", delta_next_crit_dp);

        /* Which side of midnight/crit? */
        printf("     Our sank: %s next midnight, %s next crit\n",
               (delta_next_mid < 0) ? "BEFORE" : "AFTER",
               (delta_next_crit < 0) ? "BEFORE" : "AFTER");
        printf("     DP sank:  %s next midnight, %s next crit\n",
               (delta_next_mid_dp < 0) ? "BEFORE" : "AFTER",
               (delta_next_crit_dp < 0) ? "BEFORE" : "AFTER");

        /* --- 4. Sunrise for the previous day (Hindu day start) ---
         * For the Bengali tithi rule, the relevant sunrise is the one
         * of the previous civil day (start of the Hindu day containing
         * the post-midnight sankranti). */
        int prev_y, prev_m, prev_d;
        prev_day_g(sank_y, sank_m, sank_d, &prev_y, &prev_m, &prev_d);
        double jd_prev_sr = sunrise_jd(gregorian_to_jd(prev_y, prev_m, prev_d), &delhi);
        char prev_sr_str[64];
        jd_to_ist_str(jd_prev_sr, prev_sr_str, sizeof(prev_sr_str));
        printf("\n  4. PREV DAY SUNRISE:   %s  (day=%04d-%02d-%02d)\n",
               prev_sr_str, prev_y, prev_m, prev_d);

        /* Also sunrise of the sankranti date itself */
        double jd_sank_sr = sunrise_jd(gregorian_to_jd(sank_y, sank_m, sank_d), &delhi);
        char sank_sr_str[64];
        jd_to_ist_str(jd_sank_sr, sank_sr_str, sizeof(sank_sr_str));
        printf("     Sank date sunrise:  %s  (day=%04d-%02d-%02d)\n",
               sank_sr_str, sank_y, sank_m, sank_d);

        /* Sunrise of the next day */
        double jd_next_sr = sunrise_jd(gregorian_to_jd(nx_y, nx_m, nx_d), &delhi);
        char next_sr_str[64];
        jd_to_ist_str(jd_next_sr, next_sr_str, sizeof(next_sr_str));
        printf("     Next day sunrise:   %s  (day=%04d-%02d-%02d)\n",
               next_sr_str, nx_y, nx_m, nx_d);

        /* --- 5. Tithi at previous day's sunrise --- */
        TithiInfo ti_prev = tithi_at_sunrise(prev_y, prev_m, prev_d, &delhi);
        int tn_prev = ti_prev.tithi_num;
        const char *tname_prev = (tn_prev >= 1 && tn_prev <= 30) ? TITHI_NAME[tn_prev] : "???";
        char ti_prev_start[64], ti_prev_end[64];
        jd_to_ist_str(ti_prev.jd_start, ti_prev_start, sizeof(ti_prev_start));
        jd_to_ist_str(ti_prev.jd_end, ti_prev_end, sizeof(ti_prev_end));

        printf("\n  5. TITHI AT PREV SUNRISE:\n");
        printf("     Tithi #%d (%s)\n", tn_prev, tname_prev);
        printf("     Start: %s\n", ti_prev_start);
        printf("     End:   %s\n", ti_prev_end);

        /* Also show tithi at sankranti-date sunrise */
        TithiInfo ti_sank = tithi_at_sunrise(sank_y, sank_m, sank_d, &delhi);
        int tn_sank = ti_sank.tithi_num;
        const char *tname_sank = (tn_sank >= 1 && tn_sank <= 30) ? TITHI_NAME[tn_sank] : "???";
        char ti_sank_start[64], ti_sank_end[64];
        jd_to_ist_str(ti_sank.jd_start, ti_sank_start, sizeof(ti_sank_start));
        jd_to_ist_str(ti_sank.jd_end, ti_sank_end, sizeof(ti_sank_end));

        printf("     --- Tithi at sank-date sunrise ---\n");
        printf("     Tithi #%d (%s)\n", tn_sank, tname_sank);
        printf("     Start: %s\n", ti_sank_start);
        printf("     End:   %s\n", ti_sank_end);

        /* --- 6. Tithi extends past sankranti? --- */
        double ti_prev_margin = (ti_prev.jd_end - jd_sank) * 1440.0;
        double ti_sank_margin = (ti_sank.jd_end - jd_sank) * 1440.0;
        double ti_prev_margin_dp = (ti_prev.jd_end - jd_sank_dp) * 1440.0;
        double ti_sank_margin_dp = (ti_sank.jd_end - jd_sank_dp) * 1440.0;

        printf("\n  6. TITHI EXTENDS PAST SANKRANTI?\n");
        printf("     Prev-day tithi end - our sank:      %+.2f min  -> %s\n",
               ti_prev_margin, (ti_prev.jd_end > jd_sank) ? "YES" : "NO");
        printf("     Prev-day tithi end - shifted sank:   %+.2f min  -> %s\n",
               ti_prev_margin_dp, (ti_prev.jd_end > jd_sank_dp) ? "YES" : "NO");
        printf("     Sank-day tithi end - our sank:      %+.2f min  -> %s\n",
               ti_sank_margin, (ti_sank.jd_end > jd_sank) ? "YES" : "NO");
        printf("     Sank-day tithi end - shifted sank:   %+.2f min  -> %s\n",
               ti_sank_margin_dp, (ti_sank.jd_end > jd_sank_dp) ? "YES" : "NO");

        /* --- 7. Our code's assignment ---
         * Trace exactly what sankranti_to_civil_day() does:
         *   a) Convert sank JD to IST date
         *   b) Compute crit = that date's midnight + 24min
         *   c) If sank <= crit: Bengali rule (Karka/Makara/tithi)
         *   d) If sank > crit: push to next day */
        double jd_sank_day_crit = gregorian_to_jd(sank_y, sank_m, sank_d)
                                  - delhi.utc_offset / 24.0 + 24.0 / 1440.0;
        int sank_le_crit = (jd_sank <= jd_sank_day_crit);

        printf("\n  7. OUR CODE ASSIGNMENT:\n");
        printf("     Sank IST date: %04d-%02d-%02d, crit=00:24 IST that day\n",
               sank_y, sank_m, sank_d);
        printf("     Sank <= crit?  %s\n", sank_le_crit ? "YES -> Bengali tithi rule" : "NO -> push to next day");

        if (sank_le_crit) {
            if (c->rashi == 4) {
                printf("     Karka override: keep this day -> %04d-%02d-%02d\n",
                       sank_y, sank_m, sank_d);
            } else if (c->rashi == 10) {
                printf("     Makara override: push to next day -> %04d-%02d-%02d\n",
                       nx_y, nx_m, nx_d);
            } else {
                int ti_ext = (ti_prev.jd_end > jd_sank) ? 1 : 0;
                if (ti_ext) {
                    printf("     Tithi extends: keep this day -> %04d-%02d-%02d\n",
                           sank_y, sank_m, sank_d);
                } else {
                    printf("     Tithi ends before: push to next day -> %04d-%02d-%02d\n",
                           nx_y, nx_m, nx_d);
                }
            }
        } else {
            printf("     -> Month starts %04d-%02d-%02d\n", nx_y, nx_m, nx_d);
        }
        printf("     RESULT: Our code says month starts %04d-%02d-%02d\n",
               c->ref_y, c->ref_m, c->ref_d);

        /* --- 8. Drikpanchang's assignment --- */
        printf("\n  8. DRIKPANCHANG:       month starts %04d-%02d-%02d\n",
               c->drik_y, c->drik_m, c->drik_d);

        /* Analysis: what rule would produce drik's answer? */
        printf("\n  ANALYSIS:\n");
        if (c->diff > 0) {
            /* Drik is 1 day later. Our code puts month start at ref_date = sank_date+1.
             * Drik puts it at ref_date+1 = sank_date+2.
             * This means: for our code, sank on sank_date is "after crit", pushing to
             * sank_date+1. For drik, the sank is treated differently such that the
             * month start is pushed one day further.
             *
             * The most likely explanation: drik's ayanamsa-shifted sankranti falls
             * after the next-day midnight+24min, causing it to be assigned to the day
             * after that. Or drik uses a different critical time. */
            printf("     Drik is 1 day LATER. Our sank at %02d:%02d IST on %04d-%02d-%02d\n",
                   0, 0, sank_y, sank_m, sank_d);
            int sh, smn, ss;
            jd_to_ist_hms(jd_sank, &sh, &smn, &ss);
            printf("     Our sank: %02d:%02d:%02d IST -> pushed to next day %04d-%02d-%02d\n",
                   sh, smn, ss, nx_y, nx_m, nx_d);

            /* If drik's sankranti (shifted) crosses into next day's post-crit zone: */
            double local_jd_dp = jd_sank_dp + 5.5 / 24.0 + 0.5;
            int dp_y, dp_m, dp_d;
            jd_to_gregorian(floor(local_jd_dp), &dp_y, &dp_m, &dp_d);
            int dsh, dsmn, dss;
            jd_to_ist_hms(jd_sank_dp, &dsh, &dsmn, &dss);
            printf("     DP shifted sank: %02d:%02d:%02d IST on %04d-%02d-%02d\n",
                   dsh, dsmn, dss, dp_y, dp_m, dp_d);

            /* Check if shifted sank crosses midnight */
            if (dp_y != sank_y || dp_m != sank_m || dp_d != sank_d) {
                printf("     ** SHIFTED SANK CROSSES INTO NEXT DAY! **\n");
                double dp_crit = gregorian_to_jd(dp_y, dp_m, dp_d)
                                 - delhi.utc_offset / 24.0 + 24.0 / 1440.0;
                int dp_le_crit = (jd_sank_dp <= dp_crit);
                printf("     DP sank <= new crit?  %s\n",
                       dp_le_crit ? "YES -> Bengali rule applies on next day" :
                                    "NO -> push to day after next");
            } else {
                /* Same day, but maybe the crit check differs */
                double dp_crit = gregorian_to_jd(dp_y, dp_m, dp_d)
                                 - delhi.utc_offset / 24.0 + 24.0 / 1440.0;
                int dp_le_crit = (jd_sank_dp <= dp_crit);
                printf("     DP sank on same day. DP sank <= crit? %s\n",
                       dp_le_crit ? "YES" : "NO");
                if (dp_le_crit) {
                    int dp_ti_ext = (ti_prev.jd_end > jd_sank_dp) ? 1 : 0;
                    printf("     DP tithi rule: tithi extends past shifted sank? %s\n",
                           dp_ti_ext ? "YES -> keep" : "NO -> push");
                }
            }
        } else {
            /* Drik is 1 day earlier. */
            printf("     Drik is 1 day EARLIER.\n");
            printf("     Likely: drik's sank falls before crit, Karka rule keeps this day.\n");
            printf("     Our sank falls after crit, pushes to next day.\n");
            printf("     Shifted sank - next crit: %+.2f min\n", delta_next_crit_dp);
        }

        printf("\n");
    }

    /* ============================================================ */
    printf("================================================================\n");
    printf("  SUMMARY TABLE\n");
    printf("================================================================\n\n");

    printf("%-4s %-8s %-6s  %-10s  %-10s  %9s  %9s  %9s  %9s\n",
           "#", "Month", "Rashi",
           "Our", "Drik",
           "Sank_IST", "NextMid", "NxtCrit", "DPNxtCrit");

    for (int i = 0; i < (int)NUM_CASES; i++) {
        Mismatch *c = &CASES[i];
        double target_long = (c->rashi == 1) ? 0.0 : (double)(c->rashi - 1) * 30.0;
        int s_y = (c->diff > 0) ? c->ref_y : c->drik_y;
        int s_m = (c->diff > 0) ? c->ref_m : c->drik_m;
        int s_d = (c->diff > 0) ? c->ref_d : c->drik_d;
        double jd_sank = sankranti_jd(gregorian_to_jd(s_y, s_m, s_d), target_long);

        /* IST date */
        double loc_jd = jd_sank + 5.5 / 24.0 + 0.5;
        int sy, sm, sd;
        jd_to_gregorian(floor(loc_jd), &sy, &sm, &sd);

        /* Next midnight */
        int ny, nm, nd;
        next_day_g(sy, sm, sd, &ny, &nm, &nd);
        double jd_next_mid = gregorian_to_jd(ny, nm, nd) - 5.5 / 24.0;
        double jd_next_crit = jd_next_mid + 24.0 / 1440.0;
        double delta_nmid = (jd_sank - jd_next_mid) * 1440.0;
        double delta_ncrit = (jd_sank - jd_next_crit) * 1440.0;

        /* Shifted sankranti */
        double dt = 0.5;
        double s1 = solar_longitude_sidereal(jd_sank + dt);
        double s2 = solar_longitude_sidereal(jd_sank - dt);
        double dm = s1 - s2;
        if (dm < 0) dm += 360.0;
        double da = 24.10 + 0.003 * (sy - 2000);
        double shift = (da / 3600.0) / dm;
        double jd_dp = jd_sank + shift;
        double delta_dp_ncrit = (jd_dp - jd_next_crit) * 1440.0;

        int sh, smn, ss;
        jd_to_ist_hms(jd_sank, &sh, &smn, &ss);

        printf("%-4d %-8s %-6s  %04d-%02d-%02d  %04d-%02d-%02d  %02d:%02d:%02d  %+7.1fmin  %+7.1fmin  %+7.1fmin\n",
               i + 1, c->bengali_month, RASHI_NAMES[c->rashi],
               c->ref_y, c->ref_m, c->ref_d,
               c->drik_y, c->drik_m, c->drik_d,
               sh, smn, ss,
               delta_nmid, delta_ncrit, delta_dp_ncrit);
    }

    printf("\n");
    printf("Legend:\n");
    printf("  Sank_IST:   sankranti time in IST (HH:MM:SS on the day before month start)\n");
    printf("  NextMid:    sankranti - next midnight (00:00 IST next day), in minutes\n");
    printf("  NxtCrit:    sankranti - next critical time (00:24 IST next day), in minutes\n");
    printf("  DPNxtCrit:  shifted sankranti - next critical time, in minutes\n");
    printf("              (shifted = our sank + ~8 min for drikpanchang ayanamsa diff)\n");
    printf("\n");
    printf("Negative values mean sankranti is BEFORE the reference time.\n");
    printf("For +1 cases: our code pushes sank to next day, but drik pushes 2 days.\n");
    printf("For -1 case:  our code pushes sank to next day, but drik keeps this day.\n");

    astro_close();
    return 0;
}
