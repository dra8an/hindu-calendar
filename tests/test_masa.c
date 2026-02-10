#include "masa.h"
#include "tithi.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <math.h>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_EQ(actual, expected, msg) do { \
    tests_run++; \
    int _a = (actual), _e = (expected); \
    if (_a == _e) { \
        tests_passed++; \
        printf("  PASS: %s (%d == %d)\n", msg, _a, _e); \
    } else { \
        printf("  FAIL: %s (got %d, expected %d)\n", msg, _a, _e); \
    } \
} while (0)

/*
 * Test masa determination against drikpanchang.com (Amanta scheme).
 */
static void test_masa_known_dates(void)
{
    printf("\n--- Masa (month) determination ---\n");
    Location delhi = DEFAULT_LOCATION;

    struct {
        int y, m, d;
        MasaName expected_masa;
        int expected_adhika;
        const char *label;
    } cases[] = {
        {2013,  1, 18, PAUSHA,      0, "2013-01-18 = Pausha"},
        {2013,  2, 10, PAUSHA,      0, "2013-02-10 = Pausha"},
        {2012,  8, 17, SHRAVANA,    0, "2012-08-17 = Shravana (Amavasya)"},
        {2012,  8, 18, BHADRAPADA,  1, "2012-08-18 = Adhika Bhadrapada"},
        {2012,  9, 18, BHADRAPADA,  0, "2012-09-18 = Nija Bhadrapada"},
        {2025,  1,  1, PAUSHA,      0, "2025-01-01 = Pausha"},
        {2025,  1, 30, MAGHA,       0, "2025-01-30 = Magha"},
        {2025,  3, 30, CHAITRA,     0, "2025-03-30 = Chaitra"},
        {2025,  4, 30, VAISHAKHA,   0, "2025-04-30 = Vaishakha"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        MasaInfo mi = masa_for_date(cases[i].y, cases[i].m, cases[i].d, &delhi);
        char buf[128];
        snprintf(buf, sizeof(buf), "%s (masa)", cases[i].label);
        ASSERT_EQ(mi.name, cases[i].expected_masa, buf);
        snprintf(buf, sizeof(buf), "%s (adhika)", cases[i].label);
        ASSERT_EQ(mi.is_adhika, cases[i].expected_adhika, buf);
    }
}

static void test_solar_rashi(void)
{
    printf("\n--- Solar rashi ---\n");

    /* At the start of Mesha (Aries), sidereal solar longitude should be ~0° */
    /* This happens around April 13-14 */
    double jd = gregorian_to_jd(2025, 4, 14);
    int rashi = solar_rashi(jd);
    printf("  2025-04-14 solar rashi: %d (Mesha=1)\n", rashi);
    /* Around Mesha/Meena transition */
    tests_run++;
    if (rashi == 1 || rashi == 12) {
        tests_passed++;
        printf("  PASS: rashi is Mesha(1) or Meena(12) around April 14\n");
    } else {
        printf("  FAIL: expected Mesha(1) or Meena(12), got %d\n", rashi);
    }

    /* In January, sun should be in Dhanu (9) or Makara (10) sidereal */
    jd = gregorian_to_jd(2025, 1, 15);
    rashi = solar_rashi(jd);
    printf("  2025-01-15 solar rashi: %d (Dhanu=9, Makara=10)\n", rashi);
    tests_run++;
    if (rashi == 9 || rashi == 10) {
        tests_passed++;
        printf("  PASS: rashi is Dhanu(9) or Makara(10) in mid-January\n");
    } else {
        printf("  FAIL: expected Dhanu(9) or Makara(10), got %d\n", rashi);
    }
}

static void test_year_determination(void)
{
    printf("\n--- Year determination ---\n");
    Location delhi = DEFAULT_LOCATION;

    MasaInfo mi = masa_for_date(2025, 1, 18, &delhi);
    ASSERT_EQ(mi.year_saka, 1946, "2025-01-18 Saka year = 1946");
    ASSERT_EQ(mi.year_vikram, 2081, "2025-01-18 Vikram year = 2081");

    mi = masa_for_date(2012, 8, 18, &delhi);
    ASSERT_EQ(mi.year_saka, 1934, "2012-08-18 Saka year = 1934");
    ASSERT_EQ(mi.year_vikram, 2069, "2012-08-18 Vikram year = 2069");
}

int main(void)
{
    astro_init(NULL);

    test_masa_known_dates();
    test_solar_rashi();
    test_year_determination();

    astro_close();

    printf("\n=== Masa tests: %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
