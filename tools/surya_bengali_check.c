/*
 * surya_bengali_check.c - Validate the Surya Siddhanta Bengali rule in C.
 *
 * Walks every sankranti in 1900-2050, applies the fitted critical-time rule,
 * and compares the resulting month-start dates against the drikpanchang scrape
 * in validation/drikpanchang/bengali_suryasiddhanta.csv.
 *
 * This validates src/surya_siddhanta.c plus the rule BEFORE wiring either into
 * solar.c, so integration starts from a known-good base.
 *
 * Build:
 *   cc -O2 -std=c99 -Ilib/moshier -Isrc -o tools/surya_bengali_check \
 *      tools/surya_bengali_check.c src/surya_siddhanta.c src/astro.c \
 *      src/date_utils.c lib/moshier/moshier_[*].c -lm
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "astro.h"
#include "date_utils.h"
#include "surya_siddhanta.h"
#include "types.h"

#define CSV "validation/drikpanchang/bengali_suryasiddhanta.csv"

/* Fitted parameters -- see Docs/SURYASIDDHANTA_PANJIKA.md section 4. */
static const double CRIT_MIN = 40.0;
static const double DAY_EDGE_MIN[13] = {
    0.0,          /* unused (rashi is 1-based) */
    7.0, 11.0, 0.0, 0.0, 0.0, 12.0, 20.0, 22.0, 19.0, 0.0, 0.0, 0.0
};

/* ---- drikpanchang month starts, as a sorted set of Gregorian dates ---- */

#define MAX_ROWS 4096
static long drik_days[MAX_ROWS];
static int drik_n = 0;

static long day_number(int y, int m, int d)
{
    return (long)floor(gregorian_to_jd(y, m, d) + 0.5);
}

static int cmp_long(const void *a, const void *b)
{
    long x = *(const long *)a, y = *(const long *)b;
    return (x > y) - (x < y);
}

static int drik_has(long day)
{
    return bsearch(&day, drik_days, drik_n, sizeof(long), cmp_long) != NULL;
}

static int load_csv(void)
{
    FILE *f = fopen(CSV, "r");
    char line[512];
    if (!f) { fprintf(stderr, "cannot open %s\n", CSV); return 0; }
    if (!fgets(line, sizeof line, f)) { fclose(f); return 0; }  /* header */
    while (fgets(line, sizeof line, f)) {
        int mo, yr, len, gy, gm, gd;
        if (sscanf(line, "%d,%d,%d,%d,%d,%d", &mo, &yr, &len, &gy, &gm, &gd) == 6) {
            if (drik_n < MAX_ROWS)
                drik_days[drik_n++] = day_number(gy, gm, gd);
        }
    }
    fclose(f);
    qsort(drik_days, drik_n, sizeof(long), cmp_long);
    return drik_n;
}

/* ---- The rule ---- */

static long predict_start(double jd_sank, int rashi, const Location *loc)
{
    /* Local midnight-based day value of the sankranti. */
    double local = jd_sank + loc->utc_offset / 24.0 + 0.5;
    double shifted = local + DAY_EDGE_MIN[rashi] / 1440.0;
    long day = (long)floor(shifted);
    double crit = (double)day + CRIT_MIN / 1440.0;
    int push;

    if (local > crit)
        return day + 1;

    if (rashi == 4)         push = 0;   /* Karkata: always this day */
    else if (rashi == 10)   push = 1;   /* Makara:  always next day */
    else {
        /* Tithi at sunrise of the preceding civil day (Sewell & Dikshit). */
        int py, pm, pd;
        double jd_prev, sr, tend;
        jd_to_gregorian((double)(day - 1) - 0.5, &py, &pm, &pd);
        jd_prev = gregorian_to_jd(py, pm, pd);
        sr = sunrise_jd(jd_prev, loc);
        tend = surya_tithi_end(sr, loc);
        push = (tend <= jd_sank);
    }
    return day + (push ? 1 : 0);
}

int main(void)
{
    Location loc = DEFAULT_LOCATION;
    double jd, jd_end;
    int matched = 0, missed = 0;

    astro_init(NULL);

    if (!load_csv()) { astro_close(); return 1; }
    printf("drikpanchang month starts loaded: %d\n", drik_n);

    jd = gregorian_to_jd(1899, 12, 1);
    jd_end = gregorian_to_jd(2050, 12, 31);

    while (jd < jd_end) {
        double lon = surya_solar_longitude(jd, &loc);
        double target = 30.0 * (floor(lon / 30.0) + 1.0);
        double jd_sank;
        int rashi;
        long start;

        if (target >= 360.0) target = 0.0;
        jd_sank = surya_sankranti(jd, target, &loc);
        if (jd_sank > jd_end) break;

        rashi = (int)(target / 30.0) + 1;
        if (rashi > 12) rashi = 1;

        start = predict_start(jd_sank, rashi, &loc);

        /* Only score sankrantis whose predicted start lies inside the data. */
        if (start >= drik_days[0] && start <= drik_days[drik_n - 1]) {
            if (drik_has(start)) {
                matched++;
            } else {
                int gy, gm, gd;
                double local = jd_sank + loc.utc_offset / 24.0 + 0.5;
                double frac = local - floor(local);
                int secs = (int)(frac * 86400.0 + 0.5);
                jd_to_gregorian(jd_sank, &gy, &gm, &gd);
                missed++;
                printf("  MISS rashi %2d  sankranti %04d-%02d-%02d %02d:%02d:%02d\n",
                       rashi, gy, gm, gd, secs / 3600, secs % 3600 / 60, secs % 60);
            }
        }
        jd = jd_sank + 1.0;
    }

    printf("\nmatched %d / %d  (%.3f%%)   missed %d\n",
           matched, matched + missed,
           100.0 * matched / (matched + missed), missed);

    astro_close();
    return missed ? 1 : 0;
}
