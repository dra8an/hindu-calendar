#include "tithi.h"
#include "masa.h"
#include "panchang.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <math.h>

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

/*
 * Reference data validated against drikpanchang.com (Amanta, New Delhi).
 * Each entry: {greg_year, greg_month, greg_day, tithi_num, masa, is_adhika, saka_year}
 */
static struct {
    int y, m, d;
    int tithi;
    int masa;
    int adhika;
    int saka;
} ref_data[] = {
    /* 2012 - year with Adhika Bhadrapada */
    {2012,  1,  1, 8,  10, 0, 1933},  /* Pausha S-8 (verified) */
    {2012,  3, 23, 1,   1, 0, 1934},  /* Chaitra S-1 (New Year) */
    {2012,  5, 20, 30,  2, 0, 1934},  /* Vaishakha Amavasya */
    {2012,  8, 17, 30,  5, 0, 1934},  /* Shravana Amavasya */
    {2012,  8, 18, 1,   6, 1, 1934},  /* Adhika Bhadrapada S-1 */
    {2012,  9, 16, 30,  6, 1, 1934},  /* Adhika Bhadrapada Amavasya */
    {2012,  9, 17, 2,   6, 0, 1934},  /* Nija Bhadrapada S-2 (S-1 skipped) */
    {2012, 11, 13, 29,  7, 0, 1934},  /* Ashvina K-14 */
    {2012, 12, 13, 30,  8, 0, 1934},  /* Kartika Amavasya */
    {2012, 12, 14, 1,   9, 0, 1934},  /* Margashirsha S-1 */

    /* 2013 */
    {2013,  1, 18, 7,  10, 0, 1934},  /* Pausha S-7 */
    {2013,  4, 10, 30, 12, 0, 1934},  /* Phalguna Amavasya */
    {2013,  4, 11, 1,   1, 0, 1935},  /* Chaitra S-1 (New Year) */

    /* 2015 - year with Adhika Ashadha */
    {2015,  7, 16, 30,  4, 1, 1937},  /* Adhika Ashadha Amavasya */
    {2015,  7, 17, 1,   4, 0, 1937},  /* Nija Ashadha S-1 */
    {2015,  8, 14, 30,  4, 0, 1937},  /* Nija Ashadha Amavasya */
    {2015,  8, 15, 1,   5, 0, 1937},  /* Shravana S-1 */

    /* 2020 */
    {2020,  1,  1, 6,  10, 0, 1941},  /* Pausha S-6 */
    {2020,  3, 25, 1,   1, 0, 1942},  /* Chaitra S-1 (New Year) */

    /* 2023 - year with Adhika Shravana */
    {2023,  7, 18, 1,   5, 1, 1945},  /* Adhika Shravana S-1 */

    /* 2025 */
    {2025,  1,  1, 2,  10, 0, 1946},  /* Pausha S-2 */
    {2025,  1, 13, 15, 10, 0, 1946},  /* Pausha Purnima */
    {2025,  1, 29, 30, 10, 0, 1946},  /* Pausha Amavasya */
    {2025,  1, 30, 1,  11, 0, 1946},  /* Magha S-1 */
    {2025,  3, 29, 30, 12, 0, 1946},  /* Phalguna Amavasya */
    {2025,  3, 30, 1,   1, 0, 1947},  /* Chaitra S-1 (New Year) */
};

static void test_validation(void)
{
    printf("\n--- Multi-year validation against drikpanchang.com ---\n");
    Location delhi = DEFAULT_LOCATION;
    int n = sizeof(ref_data) / sizeof(ref_data[0]);

    for (int i = 0; i < n; i++) {
        TithiInfo ti = tithi_at_sunrise(ref_data[i].y, ref_data[i].m, ref_data[i].d, &delhi);
        MasaInfo mi = masa_for_date(ref_data[i].y, ref_data[i].m, ref_data[i].d, &delhi);

        char buf[256];

        /* Check tithi */
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d tithi (got %d, expected %d)",
                 ref_data[i].y, ref_data[i].m, ref_data[i].d,
                 ti.tithi_num, ref_data[i].tithi);
        check(ti.tithi_num == ref_data[i].tithi, buf);

        /* Check masa */
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d masa (got %d, expected %d)",
                 ref_data[i].y, ref_data[i].m, ref_data[i].d,
                 mi.name, ref_data[i].masa);
        check((int)mi.name == ref_data[i].masa, buf);

        /* Check adhika */
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d adhika (got %d, expected %d)",
                 ref_data[i].y, ref_data[i].m, ref_data[i].d,
                 mi.is_adhika, ref_data[i].adhika);
        check(mi.is_adhika == ref_data[i].adhika, buf);

        /* Check saka year */
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d saka (got %d, expected %d)",
                 ref_data[i].y, ref_data[i].m, ref_data[i].d,
                 mi.year_saka, ref_data[i].saka);
        check(mi.year_saka == ref_data[i].saka, buf);
    }
}

int main(void)
{
    astro_init(NULL);

    test_validation();

    astro_close();

    printf("\n=== Validation: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}
