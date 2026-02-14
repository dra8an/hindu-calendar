/*
 * Compute corrected expected values for wrong edge case entries.
 * For each entry where drikpanchang shows "last day of previous month"
 * instead of "day 1 of new month", compute what the correct expected
 * month/day/year should be.
 */
#include "solar.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>

int main(void)
{
    astro_init(NULL);
    Location loc = DEFAULT_LOCATION;

    struct { int gy, gm, gd; SolarCalendarType type; const char *enum_str; } wrong[] = {
        /* Tamil — 6 entries with delta 0 to -7.7 */
        {1962,  2, 12, SOLAR_CAL_TAMIL,     "SOLAR_CAL_TAMIL,"},
        {1969,  4, 13, SOLAR_CAL_TAMIL,     "SOLAR_CAL_TAMIL,"},
        {1971, 11, 16, SOLAR_CAL_TAMIL,     "SOLAR_CAL_TAMIL,"},
        {2032,  5, 14, SOLAR_CAL_TAMIL,     "SOLAR_CAL_TAMIL,"},
        {1923,  2, 12, SOLAR_CAL_TAMIL,     "SOLAR_CAL_TAMIL,"},
        {1964,  1, 14, SOLAR_CAL_TAMIL,     "SOLAR_CAL_TAMIL,"},
        /* Malayalam — 15 entries with delta 0 to -9.3 */
        {1913,  6, 14, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {2021, 11, 16, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1982, 11, 16, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1943, 11, 16, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1976,  4, 13, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1934,  2, 12, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1996,  5, 14, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1904, 11, 15, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1915,  3, 14, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1901,  1, 13, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {2032,  3, 14, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1937,  4, 13, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {2023,  8, 17, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1973,  2, 12, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
        {1952,  6, 14, SOLAR_CAL_MALAYALAM, "SOLAR_CAL_MALAYALAM,"},
    };
    int n = sizeof(wrong) / sizeof(wrong[0]);

    for (int i = 0; i < n; i++) {
        int gy = wrong[i].gy, gm = wrong[i].gm, gd = wrong[i].gd;
        SolarCalendarType type = wrong[i].type;

        /* Our current (wrong) prediction */
        SolarDate cur = gregorian_to_solar(gy, gm, gd, &loc, type);

        /* Previous day — should be in the previous month, well within it */
        double jd_prev = gregorian_to_jd(gy, gm, gd) - 1.0;
        int py, pm, pd;
        jd_to_gregorian(jd_prev, &py, &pm, &pd);
        SolarDate prev = gregorian_to_solar(py, pm, pd, &loc, type);

        /* Corrected: same month as prev day, day = prev.day + 1 */
        printf("    {%04d, %2d, %2d, %-22s %2d, %2d, %4d},  "
               "/* CORRECTED: was month %d day %d year %d; "
               "drikpanchang: %s %d, %d %s */\n",
               gy, gm, gd, wrong[i].enum_str,
               prev.month, prev.day + 1, prev.year,
               cur.month, cur.day, cur.year,
               solar_month_name(prev.month, type), prev.day + 1, prev.year,
               solar_era_name(type));
    }

    astro_close();
    return 0;
}
