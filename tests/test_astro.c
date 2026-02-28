#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_NEAR(actual, expected, tolerance, msg) do { \
    tests_run++; \
    double _a = (actual), _e = (expected), _t = (tolerance); \
    if (fabs(_a - _e) <= _t) { \
        tests_passed++; \
        printf("  PASS: %s (%.6f ~= %.6f)\n", msg, _a, _e); \
    } else { \
        printf("  FAIL: %s (got %.6f, expected %.6f, diff %.6f)\n", \
               msg, _a, _e, fabs(_a - _e)); \
    } \
} while (0)

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

static void test_julian_day(void)
{
    printf("\n--- Julian Day conversion ---\n");

    /* J2000.0 = 2000-01-01 12:00 TT ≈ JD 2451545.0 at noon */
    double jd = gregorian_to_jd(2000, 1, 1);
    ASSERT_NEAR(jd, 2451544.5, 0.001, "J2000 midnight");

    /* Unix epoch: 1970-01-01 */
    jd = gregorian_to_jd(1970, 1, 1);
    ASSERT_NEAR(jd, 2440587.5, 0.001, "Unix epoch");

    /* Round-trip */
    int y, m, d;
    jd_to_gregorian(2451544.5, &y, &m, &d);
    ASSERT_EQ(y, 2000, "Round-trip year");
    ASSERT_EQ(m, 1, "Round-trip month");
    ASSERT_EQ(d, 1, "Round-trip day");
}

static void test_day_of_week(void)
{
    printf("\n--- Day of week ---\n");

    /* 2013-01-18 was a Friday. ISO convention: 0=Mon,...,4=Fri */
    double jd = gregorian_to_jd(2013, 1, 18);
    int dow = day_of_week(jd);
    ASSERT_EQ(dow, 4, "2013-01-18 = Friday (4)");

    /* 2025-01-01 was a Wednesday. ISO convention: 2=Wed */
    jd = gregorian_to_jd(2025, 1, 1);
    dow = day_of_week(jd);
    ASSERT_EQ(dow, 2, "2025-01-01 = Wednesday (2)");
}

static void test_solar_longitude(void)
{
    printf("\n--- Solar longitude ---\n");

    /* Just verify we get reasonable values (0-360) */
    double jd = gregorian_to_jd(2025, 1, 1);
    double lon = solar_longitude(jd);
    printf("  Solar longitude at 2025-01-01: %.4f deg (tropical)\n", lon);
    /* Sun should be around Sagittarius/Capricorn (270° tropical) in January */
    ASSERT_NEAR(lon, 280.0, 15.0, "Solar long ~280 in Jan");

    double sid = solar_longitude_sidereal(jd);
    printf("  Solar longitude at 2025-01-01: %.4f deg (sidereal)\n", sid);
    /* Sidereal should be about 24° less due to ayanamsa */
    double ayan = get_ayanamsa(jd);
    printf("  Ayanamsa: %.4f deg\n", ayan);
    ASSERT_NEAR(ayan, 24.2, 0.5, "Ayanamsa ~24.2 in 2025");
}

static void test_sunrise(void)
{
    printf("\n--- Sunrise ---\n");
    Location delhi = DEFAULT_LOCATION;

    /* 2013-01-18: drikpanchang says sunrise at 07:15 IST in Delhi */
    double jd = gregorian_to_jd(2013, 1, 18);
    double sr = sunrise_jd(jd, &delhi);

    /* Convert to IST hours */
    double sr_local = sr + 5.5 / 24.0;
    double frac = (sr_local + 0.5) - floor(sr_local + 0.5);
    double hours = frac * 24.0;
    printf("  Sunrise JD: %.6f, IST hours: %.4f (%.0fh %02.0fm)\n",
           sr, hours, floor(hours), (hours - floor(hours)) * 60);

    /* Should be around 7.25 hours (7:15 AM) */
    ASSERT_NEAR(hours, 7.26, 0.05, "Delhi sunrise ~07:15 on 2013-01-18");

    /* 2012-08-18: drikpanchang says sunrise at 05:52 IST */
    jd = gregorian_to_jd(2012, 8, 18);
    sr = sunrise_jd(jd, &delhi);
    sr_local = sr + 5.5 / 24.0;
    frac = (sr_local + 0.5) - floor(sr_local + 0.5);
    hours = frac * 24.0;
    ASSERT_NEAR(hours, 5.87, 0.05, "Delhi sunrise ~05:52 on 2012-08-18");
}

int main(void)
{
    astro_init(NULL);

    test_julian_day();
    test_day_of_week();
    test_solar_longitude();
    test_sunrise();

    astro_close();

    printf("\n=== Astro tests: %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
