#include "astro.h"
#include "tithi.h"
#include "masa.h"
#include "date_utils.h"
#include "dst.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Days in a Gregorian month */
static int days_in_month(int year, int month)
{
    static const int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
            return 29;
    }
    return mdays[month];
}

int main(int argc, char *argv[])
{
    int start_year = 1900;
    int end_year = 2050;
    const char *out_dir = NULL;
    int use_us_eastern = 0;
    double custom_lat = 0, custom_lon = 0;
    int have_location = 0;
    double custom_utc = 0;
    int have_utc = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out_dir = argv[++i];
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            i++;
            if (sscanf(argv[i], "%lf,%lf", &custom_lat, &custom_lon) == 2) {
                have_location = 1;
            } else {
                fprintf(stderr, "ERROR: -l expects LAT,LON (e.g., 40.7128,-74.0060)\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            custom_utc = atof(argv[++i]);
            have_utc = 1;
        } else if (strcmp(argv[i], "-tz") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "us_eastern") == 0) {
                use_us_eastern = 1;
            } else {
                fprintf(stderr, "ERROR: unknown timezone: %s (supported: us_eastern)\n", argv[i]);
                return 1;
            }
        } else if (!out_dir && i + 1 < argc) {
            /* Legacy positional args: start_year end_year */
            start_year = atoi(argv[i]);
            end_year = atoi(argv[++i]);
        }
    }

    astro_init(NULL);
    Location loc = DEFAULT_LOCATION;

    if (have_location) {
        loc.latitude = custom_lat;
        loc.longitude = custom_lon;
    }
    if (have_utc) {
        loc.utc_offset = custom_utc;
    }

    if (have_location || use_us_eastern) {
        fprintf(stderr, "Location: %.4f, %.4f\n", loc.latitude, loc.longitude);
        if (use_us_eastern)
            fprintf(stderr, "Timezone: US Eastern (DST-aware)\n");
        else
            fprintf(stderr, "UTC offset: %.1f\n", loc.utc_offset);
    }

    FILE *out = stdout;
    if (out_dir) {
        char path[512];
        snprintf(path, sizeof(path), "%s/ref_1900_2050.csv", out_dir);
        out = fopen(path, "w");
        if (!out) {
            fprintf(stderr, "ERROR: cannot open %s for writing\n", path);
            return 1;
        }
        fprintf(stderr, "Writing to %s\n", path);
    }

    fprintf(out, "year,month,day,tithi,masa,adhika,saka\n");

    for (int y = start_year; y <= end_year; y++) {
        for (int m = 1; m <= 12; m++) {
            int ndays = days_in_month(y, m);
            for (int d = 1; d <= ndays; d++) {
                if (use_us_eastern)
                    loc.utc_offset = us_eastern_offset(y, m, d);
                TithiInfo ti = tithi_at_sunrise(y, m, d, &loc);
                MasaInfo mi = masa_for_date(y, m, d, &loc);
                fprintf(out, "%d,%d,%d,%d,%d,%d,%d\n",
                       y, m, d, ti.tithi_num, (int)mi.name,
                       mi.is_adhika, mi.year_saka);
            }
        }
        fprintf(stderr, "Generated year %d\n", y);
    }

    if (out != stdout)
        fclose(out);
    astro_close();
    return 0;
}
