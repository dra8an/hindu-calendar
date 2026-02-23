/* Test: compare disc-center vs disc-edge sunrise for the 35 mismatch dates.
 * Shows whether switching to disc edge resolves mismatches with drikpanchang.
 *
 * Disc center = production moshier_sunrise (Sinclair refraction, h0 ~ -0.612°)
 * Disc edge   = disc center h0 minus solar semi-diameter (~0.266°) */
#include <stdio.h>
#include <math.h>
#include "moshier.h"

extern double moshier_lunar_longitude(double jd_ut);
extern double moshier_solar_longitude(double jd_ut);
extern double moshier_julday(int year, int month, int day, double hour);
extern void   moshier_revjul(double jd, int *y, int *m, int *d, double *h);
extern double moshier_sunrise(double jd_ut, double lon, double lat, double alt);

/* We need access to internal helpers to build a configurable-h0 sunrise */
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

/* Sunrise with configurable h0, using proper 0h UT reference.
 * jd_start = any JD before the expected sunrise (e.g. midnight IST). */
static double sunrise_h0(double jd_start, double h0) {
    /* Find 0h UT of the UT date */
    int yr, mo, dy;
    double hr;
    moshier_revjul(jd_start, &yr, &mo, &dy, &hr);
    double jd_0h = moshier_julday(yr, mo, dy, 0.0);

    double phi = LAT * DEG2RAD;

    /* Apparent sidereal time at 0h UT (GAST) */
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
    /* If result is before jd_start, try next day */
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

static void jd_to_ist(double jd, int *h, int *m, int *s) {
    double local = jd + 5.5 / 24.0 + 0.5;
    double frac = local - floor(local);
    double secs = frac * 86400.0;
    *h = (int)(secs / 3600.0);
    *m = (int)(fmod(secs, 3600.0) / 60.0);
    *s = (int)(fmod(secs, 60.0));
}

/* Sinclair refraction (same as moshier_rise.c) */
static double sinclair_h0(void) {
    double r = 34.46;
    double atpress = 1013.25, attemp = 0.0;
    r = ((atpress - 80.0) / 930.0 / (1.0 + 0.00008 * (r + 39.0) * (attemp - 10.0)) * r) / 60.0;
    return -r;
}

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

    /* Production disc-center h0 (Sinclair refraction) */
    double h0_center = sinclair_h0();
    /* Disc edge: subtract solar semi-diameter (~0.266°) */
    double h0_edge = h0_center - 0.266;

    printf("Sinclair h0 = %.4f°, disc edge h0 = %.4f°\n\n", h0_center, h0_edge);

    int prod_match = 0, center_match = 0, edge_match = 0;

    printf("%-12s  Drik  Prod  Center Edge   ProdSR     CenterSR   EdgeSR     ProdDiff  EdgeDiff  Prod=Drik  Edge=Drik\n", "Date");
    printf("%-12s  ----  ----  ------ ----   ------     --------   ------     --------  --------  ---------  ---------\n", "----------");

    for (int i = 0; i < n; i++) {
        int y = DATES[i].y, m = DATES[i].m, d = DATES[i].d;
        int drik = DATES[i].drik;

        double jd = moshier_julday(y, m, d, 0.0);
        double jd_start = jd - 5.5 / 24.0;  /* midnight IST */

        /* Production sunrise (disc center, from moshier_rise.c) */
        double sr_prod = moshier_sunrise(jd_start, LON, LAT, 0.0);

        /* Reimplemented center (should match production) */
        double sr_center = sunrise_h0(jd_start, h0_center);

        /* Disc edge */
        double sr_edge = sunrise_h0(jd_start, h0_edge);

        int t_prod   = tithi_at(sr_prod);
        int t_center = tithi_at(sr_center);
        int t_edge   = tithi_at(sr_edge);

        /* Difference from production in seconds */
        double prod_vs_center = (sr_prod - sr_center) * 86400.0;
        double edge_vs_prod = (sr_edge - sr_prod) * 86400.0;

        int ph, pm2, ps, ch, cm2, cs, eh, em2, es;
        jd_to_ist(sr_prod, &ph, &pm2, &ps);
        jd_to_ist(sr_center, &ch, &cm2, &cs);
        jd_to_ist(sr_edge, &eh, &em2, &es);

        int p_eq_d = (t_prod == drik);
        int e_eq_d = (t_edge == drik);
        if (p_eq_d) prod_match++;
        if (e_eq_d) edge_match++;

        printf("%04d-%02d-%02d    %2d    %2d    %2d    %2d   %02d:%02d:%02d  %02d:%02d:%02d  %02d:%02d:%02d  %+6.1fs   %+6.1fs    %s          %s\n",
               y, m, d, drik, t_prod, t_center, t_edge,
               ph, pm2, ps, ch, cm2, cs, eh, em2, es,
               prod_vs_center, edge_vs_prod,
               p_eq_d ? "  " : "X ", e_eq_d ? "  " : "X ");
    }

    printf("\nProduction (disc center) matches drikpanchang: %d/%d\n", prod_match, n);
    printf("Disc edge               matches drikpanchang: %d/%d\n", edge_match, n);
    printf("Disc edge resolves: %d of %d mismatches\n", edge_match - prod_match, n);

    return 0;
}
