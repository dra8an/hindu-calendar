#include "astro.h"
#include "tithi.h"
#include "masa.h"
#include "date_utils.h"
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

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out_dir = argv[++i];
        } else if (!out_dir && i + 1 < argc) {
            /* Legacy positional args: start_year end_year */
            start_year = atoi(argv[i]);
            end_year = atoi(argv[++i]);
        }
    }

    astro_init(NULL);
    Location loc = DEFAULT_LOCATION;

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
