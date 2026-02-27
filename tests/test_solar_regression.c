#include "solar.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Solar calendar CSV regression test.
 * Reads the generated month-boundary CSVs and verifies our code still
 * produces the same results.  For each month row we check:
 *   1. First day: month, year, day==1
 *   2. Last day:  day == expected length
 */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static void check(int condition, const char *msg)
{
    tests_run++;
    if (condition) {
        tests_passed++;
    } else {
        tests_failed++;
        printf("  FAIL: %s\n", msg);
    }
}

static const char *find_csv(const char *base)
{
    /* Try common locations */
    static char path[512];
    const char *prefixes[] = {
#ifdef USE_SWISSEPH
        "validation/se/solar/", "../validation/se/solar/",
#else
        "validation/moshier/solar/", "../validation/moshier/solar/",
#endif
        NULL
    };
    for (int i = 0; prefixes[i]; i++) {
        snprintf(path, sizeof(path), "%s%s", prefixes[i], base);
        FILE *f = fopen(path, "r");
        if (f) { fclose(f); return path; }
    }
    return NULL;
}

/* Days in a Gregorian month */
static int days_in_gmonth(int year, int month)
{
    static const int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
            return 29;
    }
    return mdays[month];
}

/* Advance a Gregorian date by n days */
static void advance_greg(int *y, int *m, int *d, int n)
{
    for (int i = 0; i < n; i++) {
        (*d)++;
        if (*d > days_in_gmonth(*y, *m)) {
            *d = 1;
            (*m)++;
            if (*m > 12) {
                *m = 1;
                (*y)++;
            }
        }
    }
}

static void test_calendar(SolarCalendarType type, const char *base,
                          const char *cal_name)
{
    const char *csv_path = find_csv(base);
    if (!csv_path) {
        printf("SKIP: %s not found\n", base);
        return;
    }

    FILE *f = fopen(csv_path, "r");
    if (!f) {
        printf("ERROR: cannot open %s\n", csv_path);
        return;
    }

    Location loc = DEFAULT_LOCATION;
    char line[256];
    int months_checked = 0;

    /* Skip header */
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return;
    }

    while (fgets(line, sizeof(line), f)) {
        int exp_month, exp_year, exp_length, gy, gm, gd;
        char month_name[64];

        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%63s",
                   &exp_month, &exp_year, &exp_length,
                   &gy, &gm, &gd, month_name) != 7)
            continue;

        char buf[256];

        /* Check first day of month: should be day 1 with correct month/year */
        SolarDate sd = gregorian_to_solar(gy, gm, gd, &loc, type);

        snprintf(buf, sizeof(buf),
                 "%s %04d-%02d-%02d: month (got %d, expected %d)",
                 cal_name, gy, gm, gd, sd.month, exp_month);
        check(sd.month == exp_month, buf);

        snprintf(buf, sizeof(buf),
                 "%s %04d-%02d-%02d: year (got %d, expected %d)",
                 cal_name, gy, gm, gd, sd.year, exp_year);
        check(sd.year == exp_year, buf);

        snprintf(buf, sizeof(buf),
                 "%s %04d-%02d-%02d: day==1 (got %d)",
                 cal_name, gy, gm, gd, sd.day);
        check(sd.day == 1, buf);

        /* Check last day of month: advance (length-1) days, verify day==length */
        int ly = gy, lm = gm, ld = gd;
        advance_greg(&ly, &lm, &ld, exp_length - 1);

        SolarDate sd_last = gregorian_to_solar(ly, lm, ld, &loc, type);

        snprintf(buf, sizeof(buf),
                 "%s %04d-%02d-%02d: last day (got %d, expected %d)",
                 cal_name, ly, lm, ld, sd_last.day, exp_length);
        check(sd_last.day == exp_length, buf);

        months_checked++;
    }

    fclose(f);
    printf("  %s: checked %d months\n", cal_name, months_checked);
}

int main(void)
{
    astro_init(NULL);

    printf("\n--- Solar calendar CSV regression ---\n");

    test_calendar(SOLAR_CAL_TAMIL,
                  "tamil_months_1900_2050.csv", "Tamil");
    test_calendar(SOLAR_CAL_BENGALI,
                  "bengali_months_1900_2050.csv", "Bengali");
    test_calendar(SOLAR_CAL_ODIA,
                  "odia_months_1900_2050.csv", "Odia");
    test_calendar(SOLAR_CAL_MALAYALAM,
                  "malayalam_months_1900_2050.csv", "Malayalam");

    astro_close();

    printf("\n=== Solar Regression: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}
