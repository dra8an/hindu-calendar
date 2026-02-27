#include "tithi.h"
#include "masa.h"
#include "astro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * CSV regression test: loads ref_1900_2050.csv and verifies our calculator
 * produces the same output. Tests every SAMPLE_STEP-th day for speed.
 * Set SAMPLE_STEP=1 to test all 55K days (slow, ~45 min).
 */
#define SAMPLE_STEP 50  /* test ~1100 days out of 55K */

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

static const char *find_csv(void)
{
    /* Try common locations relative to where the test might run */
    static const char *paths[] = {
#ifdef USE_SWISSEPH
        "validation/se/ref_1900_2050.csv",
        "../validation/se/ref_1900_2050.csv",
#else
        "validation/moshier/ref_1900_2050.csv",
        "../validation/moshier/ref_1900_2050.csv",
#endif
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        FILE *f = fopen(paths[i], "r");
        if (f) { fclose(f); return paths[i]; }
    }
    return NULL;
}

int main(void)
{
    const char *csv_path = find_csv();
    if (!csv_path) {
        printf("SKIP: ref_1900_2050.csv not found\n");
        return 0;
    }

    FILE *f = fopen(csv_path, "r");
    if (!f) {
        printf("ERROR: cannot open %s\n", csv_path);
        return 1;
    }

    astro_init(NULL);
    Location delhi = DEFAULT_LOCATION;

    char line[256];
    int line_num = 0;
    int sampled = 0;

    /* Skip header */
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        astro_close();
        return 1;
    }

    while (fgets(line, sizeof(line), f)) {
        line_num++;
        if ((line_num - 1) % SAMPLE_STEP != 0)
            continue;

        int y, m, d, tithi, masa, adhika, saka;
        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%d", &y, &m, &d, &tithi, &masa, &adhika, &saka) != 7)
            continue;

        TithiInfo ti = tithi_at_sunrise(y, m, d, &delhi);
        MasaInfo mi = masa_for_date(y, m, d, &delhi);

        char buf[256];

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d tithi (got %d, expected %d)",
                 y, m, d, ti.tithi_num, tithi);
        check(ti.tithi_num == tithi, buf);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d masa (got %d, expected %d)",
                 y, m, d, (int)mi.name, masa);
        check((int)mi.name == masa, buf);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d adhika (got %d, expected %d)",
                 y, m, d, mi.is_adhika, adhika);
        check(mi.is_adhika == adhika, buf);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d saka (got %d, expected %d)",
                 y, m, d, mi.year_saka, saka);
        check(mi.year_saka == saka, buf);

        sampled++;
    }

    fclose(f);
    astro_close();

    printf("\n=== CSV Regression: %d/%d passed, %d failed (%d days sampled from CSV) ===\n",
           tests_passed, tests_run, tests_failed, sampled);
    return (tests_failed == 0) ? 0 : 1;
}
