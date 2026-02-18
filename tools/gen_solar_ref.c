#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

static void generate_csv(SolarCalendarType type, const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "ERROR: cannot open %s for writing\n", filename);
        exit(1);
    }

    const char *cal_names[] = {"Tamil", "Bengali", "Odia", "Malayalam"};
    Location loc = DEFAULT_LOCATION;

    fprintf(f, "month,year,length,greg_year,greg_month,greg_day,month_name\n");

    /* Track current month start */
    int prev_month = -1;
    int prev_year = -1;
    int start_gy = 0, start_gm = 0, start_gd = 0;
    int days_in_current = 0;
    int months_written = 0;
    int first_complete = 0;  /* skip partial first month */

    for (int y = 1900; y <= 2050; y++) {
        for (int m = 1; m <= 12; m++) {
            int ndays = days_in_month(y, m);
            for (int d = 1; d <= ndays; d++) {
                SolarDate sd = gregorian_to_solar(y, m, d, &loc, type);

                if (prev_month < 0) {
                    /* First day: initialize tracking (partial month) */
                    prev_month = sd.month;
                    prev_year = sd.year;
                    start_gy = y;
                    start_gm = m;
                    start_gd = d;
                    days_in_current = 1;
                } else if (sd.month != prev_month || sd.year != prev_year) {
                    /* Month changed: emit row for previous month
                     * (skip the first partial month) */
                    if (first_complete) {
                        fprintf(f, "%d,%d,%d,%d,%d,%d,%s\n",
                                prev_month, prev_year, days_in_current,
                                start_gy, start_gm, start_gd,
                                solar_month_name(prev_month, type));
                        months_written++;
                    }
                    first_complete = 1;

                    /* Start tracking new month */
                    prev_month = sd.month;
                    prev_year = sd.year;
                    start_gy = y;
                    start_gm = m;
                    start_gd = d;
                    days_in_current = 1;
                } else {
                    days_in_current++;
                }
            }
        }
        fprintf(stderr, "%s: generated year %d\n", cal_names[type], y);
    }

    /* Don't emit final partial month (data ends at 2050-12-31) */

    fclose(f);
    fprintf(stderr, "%s: wrote %d months to %s\n",
            cal_names[type], months_written, filename);
}

int main(int argc, char *argv[])
{
    const char *out_dir = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out_dir = argv[++i];
        }
    }

    const char *prefix = out_dir ? out_dir : "validation/se";
    char solar_dir[512];
    snprintf(solar_dir, sizeof(solar_dir), "%s/solar", prefix);
    mkdir(solar_dir, 0755);

    astro_init(NULL);

    char path[512];
    snprintf(path, sizeof(path), "%s/solar/tamil_months_1900_2050.csv", prefix);
    generate_csv(SOLAR_CAL_TAMIL, path);

    snprintf(path, sizeof(path), "%s/solar/bengali_months_1900_2050.csv", prefix);
    generate_csv(SOLAR_CAL_BENGALI, path);

    snprintf(path, sizeof(path), "%s/solar/odia_months_1900_2050.csv", prefix);
    generate_csv(SOLAR_CAL_ODIA, path);

    snprintf(path, sizeof(path), "%s/solar/malayalam_months_1900_2050.csv", prefix);
    generate_csv(SOLAR_CAL_MALAYALAM, path);

    astro_close();
    fprintf(stderr, "Done.\n");
    return 0;
}
