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

    /* ---- Batch-verified adhika tithi dates (1900-1919), 21 dates ---- */
    {1900,  1, 20, 19, 10, 0, 1821},
    {1901,  1, 10, 20, 10, 0, 1822},
    {1901, 12, 11, 30,  8, 0, 1823},
    {1903,  1,  1,  2, 10, 0, 1824},
    {1903, 10, 31, 10,  8, 0, 1825},
    {1904, 10, 20, 11,  7, 0, 1826},
    {1905, 11, 11, 14,  8, 0, 1827},
    {1906,  9,  7, 19,  6, 0, 1828},
    {1907,  8,  7, 28,  4, 0, 1829},
    {1908,  7,  3,  4,  4, 0, 1830},
    {1909,  6, 23,  5,  4, 0, 1831},
    {1910,  5, 22, 13,  2, 0, 1832},
    {1911,  5, 10, 12,  2, 0, 1833},
    {1912,  5, 11, 24,  2, 0, 1834},
    {1913,  3, 30, 23, 12, 0, 1834},
    {1914,  6, 15, 22,  3, 0, 1836},
    {1915,  6, 16,  3,  3, 0, 1837},
    {1916,  6,  3,  2,  3, 0, 1838},
    {1917,  6, 26,  6,  4, 0, 1839},
    {1918,  4, 23, 12,  1, 0, 1840},
    {1919,  4, 11, 11,  1, 0, 1841},

    /* ---- Batch-verified adhika tithi dates (1990-2020), 31 dates ---- */
    {1990, 11, 13, 26,  8, 0, 1912},
    {1991, 11, 15,  8,  8, 0, 1913},
    {1992, 11,  5, 10,  8, 0, 1914},
    {1993,  8, 31, 14,  6, 1, 1915},  /* Adhika Bhadrapada */
    {1994,  9, 23, 18,  6, 0, 1916},
    {1995,  8, 23, 27,  5, 0, 1917},
    {1996,  8, 11, 27,  4, 0, 1918},
    {1997,  9,  3,  1,  6, 0, 1919},
    {1998,  8,  1,  8,  5, 0, 1920},
    {1999,  9, 15,  5,  6, 0, 1921},
    {2000, 10,  7,  9,  7, 0, 1922},
    {2001,  8,  2, 13,  5, 0, 1923},
    {2002,  8,  3, 24,  4, 0, 1924},
    {2003,  5, 30, 29,  2, 0, 1925},
    {2004,  6, 20,  2,  4, 0, 1926},
    {2005,  6,  9,  2,  3, 0, 1927},
    {2006,  4,  6,  8,  1, 0, 1928},
    {2007,  6,  1, 15,  3, 1, 1929},  /* Adhika Jyeshtha */
    {2008,  4, 28, 22,  1, 0, 1930},
    {2009,  3, 17, 21, 12, 0, 1930},
    {2010,  3, 20,  4,  1, 0, 1932},
    {2011,  1, 14,  9, 10, 0, 1932},
    {2012,  1,  5, 11, 10, 0, 1933},
    {2013,  1, 26, 14, 10, 0, 1934},
    {2013, 12, 26, 23,  9, 0, 1935},
    {2015,  2,  6, 17, 11, 0, 1936},
    {2016,  2, 27, 19, 11, 0, 1937},
    {2017,  2, 16, 20, 11, 0, 1938},
    {2018,  6,  4, 20,  3, 1, 1940},  /* Adhika Jyeshtha */
    {2019,  4,  2, 27, 12, 0, 1940},
    {2020,  3, 21, 27, 12, 0, 1941},

    /* ---- Batch-verified kshaya tithi dates (1900-1919), 21 dates ---- */
    {1900,  1,  6,  6, 10, 0, 1821},
    {1901,  1,  4, 15, 10, 0, 1822},
    {1901, 12, 22, 12,  9, 0, 1823},
    {1903,  1, 11, 13, 10, 0, 1824},
    {1903, 12, 11, 23,  9, 0, 1825},
    {1904, 11, 27, 20,  8, 0, 1826},
    {1905, 12, 19, 23,  9, 0, 1827},
    {1906, 10, 26, 10,  8, 0, 1828},
    {1907, 11,  5, 30,  7, 0, 1829},
    {1908,  9, 14, 20,  6, 0, 1830},
    {1909,  9, 27, 13,  6, 0, 1831},
    {1910, 10, 16, 13,  7, 0, 1832},
    {1911,  8, 26,  2,  6, 0, 1833},
    {1912,  9,  6, 25,  5, 0, 1834},
    {1913, 10,  2,  3,  7, 0, 1835},
    {1914, 10, 21,  3,  8, 0, 1836},
    {1915, 10, 17, 10,  7, 0, 1837},
    {1916, 11,  6, 12,  8, 0, 1838},
    {1917, 10, 25, 10,  7, 0, 1839},
    {1918,  9, 24, 20,  6, 0, 1840},
    {1919, 10, 13, 20,  7, 0, 1841},

    /* ---- Batch-verified kshaya tithi dates (1992-2022), 31 dates ---- */
    {1992,  8, 29,  2,  6, 0, 1914},
    {1993,  7, 24,  6,  5, 0, 1915},
    {1994,  7, 22, 15,  4, 0, 1916},
    {1995,  7, 16, 20,  4, 0, 1917},
    {1996,  7, 28, 13,  4, 0, 1918},
    {1997,  8, 15, 12,  5, 0, 1919},
    {1998,  7, 18, 25,  4, 0, 1920},
    {1999,  8,  7, 26,  4, 0, 1921},
    {2000,  8, 26, 27,  5, 0, 1922},
    {2001,  7, 27,  8,  5, 0, 1923},
    {2002,  8,  7, 29,  4, 0, 1924},
    {2003,  7, 13, 15,  4, 0, 1925},
    {2004,  7,  6, 20,  4, 0, 1926},
    {2005,  7, 19, 13,  4, 0, 1927},
    {2006,  6, 20, 25,  3, 0, 1928},
    {2007,  7, 10, 26,  3, 0, 1929},
    {2008,  6, 27, 24,  3, 0, 1930},
    {2009,  6, 22, 30,  3, 0, 1931},
    {2010,  5, 27, 15,  2, 0, 1932},
    {2011,  4, 19, 17,  1, 0, 1933},
    {2012,  5,  7, 17,  2, 0, 1934},
    {2013,  5, 27, 18,  2, 0, 1935},
    {2014,  4, 21, 22,  1, 0, 1936},
    {2015,  6, 12, 26,  3, 0, 1937},
    {2016,  6,  7,  3,  3, 0, 1938},
    {2017,  7, 20, 27,  4, 0, 1939},
    {2018,  8, 14,  4,  5, 0, 1940},
    {2019,  8,  2,  2,  5, 0, 1941},
    {2020,  8, 20,  2,  6, 0, 1942},
    {2021,  8, 17, 10,  5, 0, 1943},
    {2022,  8, 13, 17,  5, 0, 1944},

    /* ---- Batch-verified kshaya tithi dates (2023-2050), 28 dates ---- */
    {2023,  9,  1, 17,  5, 0, 1945},
    {2024,  7, 25, 20,  4, 0, 1946},
    {2025,  8, 14, 21,  5, 0, 1947},
    {2026,  8, 11, 29,  4, 0, 1948},
    {2027,  8,  5,  4,  5, 0, 1949},
    {2028,  9, 18, 30,  6, 0, 1950},
    {2029, 10,  7, 30,  6, 0, 1951},
    {2030, 10, 26, 30,  7, 0, 1952},
    {2031, 10, 22,  7,  7, 0, 1953},
    {2032,  9, 22, 19,  6, 0, 1954},
    {2033, 10,  5, 12,  7, 0, 1955},
    {2034,  9, 29, 17,  6, 0, 1956},
    {2035,  9,  3,  2,  6, 0, 1957},
    {2036,  9, 21,  2,  7, 0, 1958},
    {2037, 10, 10,  2,  7, 0, 1959},
    {2038, 10,  4,  7,  7, 0, 1960},
    {2039, 10, 15, 28,  7, 1, 1961},  /* Adhika Ashvina */
    {2040,  9, 19, 14,  6, 0, 1962},
    {2041,  9, 13, 19,  6, 0, 1963},
    {2042,  9, 25, 11,  6, 0, 1964},
    {2043,  8, 28, 24,  5, 0, 1965},
    {2044,  8, 24,  2,  6, 0, 1966},
    {2045,  8, 12, 30,  4, 0, 1967},
    {2046,  8, 31, 30,  5, 0, 1968},
    {2047,  8, 25,  5,  6, 0, 1969},
    {2048, 10, 15,  9,  7, 0, 1970},
    {2049, 10,  3,  7,  7, 0, 1971},
    {2050,  9,  4, 19,  6, 1, 1972},  /* Adhika Bhadrapada */
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
