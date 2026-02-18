#include "tithi.h"
#include "masa.h"
#include "astro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Tests all adhika (repeated) and kshaya (skipped) tithi days from
 * adhika_kshaya_tithis.csv — 4,269 edge-case days across 1900-2050.
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

static const char *find_csv(void)
{
    static const char *paths[] = {
        "validation/se/adhika_kshaya_tithis.csv",
        "../validation/se/adhika_kshaya_tithis.csv",
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
        printf("SKIP: adhika_kshaya_tithis.csv not found\n");
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
    int count = 0;

    /* Skip header */
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        astro_close();
        return 1;
    }

    while (fgets(line, sizeof(line), f)) {
        int y, m, d, tithi, masa, adhika, saka;
        char type[16];
        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%d,%15s",
                   &y, &m, &d, &tithi, &masa, &adhika, &saka, type) != 8)
            continue;

        TithiInfo ti = tithi_at_sunrise(y, m, d, &delhi);
        MasaInfo mi = masa_for_date(y, m, d, &delhi);

        char buf[256];

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d [%s] tithi (got %d, exp %d)",
                 y, m, d, type, ti.tithi_num, tithi);
        check(ti.tithi_num == tithi, buf);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d [%s] masa (got %d, exp %d)",
                 y, m, d, type, (int)mi.name, masa);
        check((int)mi.name == masa, buf);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d [%s] adhika (got %d, exp %d)",
                 y, m, d, type, mi.is_adhika, adhika);
        check(mi.is_adhika == adhika, buf);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d [%s] saka (got %d, exp %d)",
                 y, m, d, type, mi.year_saka, saka);
        check(mi.year_saka == saka, buf);

        count++;
    }

    fclose(f);
    astro_close();

    printf("\n=== Adhika/Kshaya Tithi: %d/%d passed, %d failed (%d days) ===\n",
           tests_passed, tests_run, tests_failed, count);
    return (tests_failed == 0) ? 0 : 1;
}
