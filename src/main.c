#include "types.h"
#include "astro.h"
#include "tithi.h"
#include "panchang.h"
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
        "  -l LAT,LON   Location (default: New Delhi 28.6139,77.2090)\n"
        "  -u OFFSET    UTC offset in hours (default: 5.5)\n"
        "  -h           Show this help\n",
        prog);
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

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-y") == 0 && i + 1 < argc) {
            year = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            month = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            day = atoi(argv[++i]);
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

    if (day > 0) {
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
