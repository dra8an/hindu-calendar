/*
 * bengali_rule_test.c -- Test alternative Bengali solar calendar critical time
 * rules to see which one explains all 8 drikpanchang.com mismatches.
 *
 * For each of 8 mismatched month boundaries, tests 5 alternative rules:
 *   (a) Sunset rule:           crit = sunset IST of the sankranti's civil day
 *   (b) Midnight - 24 min:    crit = 23:36 IST (previous day)
 *   (c) Midnight flat:        crit = 00:00 IST exactly
 *   (d) Midnight + 24 min with DP ayanamsa: our current rule but with
 *       drikpanchang's ~9.5 min shifted sankranti time
 *   (e) Sunset - ayanamsa buffer: like Tamil/Malayalam, sunset minus some buffer
 *
 * Build:
 *   cc -O2 -Isrc -Ilib/moshier tools/bengali_rule_test.c \
 *      src/panchang.c src/solar.c src/astro.c src/date_utils.c \
 *      src/tithi.c src/masa.c lib/moshier/moshier_*.c \
 *      -lm -o tools/bengali_rule_test
 *
 * Run:
 *   ./tools/bengali_rule_test
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
    { 1933,10,17,  1933,10,18,  +1,  7, "Kartik"   },   /* Tula */
    { 1952, 7,17,  1952, 7,16,  -1,  4, "Srabon"   },   /* Karka */
    { 1958,12,16,  1958,12,17,  +1,  9, "Poush"    },   /* Dhanu */
    { 1972,10,17,  1972,10,18,  +1,  7, "Kartik"   },   /* Tula */
    { 1974, 9,17,  1974, 9,18,  +1,  6, "Ashshin"  },   /* Kanya */
    { 1976,10,17,  1976,10,18,  +1,  7, "Kartik"   },   /* Tula */
    { 2011,10,18,  2011,10,19,  +1,  7, "Kartik"   },   /* Tula */
    { 2013, 9,17,  2013, 9,18,  +1,  6, "Ashshin"  },   /* Kanya */
};
#define NUM_CASES (sizeof(CASES) / sizeof(CASES[0]))

/* Helper: JD to IST time string (HH:MM:SS) */
static void jd_to_ist_hms(double jd_ut, int *h, int *m, int *s)
{
    double ist = jd_ut + 5.5 / 24.0;
    double frac = ist + 0.5 - floor(ist + 0.5);
    double secs = frac * 86400.0;
    *h = (int)(secs / 3600.0);
    *m = (int)(fmod(secs, 3600.0) / 60.0);
    *s = (int)(fmod(secs, 60.0));
}

/* Helper: JD to full IST date-time string */
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

/* Helper: get IST date from JD */
static void jd_to_ist_date(double jd_ut, int *y, int *m, int *d)
{
    double local_jd = jd_ut + 5.5 / 24.0 + 0.5;
    jd_to_gregorian(floor(local_jd), y, m, d);
}

/* Helper: next day */
static void next_day_g(int y, int m, int d, int *ny, int *nm, int *nd)
{
    double jd = gregorian_to_jd(y, m, d) + 1.0;
    jd_to_gregorian(jd, ny, nm, nd);
}

/* Helper: previous day */
static void prev_day_g(int y, int m, int d, int *py, int *pm, int *pd)
{
    double jd = gregorian_to_jd(y, m, d) - 1.0;
    jd_to_gregorian(jd, py, pm, pd);
}

/*
 * Compute drikpanchang-shifted sankranti time.
 * DP uses a Lahiri ayanamsa ~24 arcsec larger, which shifts sankranti later.
 */
static double shifted_sankranti(double jd_sank, int sank_year)
{
    double dt = 0.5;
    double sid_plus = solar_longitude_sidereal(jd_sank + dt);
    double sid_minus = solar_longitude_sidereal(jd_sank - dt);
    double daily_motion = sid_plus - sid_minus;
    if (daily_motion < 0) daily_motion += 360.0;

    double da_arcsec = 24.10 + 0.003 * (sank_year - 2000);
    double shift_days = (da_arcsec / 3600.0) / daily_motion;
    return jd_sank + shift_days;
}

/*
 * Bengali assignment logic: given a sankranti JD and rashi, determine
 * which civil day "owns" it using the given critical time JD.
 *
 * Algorithm:
 *   1. Convert sankranti to IST date
 *   2. If sank <= crit: apply Bengali tithi rule
 *      - Karka (4): keep this day (always "before midnight")
 *      - Makara (10): push to next day (always "after midnight")
 *      - Others: if tithi at previous sunrise extends past sank -> this day,
 *                else -> next day
 *   3. If sank > crit: push to next day
 *
 * Returns the Gregorian date of the month start.
 */
static void bengali_assign(double jd_sank, double jd_crit, int rashi,
                           const Location *loc, int *gy, int *gm, int *gd)
{
    /* IST date of sankranti */
    int sy, sm, sd;
    jd_to_ist_date(jd_sank, &sy, &sm, &sd);
    double jd_day = gregorian_to_jd(sy, sm, sd);

    if (jd_sank <= jd_crit) {
        /* Bengali tithi rule */
        if (rashi == 4) {
            /* Karka: always keep this day */
            *gy = sy; *gm = sm; *gd = sd;
        } else if (rashi == 10) {
            /* Makara: always push to next day */
            jd_to_gregorian(jd_day + 1.0, gy, gm, gd);
        } else {
            /* Others: tithi-based */
            int py, pm, pd;
            jd_to_gregorian(jd_day - 1.0, &py, &pm, &pd);
            TithiInfo ti = tithi_at_sunrise(py, pm, pd, loc);
            if (ti.jd_end > jd_sank) {
                /* Tithi extends past sankranti: keep this day */
                *gy = sy; *gm = sm; *gd = sd;
            } else {
                /* Tithi ends before sankranti: push to next day */
                jd_to_gregorian(jd_day + 1.0, gy, gm, gd);
            }
        }
    } else {
        /* After critical time: push to next day */
        int ny, nm, nd;
        next_day_g(sy, sm, sd, &ny, &nm, &nd);
        *gy = ny; *gm = nm; *gd = nd;
    }
}

/*
 * Check if an assignment matches drikpanchang's expected date.
 * Returns 1 if match, 0 if mismatch.
 */
static int check_match(int gy, int gm, int gd, const Mismatch *c)
{
    return (gy == c->drik_y && gm == c->drik_m && gd == c->drik_d);
}

int main(void)
{
    astro_init(NULL);
    Location delhi = DEFAULT_LOCATION;

    printf("================================================================\n");
    printf("  Bengali Solar Calendar: Alternative Rule Testing\n");
    printf("  Testing 5 rules against 8 drikpanchang.com mismatches\n");
    printf("  Location: Delhi (28.6139N, 77.2090E, UTC+5:30)\n");
    printf("================================================================\n\n");

    int rule_correct[5] = {0}; /* count of correct predictions per rule */
    const char *rule_names[] = {
        "(a) Sunset rule",
        "(b) Midnight - 24 min (23:36 IST)",
        "(c) Midnight flat (00:00 IST)",
        "(d) Midnight+24 with DP ayanamsa",
        "(e) Sunset - 8 min buffer"
    };

    for (int i = 0; i < (int)NUM_CASES; i++) {
        Mismatch *c = &CASES[i];

        printf("=== Case %d: %s (rashi %d = %s), diff=%+d ===\n",
               i + 1, c->bengali_month, c->rashi, RASHI_NAMES[c->rashi], c->diff);
        printf("  Our month start:  %04d-%02d-%02d\n",
               c->ref_y, c->ref_m, c->ref_d);
        printf("  Drik month start: %04d-%02d-%02d\n",
               c->drik_y, c->drik_m, c->drik_d);

        /* Find the sankranti.
         * Search near the earlier date (sankranti is always before the month start). */
        double target_long = (c->rashi == 1) ? 0.0 : (double)(c->rashi - 1) * 30.0;
        int search_y = (c->diff > 0) ? c->ref_y : c->drik_y;
        int search_m = (c->diff > 0) ? c->ref_m : c->drik_m;
        int search_d = (c->diff > 0) ? c->ref_d : c->drik_d;
        double jd_search = gregorian_to_jd(search_y, search_m, search_d);
        double jd_sank = sankranti_jd(jd_search, target_long);

        /* IST date of sankranti */
        int sank_y, sank_m, sank_d;
        jd_to_ist_date(jd_sank, &sank_y, &sank_m, &sank_d);
        double jd_sank_day = gregorian_to_jd(sank_y, sank_m, sank_d);

        char sank_str[64];
        jd_to_ist_str(jd_sank, sank_str, sizeof(sank_str));
        printf("  Sankranti time:   %s\n", sank_str);

        /* Compute DP shifted sankranti */
        double jd_dp = shifted_sankranti(jd_sank, sank_y);
        char dp_str[64];
        jd_to_ist_str(jd_dp, dp_str, sizeof(dp_str));
        double shift_min = (jd_dp - jd_sank) * 1440.0;
        printf("  DP shifted sank:  %s  (shift=+%.1f min)\n", dp_str, shift_min);

        /* DP shifted sankranti's IST date (may differ from sank date if it crosses midnight) */
        int dp_y, dp_m, dp_d;
        jd_to_ist_date(jd_dp, &dp_y, &dp_m, &dp_d);
        double jd_dp_day = gregorian_to_jd(dp_y, dp_m, dp_d);

        /* Sunset of the sankranti's civil day */
        double jd_sunset = sunset_jd(jd_sank_day, &delhi);
        char sunset_str[64];
        jd_to_ist_str(jd_sunset, sunset_str, sizeof(sunset_str));
        printf("  Sunset on %04d-%02d-%02d: %s\n",
               sank_y, sank_m, sank_d, sunset_str);

        /* Tithi at previous day's sunrise (for the Bengali tithi rule) */
        int py, pm, pd;
        prev_day_g(sank_y, sank_m, sank_d, &py, &pm, &pd);
        TithiInfo ti_prev = tithi_at_sunrise(py, pm, pd, &delhi);
        char ti_end_str[64];
        jd_to_ist_str(ti_prev.jd_end, ti_end_str, sizeof(ti_end_str));
        int tn = ti_prev.tithi_num;
        printf("  Tithi at %04d-%02d-%02d sunrise: #%d (%s), ends %s\n",
               py, pm, pd, tn, (tn >= 1 && tn <= 30) ? TITHI_NAME[tn] : "???",
               ti_end_str);

        /* Also show tithi at sank_date sunrise (used when sank shifts to next day) */
        TithiInfo ti_sank = tithi_at_sunrise(sank_y, sank_m, sank_d, &delhi);
        char ti_sank_end_str[64];
        jd_to_ist_str(ti_sank.jd_end, ti_sank_end_str, sizeof(ti_sank_end_str));
        int tn_s = ti_sank.tithi_num;
        printf("  Tithi at %04d-%02d-%02d sunrise: #%d (%s), ends %s\n",
               sank_y, sank_m, sank_d, tn_s,
               (tn_s >= 1 && tn_s <= 30) ? TITHI_NAME[tn_s] : "???",
               ti_sank_end_str);

        /* Next day after sankranti date */
        int nx_y, nx_m, nx_d;
        next_day_g(sank_y, sank_m, sank_d, &nx_y, &nx_m, &nx_d);

        printf("\n  --- Testing rules ---\n");

        /* (a) Sunset rule: crit = sunset of sankranti's civil day.
         * If sank is in the evening (before midnight), sunset is on that day.
         * For post-midnight sankrantis (after 00:00), the "current day"
         * in sunset context is actually the previous calendar day.
         * We use the IST date of the sankranti and compute sunset for that date. */
        {
            /* The relevant sunset is from the previous day if sank is post-midnight
             * (i.e., IST hour < 12). For evening sankrantis (hour >= 12), use same day. */
            int sh, sm_t, ss;
            jd_to_ist_hms(jd_sank, &sh, &sm_t, &ss);
            double jd_sunset_day;
            int sunset_base_y, sunset_base_m, sunset_base_d;
            if (sh < 12) {
                /* Post-midnight: sankranti is early morning, relevant sunset is previous evening */
                prev_day_g(sank_y, sank_m, sank_d, &sunset_base_y, &sunset_base_m, &sunset_base_d);
            } else {
                /* Evening: sunset of this day */
                sunset_base_y = sank_y; sunset_base_m = sank_m; sunset_base_d = sank_d;
            }
            jd_sunset_day = gregorian_to_jd(sunset_base_y, sunset_base_m, sunset_base_d);
            double crit_a = sunset_jd(jd_sunset_day, &delhi);

            int gy, gm, gd;
            bengali_assign(jd_sank, crit_a, c->rashi, &delhi, &gy, &gm, &gd);
            int ok = check_match(gy, gm, gd, c);
            rule_correct[0] += ok;
            char crit_str[64];
            jd_to_ist_str(crit_a, crit_str, sizeof(crit_str));
            printf("  (a) Sunset rule:     crit=%s -> %04d-%02d-%02d  %s\n",
                   crit_str, gy, gm, gd, ok ? "OK" : "FAIL");
        }

        /* (b) Midnight - 24 min (23:36 IST of sankranti date or prev day depending on sank time).
         * If sankranti is post-midnight (00:00-01:00), the 23:36 IST is from the previous day. */
        {
            int sh, sm_t, ss;
            jd_to_ist_hms(jd_sank, &sh, &sm_t, &ss);
            /* 23:36 IST = midnight of the NEXT day minus 24 min.
             * For a sank at 23:47 IST on Oct 16, the relevant 23:36 is Oct 16 23:36.
             * For a sank at 00:31 IST on Jul 16, the relevant 23:36 is Jul 15 23:36 (prev day).
             * The "anchor day" is the day BEFORE the IST date if sank hour < 12,
             * or the IST date itself if sank hour >= 12.
             * Actually, the rule should be: if sank falls between 23:36 of day D and
             * 23:36 of day D+1, it's in the "midnight zone" of the night D->D+1.
             * crit = the 23:36 IST that is >= sank, i.e. the END of the zone. */
            /* Simpler: 23:36 IST = the day D that contains the midnight that sank is near.
             * If sank is at 23:47 on Oct 16, the midnight is Oct 17 00:00, crit = Oct 16 23:36
             * If sank is at 00:31 on Jul 16, the midnight is Jul 16 00:00, crit = Jul 15 23:36 */
            int crit_day_y, crit_day_m, crit_day_d;
            if (sh < 12) {
                /* Post-midnight: 23:36 of previous calendar day */
                prev_day_g(sank_y, sank_m, sank_d, &crit_day_y, &crit_day_m, &crit_day_d);
            } else {
                /* Pre-midnight: 23:36 of this calendar day */
                crit_day_y = sank_y; crit_day_m = sank_m; crit_day_d = sank_d;
            }
            /* 23:36 IST = 18:06 UTC = JD_midnight + 18.1/24 */
            double jd_crit_day = gregorian_to_jd(crit_day_y, crit_day_m, crit_day_d);
            double crit_b = jd_crit_day + (23.0 * 60.0 + 36.0) / 1440.0
                           - delhi.utc_offset / 24.0;

            int gy, gm, gd;
            bengali_assign(jd_sank, crit_b, c->rashi, &delhi, &gy, &gm, &gd);
            int ok = check_match(gy, gm, gd, c);
            rule_correct[1] += ok;
            char crit_str[64];
            jd_to_ist_str(crit_b, crit_str, sizeof(crit_str));
            printf("  (b) Mid-24min rule:  crit=%s -> %04d-%02d-%02d  %s\n",
                   crit_str, gy, gm, gd, ok ? "OK" : "FAIL");
        }

        /* (c) Midnight flat (00:00 IST exactly).
         * The relevant midnight is the one closest to the sankranti.
         * For a sank at 23:47, that's the upcoming midnight (next day 00:00).
         * For a sank at 00:31, that's the preceding midnight (this day 00:00). */
        {
            /* The midnight of the IST date = start of the IST date.
             * JD for midnight IST of date D = gregorian_to_jd(D) - 5.5/24 */
            double jd_midnight = jd_sank_day - delhi.utc_offset / 24.0;

            int gy, gm, gd;
            bengali_assign(jd_sank, jd_midnight, c->rashi, &delhi, &gy, &gm, &gd);
            int ok = check_match(gy, gm, gd, c);
            rule_correct[2] += ok;
            char crit_str[64];
            jd_to_ist_str(jd_midnight, crit_str, sizeof(crit_str));
            printf("  (c) Midnight flat:   crit=%s -> %04d-%02d-%02d  %s\n",
                   crit_str, gy, gm, gd, ok ? "OK" : "FAIL");
        }

        /* (d) Midnight + 24 min with DP ayanamsa:
         * Use DP's shifted sankranti time, then apply our standard rule
         * (midnight + 24 min, Karka=C, Makara=W, tithi rule for others).
         * The shifted sank might land on a different IST date. */
        {
            /* IST date of the DP shifted sankranti */
            double jd_dp_day_val = gregorian_to_jd(dp_y, dp_m, dp_d);

            /* crit = midnight of the DP shifted sankranti's IST date + 24 min */
            double crit_d = jd_dp_day_val - delhi.utc_offset / 24.0
                           + 24.0 / (24.0 * 60.0);

            int gy, gm, gd;
            bengali_assign(jd_dp, crit_d, c->rashi, &delhi, &gy, &gm, &gd);
            int ok = check_match(gy, gm, gd, c);
            rule_correct[3] += ok;
            char crit_str[64];
            jd_to_ist_str(crit_d, crit_str, sizeof(crit_str));
            printf("  (d) Mid+24 w/DP aya: crit=%s, sank=%s -> %04d-%02d-%02d  %s\n",
                   crit_str, dp_str, gy, gm, gd, ok ? "OK" : "FAIL");

            /* Show detailed trace for rule (d) */
            int dsh, dsm, dss;
            jd_to_ist_hms(jd_dp, &dsh, &dsm, &dss);
            printf("       DP sank %02d:%02d:%02d IST on %04d-%02d-%02d, "
                   "crit=00:24, sank%scrit",
                   dsh, dsm, dss, dp_y, dp_m, dp_d,
                   (jd_dp <= crit_d) ? "<=" : ">");
            if (jd_dp <= crit_d) {
                if (c->rashi == 4)
                    printf(" -> Karka keep\n");
                else if (c->rashi == 10)
                    printf(" -> Makara push\n");
                else {
                    /* Need tithi at the day before the DP sank's IST date */
                    int dpy, dpm, dpd;
                    prev_day_g(dp_y, dp_m, dp_d, &dpy, &dpm, &dpd);
                    TithiInfo ti_dp = tithi_at_sunrise(dpy, dpm, dpd, &delhi);
                    int extends = (ti_dp.jd_end > jd_dp) ? 1 : 0;
                    printf(" -> tithi #%d at %04d-%02d-%02d, end-sank=%.1f min, %s\n",
                           ti_dp.tithi_num, dpy, dpm, dpd,
                           (ti_dp.jd_end - jd_dp) * 1440.0,
                           extends ? "extends->keep" : "ends->push");
                }
            } else {
                printf(" -> push to next day\n");
            }
        }

        /* (e) Sunset - 8 min buffer (like Tamil).
         * Same as (a) but subtract 8 minutes from sunset. */
        {
            int sh, sm_t, ss;
            jd_to_ist_hms(jd_sank, &sh, &sm_t, &ss);
            double jd_sunset_day;
            int sunset_base_y, sunset_base_m, sunset_base_d;
            if (sh < 12) {
                prev_day_g(sank_y, sank_m, sank_d, &sunset_base_y, &sunset_base_m, &sunset_base_d);
            } else {
                sunset_base_y = sank_y; sunset_base_m = sank_m; sunset_base_d = sank_d;
            }
            jd_sunset_day = gregorian_to_jd(sunset_base_y, sunset_base_m, sunset_base_d);
            double crit_e = sunset_jd(jd_sunset_day, &delhi) - 8.0 / (24.0 * 60.0);

            int gy, gm, gd;
            bengali_assign(jd_sank, crit_e, c->rashi, &delhi, &gy, &gm, &gd);
            int ok = check_match(gy, gm, gd, c);
            rule_correct[4] += ok;
            char crit_str[64];
            jd_to_ist_str(crit_e, crit_str, sizeof(crit_str));
            printf("  (e) Sunset-8min:     crit=%s -> %04d-%02d-%02d  %s\n",
                   crit_str, gy, gm, gd, ok ? "OK" : "FAIL");
        }

        printf("\n");
    }

    /* ============================================================ */
    printf("================================================================\n");
    printf("  SUMMARY TABLE\n");
    printf("================================================================\n\n");

    printf("%-4s %-8s %-6s  %-10s  %-10s  |  (a)  (b)  (c)  (d)  (e)\n",
           "#", "Month", "Rashi", "Our", "Drik");
    printf("---- -------- ------  ----------  ----------  |  ---  ---  ---  ---  ---\n");

    int per_case_ok[8][5] = {{0}};

    for (int i = 0; i < (int)NUM_CASES; i++) {
        Mismatch *c = &CASES[i];

        /* Recompute results for the summary (avoid storing globally) */
        double target_long = (c->rashi == 1) ? 0.0 : (double)(c->rashi - 1) * 30.0;
        int search_y = (c->diff > 0) ? c->ref_y : c->drik_y;
        int search_m = (c->diff > 0) ? c->ref_m : c->drik_m;
        int search_d = (c->diff > 0) ? c->ref_d : c->drik_d;
        double jd_sank = sankranti_jd(gregorian_to_jd(search_y, search_m, search_d),
                                       target_long);

        int sank_y, sank_m, sank_d;
        jd_to_ist_date(jd_sank, &sank_y, &sank_m, &sank_d);
        double jd_sank_day = gregorian_to_jd(sank_y, sank_m, sank_d);

        double jd_dp = shifted_sankranti(jd_sank, sank_y);
        int dp_y, dp_m, dp_d;
        jd_to_ist_date(jd_dp, &dp_y, &dp_m, &dp_d);

        int sh, sm_t, ss;
        jd_to_ist_hms(jd_sank, &sh, &sm_t, &ss);

        /* (a) Sunset */
        {
            int sb_y, sb_m, sb_d;
            if (sh < 12) { prev_day_g(sank_y, sank_m, sank_d, &sb_y, &sb_m, &sb_d); }
            else { sb_y = sank_y; sb_m = sank_m; sb_d = sank_d; }
            double crit = sunset_jd(gregorian_to_jd(sb_y, sb_m, sb_d), &delhi);
            int gy, gm, gd;
            bengali_assign(jd_sank, crit, c->rashi, &delhi, &gy, &gm, &gd);
            per_case_ok[i][0] = check_match(gy, gm, gd, c);
        }
        /* (b) Mid-24 */
        {
            int cd_y, cd_m, cd_d;
            if (sh < 12) { prev_day_g(sank_y, sank_m, sank_d, &cd_y, &cd_m, &cd_d); }
            else { cd_y = sank_y; cd_m = sank_m; cd_d = sank_d; }
            double crit = gregorian_to_jd(cd_y, cd_m, cd_d)
                         + (23.0 * 60.0 + 36.0) / 1440.0
                         - delhi.utc_offset / 24.0;
            int gy, gm, gd;
            bengali_assign(jd_sank, crit, c->rashi, &delhi, &gy, &gm, &gd);
            per_case_ok[i][1] = check_match(gy, gm, gd, c);
        }
        /* (c) Midnight flat */
        {
            double crit = jd_sank_day - delhi.utc_offset / 24.0;
            int gy, gm, gd;
            bengali_assign(jd_sank, crit, c->rashi, &delhi, &gy, &gm, &gd);
            per_case_ok[i][2] = check_match(gy, gm, gd, c);
        }
        /* (d) Mid+24 with DP ayanamsa */
        {
            double jd_dp_day_val = gregorian_to_jd(dp_y, dp_m, dp_d);
            double crit = jd_dp_day_val - delhi.utc_offset / 24.0
                         + 24.0 / (24.0 * 60.0);
            int gy, gm, gd;
            bengali_assign(jd_dp, crit, c->rashi, &delhi, &gy, &gm, &gd);
            per_case_ok[i][3] = check_match(gy, gm, gd, c);
        }
        /* (e) Sunset - 8min */
        {
            int sb_y, sb_m, sb_d;
            if (sh < 12) { prev_day_g(sank_y, sank_m, sank_d, &sb_y, &sb_m, &sb_d); }
            else { sb_y = sank_y; sb_m = sank_m; sb_d = sank_d; }
            double crit = sunset_jd(gregorian_to_jd(sb_y, sb_m, sb_d), &delhi)
                         - 8.0 / (24.0 * 60.0);
            int gy, gm, gd;
            bengali_assign(jd_sank, crit, c->rashi, &delhi, &gy, &gm, &gd);
            per_case_ok[i][4] = check_match(gy, gm, gd, c);
        }

        printf("%-4d %-8s %-6s  %04d-%02d-%02d  %04d-%02d-%02d  |   %s    %s    %s    %s    %s\n",
               i + 1, c->bengali_month, RASHI_NAMES[c->rashi],
               c->ref_y, c->ref_m, c->ref_d,
               c->drik_y, c->drik_m, c->drik_d,
               per_case_ok[i][0] ? "Y" : "N",
               per_case_ok[i][1] ? "Y" : "N",
               per_case_ok[i][2] ? "Y" : "N",
               per_case_ok[i][3] ? "Y" : "N",
               per_case_ok[i][4] ? "Y" : "N");
    }

    printf("---- -------- ------  ----------  ----------  |  ---  ---  ---  ---  ---\n");
    printf("%-51s|  %d/8  %d/8  %d/8  %d/8  %d/8\n",
           "TOTAL CORRECT:",
           rule_correct[0], rule_correct[1], rule_correct[2],
           rule_correct[3], rule_correct[4]);

    printf("\n");
    printf("Rules:\n");
    for (int r = 0; r < 5; r++) {
        printf("  %s: %d/8 correct%s\n",
               rule_names[r], rule_correct[r],
               (rule_correct[r] == 8) ? "  *** ALL CORRECT ***" : "");
    }

    printf("\n");
    printf("Notes:\n");
    printf("  (a) Sunset: crit = sunset IST on the relevant day.\n");
    printf("      If sank > sunset -> push to next day, Bengali tithi rule applies.\n");
    printf("  (b) Mid-24min: crit = 23:36 IST (midnight minus 24 min).\n");
    printf("      Symmetric opposite of our current midnight+24min rule.\n");
    printf("  (c) Midnight flat: crit = 00:00 IST exactly.\n");
    printf("      No buffer zone around midnight.\n");
    printf("  (d) DP ayanamsa: our current midnight+24min rule, but with DP's\n");
    printf("      shifted sankranti (our time + ~9.5 min for ayanamsa difference).\n");
    printf("  (e) Sunset-8min: like Tamil, subtract 8 min buffer from sunset.\n");

    astro_close();
    return 0;
}
