#include "types.h"
#include "astro.h"
#include "tithi.h"
#include "panchang.h"
#include "solar.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options]\n"
        "  -y YEAR      Gregorian year (default: current)\n"
        "  -m MONTH     Gregorian month 1-12 (default: current)\n"
        "  -d DAY       Specific day (if omitted, shows full month)\n"
        "  -s TYPE      Solar calendar: tamil, bengali, odia, malayalam\n"
        "               (if omitted, shows lunisolar panchang)\n"
        "  -l LAT,LON   Location (default: New Delhi 28.6139,77.2090)\n"
        "  -u OFFSET    UTC offset in hours (default: 5.5)\n"
        "  -h           Show this help\n",
        prog);
}

/* Days in a Gregorian month */
static int days_in_greg_month(int year, int month)
{
    static const int mdays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
            return 29;
    }
    return mdays[month];
}

static int parse_solar_type(const char *str, SolarCalendarType *out)
{
    if (strcmp(str, "tamil") == 0)     { *out = SOLAR_CAL_TAMIL; return 1; }
    if (strcmp(str, "bengali") == 0)   { *out = SOLAR_CAL_BENGALI; return 1; }
    if (strcmp(str, "odia") == 0)      { *out = SOLAR_CAL_ODIA; return 1; }
    if (strcmp(str, "malayalam") == 0) { *out = SOLAR_CAL_MALAYALAM; return 1; }
    return 0;
}

static void print_solar_month(int year, int month, const Location *loc,
                               SolarCalendarType type)
{
    int ndays = days_in_greg_month(year, month);

    /* Print header using first day's solar date for context */
    SolarDate sd1 = gregorian_to_solar(year, month, 1, loc, type);
    const char *era = solar_era_name(type);
    const char *mname = solar_month_name(sd1.month, type);
    printf("%s Solar Calendar — %s %d (%s)\n",
           type == SOLAR_CAL_TAMIL ? "Tamil" :
           type == SOLAR_CAL_BENGALI ? "Bengali" :
           type == SOLAR_CAL_ODIA ? "Odia" : "Malayalam",
           mname, sd1.year, era);
    printf("Gregorian %04d-%02d\n\n", year, month);

    printf("%-12s %-5s %-20s %s\n",
           "Date", "Day", "Solar Date", "");
    printf("%-12s %-5s %-20s\n",
           "----------", "---", "--------------------");

    for (int d = 1; d <= ndays; d++) {
        SolarDate sd = gregorian_to_solar(year, month, d, loc, type);
        double jd = gregorian_to_jd(year, month, d);
        int dow = day_of_week(jd);

        printf("%04d-%02d-%02d   %-5s %s %d, %d\n",
               year, month, d,
               day_of_week_short(dow),
               solar_month_name(sd.month, type),
               sd.day, sd.year);
    }
}

static const char *RASHI_NAMES[] = {
    "", "Mesha", "Vrishabha", "Mithuna", "Karka", "Simha", "Kanya",
    "Tula", "Vrishchika", "Dhanu", "Makara", "Kumbha", "Meena"
};

static void print_solar_day(int year, int month, int day, const Location *loc,
                             SolarCalendarType type)
{
    SolarDate sd = gregorian_to_solar(year, month, day, loc, type);
    double jd = gregorian_to_jd(year, month, day);
    int dow = day_of_week(jd);
    const char *era = solar_era_name(type);
    const char *cal_name =
        type == SOLAR_CAL_TAMIL ? "Tamil" :
        type == SOLAR_CAL_BENGALI ? "Bengali" :
        type == SOLAR_CAL_ODIA ? "Odia" : "Malayalam";

    printf("Date:         %04d-%02d-%02d (%s)\n", year, month, day,
           day_of_week_name(dow));
    printf("Calendar:     %s Solar\n", cal_name);
    printf("Solar Date:   %s %d, %d (%s)\n",
           solar_month_name(sd.month, type), sd.day, sd.year, era);
    printf("Rashi:        %s\n",
           sd.rashi >= 1 && sd.rashi <= 12 ? RASHI_NAMES[sd.rashi] : "???");
}

int main(int argc, char *argv[])
{
    /* Defaults: current date, New Delhi */
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    int year = tm->tm_year + 1900;
    int month = tm->tm_mon + 1;
    int day = 0;  /* 0 = show full month */
    Location loc = DEFAULT_LOCATION;
    int solar_mode = 0;
    SolarCalendarType solar_type = SOLAR_CAL_TAMIL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-y") == 0 && i + 1 < argc) {
            year = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            month = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            day = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            i++;
            if (!parse_solar_type(argv[i], &solar_type)) {
                fprintf(stderr, "Error: unknown solar calendar type '%s'\n", argv[i]);
                fprintf(stderr, "Valid types: tamil, bengali, odia, malayalam\n");
                return 1;
            }
            solar_mode = 1;
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            i++;
            if (sscanf(argv[i], "%lf,%lf", &loc.latitude, &loc.longitude) != 2) {
                fprintf(stderr, "Error: invalid location format. Use LAT,LON\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            loc.utc_offset = atof(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (month < 1 || month > 12) {
        fprintf(stderr, "Error: month must be 1-12\n");
        return 1;
    }

    astro_init(NULL);

    if (solar_mode) {
        if (day > 0) {
            print_solar_day(year, month, day, &loc, solar_type);
        } else {
            print_solar_month(year, month, &loc, solar_type);
        }
    } else if (day > 0) {
        /* Single day */
        PanchangDay pd;
        pd.greg_year = year;
        pd.greg_month = month;
        pd.greg_day = day;

        double jd = gregorian_to_jd(year, month, day);
        pd.jd_sunrise = sunrise_jd(jd, &loc);
        pd.tithi = tithi_at_sunrise(year, month, day, &loc);
        pd.hindu_date = gregorian_to_hindu(year, month, day, &loc);

        print_day_panchang(&pd, loc.utc_offset);
    } else {
        /* Full month */
        printf("Hindu Calendar — %04d-%02d (%.4f°N, %.4f°E, UTC%+.1f)\n\n",
               year, month, loc.latitude, loc.longitude, loc.utc_offset);

        PanchangDay days[31];
        int count;
        generate_month_panchang(year, month, &loc, days, &count);
        print_month_panchang(days, count, loc.utc_offset);
    }

    astro_close();
    return 0;
}
