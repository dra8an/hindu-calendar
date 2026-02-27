/*
 * bengali_buffer_sweep.c -- Sweep Bengali critical-time buffer to find optimal value.
 *
 * For each buffer from 0 to 30 min (0.5 min steps), modifies the Bengali
 * critical time to midnight + (24 - buffer) min, recomputes all 1,811
 * Bengali month start dates (1900-2050), and compares against drikpanchang.
 *
 * Algorithm:
 *   1. Find all sankrantis (rashi 1..12) for Gregorian years 1900-2050.
 *   2. For each buffer value, compute the civil day for each sankranti
 *      using the modified critical time and the Bengali tithi-based rule.
 *   3. Compare (month, year, greg_date) against the drikpanchang CSV.
 *   4. Print a summary table of buffer vs mismatch count.
 *
 * Build:
 *   cc -O2 -Isrc -Ilib/moshier tools/bengali_buffer_sweep.c \
 *      src/panchang.c src/solar.c src/astro.c src/date_utils.c \
 *      src/tithi.c src/masa.c lib/moshier/moshier_*.c \
 *      -lm -o tools/bengali_buffer_sweep
 *
 * Run:
 *   ./tools/bengali_buffer_sweep
 */

#include "astro.h"
#include "solar.h"
#include "date_utils.h"
#include "tithi.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ---- Constants ---- */
#define MAX_SANKRANTIS 2000
#define MAX_DRIK_ROWS  2000
#define IST_OFFSET     5.5   /* hours */

/* ---- Drikpanchang reference data ---- */
typedef struct {
    int month;      /* Bengali month (1-12) */
    int year;       /* Bengali year (Bangabda) */
    int length;     /* Month length in days */
    int greg_year;
    int greg_month;
    int greg_day;
    char month_name[32];
} DrikRow;

static DrikRow drik_data[MAX_DRIK_ROWS];
static int drik_count = 0;

static void load_drikpanchang_csv(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "ERROR: cannot open %s\n", path);
        exit(1);
    }

    char line[256];
    /* Skip header */
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return;
    }

    while (fgets(line, sizeof(line), f)) {
        DrikRow *r = &drik_data[drik_count];
        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%31s",
                   &r->month, &r->year, &r->length,
                   &r->greg_year, &r->greg_month, &r->greg_day,
                   r->month_name) >= 6) {
            drik_count++;
            if (drik_count >= MAX_DRIK_ROWS) break;
        }
    }
    fclose(f);
    fprintf(stderr, "Loaded %d drikpanchang rows\n", drik_count);
}

/* ---- Sankranti data ---- */
typedef struct {
    int rashi;             /* 1-12 */
    double jd_sankranti;   /* Exact JD UT */
    int greg_year;         /* Approximate Gregorian year */
} SankrantiEntry;

static SankrantiEntry sankrantis[MAX_SANKRANTIS];
static int sank_count = 0;

static void find_all_sankrantis(void)
{
    /* For each Gregorian year 1899-2051, find all 12 sankrantis.
     * We start from 1899 because the first months of 1900 in Bengali
     * calendar correspond to sankrantis in late 1899. */
    for (int gy = 1899; gy <= 2051; gy++) {
        for (int rashi = 1; rashi <= 12; rashi++) {
            double target_long = (double)(rashi - 1) * 30.0;
            /* Approximate Gregorian month for this rashi: Mesha~Apr, Vrisha~May, etc. */
            int approx_month = 3 + rashi;
            int est_year = gy;
            if (approx_month > 12) {
                approx_month -= 12;
                est_year++;
            }
            double jd_est = gregorian_to_jd(est_year, approx_month, 14);
            double jd_sank = sankranti_jd(jd_est, target_long);

            /* Check it's actually in the right year range */
            double local_jd = jd_sank + IST_OFFSET / 24.0 + 0.5;
            int sy, sm, sd;
            jd_to_gregorian(floor(local_jd), &sy, &sm, &sd);

            /* Only keep sankrantis whose civil day falls in 1900-2050 range */
            if (sy < 1899 || sy > 2051) continue;

            /* Check for duplicates (same rashi, nearby JD) */
            int dup = 0;
            for (int i = sank_count - 1; i >= 0 && i >= sank_count - 15; i--) {
                if (sankrantis[i].rashi == rashi &&
                    fabs(sankrantis[i].jd_sankranti - jd_sank) < 10.0) {
                    dup = 1;
                    break;
                }
            }
            if (dup) continue;

            if (sank_count >= MAX_SANKRANTIS) {
                fprintf(stderr, "ERROR: too many sankrantis\n");
                exit(1);
            }
            sankrantis[sank_count].rashi = rashi;
            sankrantis[sank_count].jd_sankranti = jd_sank;
            sankrantis[sank_count].greg_year = sy;
            sank_count++;
        }
    }

    /* Sort by JD */
    for (int i = 0; i < sank_count - 1; i++) {
        for (int j = i + 1; j < sank_count; j++) {
            if (sankrantis[j].jd_sankranti < sankrantis[i].jd_sankranti) {
                SankrantiEntry tmp = sankrantis[i];
                sankrantis[i] = sankrantis[j];
                sankrantis[j] = tmp;
            }
        }
    }

    fprintf(stderr, "Found %d sankrantis\n", sank_count);
}

/* ---- Bengali month computation with custom buffer ----
 *
 * Replicates sankranti_to_civil_day() logic from solar.c but with a
 * parameterized critical time: midnight + (24 - buffer) / (24*60).
 */

static void bengali_civil_day(double jd_sankranti, int rashi,
                               double buffer_min, const Location *loc,
                               int *gy, int *gm, int *gd)
{
    /* Convert sankranti JD (UT) to local IST date */
    double local_jd = jd_sankranti + loc->utc_offset / 24.0 + 0.5;
    int sy, sm, sd;
    jd_to_gregorian(floor(local_jd), &sy, &sm, &sd);

    /* Compute custom critical time:
     * midnight UT of the IST date, + (24 - buffer) minutes
     * jd_midnight_ut for IST date = gregorian_to_jd(sy,sm,sd) - utc_offset/24
     * Then add (24 - buffer) / (24*60) days */
    double jd_day = gregorian_to_jd(sy, sm, sd);
    double jd_midnight_ut = jd_day - loc->utc_offset / 24.0;
    double crit_offset_min = 24.0 - buffer_min;
    double crit = jd_midnight_ut + crit_offset_min / (24.0 * 60.0);

    if (jd_sankranti <= crit) {
        /* In the midnight zone: apply Bengali tithi-based rule */
        /* Karka (rashi 4): always "before midnight" -> this day */
        if (rashi == 4) {
            *gy = sy; *gm = sm; *gd = sd;
            return;
        }
        /* Makara (rashi 10): always "after midnight" -> next day */
        if (rashi == 10) {
            jd_to_gregorian(jd_day + 1.0, gy, gm, gd);
            return;
        }
        /* Others: tithi at previous day's sunrise extends past sankranti? */
        int py, pm, pd;
        jd_to_gregorian(jd_day - 1.0, &py, &pm, &pd);
        TithiInfo ti = tithi_at_sunrise(py, pm, pd, loc);
        if (ti.jd_end <= jd_sankranti) {
            /* Tithi ends before sankranti -> "after midnight" -> next day */
            jd_to_gregorian(jd_day + 1.0, gy, gm, gd);
        } else {
            /* Tithi extends past sankranti -> "before midnight" -> this day */
            *gy = sy; *gm = sm; *gd = sd;
        }
    } else {
        /* After critical time -> next day */
        jd_to_gregorian(jd_day + 1.0, gy, gm, gd);
    }
}

/* ---- Bengali month/year from rashi and civil day ---- */

static int rashi_to_bengali_month(int rashi)
{
    /* Bengali months start from Mesha (rashi 1) = Boishakh (month 1) */
    return rashi;  /* 1:1 mapping */
}

static int bengali_year(int greg_year, int greg_month, int greg_day,
                         double jd_day, double buffer_min, const Location *loc)
{
    /* Bengali year = gy - 593 on/after Mesha, gy - 594 before.
     * Find when Mesha (rashi 1) starts this Gregorian year. */
    double jd_mesha_est = gregorian_to_jd(greg_year, 4, 14);
    double jd_mesha = sankranti_jd(jd_mesha_est, 0.0);  /* target 0 deg */

    int my, mm, md;
    bengali_civil_day(jd_mesha, 1, buffer_min, loc, &my, &mm, &md);
    double jd_mesha_civil = gregorian_to_jd(my, mm, md);

    if (jd_day >= jd_mesha_civil) {
        return greg_year - 593;
    } else {
        return greg_year - 594;
    }
}

/* ---- Month-start computation ----
 *
 * For each sankranti, determine the month start date (civil day),
 * Bengali month number, and Bengali year.
 */

typedef struct {
    int bengali_month;  /* 1-12 */
    int bengali_year;   /* Bangabda */
    int greg_year;
    int greg_month;
    int greg_day;
    int rashi;
} MonthStart;

static MonthStart month_starts[MAX_SANKRANTIS];
static int month_start_count = 0;

static void compute_month_starts(double buffer_min, const Location *loc)
{
    month_start_count = 0;

    for (int i = 0; i < sank_count; i++) {
        int gy, gm, gd;
        bengali_civil_day(sankrantis[i].jd_sankranti, sankrantis[i].rashi,
                          buffer_min, loc, &gy, &gm, &gd);

        /* Skip if outside 1900-2050 range */
        if (gy < 1900 || gy > 2050) continue;
        /* Also skip if it's at the very end (2050-12-31 or later) */
        if (gy == 2050 && gm == 12 && gd > 31) continue;

        int bm = rashi_to_bengali_month(sankrantis[i].rashi);
        double jd_day = gregorian_to_jd(gy, gm, gd);
        int by = bengali_year(gy, gm, gd, jd_day, buffer_min, loc);

        month_starts[month_start_count].bengali_month = bm;
        month_starts[month_start_count].bengali_year = by;
        month_starts[month_start_count].greg_year = gy;
        month_starts[month_start_count].greg_month = gm;
        month_starts[month_start_count].greg_day = gd;
        month_starts[month_start_count].rashi = sankrantis[i].rashi;
        month_start_count++;
    }
}

/* ---- Compare against drikpanchang ---- */

typedef struct {
    int greg_year, greg_month, greg_day;
    int drik_greg_year, drik_greg_month, drik_greg_day;
    int bengali_month;
    int bengali_year;
    char month_name[32];
} MismatchInfo;

static MismatchInfo mismatches[500];

static int compare_with_drikpanchang(MismatchInfo *out_mismatches)
{
    int mismatch_count = 0;

    /* For each drikpanchang row, find the matching month_start entry
     * (same Bengali month and year) and compare Gregorian start date. */
    for (int d = 0; d < drik_count; d++) {
        DrikRow *dr = &drik_data[d];

        /* Find matching computed month start */
        int found = 0;
        for (int m = 0; m < month_start_count; m++) {
            if (month_starts[m].bengali_month == dr->month &&
                month_starts[m].bengali_year == dr->year) {
                found = 1;
                if (month_starts[m].greg_year != dr->greg_year ||
                    month_starts[m].greg_month != dr->greg_month ||
                    month_starts[m].greg_day != dr->greg_day) {
                    /* Mismatch! */
                    if (out_mismatches && mismatch_count < 500) {
                        out_mismatches[mismatch_count].greg_year = month_starts[m].greg_year;
                        out_mismatches[mismatch_count].greg_month = month_starts[m].greg_month;
                        out_mismatches[mismatch_count].greg_day = month_starts[m].greg_day;
                        out_mismatches[mismatch_count].drik_greg_year = dr->greg_year;
                        out_mismatches[mismatch_count].drik_greg_month = dr->greg_month;
                        out_mismatches[mismatch_count].drik_greg_day = dr->greg_day;
                        out_mismatches[mismatch_count].bengali_month = dr->month;
                        out_mismatches[mismatch_count].bengali_year = dr->year;
                        strncpy(out_mismatches[mismatch_count].month_name,
                                dr->month_name, 31);
                        out_mismatches[mismatch_count].month_name[31] = '\0';
                    }
                    mismatch_count++;
                }
                break;
            }
        }
        if (!found) {
            /* Could not find this month — count as mismatch */
            if (out_mismatches && mismatch_count < 500) {
                out_mismatches[mismatch_count].greg_year = 0;
                out_mismatches[mismatch_count].greg_month = 0;
                out_mismatches[mismatch_count].greg_day = 0;
                out_mismatches[mismatch_count].drik_greg_year = dr->greg_year;
                out_mismatches[mismatch_count].drik_greg_month = dr->greg_month;
                out_mismatches[mismatch_count].drik_greg_day = dr->greg_day;
                out_mismatches[mismatch_count].bengali_month = dr->month;
                out_mismatches[mismatch_count].bengali_year = dr->year;
                strncpy(out_mismatches[mismatch_count].month_name,
                        dr->month_name, 31);
                out_mismatches[mismatch_count].month_name[31] = '\0';
            }
            mismatch_count++;
        }
    }

    return mismatch_count;
}

/* ---- Main ---- */

int main(void)
{
    astro_init(NULL);
    Location delhi = DEFAULT_LOCATION;

    const char *csv_path = "scraper/data/solar/parsed/bengali.csv";
    load_drikpanchang_csv(csv_path);

    fprintf(stderr, "Finding all sankrantis 1899-2051...\n");
    find_all_sankrantis();

    printf("================================================================\n");
    printf("  Bengali Solar Calendar: Critical-Time Buffer Sweep\n");
    printf("  Location: Delhi (28.6139N, 77.2090E, UTC+5:30)\n");
    printf("  Critical time = midnight + (24 - buffer) min IST\n");
    printf("  Comparing %d month starts against drikpanchang.com\n", drik_count);
    printf("================================================================\n\n");

    /* Header */
    printf("%-10s  %-10s  %-10s  %s\n",
           "Buffer", "CritTime", "Mismatches", "Mismatched dates (ours -> drik)");
    printf("%-10s  %-10s  %-10s  %s\n",
           "(min)", "(IST)", "", "");
    printf("----------  ----------  ----------  "
           "-------------------------------------------\n");

    int best_buffer_idx = -1;
    int best_mismatches = 99999;
    double best_buffer = 0.0;

    /* Sweep buffer from 0 to 30 min in 0.5 min steps */
    for (int bi = 0; bi <= 60; bi++) {
        double buffer_min = bi * 0.5;
        double crit_offset = 24.0 - buffer_min;  /* minutes past midnight */

        compute_month_starts(buffer_min, &delhi);
        int nm = compare_with_drikpanchang(mismatches);

        /* Format critical time as HH:MM */
        int crit_h = (int)(crit_offset / 60.0);
        int crit_m = (int)(crit_offset - crit_h * 60.0);
        /* Handle negative offsets (buffer > 24 means before midnight) */
        char crit_str[32];
        if (crit_offset >= 0) {
            snprintf(crit_str, sizeof(crit_str), "%02d:%02d", crit_h, crit_m);
        } else {
            /* Before midnight: 24h + negative offset */
            double before_mid = 24.0 * 60.0 + crit_offset;
            int bh = (int)(before_mid / 60.0);
            int bm_val = (int)(before_mid - bh * 60.0);
            snprintf(crit_str, sizeof(crit_str), "%02d:%02d", bh, bm_val);
        }

        /* Build mismatch detail string */
        char detail[4096] = "";
        int dlen = 0;
        for (int i = 0; i < nm && i < 500; i++) {
            char entry[128];
            if (mismatches[i].greg_year == 0) {
                snprintf(entry, sizeof(entry), "[NOT FOUND %s %d/%d]",
                         mismatches[i].month_name,
                         mismatches[i].bengali_month,
                         mismatches[i].bengali_year);
            } else {
                snprintf(entry, sizeof(entry), "%04d-%02d-%02d->%02d-%02d",
                         mismatches[i].greg_year,
                         mismatches[i].greg_month,
                         mismatches[i].greg_day,
                         mismatches[i].drik_greg_month,
                         mismatches[i].drik_greg_day);
            }
            int elen = (int)strlen(entry);
            if (dlen + elen + 3 < (int)sizeof(detail)) {
                if (dlen > 0) {
                    detail[dlen++] = ',';
                    detail[dlen++] = ' ';
                }
                memcpy(detail + dlen, entry, elen);
                dlen += elen;
                detail[dlen] = '\0';
            }
        }

        printf("%-10.1f  %-10s  %-10d  %s\n",
               buffer_min, crit_str, nm, detail);

        if (nm < best_mismatches) {
            best_mismatches = nm;
            best_buffer = buffer_min;
            best_buffer_idx = bi;
        }
    }

    printf("\n================================================================\n");
    printf("  OPTIMAL BUFFER: %.1f min  (critical time = midnight + %.1f min)\n",
           best_buffer, 24.0 - best_buffer);
    printf("  Mismatches at optimum: %d out of %d  (%.3f%%)\n",
           best_mismatches, drik_count,
           100.0 * (double)best_mismatches / (double)drik_count);
    printf("================================================================\n\n");

    /* Print detailed mismatches at optimal buffer */
    printf("Mismatches at optimal buffer (%.1f min):\n", best_buffer);
    printf("%-4s  %-10s  %-6s  %-12s  %-12s  %-12s\n",
           "#", "Month", "Year", "Our Start", "Drik Start", "Month Name");
    printf("----  ----------  ------  ------------  ------------  ------------\n");

    compute_month_starts(best_buffer, &delhi);
    int nm_best = compare_with_drikpanchang(mismatches);
    for (int i = 0; i < nm_best && i < 500; i++) {
        printf("%-4d  %-10d  %-6d  %04d-%02d-%02d    %04d-%02d-%02d    %s\n",
               i + 1,
               mismatches[i].bengali_month,
               mismatches[i].bengali_year,
               mismatches[i].greg_year,
               mismatches[i].greg_month,
               mismatches[i].greg_day,
               mismatches[i].drik_greg_year,
               mismatches[i].drik_greg_month,
               mismatches[i].drik_greg_day,
               mismatches[i].month_name);
    }

    /* Also show current production (buffer=0, crit=midnight+24min) */
    printf("\nFor reference, current production (buffer=0, crit=midnight+24min):\n");
    compute_month_starts(0.0, &delhi);
    int nm_prod = compare_with_drikpanchang(mismatches);
    printf("  Mismatches: %d out of %d  (%.3f%%)\n",
           nm_prod, drik_count,
           100.0 * (double)nm_prod / (double)drik_count);
    for (int i = 0; i < nm_prod && i < 500; i++) {
        printf("  %d. Month %d/%d: %04d-%02d-%02d (ours) vs %04d-%02d-%02d (drik) [%s]\n",
               i + 1,
               mismatches[i].bengali_month,
               mismatches[i].bengali_year,
               mismatches[i].greg_year,
               mismatches[i].greg_month,
               mismatches[i].greg_day,
               mismatches[i].drik_greg_year,
               mismatches[i].drik_greg_month,
               mismatches[i].drik_greg_day,
               mismatches[i].month_name);
    }

    astro_close();
    return 0;
}
