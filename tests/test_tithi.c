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
 * Test data validated against drikpanchang.com for New Delhi.
 * Format: {year, month, day, expected_tithi_num, expected_paksha}
 */
static void test_tithi_known_dates(void)
{
    printf("\n--- Tithi at sunrise (New Delhi) ---\n");
    Location delhi = DEFAULT_LOCATION;

    struct {
        int y, m, d;
        int tithi;
        Paksha paksha;
        const char *label;
    } cases[] = {
        /* From drikpanchang.com */
        {2013,  1, 18,  7, SHUKLA_PAKSHA,  "2013-01-18 = Shukla Saptami"},
        {2012,  8, 18,  1, SHUKLA_PAKSHA,  "2012-08-18 = Shukla Pratipada"},
        {2025,  1, 13, 15, SHUKLA_PAKSHA,  "2025-01-13 = Purnima"},
        {2025,  1, 29, 30, KRISHNA_PAKSHA, "2025-01-29 = Amavasya"},
        {2025,  1,  1,  2, SHUKLA_PAKSHA,  "2025-01-01 = Shukla Dwitiya"},
        {2025,  1, 14, 16, KRISHNA_PAKSHA, "2025-01-14 = Krishna Pratipada"},
        {2025,  1, 30,  1, SHUKLA_PAKSHA,  "2025-01-30 = Shukla Pratipada"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        TithiInfo ti = tithi_at_sunrise(cases[i].y, cases[i].m, cases[i].d, &delhi);
        ASSERT_EQ(ti.tithi_num, cases[i].tithi, cases[i].label);
        ASSERT_EQ(ti.paksha, cases[i].paksha,
                  cases[i].paksha == SHUKLA_PAKSHA ? "  paksha = Shukla" : "  paksha = Krishna");
    }
}

static void test_kshaya_tithi(void)
{
    printf("\n--- Kshaya (skipped) tithi detection ---\n");
    Location delhi = DEFAULT_LOCATION;

    /* Jan 2025: S-12 on Jan 11, S-14 on Jan 12 → S-13 is kshaya */
    TithiInfo ti = tithi_at_sunrise(2025, 1, 11, &delhi);
    printf("  2025-01-11: tithi %d, kshaya flag = %d\n", ti.tithi_num, ti.is_kshaya);
    ASSERT_EQ(ti.is_kshaya, 1, "2025-01-11 should have kshaya (S-13 skipped)");

    /* Jan 12 should NOT be kshaya itself */
    ti = tithi_at_sunrise(2025, 1, 12, &delhi);
    ASSERT_EQ(ti.is_kshaya, 0, "2025-01-12 not kshaya");
}

static void test_adhika_tithi(void)
{
    printf("\n--- Adhika (repeated) tithi detection ---\n");
    Location delhi = DEFAULT_LOCATION;

    /* Jan 2025: K-5 on both Jan 18 and Jan 19 → adhika tithi */
    TithiInfo ti18 = tithi_at_sunrise(2025, 1, 18, &delhi);
    TithiInfo ti19 = tithi_at_sunrise(2025, 1, 19, &delhi);
    printf("  2025-01-18: tithi %d\n", ti18.tithi_num);
    printf("  2025-01-19: tithi %d\n", ti19.tithi_num);
    ASSERT_EQ(ti18.tithi_num, ti19.tithi_num,
              "Jan 18 and Jan 19 same tithi (adhika)");
}

static void test_lunar_phase(void)
{
    printf("\n--- Lunar phase ---\n");

    /* At full moon, phase should be ~180 */
    /* At new moon, phase should be ~0 or ~360 */
    double jd_purnima = gregorian_to_jd(2025, 1, 13);
    double sr = sunrise_jd(jd_purnima, &(Location)DEFAULT_LOCATION);
    double phase = lunar_phase(sr);
    printf("  Phase at 2025-01-13 sunrise (Purnima): %.2f deg\n", phase);
    /* Should be between 168-180 (end of Shukla Chaturdashi / Purnima) */
    tests_run++;
    if (phase > 156.0 && phase < 192.0) {
        tests_passed++;
        printf("  PASS: phase near 180 at Purnima\n");
    } else {
        printf("  FAIL: phase %.2f not near 180\n", phase);
    }
}

int main(void)
{
    astro_init(NULL);

    test_tithi_known_dates();
    test_kshaya_tithi();
    test_adhika_tithi();
    test_lunar_phase();

    astro_close();

    printf("\n=== Tithi tests: %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
