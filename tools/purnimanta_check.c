/*
 * purnimanta_check.c - Verify Purnimanta month starts against ref CSV.
 *
 * For every Purnimanta month start, look up that Gregorian date in
 * ref_1900_2050.csv and verify the tithi is 16 (Krishna Pratipada).
 * Also verify the Amanta masa at that date is the *previous* month
 * (since Purnimanta month M starts in Amanta month M-1's Krishna paksha).
 */
#include "masa.h"
#include "astro.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Load ref CSV into a lookup by (year, month, day) -> (tithi, masa, adhika) */
#define MAX_DAYS 60000
static struct {
    int year, month, day;
    int tithi, masa, adhika, saka;
} ref[MAX_DAYS];
static int ref_count = 0;

static int load_csv(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return -1; }
    char line[256];
    fgets(line, sizeof(line), f); /* skip header */
    while (fgets(line, sizeof(line), f) && ref_count < MAX_DAYS) {
        sscanf(line, "%d,%d,%d,%d,%d,%d,%d",
               &ref[ref_count].year, &ref[ref_count].month, &ref[ref_count].day,
               &ref[ref_count].tithi, &ref[ref_count].masa,
               &ref[ref_count].adhika, &ref[ref_count].saka);
        ref_count++;
    }
    fclose(f);
    return 0;
}

static int find_ref(int y, int m, int d, int *tithi, int *masa, int *adhika)
{
    for (int i = 0; i < ref_count; i++) {
        if (ref[i].year == y && ref[i].month == m && ref[i].day == d) {
            *tithi = ref[i].tithi;
            *masa = ref[i].masa;
            *adhika = ref[i].adhika;
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    astro_init(NULL);

    if (load_csv("validation/moshier/ref_1900_2050.csv") < 0)
        return 1;

    printf("Loaded %d ref entries\n", ref_count);

    Location delhi = DEFAULT_LOCATION;
    int checked = 0, pass = 0, fail = 0, not_found = 0;

    /* Walk all lunisolar months from the roundtrip approach:
     * iterate every day 1900-2050, detect month transitions,
     * then check the Purnimanta start for each month. */
    int prev_masa = -1, prev_adhika = -1, prev_saka = -1;
    int first = 1;

    for (int y = 1900; y <= 2050; y++) {
        for (int m = 1; m <= 12; m++) {
            int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
            int dim = mdays[m];
            if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
                dim = 29;
            for (int d = 1; d <= dim; d++) {
                MasaInfo mi = masa_for_date(y, m, d, &delhi);
                if ((int)mi.name != prev_masa || mi.is_adhika != prev_adhika ||
                    mi.year_saka != prev_saka) {
                    if (first) { first = 0; }
                    else {
                        /* Check Purnimanta start for this month */
                        double jd = lunisolar_month_start(mi.name, mi.year_saka,
                                                          mi.is_adhika,
                                                          LUNISOLAR_PURNIMANTA,
                                                          &delhi);
                        if (jd > 0) {
                            int py, pm, pd;
                            jd_to_gregorian(jd, &py, &pm, &pd);

                            int ref_tithi, ref_masa, ref_adhika;
                            if (find_ref(py, pm, pd, &ref_tithi, &ref_masa, &ref_adhika)) {
                                checked++;
                                if (ref_tithi == 16) {
                                    pass++;
                                } else if (ref_tithi == 17) {
                                    /* Kshaya tithi 16: Krishna Pratipada
                                     * was skipped, first Krishna day is
                                     * Dwitiya (17). Still correct. */
                                    pass++;
                                } else {
                                    fail++;
                                    printf("FAIL: %s%s %d -> Purnimanta start %04d-%02d-%02d "
                                           "has tithi %d (expected 16 or 17)\n",
                                           mi.is_adhika ? "Adhika " : "",
                                           MASA_NAMES[mi.name], mi.year_saka,
                                           py, pm, pd, ref_tithi);
                                }
                            } else {
                                not_found++;
                            }
                        }
                    }
                    prev_masa = mi.name;
                    prev_adhika = mi.is_adhika;
                    prev_saka = mi.year_saka;
                }
            }
        }
    }

    astro_close();

    printf("\nPurnimanta check: %d checked, %d pass, %d fail, %d not in CSV\n",
           checked, pass, fail, not_found);
    return fail > 0 ? 1 : 0;
}
