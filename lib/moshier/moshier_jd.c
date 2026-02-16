/*
 * moshier_jd.c — Julian Day <-> Gregorian calendar conversion
 *
 * Algorithms from Meeus, "Astronomical Algorithms", 2nd ed., Ch. 7.
 */
#include "moshier.h"
#include <math.h>

double moshier_julday(int year, int month, int day, double hour)
{
    int y = year, m = month;
    if (m <= 2) {
        y -= 1;
        m += 12;
    }
    int A = y / 100;
    int B = 2 - A + A / 4;
    return floor(365.25 * (y + 4716)) + floor(30.6001 * (m + 1))
           + day + hour / 24.0 + B - 1524.5;
}

void moshier_revjul(double jd, int *year, int *month, int *day, double *hour)
{
    double Z, F, A, alpha, B, C, D, E;
    jd += 0.5;
    Z = floor(jd);
    F = jd - Z;
    if (Z < 2299161.0) {
        A = Z;
    } else {
        alpha = floor((Z - 1867216.25) / 36524.25);
        A = Z + 1 + alpha - floor(alpha / 4.0);
    }
    B = A + 1524;
    C = floor((B - 122.1) / 365.25);
    D = floor(365.25 * C);
    E = floor((B - D) / 30.6001);
    double d = B - D - floor(30.6001 * E) + F;
    *day = (int)d;
    *hour = (d - *day) * 24.0;
    if (E < 14)
        *month = (int)E - 1;
    else
        *month = (int)E - 13;
    if (*month > 2)
        *year = (int)C - 4716;
    else
        *year = (int)C - 4715;
}

int moshier_day_of_week(double jd)
{
    /* SE convention: 0=Mon, 1=Tue, ..., 6=Sun
     * Same formula as swe_day_of_week() */
    return (((int)floor(jd - 2433282 - 1.5) % 7) + 7) % 7;
}
