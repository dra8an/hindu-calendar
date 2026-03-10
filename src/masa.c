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

    /* Sample lunar phase at 9 points spanning (start-2) to (start+2).
     * 9 points with 0.5-day spacing covers a 4-day window, sufficient
     * because the new moon estimate from the tithi hint is within ~1 day. */
    double x[9], y[9];
    for (int i = 0; i < 9; i++) {
        x[i] = -2.0 + i * 0.5;
        y[i] = lunar_phase(start + x[i]);
    }
    unwrap_angles(y, 9);

    /* New moon = lunar phase of 360 degrees (= 0 wrapped) */
    double y0 = inverse_lagrange(x, y, 9, 360.0);
    return start + y0;
}

double new_moon_after(double jd_ut, int tithi_hint)
{
    /* Approximate start: go forward roughly (30 - tithi_hint) days */
    double start = jd_ut + (30 - tithi_hint);

    double x[9], y[9];
    for (int i = 0; i < 9; i++) {
        x[i] = -2.0 + i * 0.5;
        y[i] = lunar_phase(start + x[i]);
    }
    unwrap_angles(y, 9);

    double y0 = inverse_lagrange(x, y, 9, 360.0);
    return start + y0;
}

double full_moon_near(double jd_ut)
{
    /* Same interpolation approach as new_moon, but targeting 180° */
    double x[9], y[9];
    for (int i = 0; i < 9; i++) {
        x[i] = -2.0 + i * 0.5;
        y[i] = lunar_phase(jd_ut + x[i]);
    }
    unwrap_angles(y, 9);

    double y0 = inverse_lagrange(x, y, 9, 180.0);
    return jd_ut + y0;
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

/* Static new moon cache: consecutive days usually bracket the same pair.
 * Extended with rashi cache to avoid redundant solar_rashi() calls. */
static double cached_last_nm = 0, cached_next_nm = 0;
static int cached_rashi_last = 0, cached_rashi_next = 0;

static MasaInfo masa_compute(double jd_rise)
{
    MasaInfo info = {0};

    /* Get tithi at sunrise for search hint */
    int t = tithi_at_moment(jd_rise);

    /* Check cache: if jd_rise is between cached new moons, reuse them */
    double last_nm, next_nm;
    int rashi_last, rashi_next;
    if (cached_last_nm > 0 && jd_rise > cached_last_nm && jd_rise < cached_next_nm) {
        last_nm = cached_last_nm;
        next_nm = cached_next_nm;
        rashi_last = cached_rashi_last;
        rashi_next = cached_rashi_next;
    } else {
        last_nm = new_moon_before(jd_rise, t);
        next_nm = new_moon_after(jd_rise, t);
        rashi_last = solar_rashi(last_nm);
        rashi_next = solar_rashi(next_nm);
        cached_last_nm = last_nm;
        cached_next_nm = next_nm;
        cached_rashi_last = rashi_last;
        cached_rashi_next = rashi_next;
    }

    info.jd_start = last_nm;
    info.jd_end = next_nm;

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

/* ---------------------------------------------------------------------------
 * LRU cache for lunisolar_month_start / lunisolar_month_length
 * 32 entries covers ~2.5 years of months — more than enough for typical use.
 * --------------------------------------------------------------------------- */
#define LRU_MONTH_SIZE 32

typedef struct {
    MasaName masa;
    int saka_year;
    int is_adhika;
    LunisolarScheme scheme;
    double jd_start;      /* cached month start (JD at 0h UT), 0 = empty */
    int length;           /* cached month length (29 or 30), 0 = not yet computed */
    unsigned int lru_seq; /* LRU sequence counter */
} LuniMonthCache;

static LuniMonthCache lru_month[LRU_MONTH_SIZE];
static unsigned int lru_month_seq = 0;

/* Find cache entry matching (masa, saka_year, is_adhika, scheme), or NULL */
static LuniMonthCache *lru_month_find(MasaName masa, int saka_year,
                                       int is_adhika, LunisolarScheme scheme)
{
    for (int i = 0; i < LRU_MONTH_SIZE; i++) {
        LuniMonthCache *e = &lru_month[i];
        if (e->jd_start != 0 &&
            e->masa == masa && e->saka_year == saka_year &&
            e->is_adhika == is_adhika && e->scheme == scheme) {
            e->lru_seq = ++lru_month_seq;
            return e;
        }
    }
    return NULL;
}

/* Insert/update entry, evicting lowest lru_seq on full cache */
static LuniMonthCache *lru_month_insert(MasaName masa, int saka_year,
                                         int is_adhika, LunisolarScheme scheme)
{
    /* Find empty slot or LRU victim */
    LuniMonthCache *victim = &lru_month[0];
    for (int i = 0; i < LRU_MONTH_SIZE; i++) {
        LuniMonthCache *e = &lru_month[i];
        if (e->jd_start == 0) {
            victim = e;
            break;
        }
        if (e->lru_seq < victim->lru_seq)
            victim = e;
    }
    victim->masa = masa;
    victim->saka_year = saka_year;
    victim->is_adhika = is_adhika;
    victim->scheme = scheme;
    victim->jd_start = 0;
    victim->length = 0;
    victim->lru_seq = ++lru_month_seq;
    return victim;
}

/* Amanta month start — find first civil day after new moon */
static double amanta_month_start(MasaName masa, int saka_year, int is_adhika,
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

    /* Step 2-3: Navigate using new moon boundaries. */
    for (int attempt = 0; attempt < 14; attempt++) {
        if (mi.name == masa && mi.is_adhika == is_adhika &&
            mi.year_saka == saka_year) {
            break;
        }

        int target_ord = saka_year * 13 + (int)masa + (is_adhika ? 0 : 1);
        int cur_ord = mi.year_saka * 13 + (int)mi.name + (mi.is_adhika ? 0 : 1);

        double jd_nav;
        if (target_ord > cur_ord) {
            jd_nav = mi.jd_end + 1.0;
        } else {
            jd_nav = mi.jd_start - 1.0;
        }
        jd_to_gregorian(jd_nav, &est_y, &est_m, &est_d);
        mi = masa_for_date(est_y, est_m, est_d, loc);
    }

    if (mi.name != masa || mi.is_adhika != is_adhika ||
        mi.year_saka != saka_year) {
        return 0;
    }

    /* Step 4: Find the first civil day of this month. */
    int nm_y, nm_m, nm_d;
    jd_to_gregorian(mi.jd_start, &nm_y, &nm_m, &nm_d);

    MasaInfo check = masa_for_date(nm_y, nm_m, nm_d, loc);
    if (check.name == masa && check.is_adhika == is_adhika &&
        check.year_saka == saka_year) {
        return gregorian_to_jd(nm_y, nm_m, nm_d);
    }

    double jd_next = gregorian_to_jd(nm_y, nm_m, nm_d) + 1;
    int ny, nmm, nd;
    jd_to_gregorian(jd_next, &ny, &nmm, &nd);
    check = masa_for_date(ny, nmm, nd, loc);
    if (check.name == masa && check.is_adhika == is_adhika &&
        check.year_saka == saka_year) {
        return jd_next;
    }

    jd_next += 1;
    jd_to_gregorian(jd_next, &ny, &nmm, &nd);
    check = masa_for_date(ny, nmm, nd, loc);
    if (check.name == masa && check.is_adhika == is_adhika &&
        check.year_saka == saka_year) {
        return jd_next;
    }

    return 0;
}

double lunisolar_month_start(MasaName masa, int saka_year, int is_adhika,
                             LunisolarScheme scheme, const Location *loc)
{
    /* Check cache */
    LuniMonthCache *cached = lru_month_find(masa, saka_year, is_adhika, scheme);
    if (cached && cached->jd_start > 0)
        return cached->jd_start;

    double result;

    if (scheme == LUNISOLAR_PURNIMANTA) {
        /* Purnimanta month M starts at the full moon of Amanta month M-1.
         * Find the Amanta start of M (new moon), then find the full moon
         * ~15 days before it (which is in Amanta month M-1). */
        double amanta_start = amanta_month_start(masa, saka_year, is_adhika, loc);
        if (amanta_start == 0) return 0;

        /* The full moon is ~15 days before the Amanta start (new moon).
         * The Amanta new moon (jd_start in MasaInfo) is what amanta_month_start
         * finds the civil day after. We need the actual new moon JD. */
        double jd_rise = sunrise_jd(amanta_start, loc);
        if (jd_rise <= 0) jd_rise = amanta_start + 0.5 - loc->utc_offset / 24.0;
        MasaInfo mi = masa_for_date_at(jd_rise, loc);

        /* mi.jd_start is the new moon starting Amanta month M.
         * The full moon is ~15 days before this new moon. */
        double jd_full = full_moon_near(mi.jd_start - 15.0);

        /* Find the first civil day on/after this full moon where
         * the tithi is in Krishna paksha (tithi 16-30). */
        int fm_y, fm_m, fm_d;
        jd_to_gregorian(jd_full, &fm_y, &fm_m, &fm_d);

        /* Try the full moon day and the next 2 days */
        for (int offset = 0; offset <= 2; offset++) {
            double jd_try = gregorian_to_jd(fm_y, fm_m, fm_d) + offset;
            int ty, tm, td;
            jd_to_gregorian(jd_try, &ty, &tm, &td);
            double jr = sunrise_jd(jd_try, loc);
            if (jr <= 0) jr = jd_try + 0.5 - loc->utc_offset / 24.0;
            int t = tithi_at_moment(jr);
            /* Krishna paksha: tithi 16-30 (first day is Krishna Pratipada = 16) */
            if (t >= 16) {
                result = jd_try;
                goto cache_and_return;
            }
        }
        return 0;  /* Should not happen */
    } else {
        /* Amanta */
        result = amanta_month_start(masa, saka_year, is_adhika, loc);
        if (result == 0) return 0;
    }

cache_and_return:
    {
        LuniMonthCache *e = lru_month_insert(masa, saka_year, is_adhika, scheme);
        e->jd_start = result;
    }
    return result;
}

int lunisolar_month_length(MasaName masa, int saka_year, int is_adhika,
                           LunisolarScheme scheme, const Location *loc)
{
    /* Check cache for pre-computed length */
    LuniMonthCache *cached = lru_month_find(masa, saka_year, is_adhika, scheme);
    if (cached && cached->length > 0)
        return cached->length;

    double jd_start = lunisolar_month_start(masa, saka_year, is_adhika, scheme, loc);
    if (jd_start == 0) return 0;

    int length = 0;

    if (scheme == LUNISOLAR_PURNIMANTA) {
        /* Find next Purnimanta month's start and subtract */
        MasaName next_masa = (masa == PHALGUNA) ? CHAITRA : (MasaName)(masa + 1);
        int next_saka = (masa == PHALGUNA) ? saka_year + 1 : saka_year;
        /* For adhika: the next month is the nija of the same name */
        int next_adhika = 0;
        if (is_adhika) {
            next_masa = masa;
            next_saka = saka_year;
        }
        double jd_next = lunisolar_month_start(next_masa, next_saka, next_adhika,
                                                scheme, loc);
        if (jd_next > 0)
            length = (int)(jd_next - jd_start);
    } else {
        /* Amanta: scan days 28-31 from start to find where the month changes */
        for (int d = 28; d <= 31; d++) {
            double jd = jd_start + d;
            int gy, gm, gd;
            jd_to_gregorian(jd, &gy, &gm, &gd);
            MasaInfo mi = masa_for_date(gy, gm, gd, loc);
            if (mi.name != masa || mi.is_adhika != is_adhika) {
                length = d;
                break;
            }
        }
    }

    /* Store length in cache */
    if (length > 0) {
        LuniMonthCache *e = lru_month_find(masa, saka_year, is_adhika, scheme);
        if (e)
            e->length = length;
    }

    return length;
}
