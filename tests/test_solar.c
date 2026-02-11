#include "solar.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <string.h>
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
 * Reference data validated against drikpanchang.com (New Delhi).
 * Each entry: {greg_year, greg_month, greg_day, calendar_type,
 *              expected_month, expected_day, expected_year}
 */
static struct {
    int gy, gm, gd;
    SolarCalendarType type;
    int exp_month;  /* Regional month number (1-12) */
    int exp_day;
    int exp_year;   /* Regional era year */
} ref_data[] = {
    /* ---- Tamil Solar Calendar (Saka era) ---- */
    /* Verified against drikpanchang.com Tamil Panchangam */

    /* 2025: Chithirai 1 = April 14 (Mesha Sankranti / Puthandu) */
    {2025,  4, 14, SOLAR_CAL_TAMIL,   1,  1, 1947},  /* Chithirai 1 */
    {2025,  4, 15, SOLAR_CAL_TAMIL,   1,  2, 1947},  /* Chithirai 2 */
    {2025,  4, 30, SOLAR_CAL_TAMIL,   1, 17, 1947},  /* Chithirai 17 */
    {2025,  4, 13, SOLAR_CAL_TAMIL,  12, 30, 1946},  /* Panguni 30 */
    {2025,  4,  1, SOLAR_CAL_TAMIL,  12, 18, 1946},  /* Panguni 18 */
    {2025,  1,  1, SOLAR_CAL_TAMIL,   9, 17, 1946},  /* Maargazhi 17 */
    {2025,  7, 15, SOLAR_CAL_TAMIL,   3, 31, 1947},  /* Aani 31 */

    /* 2000 */
    {2000,  4, 13, SOLAR_CAL_TAMIL,   1,  1, 1922},  /* Chithirai 1 */
    {2000,  4, 14, SOLAR_CAL_TAMIL,   1,  2, 1922},  /* Chithirai 2 */
    {2000,  1,  1, SOLAR_CAL_TAMIL,   9, 17, 1921},  /* Maargazhi 17 */

    /* ---- Bengali Solar Calendar (Bangabda era) ---- */
    /* Verified against drikpanchang.com Bengali Panjika */

    /* 2025: Boishakh 1 = April 15 (Pohela Boishakh) */
    {2025,  4, 15, SOLAR_CAL_BENGALI,  1,  1, 1432},  /* Boishakh 1 */
    {2025,  4, 16, SOLAR_CAL_BENGALI,  1,  2, 1432},  /* Boishakh 2 */
    {2025,  4, 30, SOLAR_CAL_BENGALI,  1, 16, 1432},  /* Boishakh 16 */
    {2025,  4, 14, SOLAR_CAL_BENGALI, 12, 31, 1431},  /* Choitro 31 */
    {2025,  4, 13, SOLAR_CAL_BENGALI, 12, 30, 1431},  /* Choitro 30 */
    {2025,  4,  1, SOLAR_CAL_BENGALI, 12, 18, 1431},  /* Choitro 18 */
    {2025,  1,  1, SOLAR_CAL_BENGALI,  9, 17, 1431},  /* Poush 17 */
    {2025,  6, 15, SOLAR_CAL_BENGALI,  2, 32, 1432},  /* Joishtho 32 */

    /* 2000 */
    {2000,  1,  1, SOLAR_CAL_BENGALI,  9, 16, 1406},  /* Poush 16 */

    /* ---- Odia Solar Calendar (Saka era) ---- */
    /* Verified against drikpanchang.com Oriya Panji */

    /* 2025: Baisakha 1 = April 14 (Mesha Sankranti / Pana Sankranti) */
    {2025,  4, 14, SOLAR_CAL_ODIA,   1,  1, 1947},  /* Baisakha 1 */
    {2025,  4, 15, SOLAR_CAL_ODIA,   1,  2, 1947},  /* Baisakha 2 */
    {2025,  4, 30, SOLAR_CAL_ODIA,   1, 17, 1947},  /* Baisakha 17 */
    {2025,  4, 13, SOLAR_CAL_ODIA,  12, 31, 1946},  /* Chaitra 31 */
    {2025,  4,  1, SOLAR_CAL_ODIA,  12, 19, 1946},  /* Chaitra 19 */
    {2025,  1,  1, SOLAR_CAL_ODIA,   9, 18, 1946},  /* Pausha 18 */
    {2025,  7, 15, SOLAR_CAL_ODIA,   3, 31, 1947},  /* Ashadha 31 */

    /* ---- Malayalam Solar Calendar (Kollam era) ---- */
    /* Verified against drikpanchang.com Malayalam Panchangam */

    /* 2025: Chingam 1 = August 17 (Malayalam New Year) */
    {2025,  8, 17, SOLAR_CAL_MALAYALAM,  1,  1, 1201},  /* Chingam 1 */
    {2025,  8, 18, SOLAR_CAL_MALAYALAM,  1,  2, 1201},  /* Chingam 2 */
    {2025,  8, 31, SOLAR_CAL_MALAYALAM,  1, 15, 1201},  /* Chingam 15 */
    {2025,  8, 16, SOLAR_CAL_MALAYALAM, 12, 31, 1200},  /* Karkadakam 31 */
    {2025,  8,  1, SOLAR_CAL_MALAYALAM, 12, 16, 1200},  /* Karkadakam 16 */
    {2025,  1,  1, SOLAR_CAL_MALAYALAM,  5, 17, 1200},  /* Dhanu 17 */
    {2025,  4, 15, SOLAR_CAL_MALAYALAM,  9,  2, 1200},  /* Medam 2 */
};

static void test_gregorian_to_solar(void)
{
    printf("\n--- Solar calendar: gregorian_to_solar ---\n");
    Location delhi = DEFAULT_LOCATION;
    int n = sizeof(ref_data) / sizeof(ref_data[0]);

    for (int i = 0; i < n; i++) {
        SolarDate sd = gregorian_to_solar(ref_data[i].gy, ref_data[i].gm,
                                          ref_data[i].gd, &delhi,
                                          ref_data[i].type);

        const char *cal_names[] = {"Tamil", "Bengali", "Odia", "Malayalam"};
        char buf[256];

        /* Check month */
        snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d month (got %d [%s], expected %d [%s])",
                 cal_names[ref_data[i].type],
                 ref_data[i].gy, ref_data[i].gm, ref_data[i].gd,
                 sd.month, solar_month_name(sd.month, ref_data[i].type),
                 ref_data[i].exp_month,
                 solar_month_name(ref_data[i].exp_month, ref_data[i].type));
        check(sd.month == ref_data[i].exp_month, buf);

        /* Check day */
        snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d day (got %d, expected %d)",
                 cal_names[ref_data[i].type],
                 ref_data[i].gy, ref_data[i].gm, ref_data[i].gd,
                 sd.day, ref_data[i].exp_day);
        check(sd.day == ref_data[i].exp_day, buf);

        /* Check year */
        snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d year (got %d, expected %d)",
                 cal_names[ref_data[i].type],
                 ref_data[i].gy, ref_data[i].gm, ref_data[i].gd,
                 sd.year, ref_data[i].exp_year);
        check(sd.year == ref_data[i].exp_year, buf);
    }
}

static void test_solar_to_gregorian_roundtrip(void)
{
    printf("\n--- Solar calendar: solar_to_gregorian roundtrip ---\n");
    Location delhi = DEFAULT_LOCATION;
    int n = sizeof(ref_data) / sizeof(ref_data[0]);

    for (int i = 0; i < n; i++) {
        SolarDate sd;
        sd.year = ref_data[i].exp_year;
        sd.month = ref_data[i].exp_month;
        sd.day = ref_data[i].exp_day;

        int gy, gm, gd;
        solar_to_gregorian(&sd, ref_data[i].type, &delhi, &gy, &gm, &gd);

        const char *cal_names[] = {"Tamil", "Bengali", "Odia", "Malayalam"};
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "%s roundtrip: %s %d, %d -> got %04d-%02d-%02d, expected %04d-%02d-%02d",
                 cal_names[ref_data[i].type],
                 solar_month_name(sd.month, ref_data[i].type), sd.day, sd.year,
                 gy, gm, gd,
                 ref_data[i].gy, ref_data[i].gm, ref_data[i].gd);
        check(gy == ref_data[i].gy && gm == ref_data[i].gm && gd == ref_data[i].gd, buf);
    }
}

static void test_sankranti_jd(void)
{
    printf("\n--- Solar calendar: sankranti_jd precision ---\n");

    /* Mesha Sankranti 2025: sun enters sidereal Aries (0 degrees).
     * The result should be mid-April, and the longitude at the result
     * should be very close to 0/360 degrees. */
    double jd_est = gregorian_to_jd(2025, 4, 14);
    double jd_mesha = sankranti_jd(jd_est, 0.0);

    double lon_at = solar_longitude_sidereal(jd_mesha);
    /* The longitude should be extremely close to 0 (or 360) */
    double err = fmod(lon_at, 360.0);
    if (err > 180.0) err = 360.0 - err;
    char buf[256];
    snprintf(buf, sizeof(buf),
             "Mesha Sankranti 2025: longitude at JD = %.6f deg (should be ~0)",
             err);
    check(err < 0.0001, buf);

    /* Verify it falls in the expected date range */
    int gy, gm, gd;
    jd_to_gregorian(jd_mesha, &gy, &gm, &gd);
    snprintf(buf, sizeof(buf),
             "Mesha Sankranti 2025: date = %04d-%02d-%02d (should be April 13-14)",
             gy, gm, gd);
    check(gy == 2025 && gm == 4 && (gd == 13 || gd == 14), buf);
}

static void test_month_names(void)
{
    printf("\n--- Solar calendar: month names ---\n");
    char buf[256];

    /* Tamil month 1 = Chithirai */
    snprintf(buf, sizeof(buf), "Tamil month 1 = %s (expected Chithirai)",
             solar_month_name(1, SOLAR_CAL_TAMIL));
    check(strcmp(solar_month_name(1, SOLAR_CAL_TAMIL), "Chithirai") == 0, buf);

    /* Bengali month 1 = Boishakh */
    snprintf(buf, sizeof(buf), "Bengali month 1 = %s (expected Boishakh)",
             solar_month_name(1, SOLAR_CAL_BENGALI));
    check(strcmp(solar_month_name(1, SOLAR_CAL_BENGALI), "Boishakh") == 0, buf);

    /* Odia month 1 = Baisakha */
    snprintf(buf, sizeof(buf), "Odia month 1 = %s (expected Baisakha)",
             solar_month_name(1, SOLAR_CAL_ODIA));
    check(strcmp(solar_month_name(1, SOLAR_CAL_ODIA), "Baisakha") == 0, buf);

    /* Malayalam month 1 = Chingam */
    snprintf(buf, sizeof(buf), "Malayalam month 1 = %s (expected Chingam)",
             solar_month_name(1, SOLAR_CAL_MALAYALAM));
    check(strcmp(solar_month_name(1, SOLAR_CAL_MALAYALAM), "Chingam") == 0, buf);

    /* Malayalam month 12 = Karkadakam */
    snprintf(buf, sizeof(buf), "Malayalam month 12 = %s (expected Karkadakam)",
             solar_month_name(12, SOLAR_CAL_MALAYALAM));
    check(strcmp(solar_month_name(12, SOLAR_CAL_MALAYALAM), "Karkadakam") == 0, buf);

    /* Era names */
    check(strcmp(solar_era_name(SOLAR_CAL_TAMIL), "Saka") == 0, "Tamil era = Saka");
    check(strcmp(solar_era_name(SOLAR_CAL_BENGALI), "Bangabda") == 0, "Bengali era = Bangabda");
    check(strcmp(solar_era_name(SOLAR_CAL_ODIA), "Saka") == 0, "Odia era = Saka");
    check(strcmp(solar_era_name(SOLAR_CAL_MALAYALAM), "Kollam") == 0, "Malayalam era = Kollam");
}

int main(void)
{
    astro_init(NULL);

    test_sankranti_jd();
    test_month_names();
    test_gregorian_to_solar();
    test_solar_to_gregorian_roundtrip();

    astro_close();

    printf("\n=== Solar: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}
