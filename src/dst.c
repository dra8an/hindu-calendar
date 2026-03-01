#include "dst.h"

/* Day of week for Gregorian date: 0=Sun, 1=Mon, ..., 6=Sat (Zeller-like) */
static int dow(int y, int m, int d)
{
    /* Tomohiko Sakamoto's algorithm */
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y--;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

/* Find the nth occurrence of weekday 'wday' (0=Sun) in month m of year y.
 * nth > 0: 1st, 2nd, ... occurrence.  nth == -1: last occurrence. */
static int nth_weekday(int y, int m, int nth, int wday)
{
    if (nth > 0) {
        /* First day of month */
        int d1_dow = dow(y, m, 1);
        int d = 1 + ((wday - d1_dow + 7) % 7) + (nth - 1) * 7;
        return d;
    }
    /* Last occurrence: find last day's dow, work backwards */
    static const int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    int last = mdays[m];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        last = 29;
    int last_dow = dow(y, m, last);
    int d = last - ((last_dow - wday + 7) % 7);
    return d;
}

/* Day-of-year for Gregorian date (1-based) */
static int day_of_year(int y, int m, int d)
{
    static const int cum[] = {0, 0,31,59,90,120,151,181,212,243,273,304,334};
    int doy = cum[m] + d;
    if (m > 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        doy++;
    return doy;
}

/* Convert month/day to day-of-year for comparison */
static int md_to_doy(int y, int m, int d) { return day_of_year(y, m, d); }

/* Check if date (m,d) is within DST period [start_m/start_d, end_m/end_d).
 * DST starts at 2:00 AM on start date (spring forward).
 * DST ends at 2:00 AM on end date (fall back).
 * We treat the entire start date as DST and the entire end date as non-DST,
 * since sunrise is always after 2:00 AM. */
static int in_dst(int y, int m, int d, int sm, int sd, int em, int ed)
{
    int doy = md_to_doy(y, m, d);
    int start = md_to_doy(y, sm, sd);
    int end = md_to_doy(y, em, ed);
    return doy >= start && doy < end;
}

double us_eastern_offset(int y, int m, int d)
{
    /* 1900-1917: No DST */
    if (y < 1918) return -5.0;

    /* 1918-1919: Last Sun March - Last Sun October */
    if (y <= 1919) {
        int sm = 3, sd = nth_weekday(y, 3, -1, 0);
        int em = 10, ed = nth_weekday(y, 10, -1, 0);
        return in_dst(y, m, d, sm, sd, em, ed) ? -4.0 : -5.0;
    }

    /* 1920-1941: No federal DST (treat as EST) */
    if (y <= 1941) return -5.0;

    /* 1942-Feb-09 to 1945-Sep-30: Year-round War Time (EDT) */
    if (y >= 1942 && y <= 1945) {
        if (y == 1942) {
            /* War Time started Feb 9, 1942 */
            return (m > 2 || (m == 2 && d >= 9)) ? -4.0 : -5.0;
        }
        if (y <= 1944) return -4.0;
        /* 1945: War Time ended Sep 30 */
        return (m < 10 || (m == 9 && d <= 30)) ? -4.0 : -5.0;
    }

    /* 1946-1966: Last Sun April - Last Sun September */
    if (y <= 1966) {
        int sd = nth_weekday(y, 4, -1, 0);
        int ed = nth_weekday(y, 9, -1, 0);
        return in_dst(y, m, d, 4, sd, 9, ed) ? -4.0 : -5.0;
    }

    /* 1967-1973: Last Sun April - Last Sun October */
    if (y <= 1973) {
        int sd = nth_weekday(y, 4, -1, 0);
        int ed = nth_weekday(y, 10, -1, 0);
        return in_dst(y, m, d, 4, sd, 10, ed) ? -4.0 : -5.0;
    }

    /* 1974: Jan 6 - Last Sun October (energy crisis) */
    if (y == 1974) {
        int ed = nth_weekday(y, 10, -1, 0);
        return in_dst(y, m, d, 1, 6, 10, ed) ? -4.0 : -5.0;
    }

    /* 1975: Last Sun Feb - Last Sun October */
    if (y == 1975) {
        int sd = nth_weekday(y, 2, -1, 0);
        int ed = nth_weekday(y, 10, -1, 0);
        return in_dst(y, m, d, 2, sd, 10, ed) ? -4.0 : -5.0;
    }

    /* 1976-1986: Last Sun April - Last Sun October */
    if (y <= 1986) {
        int sd = nth_weekday(y, 4, -1, 0);
        int ed = nth_weekday(y, 10, -1, 0);
        return in_dst(y, m, d, 4, sd, 10, ed) ? -4.0 : -5.0;
    }

    /* 1987-2006: First Sun April - Last Sun October */
    if (y <= 2006) {
        int sd = nth_weekday(y, 4, 1, 0);
        int ed = nth_weekday(y, 10, -1, 0);
        return in_dst(y, m, d, 4, sd, 10, ed) ? -4.0 : -5.0;
    }

    /* 2007-2050: Second Sun March - First Sun November */
    {
        int sd = nth_weekday(y, 3, 2, 0);
        int ed = nth_weekday(y, 11, 1, 0);
        return in_dst(y, m, d, 3, sd, 11, ed) ? -4.0 : -5.0;
    }
}
