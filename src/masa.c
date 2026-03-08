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

/* Static new moon cache: consecutive days usually bracket the same pair. */
static double cached_last_nm = 0, cached_next_nm = 0;

static MasaInfo masa_compute(double jd_rise)
{
    MasaInfo info = {0};

    /* Get tithi at sunrise for search hint */
    int t = tithi_at_moment(jd_rise);

    /* Check cache: if jd_rise is between cached new moons, reuse them */
    double last_nm, next_nm;
    if (cached_last_nm > 0 && jd_rise > cached_last_nm && jd_rise < cached_next_nm) {
        last_nm = cached_last_nm;
        next_nm = cached_next_nm;
    } else {
        last_nm = new_moon_before(jd_rise, t);
        next_nm = new_moon_after(jd_rise, t);
        cached_last_nm = last_nm;
        cached_next_nm = next_nm;
    }

    info.jd_start = last_nm;
    info.jd_end = next_nm;

    /* Determine rashi at each new moon */
    int rashi_last = solar_rashi(last_nm);
    int rashi_next = solar_rashi(next_nm);

    /* Adhika (leap) month: same rashi at both new moons */
    info.is_adhika = (rashi_last == rashi_next) ? 1 : 0;

    /* Month name = rashi + 1 (Mesha=1 → Chaitra=1, etc.) */
    int masa_num = rashi_last + 1;
    if (masa_num > 12) masa_num -= 12;
    info.name = (MasaName)masa_num;

    /* Year determination */
    info.year_saka = hindu_year_saka(jd_rise, masa_num);
    info.year_vikram = hindu_year_vikram(info.year_saka);

    return info;
}

MasaInfo masa_for_date(int year, int month, int day, const Location *loc)
{
    double jd = gregorian_to_jd(year, month, day);
    double jd_rise = sunrise_jd(jd, loc);
    if (jd_rise <= 0) {
        jd_rise = jd + 0.5 - loc->utc_offset / 24.0;
    }
    return masa_compute(jd_rise);
}

MasaInfo masa_for_date_at(double jd_rise, const Location *loc)
{
    (void)loc;
    return masa_compute(jd_rise);
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

double lunisolar_month_start(MasaName masa, int saka_year, int is_adhika,
                             const Location *loc)
{
    /* Step 1: Estimate approximate Gregorian date in the target month.
     * Masa 1 (Chaitra) ≈ April, Masa 2 (Vaishakha) ≈ May, etc. */
    int gy = saka_year + 78;
    int approx_gm = (int)masa + 3;
    if (approx_gm > 12) {
        approx_gm -= 12;
        gy++;
    }

    /* Start at the 15th of the estimated month */
    int est_y = gy, est_m = approx_gm, est_d = 15;
    MasaInfo mi = masa_for_date(est_y, est_m, est_d, loc);

    /* Step 2-3: Navigate using new moon boundaries.
     * Each MasaInfo has jd_start/jd_end (new moons bracketing the month).
     * To go forward, jump past jd_end; to go backward, jump before jd_start. */
    for (int attempt = 0; attempt < 14; attempt++) {
        if (mi.name == masa && mi.is_adhika == is_adhika &&
            mi.year_saka == saka_year) {
            break;  /* Found it */
        }

        /* Determine which direction to search */
        int target_ord = saka_year * 13 + (int)masa + (is_adhika ? 0 : 1);
        int cur_ord = mi.year_saka * 13 + (int)mi.name + (mi.is_adhika ? 0 : 1);

        double jd_nav;
        if (target_ord > cur_ord) {
            /* Need to go forward — jump 1 day past end of current month */
            jd_nav = mi.jd_end + 1.0;
        } else {
            /* Need to go backward — jump 1 day before start of current month */
            jd_nav = mi.jd_start - 1.0;
        }
        jd_to_gregorian(jd_nav, &est_y, &est_m, &est_d);
        mi = masa_for_date(est_y, est_m, est_d, loc);
    }

    /* Verify we found the right month */
    if (mi.name != masa || mi.is_adhika != is_adhika ||
        mi.year_saka != saka_year) {
        return 0;  /* Not found */
    }

    /* Step 4: Find the first civil day of this month.
     * mi.jd_start is the new moon (Amavasya). The next day is typically
     * Shukla Pratipada, the first day of the new month. */
    int nm_y, nm_m, nm_d;
    jd_to_gregorian(mi.jd_start, &nm_y, &nm_m, &nm_d);

    /* Try the Amavasya day itself — if new moon is well before sunrise,
     * this day may already belong to the new month */
    MasaInfo check = masa_for_date(nm_y, nm_m, nm_d, loc);
    if (check.name == masa && check.is_adhika == is_adhika &&
        check.year_saka == saka_year) {
        return gregorian_to_jd(nm_y, nm_m, nm_d);
    }

    /* Try the next day (most common case) */
    double jd_next = gregorian_to_jd(nm_y, nm_m, nm_d) + 1;
    int ny, nmm, nd;
    jd_to_gregorian(jd_next, &ny, &nmm, &nd);
    check = masa_for_date(ny, nmm, nd, loc);
    if (check.name == masa && check.is_adhika == is_adhika &&
        check.year_saka == saka_year) {
        return jd_next;
    }

    /* Try day after that (rare edge case) */
    jd_next += 1;
    jd_to_gregorian(jd_next, &ny, &nmm, &nd);
    check = masa_for_date(ny, nmm, nd, loc);
    if (check.name == masa && check.is_adhika == is_adhika &&
        check.year_saka == saka_year) {
        return jd_next;
    }

    return 0;  /* Should not happen */
}

int lunisolar_month_length(MasaName masa, int saka_year, int is_adhika,
                           const Location *loc)
{
    double jd_start = lunisolar_month_start(masa, saka_year, is_adhika, loc);
    if (jd_start == 0) return 0;

    /* Check days 28-31 from start to find where the month changes */
    for (int d = 28; d <= 31; d++) {
        double jd = jd_start + d;
        int gy, gm, gd;
        jd_to_gregorian(jd, &gy, &gm, &gd);
        MasaInfo mi = masa_for_date(gy, gm, gd, loc);
        if (mi.name != masa || mi.is_adhika != is_adhika) {
            return d;
        }
    }

    return 0;  /* Should not happen */
}
