/*
 * Print exact sankranti times for Odia boundary cases.
 * Checks all sankrantis from 1900-2050 that fall between 18:00-02:00 IST
 * to find the exact drikpanchang cutoff time.
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include <stdio.h>
#include <math.h>

static void jd_to_ist_time(double jd_ut, int *y, int *m, int *d, int *hh, int *mm, int *ss)
{
    double local = jd_ut + 5.5 / 24.0 + 0.5;
    double day_frac = local - floor(local);
    jd_to_gregorian(floor(local), y, m, d);
    double secs = day_frac * 86400.0;
    *hh = (int)(secs / 3600.0);
    *mm = (int)(fmod(secs, 3600.0) / 60.0);
    *ss = (int)(fmod(secs, 60.0));
}

int main(void)
{
    astro_init(NULL);
    Location loc = DEFAULT_LOCATION;

    /* Print specific boundary cases: the 4 confirmed "next day" and
     * the 4 newly-discovered "current day" cases */
    printf("Specific boundary cases:\n");
    printf("========================\n\n");

    struct { int gy, gm, gd; int rashi; const char *note; } cases[] = {
        /* Confirmed: drikpanchang says NEXT day */
        {2026, 7, 16, 4, "CONFIRMED next day (Karka/Shravana)"},
        {2022, 7, 16, 4, "CONFIRMED next day (Karka/Shravana)"},
        {2018, 7, 16, 4, "CONFIRMED next day (Karka/Shravana)"},
        {2001, 4, 13, 1, "CONFIRMED next day (Mesha/Baisakha)"},
        /* Failing: drikpanchang says CURRENT day */
        {2025, 2, 12, 11, "VALIDATED current day (Kumbha/Phalguna)"},
        {2025, 3, 14, 12, "VALIDATED current day (Meena/Chaitra)"},
        {2030, 10, 17, 7, "VALIDATED current day (Tula/Kartika)"},
        {2030, 11, 16, 8, "VALIDATED current day (Vrishchika/Margashirsha)"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);

    printf("%-12s %-6s %-10s %-12s %-12s %-8s %s\n",
           "Greg Date", "Rashi", "IST Time", "Sunset IST", "Diff(s-sun)",
           "Mid Rule", "Note");
    printf("%-12s %-6s %-10s %-12s %-12s %-8s %s\n",
           "----------", "-----", "---------", "----------", "-----------",
           "--------", "----");

    for (int i = 0; i < ncases; i++) {
        int gy = cases[i].gy, gm = cases[i].gm, gd = cases[i].gd;
        int rashi = cases[i].rashi;
        double target_long = (rashi - 1) * 30.0;
        double jd_est = gregorian_to_jd(gy, gm, gd);
        double jd_sank = sankranti_jd(jd_est, target_long);

        int sy, sm, sd, sh, smin, ssec;
        jd_to_ist_time(jd_sank, &sy, &sm, &sd, &sh, &smin, &ssec);

        /* Sunset */
        double jd_day = gregorian_to_jd(sy, sm, sd);
        double ss = sunset_jd(jd_day, &loc);
        int suny, sunm, sund, sunh, sunmin, sunsec;
        jd_to_ist_time(ss, &suny, &sunm, &sund, &sunh, &sunmin, &sunsec);

        /* Difference in hours */
        double diff_h = (jd_sank - ss) * 24.0;

        /* Midnight rule result */
        double midnight_crit = jd_day + 1.0 - loc.utc_offset / 24.0;
        const char *mid_result = (jd_sank <= midnight_crit) ? "current" : "next";

        printf("%04d-%02d-%02d   %-6d %02d:%02d:%02d  %02d:%02d:%02d    %+.2fh      %-8s %s\n",
               sy, sm, sd, rashi,
               sh, smin, ssec,
               sunh, sunmin, sunsec,
               diff_h,
               mid_result,
               cases[i].note);
    }

    /* Now scan ALL sankrantis 1900-2050 in the 18:00-02:00 range */
    printf("\n\nAll sankrantis between 18:00-02:00 IST (sorted by IST hour):\n");
    printf("============================================================\n\n");

    typedef struct {
        int gy, gm, gd, rashi, hh, mm, ss;
        double hours_after_sunset;
    } Entry;

    Entry entries[2000];
    int nentries = 0;

    for (int gy = 1900; gy <= 2050; gy++) {
        for (int rashi = 1; rashi <= 12; rashi++) {
            double target_long = (rashi - 1) * 30.0;
            int approx_month = 3 + rashi;
            int approx_year = gy;
            if (approx_month > 12) { approx_month -= 12; approx_year++; }
            double jd_est2 = gregorian_to_jd(approx_year, approx_month, 14);
            double jd_sank2 = sankranti_jd(jd_est2, target_long);

            int sy2, sm2, sd2, sh2, smin2, ssec2;
            jd_to_ist_time(jd_sank2, &sy2, &sm2, &sd2, &sh2, &smin2, &ssec2);

            if (sh2 < 18 && sh2 >= 2) continue;

            double jd_day2 = gregorian_to_jd(sy2, sm2, sd2);
            double ss2 = sunset_jd(jd_day2, &loc);
            double diff_h2 = (jd_sank2 - ss2) * 24.0;

            if (nentries < 2000) {
                entries[nentries].gy = sy2;
                entries[nentries].gm = sm2;
                entries[nentries].gd = sd2;
                entries[nentries].rashi = rashi;
                entries[nentries].hh = sh2;
                entries[nentries].mm = smin2;
                entries[nentries].ss = ssec2;
                entries[nentries].hours_after_sunset = diff_h2;
                nentries++;
            }
        }
    }

    /* Sort by IST hour */
    for (int i = 0; i < nentries - 1; i++) {
        for (int j = i + 1; j < nentries; j++) {
            double ti = entries[i].hh * 3600 + entries[i].mm * 60 + entries[i].ss;
            double tj = entries[j].hh * 3600 + entries[j].mm * 60 + entries[j].ss;
            /* Handle wrap: 0-2 should sort after 22-23 */
            if (entries[i].hh < 12) ti += 86400;
            if (entries[j].hh < 12) tj += 86400;
            if (ti > tj) {
                Entry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }

    printf("%-12s %-6s %-10s %+10s\n", "Greg Date", "Rashi", "IST Time", "Hrs>sunset");
    printf("%-12s %-6s %-10s %10s\n", "----------", "-----", "---------", "----------");
    for (int i = 0; i < nentries; i++) {
        Entry *e = &entries[i];
        printf("%04d-%02d-%02d   %-6d %02d:%02d:%02d  %+.2fh\n",
               e->gy, e->gm, e->gd, e->rashi,
               e->hh, e->mm, e->ss,
               e->hours_after_sunset);
    }
    printf("\nTotal: %d sankrantis\n", nentries);

    astro_close();
    return 0;
}
