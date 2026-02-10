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
    /* ---- Spot-checked against drikpanchang.com (30 dates, 1950-2045) ---- */

    /* 1950 */
    {1950,  1,  1, 12, 10, 0, 1871},  /* Pausha S-12 */

    /* 1975 */
    {1975,  7, 15,  7,  4, 0, 1897},  /* Ashadha S-7 */

    /* 1990 */
    {1990,  3, 10, 14, 12, 0, 1911},  /* Phalguna S-14 */

    /* 2000 */
    {2000,  1,  1, 25,  9, 0, 1921},  /* Margashirsha K-10 */
    {2000,  4, 15, 12,  1, 0, 1922},  /* Chaitra S-12 */

    /* 2001 */
    {2001,  6, 21, 30,  3, 0, 1923},  /* Jyeshtha Amavasya */

    /* 2003 */
    {2003, 10, 10, 15,  7, 0, 1925},  /* Ashvina Purnima */

    /* 2004 - year with Adhika Shravana */
    {2004,  7, 18,  1,  5, 1, 1926},  /* Adhika Shravana S-1 */
    {2004,  8,  1, 16,  5, 1, 1926},  /* Adhika Shravana K-1 */

    /* 2006 */
    {2006, 10, 22, 30,  7, 0, 1928},  /* Ashvina Amavasya */

    /* 2007 - year with Adhika Jyeshtha */
    {2007,  3, 15, 26, 12, 0, 1928},  /* Phalguna K-11 */
    {2007,  6, 20,  6,  3, 0, 1929},  /* Jyeshtha S-6 (nija, not adhika) */

    /* 2010 */
    {2010,  8, 12,  3,  5, 0, 1932},  /* Shravana S-3 */
    {2010,  9,  4, 25,  5, 0, 1932},  /* Shravana K-10 */

    /* ---- Original hand-verified dates ---- */

    /* 2012 - year with Adhika Bhadrapada */
    {2012,  1,  1, 8,  10, 0, 1933},  /* Pausha S-8 */
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
    {2015,  7, 18,  2,  4, 0, 1937},  /* Nija Ashadha S-2 */
    {2015,  8, 14, 30,  4, 0, 1937},  /* Nija Ashadha Amavasya */
    {2015,  8, 15, 1,   5, 0, 1937},  /* Shravana S-1 */

    /* 2018 - year with Adhika Jyeshtha */
    {2018,  1,  1, 14, 10, 0, 1939},  /* Pausha S-14 */
    {2018,  5, 17,  2,  3, 1, 1940},  /* Adhika Jyeshtha S-2 */

    /* 2019 */
    {2019,  6, 15, 13,  3, 0, 1941},  /* Jyeshtha S-13 */

    /* 2020 - year with Adhika Ashvina */
    {2020,  1,  1, 6,  10, 0, 1941},  /* Pausha S-6 */
    {2020,  3, 25, 1,   1, 0, 1942},  /* Chaitra S-1 (New Year) */
    {2020,  9, 18,  1,  7, 1, 1942},  /* Adhika Ashvina S-1 */
    {2020, 11, 12, 27,  7, 0, 1942},  /* Ashvina K-12 (nija) */

    /* 2021 */
    {2021,  8, 13,  5,  5, 0, 1943},  /* Shravana S-5 */

    /* 2022 */
    {2022,  4, 30, 30,  1, 0, 1944},  /* Chaitra Amavasya */

    /* 2023 - year with Adhika Shravana */
    {2023,  7, 18, 1,   5, 1, 1945},  /* Adhika Shravana S-1 */
    {2023,  8, 18,  2,  5, 0, 1945},  /* Shravana S-2 (nija) */

    /* 2024 */
    {2024,  9,  1, 29,  5, 0, 1946},  /* Shravana K-14 */

    /* 2025 */
    {2025,  1,  1, 2,  10, 0, 1946},  /* Pausha S-2 */
    {2025,  1, 13, 15, 10, 0, 1946},  /* Pausha Purnima */
    {2025,  1, 29, 30, 10, 0, 1946},  /* Pausha Amavasya */
    {2025,  1, 30, 1,  11, 0, 1946},  /* Magha S-1 */
    {2025,  3, 29, 30, 12, 0, 1946},  /* Phalguna Amavasya */
    {2025,  3, 30, 1,   1, 0, 1947},  /* Chaitra S-1 (New Year) */

    /* 2026 */
    {2026,  2, 14, 27, 11, 0, 1947},  /* Magha K-12 */

    /* 2027 */
    {2027, 12, 21, 24,  9, 0, 1949},  /* Margashirsha K-9 */

    /* 2030 */
    {2030,  5,  8,  5,  2, 0, 1952},  /* Vaishakha S-5 */

    /* 2045 */
    {2045, 11, 15,  7,  8, 0, 1967},  /* Kartika S-7 */
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
