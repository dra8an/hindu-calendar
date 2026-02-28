/* Diagnose the 16 remaining lunisolar mismatches.
 * For each date: compute sunrise, tithi at sunrise, and how close
 * the tithi boundary is to sunrise. */
#include "astro.h"
#include "date_utils.h"
#include "tithi.h"
#include <stdio.h>
#include <math.h>

static void diag(int y, int m, int d, int dp_tithi)
{
    Location delhi = DEFAULT_LOCATION;
    double jd = gregorian_to_jd(y, m, d);
    double sr = sunrise_jd(jd, &delhi);

    TithiInfo ti = tithi_at_sunrise(y, m, d, &delhi);

    /* Tithi boundary: ti.jd_start is when current tithi began,
     * ti.jd_end is when it ends. The margin is how far sunrise
     * is from the nearest boundary. */
    double margin_start = (sr - ti.jd_start) * 24.0 * 60.0;  /* minutes since tithi start */
    double margin_end = (ti.jd_end - sr) * 24.0 * 60.0;      /* minutes until tithi end */
    double nearest = (margin_start < margin_end) ? margin_start : margin_end;
    const char *which = (margin_start < margin_end) ? "after_start" : "before_end";

    /* Sunrise IST */
    double sr_ist = (sr + 5.5/24.0 + 0.5);
    sr_ist = (sr_ist - floor(sr_ist)) * 24.0;
    int hh = (int)sr_ist;
    int mm = (int)((sr_ist - hh) * 60.0);
    int ss = (int)(((sr_ist - hh) * 60.0 - mm) * 60.0);

    int diff = dp_tithi - ti.tithi_num;
    printf("%04d-%02d-%02d  sr=%02d:%02d:%02d  ours=%2d  dp=%2d  diff=%+d  "
           "margin=%.1f min %s\n",
           y, m, d, hh, mm, ss, ti.tithi_num, dp_tithi, diff,
           nearest, which);
}

int main(void)
{
    astro_init(NULL);

    printf("Date        Sunrise     Ours  DP  Diff  Margin\n");
    printf("----------- ----------- ----  --  ----  ------\n");

    diag(1929, 11, 26, 26);
    diag(1930, 10, 31, 10);
    diag(1936, 12, 29, 17);
    diag(1982,  3,  7, 12);
    diag(2001,  1, 19, 26);
    diag(2007,  8, 15,  3);
    diag(2018,  5, 19,  5);
    diag(2020, 11,  6, 21);
    diag(2026,  6, 30, 16);
    diag(2028,  3, 11, 16);
    diag(2028, 11, 13, 27);
    diag(2041, 11, 14, 22);
    diag(2045,  1, 17, 30);
    diag(2046,  5, 22, 18);
    diag(2046, 12, 21, 24);
    diag(2049, 10, 16, 21);

    astro_close();
    return 0;
}
