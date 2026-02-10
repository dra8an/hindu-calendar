#include "masa.h"
#include "tithi.h"
#include "astro.h"
#include "date_utils.h"
#include <math.h>
#include <stdio.h>

/*
 * Inverse Lagrange interpolation.
 * Given n data points (x[i], y[i]), find x value where y = ya.
 */
static double inverse_lagrange(const double *x, const double *y, int n, double ya)
{
    double total = 0.0;
    for (int i = 0; i < n; i++) {
        double numer = 1.0;
        double denom = 1.0;
        for (int j = 0; j < n; j++) {
            if (j != i) {
                numer *= (ya - y[j]);
                denom *= (y[i] - y[j]);
            }
        }
        total += numer * x[i] / denom;
    }
    return total;
}

/*
 * Unwrap angles: add 360 to elements so they are monotonically increasing.
 * Modifies the array in place.
 */
static void unwrap_angles(double *angles, int n)
{
    for (int i = 1; i < n; i++) {
        if (angles[i] < angles[i - 1])
            angles[i] += 360.0;
    }
}

double new_moon_before(double jd_ut, int tithi_hint)
{
    /* Approximate start: go back roughly tithi_hint days */
    double start = jd_ut - tithi_hint;

    /* Sample lunar phase at 17 points spanning (start-2) to (start+2) */
    double x[17], y[17];
    for (int i = 0; i < 17; i++) {
        x[i] = -2.0 + i * 0.25;
        y[i] = lunar_phase(start + x[i]);
    }
    unwrap_angles(y, 17);

    /* New moon = lunar phase of 360 degrees (= 0 wrapped) */
    double y0 = inverse_lagrange(x, y, 17, 360.0);
    return start + y0;
}

double new_moon_after(double jd_ut, int tithi_hint)
{
    /* Approximate start: go forward roughly (30 - tithi_hint) days */
    double start = jd_ut + (30 - tithi_hint);

    double x[17], y[17];
    for (int i = 0; i < 17; i++) {
        x[i] = -2.0 + i * 0.25;
        y[i] = lunar_phase(start + x[i]);
    }
    unwrap_angles(y, 17);

    double y0 = inverse_lagrange(x, y, 17, 360.0);
    return start + y0;
}

int solar_rashi(double jd_ut)
{
    double nirayana = solar_longitude_sidereal(jd_ut);
    int rashi = (int)ceil(nirayana / 30.0);
    if (rashi <= 0) rashi = 12;
    if (rashi > 12) rashi = rashi % 12;
    if (rashi == 0) rashi = 12;
    return rashi;
}

MasaInfo masa_for_date(int year, int month, int day, const Location *loc)
{
    MasaInfo info = {0};

    double jd = gregorian_to_jd(year, month, day);
    double jd_rise = sunrise_jd(jd, loc);
    if (jd_rise <= 0) {
        jd_rise = jd + 0.5 - loc->utc_offset / 24.0;
    }

    /* Get tithi at sunrise for search hint */
    int t = tithi_at_moment(jd_rise);

    /* Find the new moons bracketing this date */
    double last_nm = new_moon_before(jd_rise, t);
    double next_nm = new_moon_after(jd_rise, t);

    info.jd_start = last_nm;
    info.jd_end = next_nm;

    /* Determine rashi at each new moon */
    int rashi_last = solar_rashi(last_nm);
    int rashi_next = solar_rashi(next_nm);

    /* Adhika (leap) month: same rashi at both new moons */
    info.is_adhika = (rashi_last == rashi_next) ? 1 : 0;

    /* Month name = rashi + 1 (Mesha=1 → Chaitra=1, etc.)
     * The mapping is: the month is named after the rashi that
     * the sun is in (or will next enter) during this lunar month.
     * Specifically: month_name = rashi_at_last_new_moon + 1 (mod 12) */
    int masa_num = rashi_last + 1;
    if (masa_num > 12) masa_num -= 12;
    info.name = (MasaName)masa_num;

    /* Year determination */
    info.year_saka = hindu_year_saka(jd_rise, masa_num);
    info.year_vikram = hindu_year_vikram(info.year_saka);

    return info;
}

int hindu_year_saka(double jd_ut, int masa_num)
{
    /* Saka era starts in 78 CE.
     * The Hindu new year starts with Chaitra (masa 1).
     * For months Chaitra onwards (1-12) in a given Gregorian year,
     * the Saka year is approximately greg_year - 78.
     * But Chaitra typically starts in March/April, so for Jan-March
     * of a Gregorian year, we may still be in the previous Saka year. */
    int gy, gm, gd;
    jd_to_gregorian(jd_ut, &gy, &gm, &gd);

    /* Kali Ahargana method (from Python reference) */
    double sidereal_year = 365.25636;
    double ahar = jd_ut - 588465.5;  /* days since Kali epoch */
    int kali = (int)((ahar + (4 - masa_num) * 30) / sidereal_year);
    int saka = kali - 3179;

    return saka;
}

int hindu_year_vikram(int saka_year)
{
    return saka_year + 135;
}
