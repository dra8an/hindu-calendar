#include "astro.h"
#include "masa.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    const char *out_dir = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
            out_dir = argv[++i];
    }

    astro_init(NULL);
    Location delhi = DEFAULT_LOCATION;

    /* Open output file */
    FILE *out = stdout;
    char path[512];
    if (out_dir) {
        snprintf(path, sizeof(path), "%s/lunisolar_months.csv", out_dir);
        out = fopen(path, "w");
        if (!out) {
            fprintf(stderr, "ERROR: cannot open %s\n", path);
            return 1;
        }
    }

    fprintf(out, "masa,is_adhika,saka_year,length,greg_year,greg_month,greg_day,masa_name\n");

    int prev_masa = -1, prev_adhika = -1, prev_saka = -1;
    int prev_y = 0, prev_m = 0, prev_d = 0;
    int count = 0;
    int transitions_seen = 0;

    for (int y = 1900; y <= 2050; y++) {
        if (y % 10 == 0)
            fprintf(stderr, "Processing %d...\n", y);
        for (int m = 1; m <= 12; m++) {
            int dim = days_in_month(y, m);
            for (int d = 1; d <= dim; d++) {
                MasaInfo mi = masa_for_date(y, m, d, &delhi);
                if ((int)mi.name != prev_masa || mi.is_adhika != prev_adhika ||
                    mi.year_saka != prev_saka) {
                    /* Output the previous month with its length.
                     * Skip first 2 transitions: the initial setup and the
                     * first partial month (which started before our scan range). */
                    transitions_seen++;
                    if (prev_masa > 0 && transitions_seen > 2) {
                        double jd_prev_start = gregorian_to_jd(prev_y, prev_m, prev_d);
                        double jd_this_start = gregorian_to_jd(y, m, d);
                        int length = (int)(jd_this_start - jd_prev_start);
                        fprintf(out, "%d,%d,%d,%d,%d,%d,%d,%s%s\n",
                                prev_masa, prev_adhika, prev_saka, length,
                                prev_y, prev_m, prev_d,
                                prev_adhika ? "Adhika " : "",
                                MASA_NAMES[prev_masa]);
                        count++;
                    }
                    prev_masa = mi.name;
                    prev_adhika = mi.is_adhika;
                    prev_saka = mi.year_saka;
                    prev_y = y;
                    prev_m = m;
                    prev_d = d;
                }
            }
        }
    }

    /* Output last month (without length since we don't know when it ends) */
    if (prev_masa > 0) {
        fprintf(out, "%d,%d,%d,0,%d,%d,%d,%s%s\n",
                prev_masa, prev_adhika, prev_saka,
                prev_y, prev_m, prev_d,
                prev_adhika ? "Adhika " : "",
                MASA_NAMES[prev_masa]);
        count++;
    }

    if (out != stdout)
        fclose(out);

    fprintf(stderr, "Generated %d month entries\n", count);
    astro_close();
    return 0;
}
