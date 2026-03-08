#include "masa.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_EQ(actual, expected, msg) do { \
    tests_run++; \
    int _a = (actual), _e = (expected); \
    if (_a == _e) { \
        tests_passed++; \
    } else { \
        printf("  FAIL: %s (got %d, expected %d)\n", msg, _a, _e); \
    } \
} while (0)

#define ASSERT_JD(jd, ey, em, ed, msg) do { \
    tests_run++; \
    int _y, _m, _d; \
    jd_to_gregorian(jd, &_y, &_m, &_d); \
    if (_y == ey && _m == em && _d == ed) { \
        tests_passed++; \
    } else { \
        printf("  FAIL: %s (got %04d-%02d-%02d, expected %04d-%02d-%02d)\n", \
               msg, _y, _m, _d, ey, em, ed); \
    } \
} while (0)

/*
 * Spot checks: known month starts verified against drikpanchang.com
 */
static void test_known_month_starts(void)
{
    printf("\n--- Known lunisolar month starts ---\n");
    Location delhi = DEFAULT_LOCATION;

    struct {
        MasaName masa;
        int saka_year;
        int is_adhika;
        int exp_y, exp_m, exp_d;
        const char *label;
    } cases[] = {
        /* 2025 months */
        {CHAITRA,      1947, 0,  2025,  3, 30, "Chaitra 1947"},
        {VAISHAKHA,    1947, 0,  2025,  4, 28, "Vaishakha 1947"},
        {JYESHTHA,     1947, 0,  2025,  5, 28, "Jyeshtha 1947"},
        {ASHADHA,      1947, 0,  2025,  6, 26, "Ashadha 1947"},
        {SHRAVANA,     1947, 0,  2025,  7, 25, "Shravana 1947"},
        {BHADRAPADA,   1947, 0,  2025,  8, 24, "Bhadrapada 1947"},
        {ASHVINA,      1947, 0,  2025,  9, 22, "Ashvina 1947"},
        {KARTIKA,      1947, 0,  2025, 10, 22, "Kartika 1947"},
        {MARGASHIRSHA,  1947, 0, 2025, 11, 21, "Margashirsha 1947"},
        {PAUSHA,       1947, 0,  2025, 12, 21, "Pausha 1947"},

        /* Adhika Bhadrapada 2012 (Saka 1934) */
        {BHADRAPADA,   1934, 1,  2012,  8, 18, "Adhika Bhadrapada 1934"},
        {BHADRAPADA,   1934, 0,  2012,  9, 17, "Nija Bhadrapada 1934"},

        /* Adhika Ashadha 2015 (Saka 1937) */
        {ASHADHA,      1937, 1,  2015,  6, 17, "Adhika Ashadha 1937"},
        {ASHADHA,      1937, 0,  2015,  7, 17, "Nija Ashadha 1937"},

        /* Year boundary: Phalguna → Chaitra */
        {PHALGUNA,     1946, 0,  2025,  2, 28, "Phalguna 1946"},
        {CHAITRA,      1946, 0,  2024,  4, 9,  "Chaitra 1946"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        double jd = lunisolar_month_start(cases[i].masa, cases[i].saka_year,
                                          cases[i].is_adhika, &delhi);
        char buf[128];
        snprintf(buf, sizeof(buf), "%s start", cases[i].label);
        ASSERT_JD(jd, cases[i].exp_y, cases[i].exp_m, cases[i].exp_d, buf);

        /* Verify masa_for_date confirms this date */
        if (jd > 0) {
            int gy, gm, gd;
            jd_to_gregorian(jd, &gy, &gm, &gd);
            MasaInfo mi = masa_for_date(gy, gm, gd, &delhi);
            snprintf(buf, sizeof(buf), "%s masa roundtrip", cases[i].label);
            ASSERT_EQ(mi.name, cases[i].masa, buf);
            snprintf(buf, sizeof(buf), "%s adhika roundtrip", cases[i].label);
            ASSERT_EQ(mi.is_adhika, cases[i].is_adhika, buf);
            snprintf(buf, sizeof(buf), "%s saka roundtrip", cases[i].label);
            ASSERT_EQ(mi.year_saka, cases[i].saka_year, buf);

            /* Verify day before belongs to previous month */
            double jd_prev = jd - 1;
            jd_to_gregorian(jd_prev, &gy, &gm, &gd);
            MasaInfo mi_prev = masa_for_date(gy, gm, gd, &delhi);
            snprintf(buf, sizeof(buf), "%s prev day different", cases[i].label);
            tests_run++;
            if (mi_prev.name != cases[i].masa || mi_prev.is_adhika != cases[i].is_adhika) {
                tests_passed++;
            } else {
                printf("  FAIL: %s (prev day has same masa)\n", buf);
            }
        }
    }
    printf("  Spot checks done: %d/%d passed\n", tests_passed, tests_run);
}

/*
 * Month length checks
 */
static void test_month_lengths(void)
{
    printf("\n--- Month lengths ---\n");
    Location delhi = DEFAULT_LOCATION;
    int saved_run = tests_run, saved_pass = tests_passed;

    /* Check a full year of month lengths */
    MasaName months[] = {CHAITRA, VAISHAKHA, JYESHTHA, ASHADHA, SHRAVANA,
                         BHADRAPADA, ASHVINA, KARTIKA, MARGASHIRSHA, PAUSHA,
                         MAGHA, PHALGUNA};
    for (int i = 0; i < 12; i++) {
        int len = lunisolar_month_length(months[i], 1947, 0, &delhi);
        char buf[64];
        snprintf(buf, sizeof(buf), "%s 1947 length in [29,30]", MASA_NAMES[months[i]]);
        tests_run++;
        if (len == 29 || len == 30) {
            tests_passed++;
        } else {
            printf("  FAIL: %s (got %d)\n", buf, len);
        }
    }

    /* Adhika month length */
    int len = lunisolar_month_length(BHADRAPADA, 1934, 1, &delhi);
    tests_run++;
    if (len == 29 || len == 30) {
        tests_passed++;
    } else {
        printf("  FAIL: Adhika Bhadrapada 1934 length (got %d)\n", len);
    }

    /* Verify start + length = next month start */
    double jd_start = lunisolar_month_start(VAISHAKHA, 1947, 0, &delhi);
    len = lunisolar_month_length(VAISHAKHA, 1947, 0, &delhi);
    double jd_next = lunisolar_month_start(JYESHTHA, 1947, 0, &delhi);
    ASSERT_EQ((int)(jd_next - jd_start), len, "Vaishakha+length = Jyeshtha start");

    printf("  Length checks: %d/%d passed\n",
           tests_passed - saved_pass, tests_run - saved_run);
}

/*
 * Roundtrip test: for a range of years, verify every month roundtrips.
 */
static void test_roundtrip(void)
{
    printf("\n--- Roundtrip test (1900-2050) ---\n");
    Location delhi = DEFAULT_LOCATION;
    int saved_run = tests_run, saved_pass = tests_passed;
    int months_tested = 0;

    /* Walk through every day, track masa transitions.
     * Skip the first month since we don't know its actual start. */
    int prev_masa = -1, prev_adhika = -1, prev_saka = -1;
    int first_transition = 1;

    for (int y = 1900; y <= 2050; y++) {
        for (int m = 1; m <= 12; m++) {
            int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
            int dim = mdays[m];
            if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
                dim = 29;
            for (int d = 1; d <= dim; d++) {
                MasaInfo mi = masa_for_date(y, m, d, &delhi);
                if ((int)mi.name != prev_masa || mi.is_adhika != prev_adhika ||
                    mi.year_saka != prev_saka) {
                    /* Skip the first month — we started mid-month */
                    if (first_transition) {
                        first_transition = 0;
                        prev_masa = mi.name;
                        prev_adhika = mi.is_adhika;
                        prev_saka = mi.year_saka;
                        continue;
                    }
                    /* New month started — verify lunisolar_month_start */
                    double jd = lunisolar_month_start(mi.name, mi.year_saka,
                                                      mi.is_adhika, &delhi);
                    tests_run++;
                    if (jd > 0) {
                        int ry, rm, rd;
                        jd_to_gregorian(jd, &ry, &rm, &rd);
                        if (ry == y && rm == m && rd == d) {
                            tests_passed++;
                        } else {
                            printf("  FAIL: %s%s %d start: expected %04d-%02d-%02d, got %04d-%02d-%02d\n",
                                   mi.is_adhika ? "Adhika " : "",
                                   MASA_NAMES[mi.name], mi.year_saka,
                                   y, m, d, ry, rm, rd);
                        }
                    } else {
                        printf("  FAIL: %s%s %d not found\n",
                               mi.is_adhika ? "Adhika " : "",
                               MASA_NAMES[mi.name], mi.year_saka);
                    }
                    months_tested++;
                    prev_masa = mi.name;
                    prev_adhika = mi.is_adhika;
                    prev_saka = mi.year_saka;
                }
            }
        }
    }

    printf("  Roundtrip: %d months tested, %d/%d passed\n",
           months_tested, tests_passed - saved_pass, tests_run - saved_run);
}

/*
 * CSV regression test: if validation/moshier/lunisolar_months.csv exists,
 * verify lunisolar_month_start() matches every entry.
 */
static void test_csv_regression(void)
{
    const char *csv_path = "validation/moshier/lunisolar_months.csv";
    FILE *f = fopen(csv_path, "r");
    if (!f) {
        printf("\n--- CSV regression: %s not found, skipping ---\n", csv_path);
        return;
    }

    printf("\n--- CSV regression test ---\n");
    Location delhi = DEFAULT_LOCATION;
    int saved_run = tests_run, saved_pass = tests_passed;

    char line[256];
    /* Skip header */
    if (fgets(line, sizeof(line), f) == NULL) { fclose(f); return; }

    while (fgets(line, sizeof(line), f)) {
        int masa_num, is_adhika, saka_year, length, gy, gm, gd;
        char masa_name[64];
        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%d,%63s",
                   &masa_num, &is_adhika, &saka_year, &length,
                   &gy, &gm, &gd, masa_name) < 7) {
            continue;
        }

        double jd = lunisolar_month_start((MasaName)masa_num, saka_year,
                                          is_adhika, &delhi);
        tests_run++;
        if (jd > 0) {
            int ry, rm, rd;
            jd_to_gregorian(jd, &ry, &rm, &rd);
            if (ry == gy && rm == gm && rd == gd) {
                tests_passed++;
            } else {
                printf("  FAIL: %s%s %d: expected %04d-%02d-%02d, got %04d-%02d-%02d\n",
                       is_adhika ? "Adhika " : "", masa_name, saka_year,
                       gy, gm, gd, ry, rm, rd);
            }
        } else {
            printf("  FAIL: %s%s %d: not found\n",
                   is_adhika ? "Adhika " : "", masa_name, saka_year);
        }

        /* Also verify length */
        if (length > 0) {
            int calc_len = lunisolar_month_length((MasaName)masa_num, saka_year,
                                                  is_adhika, &delhi);
            tests_run++;
            if (calc_len == length) {
                tests_passed++;
            } else {
                printf("  FAIL: %s%s %d length: expected %d, got %d\n",
                       is_adhika ? "Adhika " : "", masa_name, saka_year,
                       length, calc_len);
            }
        }
    }

    fclose(f);
    printf("  CSV regression: %d/%d passed\n",
           tests_passed - saved_pass, tests_run - saved_run);
}

int main(void)
{
    astro_init(NULL);

    test_known_month_starts();
    test_month_lengths();
    test_roundtrip();
    test_csv_regression();

    astro_close();

    printf("\n=== Lunisolar month tests: %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
