/*
 * test_surya_bengali.c - Bengali Suryasiddhanta panjika regression.
 *
 * Verifies SOLAR_CAL_BENGALI_SURYA against the drikpanchang.com scrape in
 * validation/drikpanchang/bengali_suryasiddhanta.csv: 1,812 month starts
 * covering 1900-2050.
 *
 * Unlike the other solar regressions, this reference is an EXTERNAL source
 * (drikpanchang's published values) rather than our own generated CSV, so a
 * failure here means we disagree with the reference implementation, not merely
 * that our own output moved.
 *
 * Exercises the full public path -- solar_month_start() and
 * gregorian_to_solar() -- rather than the engine in isolation.
 *
 * See Docs/SURYASIDDHANTA_PANJIKA.md.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "astro.h"
#include "date_utils.h"
#include "solar.h"
#include "types.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

static void check(int cond, const char *fmt, ...)
{
    tests_run++;
    if (cond) {
        tests_passed++;
    } else {
        tests_failed++;
        if (tests_failed <= 20) {
            va_list ap;
            va_start(ap, fmt);
            printf("  FAIL: ");
            vprintf(fmt, ap);
            printf("\n");
            va_end(ap);
        }
    }
}

int main(void)
{
    const char *paths[] = {
        "validation/drikpanchang/bengali_suryasiddhanta.csv",
        "../validation/drikpanchang/bengali_suryasiddhanta.csv",
    };
    FILE *f = NULL;
    char line[512];
    Location loc = DEFAULT_LOCATION;
    int rows = 0;

    for (size_t i = 0; i < sizeof paths / sizeof paths[0] && !f; i++)
        f = fopen(paths[i], "r");

    if (!f) {
        printf("SKIP: bengali_suryasiddhanta.csv not found\n");
        return 0;
    }

    astro_init(NULL);
    printf("\n--- Bengali Suryasiddhanta vs drikpanchang (1900-2050) ---\n");

    if (!fgets(line, sizeof line, f)) { fclose(f); astro_close(); return 1; }

    while (fgets(line, sizeof line, f)) {
        int mo, yr, len, gy, gm, gd;
        double jd_start;
        int sy, sm, sd;

        if (sscanf(line, "%d,%d,%d,%d,%d,%d", &mo, &yr, &len, &gy, &gm, &gd) != 6)
            continue;
        rows++;

        /* Month start must land on the drikpanchang date. */
        jd_start = solar_month_start(mo, yr, SOLAR_CAL_BENGALI_SURYA, &loc);
        jd_to_gregorian(jd_start, &sy, &sm, &sd);
        check(sy == gy && sm == gm && sd == gd,
              "month %d year %d: expected %04d-%02d-%02d, got %04d-%02d-%02d",
              mo, yr, gy, gm, gd, sy, sm, sd);

        /* And that date must itself be day 1 of that month. */
        SolarDate got = gregorian_to_solar(gy, gm, gd, &loc,
                                           SOLAR_CAL_BENGALI_SURYA);
        check(got.day == 1 && got.month == mo && got.year == yr,
              "%04d-%02d-%02d: expected month %d day 1 year %d, got month %d day %d year %d",
              gy, gm, gd, mo, yr, got.month, got.day, got.year);
    }
    fclose(f);

    printf("\n=== Surya Bengali: %d/%d passed, %d failed (%d months) ===\n",
           tests_passed, tests_run, tests_failed, rows);

    astro_close();
    return tests_failed ? 1 : 0;
}
