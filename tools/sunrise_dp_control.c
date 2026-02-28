/* Compare our sunrise times against drikpanchang for NON-mismatch control dates */
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <math.h>

static void print_sunrise(int y, int m, int d, const char *dp_sunrise)
{
    Location delhi = DEFAULT_LOCATION;
    double jd = gregorian_to_jd(y, m, d);
    double sr = sunrise_jd(jd, &delhi);

    double sr_ist = sr + 5.5 / 24.0;
    double frac = (sr_ist + 0.5) - floor(sr_ist + 0.5);
    double hours = frac * 24.0;
    int hh = (int)hours;
    int mm = (int)((hours - hh) * 60.0);
    int ss = (int)(((hours - hh) * 60.0 - mm) * 60.0);

    int dp_hh = 0, dp_mm = 0;
    sscanf(dp_sunrise, "%d:%d", &dp_hh, &dp_mm);

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

    print_sunrise(1920, 6, 15, "05:23");
    print_sunrise(1940, 3, 15, "06:31");
    print_sunrise(1950, 9, 15, "06:05");
    print_sunrise(1960, 1, 15, "07:15");
    print_sunrise(1970, 7, 15, "05:33");
    print_sunrise(1980, 4, 15, "05:56");
    print_sunrise(1990, 11, 15, "06:43");
    print_sunrise(2000, 2, 15, "07:00");
    print_sunrise(2010, 8, 15, "05:50");
    print_sunrise(2020, 5, 15, "05:30");
    print_sunrise(2025, 1, 15, "07:15");
    print_sunrise(2025, 6, 15, "05:23");
    print_sunrise(2025, 12, 15, "07:06");
    print_sunrise(2030, 3, 15, "06:31");
    print_sunrise(2040, 10, 15, "06:22");

    astro_close();
    return 0;
}
