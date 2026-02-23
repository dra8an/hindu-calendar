/* Diagnostic: for 35 drikpanchang mismatch dates, show sunrise time,
 * tithi at sunrise, tithi boundary times, and margin to nearest boundary.
 * Helps determine whether mismatches are due to sunrise or ephemeris differences. */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

extern double moshier_lunar_longitude(double jd_ut);
extern double moshier_solar_longitude(double jd_ut);
extern double moshier_julday(int year, int month, int day, double hour);
extern double moshier_sunrise(double jd_ut, double lon, double lat, double alt);
extern double moshier_sunset(double jd_ut, double lon, double lat, double alt);

#define LAT  28.6139
#define LON  77.2090

static double lunar_phase(double jd) {
    double moon = moshier_lunar_longitude(jd);
    double sun  = moshier_solar_longitude(jd);
    double p = fmod(moon - sun, 360.0);
    if (p < 0) p += 360.0;
    return p;
}

static int tithi_at(double jd) {
    double p = lunar_phase(jd);
    int t = (int)(p / 12.0) + 1;
    return t > 30 ? 30 : t;
}

/* Bisection to find when lunar_phase crosses target_phase */
static double find_boundary(double lo, double hi, int target_tithi) {
    double target = (target_tithi - 1) * 12.0;
    for (int i = 0; i < 50; i++) {
        double mid = (lo + hi) / 2.0;
        double p = lunar_phase(mid);
        double diff = p - target;
        if (diff > 180.0) diff -= 360.0;
        if (diff < -180.0) diff += 360.0;
        if (diff >= 0) hi = mid; else lo = mid;
    }
    return (lo + hi) / 2.0;
}

static void jd_to_ist_str(double jd, char *buf) {
    double local = jd + 5.5 / 24.0 + 0.5;
    double frac = local - floor(local);
    double secs = frac * 86400.0;
    int h = (int)(secs / 3600.0);
    int m = (int)(fmod(secs, 3600.0) / 60.0);
    int s = (int)(fmod(secs, 60.0));
    sprintf(buf, "%02d:%02d:%02d", h, m, s);
}

/* All 35 mismatch dates: year, month, day, our_tithi, drik_tithi */
static const struct { int y, m, d, our, drik; } DATES[] = {
    {1902, 5,30, 23,22}, {1903, 5,18, 22,21}, {1908, 3,17, 15,14},
    {1909,10,11, 28,27}, {1909,12, 1, 20,19}, {1911, 8,26,  3, 2},
    {1912,12,14,  6, 5}, {1915,12, 5, 29,28}, {1916, 2,24, 21,20},
    {1920,10,12,  1,30}, {1924, 2, 5,  1,30}, {1925, 3, 3,  9, 8},
    {1932, 5,15, 10, 9}, {1939, 7,23,  8, 7}, {1940, 2, 3, 26,25},
    {1943,12,17, 21,20}, {1946, 1,29, 27,26}, {1951, 6, 8,  4, 3},
    {1956, 5,29, 20,19}, {1957, 8,28,  4, 3}, {1965, 5, 6,  6, 5},
    {1966, 1, 8, 17,16}, {1966, 8, 9, 23,22}, {1966,10,25, 12,11},
    {1968, 3,11, 12,11}, {1968, 5,24, 28,27}, {1972, 4, 1, 18,17},
    {1974,12,19,  6, 5}, {1978, 9,15, 14,13}, {1982, 3, 7, 13,12},
    {1987,12,18, 28,27}, {2007,10, 9, 29,28}, {2014, 5,22, 24,23},
    {2045, 1,17, 29,30}, {2046, 5,22, 17,18},
};

int main(void) {
    int n = sizeof(DATES) / sizeof(DATES[0]);

    printf("%-12s %-10s  Our Drik  %-10s  %-10s  %8s  %s\n",
           "Date", "Sunrise", "BdyBefore", "BdyAfter", "Margin", "Notes");
    printf("%-12s %-10s  --- ----  %-10s  %-10s  %8s  %s\n",
           "----------", "--------", "--------", "--------", "------", "-----");

    for (int i = 0; i < n; i++) {
        int y = DATES[i].y, m = DATES[i].m, d = DATES[i].d;
        int our = DATES[i].our, drik = DATES[i].drik;

        double jd = moshier_julday(y, m, d, 0.0);
        double jd_start = jd - 5.5 / 24.0;
        double sr = moshier_sunrise(jd_start, LON, LAT, 0.0);

        int t = tithi_at(sr);

        /* Find the tithi boundary just before sunrise (start of current tithi) */
        double bdy_before = find_boundary(sr - 2.0, sr, t);
        /* Find the tithi boundary just after sunrise (end of current tithi) */
        int next_t = (t % 30) + 1;
        double bdy_after = find_boundary(sr, sr + 2.0, next_t);

        double margin_before = (sr - bdy_before) * 1440.0; /* minutes */
        double margin_after  = (bdy_after - sr) * 1440.0;
        double margin = fmin(margin_before, margin_after);
        const char *closest = (margin_before < margin_after) ? "start" : "end";

        /* Phase info */
        double phase = lunar_phase(sr);
        double in_tithi = fmod(phase, 12.0);  /* degrees into current tithi */

        char sr_str[16], bb_str[16], ba_str[16];
        jd_to_ist_str(sr, sr_str);
        jd_to_ist_str(bdy_before, bb_str);
        jd_to_ist_str(bdy_after, ba_str);

        /* Check previous day tithi */
        double prev_sr = moshier_sunrise(jd_start - 1.0, LON, LAT, 0.0);
        int prev_t = tithi_at(prev_sr);

        printf("%04d-%02d-%02d %s   %2d  %2d  %s   %s  %5.1f min  "
               "near %s  phase=%.3f° prev_t=%d\n",
               y, m, d, sr_str, our, drik, bb_str, ba_str,
               margin, closest, in_tithi, prev_t);
    }

    return 0;
}
