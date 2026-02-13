/*
 * solar_boundary_scan.c — Find the 100 closest sankrantis to each solar
 * calendar's critical time, for manual verification against drikpanchang.com.
 *
 * Scans all 1,812 sankrantis (12 × 151 years, 1900–2050) for each of the
 * 4 calendars (Tamil, Bengali, Odia, Malayalam).  For each sankranti, computes
 * delta = sankranti_jd − critical_time_jd in minutes.  Sorts by |delta| and
 * outputs two files:
 *   1. validation/solar_edge_cases.csv — human-readable for manual verification
 *   2. tests/test_solar_edge.c — compilable test with our predictions pre-filled
 *
 * Build:
 *   make && cc -O2 -Isrc -Ilib/swisseph tools/solar_boundary_scan.c \
 *       build/*.o build/swe/*.o -lm -o build/solar_boundary_scan
 *
 * Run:
 *   ./build/solar_boundary_scan
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define TOP_N        100
#define YEAR_START   1900
#define YEAR_END     2050
#define TOTAL_SANK   ((YEAR_END - YEAR_START + 1) * 12)   /* 1812 */

/* ---- Replicate the 4 critical-time rules from solar.c (they are static) ---- */

static double critical_time_jd_local(double jd_midnight_ut, const Location *loc,
                                      SolarCalendarType type)
{
    switch (type) {
    case SOLAR_CAL_TAMIL:
        return sunset_jd(jd_midnight_ut, loc);

    case SOLAR_CAL_BENGALI:
        /* midnight + 24 min buffer */
        return jd_midnight_ut - loc->utc_offset / 24.0 + 24.0 / (24.0 * 60.0);

    case SOLAR_CAL_ODIA:
        /* Fixed 22:12 IST = 16:42 UTC = 16.7h UTC */
        return jd_midnight_ut + 16.7 / 24.0;

    case SOLAR_CAL_MALAYALAM:
        {
            double sr = sunrise_jd(jd_midnight_ut, loc);
            double ss = sunset_jd(jd_midnight_ut, loc);
            return sr + 0.6 * (ss - sr);
        }
    }
    return jd_midnight_ut;
}

/* ---- Replicate sankranti_to_civil_day logic ---- */

static void sankranti_to_civil_day_local(double jd_sankranti, const Location *loc,
                                          SolarCalendarType type,
                                          int *gy, int *gm, int *gd)
{
    double local_jd = jd_sankranti + loc->utc_offset / 24.0 + 0.5;
    int sy, sm, sd;
    jd_to_gregorian(floor(local_jd), &sy, &sm, &sd);

    double jd_day = gregorian_to_jd(sy, sm, sd);
    double crit = critical_time_jd_local(jd_day, loc, type);

    if (jd_sankranti <= crit) {
        *gy = sy; *gm = sm; *gd = sd;
    } else {
        double next_jd = jd_day + 1.0;
        jd_to_gregorian(next_jd, gy, gm, gd);
    }
}

/* ---- Entry for sorting ---- */

typedef struct {
    int sank_gy, sank_gm, sank_gd;     /* Gregorian date of sankranti's local day */
    int sank_hh, sank_mm, sank_ss;      /* Sankranti time in IST */
    int crit_hh, crit_mm, crit_ss;      /* Critical time in IST */
    double delta_min;                    /* sankranti - critical time, in minutes */
    int day1_gy, day1_gm, day1_gd;      /* Our computed first day of new month */
    int our_month;                       /* Regional month number */
    int our_year;                        /* Regional era year */
    int rashi;                           /* Which rashi sun is entering (1-12) */
} Entry;

/* JD (UT) → IST components */
static void jd_to_ist(double jd_ut, int *y, int *m, int *d, int *hh, int *mm, int *ss)
{
    double local = jd_ut + 5.5 / 24.0 + 0.5;
    double day_frac = local - floor(local);
    jd_to_gregorian(floor(local), y, m, d);
    double secs = day_frac * 86400.0;
    *hh = (int)(secs / 3600.0);
    *mm = (int)(fmod(secs, 3600.0) / 60.0);
    *ss = (int)(fmod(secs, 60.0));
}

static int cmp_by_abs_delta(const void *a, const void *b)
{
    double da = fabs(((const Entry *)a)->delta_min);
    double db = fabs(((const Entry *)b)->delta_min);
    if (da < db) return -1;
    if (da > db) return  1;
    return 0;
}

/* ---- Rashi to regional month (replicate from solar.c) ---- */

static int rashi_to_regional_month(int rashi, SolarCalendarType type)
{
    int first_rashi = (type == SOLAR_CAL_MALAYALAM) ? 5 : 1;
    int m = rashi - first_rashi + 1;
    if (m <= 0) m += 12;
    return m;
}

static const char *cal_name(SolarCalendarType type)
{
    switch (type) {
    case SOLAR_CAL_TAMIL:     return "TAMIL";
    case SOLAR_CAL_BENGALI:   return "BENGALI";
    case SOLAR_CAL_ODIA:      return "ODIA";
    case SOLAR_CAL_MALAYALAM: return "MALAYALAM";
    }
    return "???";
}

static const char *cal_crit_desc(SolarCalendarType type)
{
    switch (type) {
    case SOLAR_CAL_TAMIL:     return "sunset";
    case SOLAR_CAL_BENGALI:   return "midnight+24min";
    case SOLAR_CAL_ODIA:      return "22:12 IST";
    case SOLAR_CAL_MALAYALAM: return "end of madhyahna";
    }
    return "???";
}

static const char *cal_type_enum(SolarCalendarType type)
{
    switch (type) {
    case SOLAR_CAL_TAMIL:     return "SOLAR_CAL_TAMIL";
    case SOLAR_CAL_BENGALI:   return "SOLAR_CAL_BENGALI";
    case SOLAR_CAL_ODIA:      return "SOLAR_CAL_ODIA";
    case SOLAR_CAL_MALAYALAM: return "SOLAR_CAL_MALAYALAM";
    }
    return "???";
}

/* ---- Main ---- */

int main(void)
{
    astro_init(NULL);
    Location loc = DEFAULT_LOCATION;

    /* Allocate entries for all sankrantis */
    Entry *entries = malloc(TOTAL_SANK * sizeof(Entry));
    if (!entries) { fprintf(stderr, "malloc failed\n"); return 1; }

    /* Open output files */
    FILE *csv = fopen("validation/solar_edge_cases.csv", "w");
    if (!csv) { fprintf(stderr, "Cannot open validation/solar_edge_cases.csv\n"); return 1; }

    FILE *testf = fopen("tests/test_solar_edge.c", "w");
    if (!testf) { fprintf(stderr, "Cannot open tests/test_solar_edge.c\n"); return 1; }

    /* Write test file header */
    fprintf(testf, "/*\n");
    fprintf(testf, " * test_solar_edge.c — Edge case tests for solar calendar boundary dates.\n");
    fprintf(testf, " *\n");
    fprintf(testf, " * Auto-generated by tools/solar_boundary_scan.c.\n");
    fprintf(testf, " * Contains the 100 closest sankrantis to each calendar's critical time\n");
    fprintf(testf, " * for Tamil, Bengali, Odia, and Malayalam calendars (1900-2050).\n");
    fprintf(testf, " *\n");
    fprintf(testf, " * Initially populated with OUR code's predictions. User verifies against\n");
    fprintf(testf, " * drikpanchang.com and updates exp values where our code disagrees.\n");
    fprintf(testf, " */\n\n");
    fprintf(testf, "#include \"solar.h\"\n");
    fprintf(testf, "#include \"astro.h\"\n");
    fprintf(testf, "#include \"date_utils.h\"\n");
    fprintf(testf, "#include <stdio.h>\n");
    fprintf(testf, "#include <string.h>\n");
    fprintf(testf, "#include <math.h>\n\n");
    fprintf(testf, "static int tests_run = 0;\n");
    fprintf(testf, "static int tests_passed = 0;\n");
    fprintf(testf, "static int tests_failed = 0;\n\n");
    fprintf(testf, "static void check(int condition, const char *msg)\n");
    fprintf(testf, "{\n");
    fprintf(testf, "    tests_run++;\n");
    fprintf(testf, "    if (condition) {\n");
    fprintf(testf, "        tests_passed++;\n");
    fprintf(testf, "    } else {\n");
    fprintf(testf, "        tests_failed++;\n");
    fprintf(testf, "        printf(\"  FAIL: %%s\\n\", msg);\n");
    fprintf(testf, "    }\n");
    fprintf(testf, "}\n\n");

    fprintf(testf, "/*\n");
    fprintf(testf, " * Edge case reference data: 100 closest sankrantis per calendar.\n");
    fprintf(testf, " * Each entry tests the first day of the new solar month.\n");
    fprintf(testf, " *\n");
    fprintf(testf, " * Fields:\n");
    fprintf(testf, " *   gy, gm, gd     — Gregorian date of our computed day 1\n");
    fprintf(testf, " *   type            — Solar calendar type\n");
    fprintf(testf, " *   exp_month       — Expected regional month number\n");
    fprintf(testf, " *   exp_day         — Expected day (should be 1)\n");
    fprintf(testf, " *   exp_year        — Expected regional era year\n");
    fprintf(testf, " *\n");
    fprintf(testf, " * The comment on each line shows delta (minutes from critical time),\n");
    fprintf(testf, " * sankranti IST, and critical time IST.\n");
    fprintf(testf, " */\n");
    fprintf(testf, "static struct {\n");
    fprintf(testf, "    int gy, gm, gd;\n");
    fprintf(testf, "    SolarCalendarType type;\n");
    fprintf(testf, "    int exp_month;\n");
    fprintf(testf, "    int exp_day;\n");
    fprintf(testf, "    int exp_year;\n");
    fprintf(testf, "} edge_cases[] = {\n");

    SolarCalendarType types[] = {
        SOLAR_CAL_TAMIL, SOLAR_CAL_BENGALI,
        SOLAR_CAL_ODIA, SOLAR_CAL_MALAYALAM
    };

    for (int t = 0; t < 4; t++) {
        SolarCalendarType type = types[t];
        int nentries = 0;

        fprintf(stderr, "Scanning %s...\n", cal_name(type));

        for (int gy = YEAR_START; gy <= YEAR_END; gy++) {
            for (int rashi = 1; rashi <= 12; rashi++) {
                double target_long = (rashi - 1) * 30.0;

                /* Approximate Gregorian month for this sankranti */
                int approx_month = 3 + rashi;
                int approx_year = gy;
                if (approx_month > 12) { approx_month -= 12; approx_year++; }

                double jd_est = gregorian_to_jd(approx_year, approx_month, 14);
                double jd_sank = sankranti_jd(jd_est, target_long);

                /* Convert sankranti to local date/time (IST) */
                int sy, sm, sd, sh, smin, ssec;
                jd_to_ist(jd_sank, &sy, &sm, &sd, &sh, &smin, &ssec);

                /* Compute critical time for that day */
                double jd_day = gregorian_to_jd(sy, sm, sd);
                double jd_crit = critical_time_jd_local(jd_day, &loc, type);

                /* Critical time in IST */
                int cy, cm, cd, ch, cmin, csec;
                jd_to_ist(jd_crit, &cy, &cm, &cd, &ch, &cmin, &csec);

                /* Delta in minutes */
                double delta_min = (jd_sank - jd_crit) * 24.0 * 60.0;

                /* Our assignment: which day is day 1? */
                int d1y, d1m, d1d;
                sankranti_to_civil_day_local(jd_sank, &loc, type, &d1y, &d1m, &d1d);

                /* Our full conversion for day 1 */
                SolarDate sdate = gregorian_to_solar(d1y, d1m, d1d, &loc, type);

                Entry *e = &entries[nentries];
                e->sank_gy = sy;  e->sank_gm = sm;  e->sank_gd = sd;
                e->sank_hh = sh;  e->sank_mm = smin; e->sank_ss = ssec;
                e->crit_hh = ch;  e->crit_mm = cmin; e->crit_ss = csec;
                e->delta_min = delta_min;
                e->day1_gy = d1y; e->day1_gm = d1m;  e->day1_gd = d1d;
                e->our_month = sdate.month;
                e->our_year = sdate.year;
                e->rashi = rashi;
                nentries++;
            }
        }

        /* Sort by |delta| */
        qsort(entries, nentries, sizeof(Entry), cmp_by_abs_delta);

        int n = (nentries < TOP_N) ? nentries : TOP_N;

        /* Write CSV section */
        fprintf(csv, "# %s - %d closest sankrantis to critical time (%s)\n",
                cal_name(type), n, cal_crit_desc(type));
        fprintf(csv, "# greg_date: Gregorian date of the sankranti's civil day\n");
        fprintf(csv, "# sank_ist: Sankranti time in IST (HH:MM:SS)\n");
        fprintf(csv, "# crit_ist: Critical time in IST (HH:MM:SS)\n");
        fprintf(csv, "# delta_min: sankranti minus critical time (negative=before, positive=after)\n");
        fprintf(csv, "# our_day1: our computed first day of new month (YYYY-MM-DD)\n");
        fprintf(csv, "# our_month: regional month number\n");
        fprintf(csv, "# our_year: regional year\n");
        fprintf(csv, "# rashi: which rashi the sun is entering (1-12)\n");
        fprintf(csv, "# exp_month,exp_day,exp_year: BLANK - user fills in from drikpanchang\n");
        fprintf(csv, "greg_date,sank_ist,crit_ist,delta_min,our_day1,our_month,our_year,rashi,exp_month,exp_day,exp_year\n");

        for (int i = 0; i < n; i++) {
            Entry *e = &entries[i];
            fprintf(csv, "%04d-%02d-%02d,%02d:%02d:%02d,%02d:%02d:%02d,%.1f,%04d-%02d-%02d,%d,%d,%d,,,\n",
                    e->sank_gy, e->sank_gm, e->sank_gd,
                    e->sank_hh, e->sank_mm, e->sank_ss,
                    e->crit_hh, e->crit_mm, e->crit_ss,
                    e->delta_min,
                    e->day1_gy, e->day1_gm, e->day1_gd,
                    e->our_month, e->our_year, e->rashi);
        }
        fprintf(csv, "\n");

        /* Write test file section */
        fprintf(testf, "\n    /* ---- %s - %d closest to %s ---- */\n",
                cal_name(type), n, cal_crit_desc(type));

        for (int i = 0; i < n; i++) {
            Entry *e = &entries[i];
            /* Build the enum string with trailing comma */
            char enum_buf[40];
            snprintf(enum_buf, sizeof(enum_buf), "%s,", cal_type_enum(type));
            fprintf(testf,
                    "    {%04d, %2d, %2d, %-22s %2d, 1, %4d},  /* delta=%+.1fmin sank=%02d:%02d:%02d crit=%02d:%02d:%02d rashi=%d */\n",
                    e->day1_gy, e->day1_gm, e->day1_gd,
                    enum_buf,
                    e->our_month, e->our_year,
                    e->delta_min,
                    e->sank_hh, e->sank_mm, e->sank_ss,
                    e->crit_hh, e->crit_mm, e->crit_ss,
                    e->rashi);
        }

        fprintf(stderr, "  %s: %d sankrantis scanned, top %d written\n",
                cal_name(type), nentries, n);
    }

    /* Close the struct array in test file */
    fprintf(testf, "};\n\n");

    /* Write test functions */
    const char *test_fn_names[] = {"tamil", "bengali", "odia", "malayalam"};
    const char *test_fn_labels[] = {"Tamil", "Bengali", "Odia", "Malayalam"};

    fprintf(testf, "static void test_edge_cases(void)\n");
    fprintf(testf, "{\n");
    fprintf(testf, "    printf(\"\\n--- Solar edge cases: boundary sankrantis ---\\n\");\n");
    fprintf(testf, "    Location delhi = DEFAULT_LOCATION;\n");
    fprintf(testf, "    int n = sizeof(edge_cases) / sizeof(edge_cases[0]);\n\n");
    fprintf(testf, "    for (int i = 0; i < n; i++) {\n");
    fprintf(testf, "        SolarDate sd = gregorian_to_solar(edge_cases[i].gy, edge_cases[i].gm,\n");
    fprintf(testf, "                                          edge_cases[i].gd, &delhi,\n");
    fprintf(testf, "                                          edge_cases[i].type);\n\n");
    fprintf(testf, "        const char *cal_names[] = {\"Tamil\", \"Bengali\", \"Odia\", \"Malayalam\"};\n");
    fprintf(testf, "        char buf[256];\n\n");
    fprintf(testf, "        /* Check month */\n");
    fprintf(testf, "        snprintf(buf, sizeof(buf), \"%%s %%04d-%%02d-%%02d month (got %%d [%%s], expected %%d [%%s])\",\n");
    fprintf(testf, "                 cal_names[edge_cases[i].type],\n");
    fprintf(testf, "                 edge_cases[i].gy, edge_cases[i].gm, edge_cases[i].gd,\n");
    fprintf(testf, "                 sd.month, solar_month_name(sd.month, edge_cases[i].type),\n");
    fprintf(testf, "                 edge_cases[i].exp_month,\n");
    fprintf(testf, "                 solar_month_name(edge_cases[i].exp_month, edge_cases[i].type));\n");
    fprintf(testf, "        check(sd.month == edge_cases[i].exp_month, buf);\n\n");
    fprintf(testf, "        /* Check day */\n");
    fprintf(testf, "        snprintf(buf, sizeof(buf), \"%%s %%04d-%%02d-%%02d day (got %%d, expected %%d)\",\n");
    fprintf(testf, "                 cal_names[edge_cases[i].type],\n");
    fprintf(testf, "                 edge_cases[i].gy, edge_cases[i].gm, edge_cases[i].gd,\n");
    fprintf(testf, "                 sd.day, edge_cases[i].exp_day);\n");
    fprintf(testf, "        check(sd.day == edge_cases[i].exp_day, buf);\n\n");
    fprintf(testf, "        /* Check year */\n");
    fprintf(testf, "        snprintf(buf, sizeof(buf), \"%%s %%04d-%%02d-%%02d year (got %%d, expected %%d)\",\n");
    fprintf(testf, "                 cal_names[edge_cases[i].type],\n");
    fprintf(testf, "                 edge_cases[i].gy, edge_cases[i].gm, edge_cases[i].gd,\n");
    fprintf(testf, "                 sd.year, edge_cases[i].exp_year);\n");
    fprintf(testf, "        check(sd.year == edge_cases[i].exp_year, buf);\n");
    fprintf(testf, "    }\n");
    fprintf(testf, "}\n\n");

    fprintf(testf, "int main(void)\n");
    fprintf(testf, "{\n");
    fprintf(testf, "    astro_init(NULL);\n\n");
    fprintf(testf, "    test_edge_cases();\n\n");
    fprintf(testf, "    astro_close();\n\n");
    fprintf(testf, "    printf(\"\\n=== Solar edge: %%d/%%d passed, %%d failed ===\\n\",\n");
    fprintf(testf, "           tests_passed, tests_run, tests_failed);\n");
    fprintf(testf, "    return (tests_failed == 0) ? 0 : 1;\n");
    fprintf(testf, "}\n");

    fclose(csv);
    fclose(testf);
    free(entries);

    fprintf(stderr, "\nDone. Files written:\n");
    fprintf(stderr, "  validation/solar_edge_cases.csv\n");
    fprintf(stderr, "  tests/test_solar_edge.c\n");

    astro_close();
    return 0;
}
