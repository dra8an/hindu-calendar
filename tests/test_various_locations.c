/*
 * test_various_locations.c - Validate against drikpanchang.com for multiple locations.
 *
 * Reads validation/moshier/various_locations.csv (scraped from drikpanchang.com)
 * and verifies our lunisolar tithi and solar calendar calculations for each entry.
 *
 * Locations: Ujjain, New York, Los Angeles
 * Calendars: lunisolar, tamil, bengali, odia, malayalam
 */
#include "tithi.h"
#include "masa.h"
#include "solar.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static SolarCalendarType parse_solar_type(const char *cal)
{
    if (strcmp(cal, "tamil") == 0) return SOLAR_CAL_TAMIL;
    if (strcmp(cal, "bengali") == 0) return SOLAR_CAL_BENGALI;
    if (strcmp(cal, "odia") == 0) return SOLAR_CAL_ODIA;
    if (strcmp(cal, "malayalam") == 0) return SOLAR_CAL_MALAYALAM;
    return -1;
}

int main(void)
{
    astro_init(NULL);

    const char *csv_path = "validation/moshier/various_locations.csv";
    FILE *f = fopen(csv_path, "r");
    if (!f) {
        printf("Cannot open %s\n", csv_path);
        return 1;
    }

    char line[512];
    /* Skip header */
    if (fgets(line, sizeof(line), f) == NULL) { fclose(f); return 1; }

    while (fgets(line, sizeof(line), f)) {
        char calendar[32], location[32];
        double lat, lon, utc_offset;
        int gy, gm, gd;
        char tithi_str[16] = "", sol_month_str[16] = "";
        char sol_day_str[16] = "", sol_year_str[16] = "";

        /* Parse CSV: split by commas manually to handle empty fields */
        char *fields[12];
        char line_copy[512];
        strncpy(line_copy, line, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';
        /* Strip trailing newline */
        char *nl = strchr(line_copy, '\n');
        if (nl) *nl = '\0';

        int nf = 0;
        char *p = line_copy;
        while (nf < 12 && p) {
            fields[nf++] = p;
            p = strchr(p, ',');
            if (p) *p++ = '\0';
        }
        if (nf < 8) continue;

        strncpy(calendar, fields[0], sizeof(calendar) - 1);
        strncpy(location, fields[1], sizeof(location) - 1);
        lat = atof(fields[2]);
        lon = atof(fields[3]);
        utc_offset = atof(fields[4]);
        gy = atoi(fields[5]);
        gm = atoi(fields[6]);
        gd = atoi(fields[7]);
        if (nf > 8) strncpy(tithi_str, fields[8], sizeof(tithi_str) - 1);
        if (nf > 9) strncpy(sol_month_str, fields[9], sizeof(sol_month_str) - 1);
        if (nf > 10) strncpy(sol_day_str, fields[10], sizeof(sol_day_str) - 1);
        if (nf > 11) strncpy(sol_year_str, fields[11], sizeof(sol_year_str) - 1);

        Location loc = {lat, lon, 0.0, utc_offset};

        if (strcmp(calendar, "lunisolar") == 0) {
            /* Lunisolar: verify tithi */
            int expected_tithi = atoi(tithi_str);
            if (expected_tithi == 0) continue;

            double jd = gregorian_to_jd(gy, gm, gd);
            double jd_rise = sunrise_jd(jd, &loc);
            if (jd_rise <= 0)
                jd_rise = jd + 0.5 - utc_offset / 24.0;
            int actual_tithi = tithi_at_moment(jd_rise);

            tests_run++;
            if (actual_tithi == expected_tithi) {
                tests_passed++;
            } else {
                tests_failed++;
                printf("  FAIL: lunisolar %s %04d-%02d-%02d: "
                       "tithi expected %d, got %d\n",
                       location, gy, gm, gd,
                       expected_tithi, actual_tithi);
            }
        } else {
            /* Solar calendar: verify month, day, year */
            int expected_month = atoi(sol_month_str);
            int expected_day = atoi(sol_day_str);
            int expected_year = atoi(sol_year_str);
            if (expected_month == 0) continue;

            SolarCalendarType type = parse_solar_type(calendar);
            if ((int)type < 0) continue;

            SolarDate sd = gregorian_to_solar(gy, gm, gd, &loc, type);

            tests_run++;
            if (sd.month == expected_month &&
                sd.day == expected_day &&
                sd.year == expected_year) {
                tests_passed++;
            } else {
                tests_failed++;
                printf("  FAIL: %s %s %04d-%02d-%02d: "
                       "expected m%d d%d y%d, got m%d d%d y%d\n",
                       calendar, location, gy, gm, gd,
                       expected_month, expected_day, expected_year,
                       sd.month, sd.day, sd.year);
            }
        }
    }

    fclose(f);
    astro_close();

    printf("\n=== Various locations: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return (tests_failed > 0) ? 1 : 0;
}
