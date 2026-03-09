#include "panchang.h"
#include "tithi.h"
#include "masa.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <math.h>

/* Days in a Gregorian month */
static int days_in_month(int year, int month)
{
    static const int mdays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
            return 29;
    }
    return mdays[month];
}

/* Cache for previous day's sunrise and tithi, avoiding redundant
 * sunrise_jd() + tithi_num_at_jd() when iterating consecutive days. */
static double cached_jd_base = 0;     /* JD at 0h of cached day */
static double cached_jd_rise = 0;     /* sunrise JD of cached day */
static int    cached_tithi = 0;       /* tithi at cached sunrise */

HinduDate gregorian_to_hindu(int year, int month, int day, const Location *loc)
{
    HinduDate hd = {0};

    /* Compute sunrise once for today */
    double jd = gregorian_to_jd(year, month, day);
    double jd_rise = sunrise_jd(jd, loc);
    if (jd_rise <= 0) {
        jd_rise = jd + 0.5 - loc->utc_offset / 24.0;
    }

    /* Lightweight tithi: just the number, no boundary finding */
    int t = tithi_num_at_jd(jd_rise);
    hd.paksha = (t <= 15) ? SHUKLA_PAKSHA : KRISHNA_PAKSHA;
    hd.tithi = (t <= 15) ? t : t - 15;

    /* Get masa using pre-computed sunrise */
    MasaInfo mi = masa_for_date_at(jd_rise, loc);

    hd.masa = mi.name;
    hd.is_adhika_masa = mi.is_adhika;
    hd.year_saka = mi.year_saka;
    hd.year_vikram = mi.year_vikram;

    /* Check for adhika tithi: same tithi as previous day's sunrise.
     * Use cache to avoid redundant sunrise computation on consecutive days. */
    int t_prev;
    double jd_prev = jd - 1.0;
    if (cached_jd_base > 0 && fabs(jd_prev - cached_jd_base) < 0.01) {
        t_prev = cached_tithi;
    } else {
        double jd_rise_prev = sunrise_jd(jd_prev, loc);
        if (jd_rise_prev <= 0) {
            jd_rise_prev = jd_prev + 0.5 - loc->utc_offset / 24.0;
        }
        t_prev = tithi_num_at_jd(jd_rise_prev);
    }
    hd.is_adhika_tithi = (t == t_prev) ? 1 : 0;

    /* Save today's data for tomorrow's adhika check */
    cached_jd_base = jd;
    cached_jd_rise = jd_rise;
    cached_tithi = t;

    return hd;
}

void generate_month_panchang(int year, int month, const Location *loc,
                             PanchangDay *days, int *count)
{
    int ndays = days_in_month(year, month);
    *count = ndays;

    for (int d = 1; d <= ndays; d++) {
        PanchangDay *pd = &days[d - 1];
        pd->greg_year = year;
        pd->greg_month = month;
        pd->greg_day = d;

        double jd = gregorian_to_jd(year, month, d);
        pd->jd_sunrise = sunrise_jd(jd, loc);

        pd->tithi = tithi_at_sunrise(year, month, d, loc);
        pd->hindu_date = gregorian_to_hindu(year, month, d, loc);
    }
}

/* Format JD as local time HH:MM:SS given UTC offset */
static void jd_to_local_time(double jd_ut, double utc_offset,
                              int *h, int *m, int *s)
{
    /* JD is noon-based: JD .0 = noon UT, JD .5 = midnight UT.
     * Add 0.5 to shift to midnight-based, then add timezone. */
    double local_jd = jd_ut + 0.5 + utc_offset / 24.0;
    double frac = local_jd - floor(local_jd);
    double hours = frac * 24.0;
    *h = (int)hours;
    *m = (int)((hours - *h) * 60.0);
    *s = (int)(((hours - *h) * 60.0 - *m) * 60.0 + 0.5);
    if (*s == 60) { *s = 0; (*m)++; }
    if (*m == 60) { *m = 0; (*h)++; }
}

void print_month_panchang(const PanchangDay *days, int count, double utc_offset)
{
    if (count <= 0) return;

    printf("%-12s %-5s %-10s %-28s %s\n",
           "Date", "Day", "Sunrise", "Tithi", "Hindu Date");
    printf("%-12s %-5s %-10s %-28s %s\n",
           "----------", "---", "--------", "----------------------------",
           "----------------------------");

    for (int i = 0; i < count; i++) {
        const PanchangDay *pd = &days[i];
        double jd = gregorian_to_jd(pd->greg_year, pd->greg_month, pd->greg_day);
        int dow = day_of_week(jd);

        /* Sunrise time */
        int sh, sm, ss;
        jd_to_local_time(pd->jd_sunrise, utc_offset, &sh, &sm, &ss);

        /* Tithi name */
        const char *paksha_str = (pd->tithi.paksha == SHUKLA_PAKSHA) ? "Shukla" : "Krishna";
        int pt = pd->tithi.paksha_tithi;
        const char *tithi_name;
        if (pd->tithi.tithi_num == 30) {
            tithi_name = "Amavasya";
        } else if (pd->tithi.tithi_num == 15) {
            tithi_name = "Purnima";
        } else {
            tithi_name = TITHI_NAMES[pt];
        }

        /* Hindu date string */
        const char *masa_str = MASA_NAMES[pd->hindu_date.masa];
        const char *adhika_prefix = pd->hindu_date.is_adhika_masa ? "Adhika " : "";

        printf("%04d-%02d-%02d   %-5s %02d:%02d:%02d   %-6s %-13s (%s-%d)   %s%s %s %d, Saka %d\n",
               pd->greg_year, pd->greg_month, pd->greg_day,
               day_of_week_short(dow),
               sh, sm, ss,
               paksha_str, tithi_name,
               (pd->tithi.paksha == SHUKLA_PAKSHA) ? "S" : "K",
               pt,
               adhika_prefix, masa_str, paksha_str, pt,
               pd->hindu_date.year_saka);
    }
}

void print_day_panchang(const PanchangDay *day, double utc_offset)
{
    double jd = gregorian_to_jd(day->greg_year, day->greg_month, day->greg_day);
    int dow = day_of_week(jd);

    int sh, sm, ss;
    jd_to_local_time(day->jd_sunrise, utc_offset, &sh, &sm, &ss);

    const char *paksha_str = (day->tithi.paksha == SHUKLA_PAKSHA) ? "Shukla" : "Krishna";
    int pt = day->tithi.paksha_tithi;
    const char *tithi_name;
    if (day->tithi.tithi_num == 30) {
        tithi_name = "Amavasya";
    } else if (day->tithi.tithi_num == 15) {
        tithi_name = "Purnima";
    } else {
        tithi_name = TITHI_NAMES[pt];
    }
    const char *masa_str = MASA_NAMES[day->hindu_date.masa];
    const char *adhika_prefix = day->hindu_date.is_adhika_masa ? "Adhika " : "";

    printf("Date:       %04d-%02d-%02d (%s)\n",
           day->greg_year, day->greg_month, day->greg_day,
           day_of_week_name(dow));
    printf("Sunrise:    %02d:%02d:%02d IST\n", sh, sm, ss);
    printf("Tithi:      %s %s (%s-%d)\n", paksha_str, tithi_name,
           (day->tithi.paksha == SHUKLA_PAKSHA) ? "S" : "K", pt);
    printf("Hindu Date: %s%s %s %d, Saka %d (Vikram %d)\n",
           adhika_prefix, masa_str, paksha_str, pt,
           day->hindu_date.year_saka, day->hindu_date.year_vikram);
    if (day->tithi.is_kshaya)
        printf("Note:       Kshaya tithi (next tithi is skipped)\n");
    if (day->hindu_date.is_adhika_tithi)
        printf("Note:       Adhika tithi (same tithi as previous day)\n");
}
