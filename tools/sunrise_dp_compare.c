/* Compare our sunrise times against drikpanchang for the 35 mismatch dates */
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <math.h>

static void print_sunrise(int y, int m, int d, const char *dp_sunrise)
{
    Location delhi = DEFAULT_LOCATION;
    double jd = gregorian_to_jd(y, m, d);
    double sr = sunrise_jd(jd, &delhi);

    /* Convert to IST */
    double sr_ist = sr + 5.5 / 24.0;
    double frac = (sr_ist + 0.5) - floor(sr_ist + 0.5);
    double hours = frac * 24.0;
    int hh = (int)hours;
    int mm = (int)((hours - hh) * 60.0);
    int ss = (int)(((hours - hh) * 60.0 - mm) * 60.0);

    /* Parse drikpanchang sunrise */
    int dp_hh = 0, dp_mm = 0;
    sscanf(dp_sunrise, "%d:%d", &dp_hh, &dp_mm);

    /* Difference in seconds (our - dp). dp has no seconds so compare at minute level */
    double our_minutes = hh * 60.0 + mm + ss / 60.0;
    double dp_minutes = dp_hh * 60.0 + dp_mm;
    double diff_sec = (our_minutes - dp_minutes) * 60.0;

    printf("%04d-%02d-%02d  ours=%02d:%02d:%02d  dp=%s  diff=%+.0fs\n",
           y, m, d, hh, mm, ss, dp_sunrise, diff_sec);
}

int main(void)
{
    astro_init(NULL);

    printf("Date        Ours        DP     Diff (ours-dp)\n");
    printf("----------- ----------- ------ --------------\n");

    print_sunrise(1902, 5, 30, "05:15");
    print_sunrise(1903, 5, 18, "05:20");
    print_sunrise(1908, 3, 17, "06:29");
    print_sunrise(1909, 10, 11, "06:19");
    print_sunrise(1909, 12, 1, "06:56");
    print_sunrise(1911, 8, 26, "05:55");
    print_sunrise(1912, 12, 14, "07:06");
    print_sunrise(1915, 12, 5, "06:59");
    print_sunrise(1916, 2, 24, "06:53");
    print_sunrise(1920, 10, 12, "06:20");
    print_sunrise(1924, 2, 5, "07:08");
    print_sunrise(1925, 3, 3, "06:45");
    print_sunrise(1932, 5, 15, "05:30");
    print_sunrise(1939, 7, 23, "05:37");
    print_sunrise(1940, 2, 3, "07:09");
    print_sunrise(1943, 12, 17, "08:07");
    print_sunrise(1946, 1, 29, "07:11");
    print_sunrise(1951, 6, 8, "05:23");
    print_sunrise(1956, 5, 29, "05:24");
    print_sunrise(1957, 8, 28, "05:57");
    print_sunrise(1965, 5, 6, "05:37");
    print_sunrise(1966, 1, 8, "07:15");
    print_sunrise(1966, 8, 9, "05:46");
    print_sunrise(1966, 10, 25, "06:28");
    print_sunrise(1968, 3, 11, "06:36");
    print_sunrise(1968, 5, 24, "05:26");
    print_sunrise(1972, 4, 1, "06:11");
    print_sunrise(1974, 12, 19, "07:08");
    print_sunrise(1978, 9, 15, "06:06");
    print_sunrise(1982, 3, 7, "06:40");
    print_sunrise(1987, 12, 18, "07:08");
    print_sunrise(2007, 10, 9, "06:18");
    print_sunrise(2014, 5, 22, "05:27");
    print_sunrise(2045, 1, 17, "07:15");
    print_sunrise(2046, 5, 22, "05:27");

    astro_close();
    return 0;
}
