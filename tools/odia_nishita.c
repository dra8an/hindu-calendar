/*
 * Compute exact apparent midnight and distance from it for all confirmed
 * Odia boundary cases. Tests whether the cutoff is a fixed offset from
 * apparent midnight (nishita/ardha-ratri).
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

int main(void)
{
    astro_init(NULL);
    Location loc = DEFAULT_LOCATION;

    struct {
        int gy, gm, gd, rashi;
        const char *verdict; /* "current" or "next" */
    } cases[] = {
        /* Confirmed by user on drikpanchang */
        {2025,  3, 14, 12, "current"},  /* 18:50 IST */
        {2030, 11, 16,  8, "current"},  /* 20:23 IST */
        {2030, 10, 17,  7, "current"},  /* 20:33 IST */
        {2025,  2, 12, 11, "current"},  /* 21:55 IST */
        {2024, 12, 15,  9, "current"},  /* 22:11 IST */
        {2013,  5, 14,  2, "next"},     /* 22:16 IST */
        {2003, 11, 16,  8, "next"},     /* 22:25 IST */
        {2018,  7, 16,  4, "next"},     /* 22:32 IST */
        {2022,  7, 16,  4, "next"},     /* 23:01 IST */
        {2026,  7, 16,  4, "next"},     /* 23:35 IST */
        {2001,  4, 13,  1, "next"},     /* 23:49 IST */
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);

    printf("Odia Nishita (Apparent Midnight) Analysis\n");
    printf("==========================================\n\n");
    printf("%-12s %-9s %-9s %-9s %-9s %-10s %-10s %-10s %s\n",
           "Date", "Sank IST", "Sunset", "Sunrise+", "App.Mid",
           "Bef.AppMid", "Night Len", "Ratio", "Verdict");
    printf("%-12s %-9s %-9s %-9s %-9s %-10s %-10s %-10s %s\n",
           "----------", "--------", "--------", "--------", "--------",
           "---------", "---------", "---------", "-------");

    for (int i = 0; i < ncases; i++) {
        int gy = cases[i].gy, gm = cases[i].gm, gd = cases[i].gd;
        int rashi = cases[i].rashi;

        /* Find sankranti */
        double target = (rashi - 1) * 30.0;
        double jd_est = gregorian_to_jd(gy, gm, gd);
        double jd_sank = sankranti_jd(jd_est, target);

        /* Sunset of the sankranti's local date */
        double sank_ist;
        jd_to_ist_hms(jd_sank, &sank_ist);

        /* Get local date of sankranti */
        double local_jd = jd_sank + loc.utc_offset / 24.0 + 0.5;
        int ly, lm, ld;
        jd_to_gregorian(floor(local_jd), &ly, &lm, &ld);
        double jd_day = gregorian_to_jd(ly, lm, ld);

        /* Sunset and next sunrise */
        double jd_ss = sunset_jd(jd_day, &loc);
        double jd_sr_next = sunrise_jd(jd_day + 1.0, &loc);

        /* Apparent midnight = midpoint of sunset and next sunrise */
        double jd_app_mid = (jd_ss + jd_sr_next) / 2.0;

        double ss_ist, sr_ist, am_ist;
        jd_to_ist_hms(jd_ss, &ss_ist);
        jd_to_ist_hms(jd_sr_next, &sr_ist);
        jd_to_ist_hms(jd_app_mid, &am_ist);
        /* Handle wrap: if apparent midnight is past midnight IST */
        if (am_ist < 12.0) am_ist += 24.0;

        /* Night length in hours */
        double night_len = (jd_sr_next - jd_ss) * 24.0;

        /* Hours before apparent midnight */
        double sank_ist_adj = sank_ist;
        if (sank_ist < 12.0) sank_ist_adj += 24.0;
        double before_am = am_ist - sank_ist_adj;

        /* Ratio: position within the night (0=sunset, 1=sunrise) */
        double night_pos = (jd_sank - jd_ss) / (jd_sr_next - jd_ss);

        printf("%04d-%02d-%02d   ", ly, lm, ld);
        print_hm(sank_ist); printf("  ");
        print_hm(ss_ist); printf("  ");
        print_hm(sr_ist); printf("  ");
        print_hm(am_ist > 24.0 ? am_ist - 24.0 : am_ist); printf("  ");

        /* Hours:min before apparent midnight */
        int bh = (int)before_am;
        int bm = (int)((before_am - bh) * 60.0);
        int bs = (int)(((before_am - bh) * 60.0 - bm) * 60.0);
        printf("%dh%02dm%02ds   ", bh, bm, bs);

        /* Night length */
        int nh = (int)night_len;
        int nm = (int)((night_len - nh) * 60.0);
        printf("%dh%02dm      ", nh, nm);

        printf("%.4f     ", night_pos);
        printf("%s\n", cases[i].verdict);
    }

    /* Also compute for specific uncertain cases */
    printf("\n\nUncertain cases (need verification):\n");
    printf("=====================================\n\n");

    struct { int gy, gm, gd, rashi; } uncertain[] = {
        {1971,  3, 14, 12},  /* 22:14 IST */
        {2040,  9, 16,  6},  /* 22:14 IST */
        {1935,  5, 14,  2},  /* 22:16 IST */
        {1912,  2, 12, 11},  /* 22:16 IST */
    };
    int nunc = sizeof(uncertain) / sizeof(uncertain[0]);

    for (int i = 0; i < nunc; i++) {
        int gy = uncertain[i].gy, gm = uncertain[i].gm, gd = uncertain[i].gd;
        int rashi = uncertain[i].rashi;
        double target = (rashi - 1) * 30.0;
        double jd_est = gregorian_to_jd(gy, gm, gd);
        double jd_sank = sankranti_jd(jd_est, target);

        double sank_ist;
        jd_to_ist_hms(jd_sank, &sank_ist);

        double local_jd = jd_sank + loc.utc_offset / 24.0 + 0.5;
        int ly, lm, ld;
        jd_to_gregorian(floor(local_jd), &ly, &lm, &ld);
        double jd_day = gregorian_to_jd(ly, lm, ld);

        double jd_ss = sunset_jd(jd_day, &loc);
        double jd_sr_next = sunrise_jd(jd_day + 1.0, &loc);
        double jd_app_mid = (jd_ss + jd_sr_next) / 2.0;

        double am_ist;
        jd_to_ist_hms(jd_app_mid, &am_ist);
        if (am_ist < 12.0) am_ist += 24.0;

        double sank_ist_adj = sank_ist < 12.0 ? sank_ist + 24.0 : sank_ist;
        double before_am = am_ist - sank_ist_adj;

        printf("%04d-%02d-%02d   sank=", ly, lm, ld);
        print_hm(sank_ist);
        printf("  app_mid=");
        print_hm(am_ist > 24.0 ? am_ist - 24.0 : am_ist);
        int bh = (int)before_am;
        int bm = (int)((before_am - bh) * 60.0);
        int bs = (int)(((before_am - bh) * 60.0 - bm) * 60.0);
        printf("  before_am=%dh%02dm%02ds\n", bh, bm, bs);
    }

    astro_close();
    return 0;
}
