#include "tithi.h"
#include "masa.h"
#include "astro.h"
#include "dst.h"
#include <stdio.h>

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

/* NYC location */
static Location nyc_loc(int y, int m, int d)
{
    Location loc = {40.7128, -74.0060, 0.0, us_eastern_offset(y, m, d)};
    return loc;
}

/*
 * Test US Eastern DST rules across all historical eras.
 */
static void test_dst_rules(void)
{
    char buf[128];

    printf("\n--- US Eastern DST rules ---\n");

    /* 1900-1917: No DST */
    snprintf(buf, sizeof(buf), "1910-06-15 = EST (-5)");
    check(us_eastern_offset(1910, 6, 15) == -5.0, buf);

    /* 1918: Last Sun March (Mar 31) - Last Sun Oct (Oct 27) */
    snprintf(buf, sizeof(buf), "1918-03-30 = EST (-5)");
    check(us_eastern_offset(1918, 3, 30) == -5.0, buf);
    snprintf(buf, sizeof(buf), "1918-03-31 = EDT (-4)");
    check(us_eastern_offset(1918, 3, 31) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1918-10-26 = EDT (-4)");
    check(us_eastern_offset(1918, 10, 26) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1918-10-27 = EST (-5)");
    check(us_eastern_offset(1918, 10, 27) == -5.0, buf);

    /* 1920-1941: No federal DST */
    snprintf(buf, sizeof(buf), "1935-07-01 = EST (-5)");
    check(us_eastern_offset(1935, 7, 1) == -5.0, buf);

    /* 1942-1945: War Time (year-round EDT) */
    snprintf(buf, sizeof(buf), "1942-02-08 = EST (-5)");
    check(us_eastern_offset(1942, 2, 8) == -5.0, buf);
    snprintf(buf, sizeof(buf), "1942-02-09 = EDT (-4, War Time starts)");
    check(us_eastern_offset(1942, 2, 9) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1943-01-01 = EDT (-4, War Time)");
    check(us_eastern_offset(1943, 1, 1) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1944-12-31 = EDT (-4, War Time)");
    check(us_eastern_offset(1944, 12, 31) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1945-09-30 = EDT (-4, War Time ends)");
    check(us_eastern_offset(1945, 9, 30) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1945-10-01 = EST (-5)");
    check(us_eastern_offset(1945, 10, 1) == -5.0, buf);

    /* 1946-1966: Last Sun April - Last Sun September */
    /* 1960: Last Sun April = Apr 24, Last Sun Sep = Sep 25 */
    snprintf(buf, sizeof(buf), "1960-04-23 = EST (-5)");
    check(us_eastern_offset(1960, 4, 23) == -5.0, buf);
    snprintf(buf, sizeof(buf), "1960-04-24 = EDT (-4)");
    check(us_eastern_offset(1960, 4, 24) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1960-07-15 = EDT (-4)");
    check(us_eastern_offset(1960, 7, 15) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1960-09-24 = EDT (-4)");
    check(us_eastern_offset(1960, 9, 24) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1960-09-25 = EST (-5)");
    check(us_eastern_offset(1960, 9, 25) == -5.0, buf);

    /* 1967-1973: Last Sun April - Last Sun October */
    /* 1970: Last Sun April = Apr 26, Last Sun Oct = Oct 25 */
    snprintf(buf, sizeof(buf), "1970-04-25 = EST (-5)");
    check(us_eastern_offset(1970, 4, 25) == -5.0, buf);
    snprintf(buf, sizeof(buf), "1970-04-26 = EDT (-4)");
    check(us_eastern_offset(1970, 4, 26) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1970-10-24 = EDT (-4)");
    check(us_eastern_offset(1970, 10, 24) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1970-10-25 = EST (-5)");
    check(us_eastern_offset(1970, 10, 25) == -5.0, buf);

    /* 1974: Energy crisis - Jan 6 start */
    snprintf(buf, sizeof(buf), "1974-01-05 = EST (-5)");
    check(us_eastern_offset(1974, 1, 5) == -5.0, buf);
    snprintf(buf, sizeof(buf), "1974-01-06 = EDT (-4, energy crisis)");
    check(us_eastern_offset(1974, 1, 6) == -4.0, buf);

    /* 1975: Last Sun Feb (Feb 23) */
    snprintf(buf, sizeof(buf), "1975-02-22 = EST (-5)");
    check(us_eastern_offset(1975, 2, 22) == -5.0, buf);
    snprintf(buf, sizeof(buf), "1975-02-23 = EDT (-4)");
    check(us_eastern_offset(1975, 2, 23) == -4.0, buf);

    /* 1976-1986: Last Sun April - Last Sun October */
    /* 1980: Last Sun April = Apr 27, Last Sun Oct = Oct 26 */
    snprintf(buf, sizeof(buf), "1980-04-26 = EST (-5)");
    check(us_eastern_offset(1980, 4, 26) == -5.0, buf);
    snprintf(buf, sizeof(buf), "1980-04-27 = EDT (-4)");
    check(us_eastern_offset(1980, 4, 27) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1980-10-25 = EDT (-4)");
    check(us_eastern_offset(1980, 10, 25) == -4.0, buf);
    snprintf(buf, sizeof(buf), "1980-10-26 = EST (-5)");
    check(us_eastern_offset(1980, 10, 26) == -5.0, buf);

    /* 1987-2006: First Sun April - Last Sun October */
    /* 2000: First Sun April = Apr 2, Last Sun Oct = Oct 29 */
    snprintf(buf, sizeof(buf), "2000-04-01 = EST (-5)");
    check(us_eastern_offset(2000, 4, 1) == -5.0, buf);
    snprintf(buf, sizeof(buf), "2000-04-02 = EDT (-4)");
    check(us_eastern_offset(2000, 4, 2) == -4.0, buf);
    snprintf(buf, sizeof(buf), "2000-10-28 = EDT (-4)");
    check(us_eastern_offset(2000, 10, 28) == -4.0, buf);
    snprintf(buf, sizeof(buf), "2000-10-29 = EST (-5)");
    check(us_eastern_offset(2000, 10, 29) == -5.0, buf);

    /* 2007+: Second Sun March - First Sun November */
    /* 2025: DST starts Mar 9, ends Nov 2 */
    snprintf(buf, sizeof(buf), "2025-03-08 = EST (-5)");
    check(us_eastern_offset(2025, 3, 8) == -5.0, buf);
    snprintf(buf, sizeof(buf), "2025-03-09 = EDT (-4)");
    check(us_eastern_offset(2025, 3, 9) == -4.0, buf);
    snprintf(buf, sizeof(buf), "2025-07-04 = EDT (-4)");
    check(us_eastern_offset(2025, 7, 4) == -4.0, buf);
    snprintf(buf, sizeof(buf), "2025-11-01 = EDT (-4)");
    check(us_eastern_offset(2025, 11, 1) == -4.0, buf);
    snprintf(buf, sizeof(buf), "2025-11-02 = EST (-5)");
    check(us_eastern_offset(2025, 11, 2) == -5.0, buf);
    snprintf(buf, sizeof(buf), "2025-12-25 = EST (-5)");
    check(us_eastern_offset(2025, 12, 25) == -5.0, buf);
}

/*
 * NYC tithi/masa validated against drikpanchang.com (geoname-id=5128581).
 * Scraped 2025 full year: 365/365 match.
 * Selected dates spanning DST transitions, seasons, adhika/kshaya scenarios.
 */
static void test_nyc_validation(void)
{
    printf("\n--- NYC tithi/masa validation against drikpanchang.com ---\n");

    struct {
        int y, m, d;
        int tithi;
        int masa;
        int adhika;
        int saka;
        const char *note;
    } cases[] = {
        /* Winter (EST) */
        {2025,  1,  1,  2, 10, 0, 1946, "Jan 1 Pausha S-2"},
        {2025,  1, 13, 15, 10, 0, 1946, "Pausha Purnima"},
        {2025,  1, 29, 30, 10, 0, 1946, "Pausha Amavasya"},
        {2025,  2, 12, 15, 11, 0, 1946, "Magha Purnima"},
        {2025,  2, 28, 1,  12, 0, 1946, "Phalguna S-1"},

        /* Around DST transition: Mar 8 (EST) -> Mar 9 (EDT) */
        {2025,  3,  8, 10, 12, 0, 1946, "Mar 8 last EST day"},
        {2025,  3,  9, 11, 12, 0, 1946, "Mar 9 first EDT day"},
        {2025,  3, 10, 12, 12, 0, 1946, "Mar 10 EDT"},

        /* Hindu New Year (NYC sunrise later than Delhi, tithi differs) */
        {2025,  3, 29, 30, 12, 0, 1946, "Phalguna Amavasya"},
        {2025,  3, 30,  2,  1, 0, 1947, "Chaitra S-2 (NYC)"},

        /* Summer (EDT) */
        {2025,  6, 21,  26, 3, 0, 1947, "Jun 21 summer solstice"},
        {2025,  7,  4,  9,  4, 0, 1947, "Jul 4"},
        {2025,  8, 15, 22,  5, 0, 1947, "Aug 15"},

        /* Around DST end: Nov 1 (EDT) -> Nov 2 (EST) */
        {2025, 11,  1, 11,  8, 0, 1947, "Nov 1 last EDT day"},
        {2025, 11,  2, 12,  8, 0, 1947, "Nov 2 first EST day"},
        {2025, 11,  3, 13,  8, 0, 1947, "Nov 3 EST"},

        /* Winter again */
        {2025, 12, 25,  6, 10, 0, 1947, "Dec 25"},
        {2025, 12, 31, 12, 10, 0, 1947, "Dec 31"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        Location loc = nyc_loc(cases[i].y, cases[i].m, cases[i].d);
        TithiInfo ti = tithi_at_sunrise(cases[i].y, cases[i].m, cases[i].d, &loc);
        MasaInfo mi = masa_for_date(cases[i].y, cases[i].m, cases[i].d, &loc);

        char buf[256];

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d tithi (got %d, expected %d) [%s]",
                 cases[i].y, cases[i].m, cases[i].d,
                 ti.tithi_num, cases[i].tithi, cases[i].note);
        check(ti.tithi_num == cases[i].tithi, buf);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d masa (got %d, expected %d)",
                 cases[i].y, cases[i].m, cases[i].d,
                 mi.name, cases[i].masa);
        check((int)mi.name == cases[i].masa, buf);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d adhika (got %d, expected %d)",
                 cases[i].y, cases[i].m, cases[i].d,
                 mi.is_adhika, cases[i].adhika);
        check(mi.is_adhika == cases[i].adhika, buf);

        snprintf(buf, sizeof(buf), "%04d-%02d-%02d saka (got %d, expected %d)",
                 cases[i].y, cases[i].m, cases[i].d,
                 mi.year_saka, cases[i].saka);
        check(mi.year_saka == cases[i].saka, buf);
    }
}

int main(void)
{
    astro_init(NULL);

    test_dst_rules();
    test_nyc_validation();

    astro_close();

    printf("\n=== NYC tests: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}
