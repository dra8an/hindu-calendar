#include "panchang.h"
#include "solar.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
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
    int year, month, day;
} DateEntry;

/* Fisher-Yates shuffle */
static void shuffle(DateEntry *arr, int n, unsigned seed)
{
    srand(seed);
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        DateEntry tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

typedef struct {
    const char *name;
    int is_solar;
    SolarCalendarType solar_type;
} BenchEntry;

int main(void)
{
    Location loc = DEFAULT_LOCATION;

    /* Build date array */
    int capacity = 55200;
    DateEntry *dates = malloc(capacity * sizeof(DateEntry));
    int n = 0;

    for (int y = 1900; y <= 2050; y++) {
        for (int m = 1; m <= 12; m++) {
            int dim = days_in_month(y, m);
            for (int d = 1; d <= dim; d++) {
                if (n >= capacity) {
                    capacity *= 2;
                    dates = realloc(dates, capacity * sizeof(DateEntry));
                }
                dates[n++] = (DateEntry){y, m, d};
            }
        }
    }

    printf("=== Performance Benchmark (RANDOM order, %d days) ===\n\n", n);

    /* Shuffle with fixed seed for reproducibility */
    shuffle(dates, n, 42);

    BenchEntry entries[] = {
        {"Lunisolar", 0, 0},
        {"Tamil",     1, SOLAR_CAL_TAMIL},
        {"Bengali",   1, SOLAR_CAL_BENGALI},
        {"Odia",      1, SOLAR_CAL_ODIA},
        {"Malayalam",  1, SOLAR_CAL_MALAYALAM},
    };
    int n_entries = (int)(sizeof(entries) / sizeof(entries[0]));

    double total_time = 0.0;
    long total_calls = 0;

    for (int e = 0; e < n_entries; e++) {
        struct timespec t0, t1;

        clock_gettime(CLOCK_MONOTONIC, &t0);

        for (int i = 0; i < n; i++) {
            if (entries[e].is_solar) {
                gregorian_to_solar(dates[i].year, dates[i].month, dates[i].day,
                                   &loc, entries[e].solar_type);
            } else {
                gregorian_to_hindu(dates[i].year, dates[i].month, dates[i].day,
                                   &loc);
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &t1);
        double secs = elapsed_sec(&t0, &t1);
        double us_per_day = (secs / n) * 1e6;

        printf("%-10s : %6d days in %7.3fs  (%5.1f us/day)\n",
               entries[e].name, n, secs, us_per_day);

        total_time += secs;
        total_calls += n;
    }

    printf("%-10s : %6ld calls in %7.3fs\n", "Total", total_calls, total_time);

    free(dates);
    return 0;
}
