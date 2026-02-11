#include "solar.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <string.h>

/*
 * Solar calendar external validation against drikpanchang.com.
 * Each entry is a first-day-of-month (day==1) verified on the website.
 *
 * Entry format: {greg_year, greg_month, greg_day, calendar_type,
 *                expected_month, expected_day(==1), expected_year}
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

static struct {
    int gy, gm, gd;
    SolarCalendarType type;
    int exp_month;
    int exp_day;
    int exp_year;
} ref_data[] = {

    /* ==== Tamil: Chithirai 1 (month 1) across 1950-2050, every 5 years ==== */
    /* Verified against drikpanchang.com Tamil Panchangam month pages */

    {1950,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1872},
    {1955,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1877},
    {1960,  4, 13, SOLAR_CAL_TAMIL,  1, 1, 1882},
    {1965,  4, 13, SOLAR_CAL_TAMIL,  1, 1, 1887},
    {1970,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1892},
    {1975,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1897},
    {1980,  4, 13, SOLAR_CAL_TAMIL,  1, 1, 1902},
    {1985,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1907},
    {1990,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1912},
    {1995,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1917},
    {2000,  4, 13, SOLAR_CAL_TAMIL,  1, 1, 1922},
    {2005,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1927},
    {2010,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1932},
    {2015,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1937},
    {2020,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1942},
    {2025,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1947},
    {2030,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1952},
    {2035,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1957},
    {2040,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1962},
    {2045,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1967},
    {2050,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1972},

    /* ==== Tamil: all 12 months of 2025 ==== */
    /* Verified against drikpanchang.com Tamil month panchangam */

    {2025,  1, 14, SOLAR_CAL_TAMIL, 10, 1, 1946},  /* Thai */
    {2025,  2, 13, SOLAR_CAL_TAMIL, 11, 1, 1946},  /* Maasi */
    {2025,  3, 15, SOLAR_CAL_TAMIL, 12, 1, 1946},  /* Panguni */
    {2025,  4, 14, SOLAR_CAL_TAMIL,  1, 1, 1947},  /* Chithirai */
    {2025,  5, 15, SOLAR_CAL_TAMIL,  2, 1, 1947},  /* Vaikaasi */
    {2025,  6, 15, SOLAR_CAL_TAMIL,  3, 1, 1947},  /* Aani */
    {2025,  7, 16, SOLAR_CAL_TAMIL,  4, 1, 1947},  /* Aadi */
    {2025,  8, 17, SOLAR_CAL_TAMIL,  5, 1, 1947},  /* Aavani */
    {2025,  9, 17, SOLAR_CAL_TAMIL,  6, 1, 1947},  /* Purattaasi */
    {2025, 10, 17, SOLAR_CAL_TAMIL,  7, 1, 1947},  /* Aippasi */
    {2025, 11, 16, SOLAR_CAL_TAMIL,  8, 1, 1947},  /* Karthikai */
    {2025, 12, 16, SOLAR_CAL_TAMIL,  9, 1, 1947},  /* Maargazhi */

    /* ==== Bengali: Boishakh 1 (month 1) across 1950-2050 ==== */
    /* Verified against drikpanchang.com Bengali Panjika + web sources */

    {1950,  4, 14, SOLAR_CAL_BENGALI,  1, 1, 1357},
    {1960,  4, 14, SOLAR_CAL_BENGALI,  1, 1, 1367},
    {1970,  4, 15, SOLAR_CAL_BENGALI,  1, 1, 1377},
    {1980,  4, 14, SOLAR_CAL_BENGALI,  1, 1, 1387},
    {1990,  4, 15, SOLAR_CAL_BENGALI,  1, 1, 1397},
    {2000,  4, 14, SOLAR_CAL_BENGALI,  1, 1, 1407},
    {2010,  4, 15, SOLAR_CAL_BENGALI,  1, 1, 1417},
    {2015,  4, 15, SOLAR_CAL_BENGALI,  1, 1, 1422},
    {2025,  4, 15, SOLAR_CAL_BENGALI,  1, 1, 1432},
    {2030,  4, 15, SOLAR_CAL_BENGALI,  1, 1, 1437},
    {2040,  4, 14, SOLAR_CAL_BENGALI,  1, 1, 1447},
    {2050,  4, 15, SOLAR_CAL_BENGALI,  1, 1, 1457},

    /* ==== Bengali: all 12 months of 2025 ==== */
    /* Verified against drikpanchang.com Bengali month panjika */

    {2025,  1, 15, SOLAR_CAL_BENGALI, 10, 1, 1431},  /* Magh */
    {2025,  2, 13, SOLAR_CAL_BENGALI, 11, 1, 1431},  /* Falgun */
    {2025,  3, 15, SOLAR_CAL_BENGALI, 12, 1, 1431},  /* Choitro */
    {2025,  4, 15, SOLAR_CAL_BENGALI,  1, 1, 1432},  /* Boishakh */
    {2025,  5, 15, SOLAR_CAL_BENGALI,  2, 1, 1432},  /* Joishtho */
    {2025,  6, 16, SOLAR_CAL_BENGALI,  3, 1, 1432},  /* Asharh */
    {2025,  7, 17, SOLAR_CAL_BENGALI,  4, 1, 1432},  /* Srabon */
    {2025,  8, 18, SOLAR_CAL_BENGALI,  5, 1, 1432},  /* Bhadro */
    {2025,  9, 18, SOLAR_CAL_BENGALI,  6, 1, 1432},  /* Ashshin */
    {2025, 10, 18, SOLAR_CAL_BENGALI,  7, 1, 1432},  /* Kartik */
    {2025, 11, 17, SOLAR_CAL_BENGALI,  8, 1, 1432},  /* Ogrohaeon */
    {2025, 12, 17, SOLAR_CAL_BENGALI,  9, 1, 1432},  /* Poush */

    /* ==== Odia: all 12 months of 2025 ==== */
    /* Verified against drikpanchang.com Oriya Panji */

    {2025,  1, 14, SOLAR_CAL_ODIA, 10, 1, 1946},  /* Magha */
    {2025,  2, 12, SOLAR_CAL_ODIA, 11, 1, 1946},  /* Phalguna */
    {2025,  3, 14, SOLAR_CAL_ODIA, 12, 1, 1946},  /* Chaitra */
    {2025,  4, 14, SOLAR_CAL_ODIA,  1, 1, 1947},  /* Baisakha */
    {2025,  5, 15, SOLAR_CAL_ODIA,  2, 1, 1947},  /* Jyeshtha */
    {2025,  6, 15, SOLAR_CAL_ODIA,  3, 1, 1947},  /* Ashadha */
    {2025,  7, 16, SOLAR_CAL_ODIA,  4, 1, 1947},  /* Shravana */
    {2025,  8, 17, SOLAR_CAL_ODIA,  5, 1, 1947},  /* Bhadrapada */
    {2025,  9, 17, SOLAR_CAL_ODIA,  6, 1, 1947},  /* Ashvina */
    {2025, 10, 17, SOLAR_CAL_ODIA,  7, 1, 1947},  /* Kartika */
    {2025, 11, 16, SOLAR_CAL_ODIA,  8, 1, 1947},  /* Margashirsha */
    {2025, 12, 16, SOLAR_CAL_ODIA,  9, 1, 1947},  /* Pausha */

    /* ==== Odia: all 12 months of 2030 ==== */
    /* Verified against drikpanchang.com Oriya Panji */

    {2030,  1, 14, SOLAR_CAL_ODIA, 10, 1, 1951},  /* Magha */
    {2030,  2, 13, SOLAR_CAL_ODIA, 11, 1, 1951},  /* Phalguna */
    {2030,  3, 15, SOLAR_CAL_ODIA, 12, 1, 1951},  /* Chaitra */
    {2030,  4, 14, SOLAR_CAL_ODIA,  1, 1, 1952},  /* Baisakha */
    {2030,  5, 15, SOLAR_CAL_ODIA,  2, 1, 1952},  /* Jyeshtha */
    {2030,  6, 15, SOLAR_CAL_ODIA,  3, 1, 1952},  /* Ashadha */
    {2030,  7, 17, SOLAR_CAL_ODIA,  4, 1, 1952},  /* Shravana */
    {2030,  8, 17, SOLAR_CAL_ODIA,  5, 1, 1952},  /* Bhadrapada */
    {2030,  9, 17, SOLAR_CAL_ODIA,  6, 1, 1952},  /* Ashvina */
    {2030, 10, 17, SOLAR_CAL_ODIA,  7, 1, 1952},  /* Kartika */
    {2030, 11, 16, SOLAR_CAL_ODIA,  8, 1, 1952},  /* Margashirsha */
    {2030, 12, 16, SOLAR_CAL_ODIA,  9, 1, 1952},  /* Pausha */

    /* ==== Malayalam: Chingam 1 (month 1) across 1950-2030, every 5 years ==== */
    /* Verified against prokerala.com Malayalam calendar */

    {1950,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1126},
    {1955,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1131},
    {1960,  8, 16, SOLAR_CAL_MALAYALAM,  1, 1, 1136},
    {1965,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1141},
    {1970,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1146},
    {1975,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1151},
    {1985,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1161},
    {1990,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1166},
    {1995,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1171},
    {2000,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1176},
    {2005,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1181},
    {2010,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1186},
    {2015,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1191},
    {2020,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1196},
    {2025,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1201},
    {2030,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1206},

    /* ==== Malayalam: all 12 months of 2025 ==== */
    /* Verified against prokerala.com and drikpanchang.com */

    {2025,  1, 14, SOLAR_CAL_MALAYALAM,  6, 1, 1200},  /* Makaram */
    {2025,  2, 13, SOLAR_CAL_MALAYALAM,  7, 1, 1200},  /* Kumbham */
    {2025,  3, 15, SOLAR_CAL_MALAYALAM,  8, 1, 1200},  /* Meenam */
    {2025,  4, 14, SOLAR_CAL_MALAYALAM,  9, 1, 1200},  /* Medam */
    {2025,  5, 15, SOLAR_CAL_MALAYALAM, 10, 1, 1200},  /* Edavam */
    {2025,  6, 15, SOLAR_CAL_MALAYALAM, 11, 1, 1200},  /* Mithunam */
    {2025,  7, 17, SOLAR_CAL_MALAYALAM, 12, 1, 1200},  /* Karkadakam */
    {2025,  8, 17, SOLAR_CAL_MALAYALAM,  1, 1, 1201},  /* Chingam */
    {2025,  9, 17, SOLAR_CAL_MALAYALAM,  2, 1, 1201},  /* Kanni */
    {2025, 10, 18, SOLAR_CAL_MALAYALAM,  3, 1, 1201},  /* Thulam */
    {2025, 11, 17, SOLAR_CAL_MALAYALAM,  4, 1, 1201},  /* Vrishchikam */
    {2025, 12, 16, SOLAR_CAL_MALAYALAM,  5, 1, 1201},  /* Dhanu */
};

static void test_solar_validation(void)
{
    printf("\n--- Solar calendar validation against drikpanchang.com ---\n");
    Location delhi = DEFAULT_LOCATION;
    int n = sizeof(ref_data) / sizeof(ref_data[0]);

    const char *cal_names[] = {"Tamil", "Bengali", "Odia", "Malayalam"};

    for (int i = 0; i < n; i++) {
        SolarDate sd = gregorian_to_solar(ref_data[i].gy, ref_data[i].gm,
                                          ref_data[i].gd, &delhi,
                                          ref_data[i].type);

        char buf[256];

        /* Check month */
        snprintf(buf, sizeof(buf),
                 "%s %04d-%02d-%02d: month (got %d [%s], expected %d [%s])",
                 cal_names[ref_data[i].type],
                 ref_data[i].gy, ref_data[i].gm, ref_data[i].gd,
                 sd.month, solar_month_name(sd.month, ref_data[i].type),
                 ref_data[i].exp_month,
                 solar_month_name(ref_data[i].exp_month, ref_data[i].type));
        check(sd.month == ref_data[i].exp_month, buf);

        /* Check day */
        snprintf(buf, sizeof(buf),
                 "%s %04d-%02d-%02d: day (got %d, expected %d)",
                 cal_names[ref_data[i].type],
                 ref_data[i].gy, ref_data[i].gm, ref_data[i].gd,
                 sd.day, ref_data[i].exp_day);
        check(sd.day == ref_data[i].exp_day, buf);

        /* Check year */
        snprintf(buf, sizeof(buf),
                 "%s %04d-%02d-%02d: year (got %d, expected %d)",
                 cal_names[ref_data[i].type],
                 ref_data[i].gy, ref_data[i].gm, ref_data[i].gd,
                 sd.year, ref_data[i].exp_year);
        check(sd.year == ref_data[i].exp_year, buf);
    }
}

int main(void)
{
    astro_init(NULL);

    test_solar_validation();

    astro_close();

    printf("\n=== Solar Validation: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}
