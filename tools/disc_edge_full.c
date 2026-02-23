/* Generate tithi reference CSV using disc-edge sunrise.
 * Output format matches drikpanchang parsed CSV: year,month,day,tithi
 * Usage: ./disc_edge_full > disc_edge_ref.csv */
#include <stdio.h>
#include <math.h>
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

/* Sinclair refraction (same as moshier_rise.c) */
static double sinclair_h0(void) {
    double r = 34.46;
    double atpress = 1013.25, attemp = 0.0;
    r = ((atpress - 80.0) / 930.0 / (1.0 + 0.00008 * (r + 39.0) * (attemp - 10.0)) * r) / 60.0;
    return -r;
}

/* Sunrise with configurable h0, using proper 0h UT reference */
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
        /* Try next UT day */
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

static int days_in_month(int year, int month) {
    static const int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0))
        return 29;
    return mdays[month];
}

int main(void) {
    double h0_edge = sinclair_h0() - 0.266;  /* disc edge */

    fprintf(stderr, "Generating disc-edge tithi CSV (h0 = %.4f°)...\n", h0_edge);
    printf("year,month,day,tithi\n");

    for (int y = 1900; y <= 2050; y++) {
        for (int m = 1; m <= 12; m++) {
            int ndays = days_in_month(y, m);
            for (int d = 1; d <= ndays; d++) {
                double jd = moshier_julday(y, m, d, 0.0);
                double jd_start = jd - 5.5 / 24.0;  /* midnight IST */
                double sr = sunrise_h0(jd_start, h0_edge);

                double moon = moshier_lunar_longitude(sr);
                double sun  = moshier_solar_longitude(sr);
                double phase = fmod(moon - sun, 360.0);
                if (phase < 0) phase += 360.0;
                int tithi = (int)(phase / 12.0) + 1;
                if (tithi > 30) tithi = 30;

                printf("%d,%d,%d,%d\n", y, m, d, tithi);
            }
        }
        fprintf(stderr, "Year %d done\n", y);
    }

    fprintf(stderr, "Done.\n");
    return 0;
}
