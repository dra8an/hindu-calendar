#include "panchang.h"
#include "solar.h"
#include "date_utils.h"
#include <stdio.h>
#include <time.h>

static double elapsed_sec(struct timespec *start, struct timespec *end)
{
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

static int days_in_month(int y, int m)
{
    int dim[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        return 29;
    return dim[m];
}

typedef struct {
    const char *name;
    int is_solar;
    SolarCalendarType solar_type;
} BenchEntry;

int main(void)
{
    Location loc = DEFAULT_LOCATION;

    BenchEntry entries[] = {
        {"Lunisolar", 0, 0},
        {"Tamil",     1, SOLAR_CAL_TAMIL},
        {"Bengali",   1, SOLAR_CAL_BENGALI},
        {"Odia",      1, SOLAR_CAL_ODIA},
        {"Malayalam",  1, SOLAR_CAL_MALAYALAM},
    };
    int n_entries = (int)(sizeof(entries) / sizeof(entries[0]));

    printf("=== Performance Benchmark ===\n");

    double total_time = 0.0;
    long total_calls = 0;

    for (int e = 0; e < n_entries; e++) {
        struct timespec t0, t1;
        long count = 0;

        clock_gettime(CLOCK_MONOTONIC, &t0);

        for (int y = 1900; y <= 2050; y++) {
            for (int m = 1; m <= 12; m++) {
                int dim = days_in_month(y, m);
                for (int d = 1; d <= dim; d++) {
                    if (entries[e].is_solar) {
                        gregorian_to_solar(y, m, d, &loc, entries[e].solar_type);
                    } else {
                        gregorian_to_hindu(y, m, d, &loc);
                    }
                    count++;
                }
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &t1);
        double secs = elapsed_sec(&t0, &t1);
        double us_per_day = (secs / count) * 1e6;

        printf("%-10s : %6ld days in %7.3fs  (%5.1f us/day)\n",
               entries[e].name, count, secs, us_per_day);

        total_time += secs;
        total_calls += count;
    }

    printf("%-10s : %6ld calls in %7.3fs\n", "Total", total_calls, total_time);

    return 0;
}
