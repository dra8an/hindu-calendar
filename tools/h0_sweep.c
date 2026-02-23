/* Find the optimal h0 (refraction angle) that minimizes drikpanchang mismatches.
 *
 * For each of the 48 boundary dates (35 disc-center mismatches + 13 disc-edge
 * regressions), binary-search for the exact h0 where the tithi flips.
 * Then determine if there's a "sweet spot" h0 range where all dates are correct.
 *
 * Also does a full 55,152-date sweep at the optimal h0 to check for collateral. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "moshier.h"

extern double moshier_lunar_longitude(double jd_ut);
extern double moshier_solar_longitude(double jd_ut);
extern double moshier_julday(int year, int month, int day, double hour);
extern void   moshier_revjul(double jd, int *y, int *m, int *d, double *h);
extern double moshier_solar_declination(double jd_ut);
extern double moshier_solar_ra(double jd_ut);
extern double moshier_nutation_longitude(double jd_ut);
extern double moshier_mean_obliquity(double jd_ut);

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)
#define LAT 28.6139
#define LON 77.2090

static double normalize_deg(double d) {
    d = fmod(d, 360.0);
    if (d < 0) d += 360.0;
    return d;
}

static double sidereal_time_0h(double jd_0h) {
    double T = (jd_0h - 2451545.0) / 36525.0;
    return normalize_deg(100.46061837 + 36000.770053608*T + 0.000387933*T*T - T*T*T/38710000.0);
}

static double sunrise_h0(double jd_start, double h0) {
    int yr, mo, dy;
    double hr;
    moshier_revjul(jd_start, &yr, &mo, &dy, &hr);
    double jd_0h = moshier_julday(yr, mo, dy, 0.0);

    double phi = LAT * DEG2RAD;
    double theta0 = sidereal_time_0h(jd_0h);
    double jd_noon = jd_0h + 0.5;
    double dpsi = moshier_nutation_longitude(jd_noon);
    double eps = moshier_mean_obliquity(jd_noon);
    theta0 += dpsi * cos(eps * DEG2RAD);

    double ra = moshier_solar_ra(jd_noon);
    double decl = moshier_solar_declination(jd_noon);
    double cos_H0 = (sin(h0 * DEG2RAD) - sin(phi) * sin(decl * DEG2RAD))
                  / (cos(phi) * cos(decl * DEG2RAD));
    if (cos_H0 < -1.0 || cos_H0 > 1.0) return 0.0;
    double H0_deg = acos(cos_H0) * RAD2DEG;

    double m0 = (ra - LON - theta0) / 360.0;
    m0 = m0 - floor(m0);
    double m = m0 - H0_deg / 360.0;
    m = m - floor(m);

    for (int iter = 0; iter < 10; iter++) {
        double jd_trial = jd_0h + m;
        double ra_i = moshier_solar_ra(jd_trial);
        double decl_i = moshier_solar_declination(jd_trial);
        double theta = theta0 + 360.985647 * m;
        double H = normalize_deg(theta + LON - ra_i);
        if (H > 180.0) H -= 360.0;
        double sin_h = sin(phi) * sin(decl_i * DEG2RAD)
                     + cos(phi) * cos(decl_i * DEG2RAD) * cos(H * DEG2RAD);
        double h = asin(sin_h) * RAD2DEG;
        double denom = 360.0 * cos(decl_i * DEG2RAD) * cos(phi) * sin(H * DEG2RAD);
        if (fabs(denom) < 1e-12) break;
        double dm = (h - h0) / denom;
        m += dm;
        if (fabs(dm) < 0.0000001) break;
    }
    if (m > 0.75) m -= 1.0;

    double result = jd_0h + m;
    if (result < jd_start - 0.0001) {
        jd_0h += 1.0;
        theta0 = sidereal_time_0h(jd_0h);
        jd_noon = jd_0h + 0.5;
        dpsi = moshier_nutation_longitude(jd_noon);
        eps = moshier_mean_obliquity(jd_noon);
        theta0 += dpsi * cos(eps * DEG2RAD);
        ra = moshier_solar_ra(jd_noon);
        decl = moshier_solar_declination(jd_noon);
        cos_H0 = (sin(h0 * DEG2RAD) - sin(phi) * sin(decl * DEG2RAD))
                / (cos(phi) * cos(decl * DEG2RAD));
        if (cos_H0 < -1.0 || cos_H0 > 1.0) return 0.0;
        H0_deg = acos(cos_H0) * RAD2DEG;
        m0 = (ra - LON - theta0) / 360.0;
        m0 = m0 - floor(m0);
        m = m0 - H0_deg / 360.0;
        m = m - floor(m);
        for (int iter = 0; iter < 10; iter++) {
            double jd_trial = jd_0h + m;
            double ra_i = moshier_solar_ra(jd_trial);
            double decl_i = moshier_solar_declination(jd_trial);
            double theta = theta0 + 360.985647 * m;
            double H = normalize_deg(theta + LON - ra_i);
            if (H > 180.0) H -= 360.0;
            double sin_h = sin(phi) * sin(decl_i * DEG2RAD)
                         + cos(phi) * cos(decl_i * DEG2RAD) * cos(H * DEG2RAD);
            double h = asin(sin_h) * RAD2DEG;
            double denom = 360.0 * cos(decl_i * DEG2RAD) * cos(phi) * sin(H * DEG2RAD);
            if (fabs(denom) < 1e-12) break;
            double dm = (h - h0) / denom;
            m += dm;
            if (fabs(dm) < 0.0000001) break;
        }
        if (m > 0.75) m -= 1.0;
        result = jd_0h + m;
    }
    return result;
}

static int tithi_at(double jd) {
    double moon = moshier_lunar_longitude(jd);
    double sun  = moshier_solar_longitude(jd);
    double phase = fmod(moon - sun, 360.0);
    if (phase < 0) phase += 360.0;
    int t = (int)(phase / 12.0) + 1;
    return t > 30 ? 30 : t;
}

static int days_in_month(int year, int month) {
    static const int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0))
        return 29;
    return mdays[month];
}

/* All 48 boundary dates: 35 disc-center mismatches + 13 disc-edge regressions.
 * drik = drikpanchang tithi (the correct answer we want to match) */
static const struct { int y, m, d, drik; } DATES[] = {
    /* 35 disc-center mismatches (drik = what drikpanchang says) */
    {1902, 5,30, 22}, {1903, 5,18, 21}, {1908, 3,17, 14},
    {1909,10,11, 27}, {1909,12, 1, 19}, {1911, 8,26,  2},
    {1912,12,14,  5}, {1915,12, 5, 28}, {1916, 2,24, 20},
    {1920,10,12, 30}, {1924, 2, 5, 30}, {1925, 3, 3,  8},
    {1932, 5,15,  9}, {1939, 7,23,  7}, {1940, 2, 3, 25},
    {1943,12,17, 20}, {1946, 1,29, 26}, {1951, 6, 8,  3},
    {1956, 5,29, 19}, {1957, 8,28,  3}, {1965, 5, 6,  5},
    {1966, 1, 8, 16}, {1966, 8, 9, 22}, {1966,10,25, 11},
    {1968, 3,11, 11}, {1968, 5,24, 27}, {1972, 4, 1, 17},
    {1974,12,19,  5}, {1978, 9,15, 13}, {1982, 3, 7, 12},
    {1987,12,18, 27}, {2007,10, 9, 28}, {2014, 5,22, 23},
    {2045, 1,17, 30}, {2046, 5,22, 18},
    /* 13 disc-edge regressions (drik = what drikpanchang says) */
    {1929,11,26, 26}, {1930,10,31, 10}, {1936,12,29, 17},
    {2001, 1,19, 26}, {2007, 8,15,  3}, {2018, 5,19,  5},
    {2020,11, 6, 21}, {2026, 6,30, 16}, {2028, 3,11, 16},
    {2028,11,13, 27}, {2041,11,14, 22}, {2046,12,21, 24},
    {2049,10,16, 21},
};

int main(int argc, char *argv[]) {
    int n = sizeof(DATES) / sizeof(DATES[0]);

    /* Phase 1: For each boundary date, find the critical h0 where tithi flips */
    printf("=== Phase 1: Critical h0 for each boundary date ===\n\n");
    printf("%-12s  Drik  Need         CriticalH0   h0_range\n", "Date");
    printf("%-12s  ----  ----------   -----------  ---------\n", "----------");

    double need_less_neg[48];   /* dates needing h0 > critical (less negative) */
    int n_less = 0;
    double need_more_neg[48];   /* dates needing h0 < critical (more negative) */
    int n_more = 0;

    for (int i = 0; i < n; i++) {
        int y = DATES[i].y, m = DATES[i].m, d = DATES[i].d;
        int drik = DATES[i].drik;

        double jd = moshier_julday(y, m, d, 0.0);
        double jd_start = jd - 5.5 / 24.0;

        /* Check tithi at disc center and disc edge to determine direction */
        double sr_center = sunrise_h0(jd_start, -0.6123);
        int t_center = tithi_at(sr_center);
        double sr_edge = sunrise_h0(jd_start, -0.8783);
        int t_edge = tithi_at(sr_edge);

        /* Binary search for the h0 where tithi flips */
        double h0_lo = -0.90;   /* more negative = earlier sunrise */
        double h0_hi = -0.55;   /* less negative = later sunrise */

        /* Determine which direction we need to go */
        int t_lo = tithi_at(sunrise_h0(jd_start, h0_lo));
        int t_hi = tithi_at(sunrise_h0(jd_start, h0_hi));

        /* Find the flip point */
        double h0_crit = -999.0;
        if (t_lo != t_hi) {
            for (int iter = 0; iter < 60; iter++) {
                double h0_mid = (h0_lo + h0_hi) / 2.0;
                int t_mid = tithi_at(sunrise_h0(jd_start, h0_mid));
                if (t_mid == t_lo)
                    h0_lo = h0_mid;
                else
                    h0_hi = h0_mid;
            }
            h0_crit = (h0_lo + h0_hi) / 2.0;
        }

        /* Determine: does drik need h0 more negative or less negative than critical? */
        const char *need;
        if (h0_crit < -900.0) {
            need = "NO FLIP";  /* tithi doesn't change across the range */
        } else {
            /* Test: which side of h0_crit gives the drik answer? */
            int t_below = tithi_at(sunrise_h0(jd_start, h0_crit - 0.01));
            if (t_below == drik) {
                need = "h0 < crit";  /* need more negative (earlier sunrise) */
                need_more_neg[n_more++] = h0_crit;
            } else {
                need = "h0 > crit";  /* need less negative (later sunrise) */
                need_less_neg[n_less++] = h0_crit;
            }
        }

        printf("%04d-%02d-%02d    %2d    %-11s  %+.6f°  (center=%d edge=%d)\n",
               y, m, d, drik, need, h0_crit, t_center, t_edge);
    }

    /* Phase 2: Find sweet spot */
    printf("\n=== Phase 2: Sweet spot analysis ===\n\n");

    /* For "h0 < crit" dates: h0 must be MORE negative than the LEAST negative critical */
    double max_need_more = -999.0;
    for (int i = 0; i < n_more; i++)
        if (need_more_neg[i] > max_need_more) max_need_more = need_more_neg[i];

    /* For "h0 > crit" dates: h0 must be LESS negative than the MOST negative critical */
    double min_need_less = 0.0;
    for (int i = 0; i < n_less; i++)
        if (need_less_neg[i] < min_need_less) min_need_less = need_less_neg[i];

    printf("Dates needing h0 MORE negative: %d (most constraining: %.6f°)\n",
           n_more, max_need_more);
    printf("Dates needing h0 LESS negative: %d (most constraining: %.6f°)\n",
           n_less, min_need_less);

    if (max_need_more < min_need_less) {
        printf("\nSweet spot EXISTS: h0 in (%.6f°, %.6f°)\n",
               max_need_more, min_need_less);
        printf("Optimal h0 (midpoint): %.6f°\n",
               (max_need_more + min_need_less) / 2.0);
    } else {
        printf("\nNO sweet spot — constraints conflict.\n");
        printf("  Need h0 < %.6f° for %d dates\n", max_need_more, n_more);
        printf("  Need h0 > %.6f° for %d dates\n", min_need_less, n_less);
    }

    /* Phase 3: Brute-force sweep of h0 values */
    printf("\n=== Phase 3: h0 sweep (boundary dates only) ===\n\n");
    printf("h0          Correct/48  Mismatches\n");
    printf("----------  ----------  ----------\n");

    int best_correct = 0;
    double best_h0 = -0.6123;

    for (int step = 0; step <= 100; step++) {
        double h0 = -0.55 - step * 0.004;  /* -0.55 to -0.95 in 0.004 steps */
        int correct = 0;

        for (int i = 0; i < n; i++) {
            double jd = moshier_julday(DATES[i].y, DATES[i].m, DATES[i].d, 0.0);
            double jd_start = jd - 5.5 / 24.0;
            double sr = sunrise_h0(jd_start, h0);
            int t = tithi_at(sr);
            if (t == DATES[i].drik) correct++;
        }

        if (correct > best_correct || (correct == best_correct && fabs(h0 + 0.7) < fabs(best_h0 + 0.7))) {
            best_correct = correct;
            best_h0 = h0;
        }

        if (step % 5 == 0)
            printf("%.4f°     %2d/48       %2d\n", h0, correct, n - correct);
    }

    printf("\nBest h0 = %.4f° with %d/%d correct (%d mismatches)\n",
           best_h0, best_correct, n, n - best_correct);

    /* Phase 4: Full 55,152-date check at best h0 (or override) */
    if (argc > 1 && (strcmp(argv[1], "--full") == 0 || strcmp(argv[1], "--h0") == 0)) {
        double full_h0 = best_h0;
        if (strcmp(argv[1], "--h0") == 0 && argc > 2)
            full_h0 = atof(argv[2]);
        printf("\n=== Phase 4: Full 55,152-date check at h0 = %.6f° ===\n\n", full_h0);
        best_h0 = full_h0;
        printf("year,month,day,tithi\n");

        /* Write to file for comparison */
        FILE *f = fopen("/tmp/h0_sweep_ref.csv", "w");
        if (f) fprintf(f, "year,month,day,tithi\n");

        int total = 0;
        for (int y = 1900; y <= 2050; y++) {
            for (int m2 = 1; m2 <= 12; m2++) {
                int ndays = days_in_month(y, m2);
                for (int d = 1; d <= ndays; d++) {
                    double jd = moshier_julday(y, m2, d, 0.0);
                    double jd_start = jd - 5.5 / 24.0;
                    double sr = sunrise_h0(jd_start, best_h0);
                    int t = tithi_at(sr);
                    if (f) fprintf(f, "%d,%d,%d,%d\n", y, m2, d, t);
                    total++;
                }
            }
            fprintf(stderr, "Year %d done\n", y);
        }

        if (f) {
            fclose(f);
            fprintf(stderr, "Wrote %d dates to /tmp/h0_sweep_ref.csv\n", total);
        }
    }

    return 0;
}
