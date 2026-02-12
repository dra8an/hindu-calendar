/*
 * Scan ALL Odia sankrantis 1900-2050 in the 22:00-22:30 IST range.
 * For each, compute distance to apparent midnight and show what our
 * code assigns vs what a pure-2h cutoff would give.
 * Sorted by "before apparent midnight" to find the tightest cases.
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include <stdio.h>
#include <math.h>

static void jd_to_ist_hms(double jd_ut, double *ist_hours)
{
    double local = jd_ut + 5.5 / 24.0 + 0.5;
    double frac = local - floor(local);
    *ist_hours = frac * 24.0;
}

static void print_hm(double hours)
{
    int h = (int)hours;
    int m = (int)((hours - h) * 60.0);
    int s = (int)(((hours - h) * 60.0 - m) * 60.0);
    printf("%02d:%02d:%02d", h, m, s);
}

typedef struct {
    int gy, gm, gd, rashi;
    double sank_ist;
    double before_am_minutes;
    int our_month, our_day;
} Entry;

int main(void)
{
    astro_init(NULL);
    Location loc = DEFAULT_LOCATION;

    Entry entries[500];
    int n = 0;

    for (int gy = 1900; gy <= 2050; gy++) {
        for (int rashi = 1; rashi <= 12; rashi++) {
            double target_long = (rashi - 1) * 30.0;
            int approx_month = 3 + rashi;
            int approx_year = gy;
            if (approx_month > 12) { approx_month -= 12; approx_year++; }
            double jd_est = gregorian_to_jd(approx_year, approx_month, 14);
            double jd_sank = sankranti_jd(jd_est, target_long);

            double sank_ist;
            jd_to_ist_hms(jd_sank, &sank_ist);

            /* Only interested in 21:30-22:30 IST range */
            if (sank_ist < 21.5 || sank_ist > 22.5) continue;

            /* Get local date */
            double local_jd = jd_sank + loc.utc_offset / 24.0 + 0.5;
            int ly, lm, ld;
            jd_to_gregorian(floor(local_jd), &ly, &lm, &ld);
            double jd_day = gregorian_to_jd(ly, lm, ld);

            /* Apparent midnight */
            double jd_ss = sunset_jd(jd_day, &loc);
            double jd_sr_next = sunrise_jd(jd_day + 1.0, &loc);
            double jd_app_mid = (jd_ss + jd_sr_next) / 2.0;

            double before_am_min = (jd_app_mid - jd_sank) * 24.0 * 60.0;

            /* What our code says */
            SolarDate sd = gregorian_to_solar(ly, lm, ld, &loc, SOLAR_CAL_ODIA);

            if (n < 500) {
                entries[n].gy = ly;
                entries[n].gm = lm;
                entries[n].gd = ld;
                entries[n].rashi = rashi;
                entries[n].sank_ist = sank_ist;
                entries[n].before_am_minutes = before_am_min;
                entries[n].our_month = sd.month;
                entries[n].our_day = sd.day;
                n++;
            }
        }
    }

    /* Sort by before_am_minutes ascending (tightest cases first) */
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (entries[i].before_am_minutes > entries[j].before_am_minutes) {
                Entry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }

    printf("Odia sankrantis 21:30-22:30 IST, sorted by distance to apparent midnight\n");
    printf("=========================================================================\n");
    printf("Cutoff = 122.0 min (2h02m) before apparent midnight\n\n");
    printf("%-12s  %-6s  %-10s  %-12s  %-10s  %-8s  %s\n",
           "Date", "Rashi", "Sank IST", "Bef.AppMid", "Our result", "Assign",
           "Note");
    printf("%-12s  %-6s  %-10s  %-12s  %-10s  %-8s  %s\n",
           "----------", "-----", "---------", "-----------", "---------", "-------",
           "----");

    for (int i = 0; i < n; i++) {
        Entry *e = &entries[i];
        int bm_total = (int)e->before_am_minutes;
        int bh = bm_total / 60;
        int bm = bm_total % 60;
        int bs = (int)((e->before_am_minutes - bm_total) * 60.0);

        const char *assign = (e->before_am_minutes >= 122.0) ? "current" : "next";
        const char *note = "";

        /* Flag cases near the cutoff (within 5 minutes) */
        if (fabs(e->before_am_minutes - 122.0) < 5.0)
            note = "*** NEAR CUTOFF ***";
        else if (fabs(e->before_am_minutes - 122.0) < 10.0)
            note = "* close *";
        /* Flag confirmed cases */
        if (e->gy == 2024 && e->gm == 12 && e->gd == 15)
            note = "CONFIRMED current";
        if (e->gy == 2013 && e->gm == 5 && e->gd == 14)
            note = "CONFIRMED next";

        printf("%04d-%02d-%02d    %-6d  ", e->gy, e->gm, e->gd, e->rashi);
        print_hm(e->sank_ist);
        printf("    %dh%02dm%02ds      ", bh, bm, bs);
        printf("M%d D%d     ", e->our_month, e->our_day);
        printf("%-8s  %s\n", assign, note);
    }

    printf("\nTotal: %d sankrantis in 21:30-22:30 IST range\n", n);

    astro_close();
    return 0;
}
