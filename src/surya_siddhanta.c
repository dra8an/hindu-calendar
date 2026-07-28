/*
 * surya_siddhanta.c - Classical Surya Siddhanta solar and lunar positions
 *
 * See surya_siddhanta.h for the API and provenance note.
 *
 * The model
 * ---------
 * Each body moves at a constant mean rate on a deferent, corrected by a single
 * epicyclic term (the manda, or "slow", equation of centre).  There are no
 * perturbation series: the Surya Siddhanta predates them by well over a
 * millennium.  A body's true longitude is
 *
 *     true = mean_longitude - arcsin(epicycle_size * sin(mean_anomaly))
 *
 * where sin/arcsin are evaluated in the text's own 24-entry table rather than
 * with the modern functions, and the epicycle size itself varies slightly with
 * anomaly (the text gives two values, at odd and even quadrant ends).
 */
#include "surya_siddhanta.h"

#include <math.h>

/* ---------------------------------------------------------------------------
 * Primitive constants
 * ---------------------------------------------------------------------------
 * The entire model derives from five integers stated in the Surya Siddhanta,
 * chapter 1 ("Of the Mean Motions of the Planets"), which gives the revolution
 * counts of the bodies and their apsides in a mahayuga along with the number of
 * civil days it contains.  Everything else below is arithmetic on these.
 *
 * Standard English reference: Burgess (1860), see Docs/LICENSING.md.  Verse
 * numbering differs between editions, so the chapter and topic are cited rather
 * than a verse range.
 *
 * These figures are historical facts, reproduced in every edition and study of
 * the text; they are not taken from any modern implementation.
 */
#define SS_SOLAR_REVS    4320000.0     /* solar revolutions per mahayuga */
#define SS_LUNAR_REVS    57753336.0    /* lunar revolutions per mahayuga */
#define SS_CIVIL_DAYS    1577917828.0  /* civil days per mahayuga */
#define SS_SOLAR_APOGEE  387.0         /* solar apogee revolutions per kalpa */
#define SS_LUNAR_APOGEE  488199.0      /* lunar apogee revolutions per mahayuga */

/* Derived mean periods, in days.
 *
 *   sidereal year     = civil_days / solar_revs
 *   anomalistic year  = civil_days_per_kalpa / (solar_revs_per_kalpa - apogee)
 *   sidereal month    = civil_days / lunar_revs
 *   anomalistic month = civil_days / (lunar_revs - lunar_apogee)
 *
 * A kalpa is 1000 mahayugas, which is why the anomalistic year scales both
 * terms by 1000: the solar apogee count is given per kalpa, not per mahayuga.
 */
#define SS_SIDEREAL_YEAR      (SS_CIVIL_DAYS / SS_SOLAR_REVS)
#define SS_ANOMALISTIC_YEAR   ((SS_CIVIL_DAYS * 1000.0) / \
                               (SS_SOLAR_REVS * 1000.0 - SS_SOLAR_APOGEE))
#define SS_SIDEREAL_MONTH     (SS_CIVIL_DAYS / SS_LUNAR_REVS)
#define SS_ANOMALISTIC_MONTH  (SS_CIVIL_DAYS / (SS_LUNAR_REVS - SS_LUNAR_APOGEE))

/* Epicycle sizes, as a fraction of the deferent.
 *
 * The text gives two values for each body -- at the ends of the odd and even
 * quadrants -- and the true size varies between them with the anomaly:
 *
 *   Sun:  14 deg   at even quadrant ends, 13 deg 40' at odd
 *   Moon: 32 deg   at even quadrant ends, 31 deg 40' at odd
 *
 * 14 * (1 - 1/42) = 13.6667 = 13 deg 40', and 32 * (1 - 1/96) = 31 deg 40',
 * which is where the contraction fractions come from.
 */
#define SS_SOLAR_EPICYCLE   (14.0 / 360.0)
#define SS_SOLAR_CONTRACT   (1.0 / 42.0)
#define SS_LUNAR_EPICYCLE   (32.0 / 360.0)
#define SS_LUNAR_CONTRACT   (1.0 / 96.0)

/* Kali Yuga epoch as an RD (Rata Die) day number: Julian 18 February 3102 BCE.
 * At this instant the mean sun and mean moon are both at 0 degrees, which is
 * the classical statement the epoch is defined by. */
#define SS_EPOCH_RD  (-1132959.0)

/* JD (noon-based) to RD: RD 1 = 0001-01-01 = JD 1721425.5 (midnight). */
#define SS_JD_TO_RD  1721424.5

/* Mean anomaly at the epoch, in turns.
 *
 * The mean longitudes are zero at the epoch but the apogees are not, so the
 * anomaly (mean longitude minus apogee) has a nonzero starting value.  These
 * are exact rationals:
 *
 *   solar:  3143/4000 turn  -- equivalently a solar apogee of 77.13 degrees,
 *                              matching the 77 degrees given in the text
 *   lunar:  3/4 turn
 *
 * The mean lunar longitude offset is exactly zero, so no constant is needed.
 */
#define SS_SOLAR_ANOM_EPOCH  (3143.0 / 4000.0)
#define SS_LUNAR_ANOM_EPOCH  (3.0 / 4.0)

/* ---------------------------------------------------------------------------
 * The sine table
 * ---------------------------------------------------------------------------
 * The Surya Siddhanta, chapter 2 ("Of the True Places of the Planets"),
 * tabulates 24 sine values at intervals of 225 minutes of arc (3 deg 45'), in
 * units of R = 3438 -- the number of arcminutes in a radian, so that
 * R*sin(90 deg) = R.
 *
 * The text states these as successive DIFFERENCES:
 *
 *   225 224 222 219 215 210 205 199 191 183 174 164
 *   154 143 131 119 106  93  79  65  51  37  22   7
 *
 * The cumulative values are given below.  They sum to exactly 3438, which is
 * the arithmetic check the text itself implies.
 */
#define SS_R          3438.0
#define SS_STEP_DEG   3.75      /* 225 arcminutes */
#define SS_TABLE_N    24

static const int SS_SINE[SS_TABLE_N + 1] = {
    0,
     225,  449,  671,  890, 1105, 1315, 1520, 1719,
    1910, 2093, 2267, 2431, 2585, 2728, 2859, 2978,
    3084, 3177, 3256, 3321, 3372, 3409, 3431, 3438
};

/*
 * ss_sine_entry - Table lookup extended to a full circle.
 *
 * The table covers the first quadrant only.  The anomaly sweeps all four, so
 * reflect the index into the first quadrant and carry the sign, exactly as the
 * classical rules prescribe.
 */
static double ss_sine_entry(int n)
{
    int quadrant;
    double sign = 1.0;

    n = ((n % (4 * SS_TABLE_N)) + 4 * SS_TABLE_N) % (4 * SS_TABLE_N);
    quadrant = n / SS_TABLE_N;
    n = n % SS_TABLE_N;

    switch (quadrant) {
    case 0: break;                                   /* sin t      */
    case 1: n = SS_TABLE_N - n; break;               /* sin(180-t) */
    case 2: sign = -1.0; break;                      /* -sin t     */
    case 3: n = SS_TABLE_N - n; sign = -1.0; break;  /* -sin(360-t)*/
    }
    return sign * SS_SINE[n] / SS_R;
}

/*
 * ss_sine - Sine by linear interpolation in the table.
 *
 *   theta: degrees (any value).
 *   Returns: amplitude in [-1, 1].
 */
static double ss_sine(double theta)
{
    double entry = theta / SS_STEP_DEG;
    double lo = floor(entry);
    double frac = entry - lo;
    return (1.0 - frac) * ss_sine_entry((int)lo) +
           frac * ss_sine_entry((int)lo + 1);
}

/*
 * ss_arcsin - Inverse of ss_sine, by search and interpolation.
 *
 *   amp: amplitude in [-1, 1].
 *   Returns: degrees in [-90, 90].
 */
static double ss_arcsin(double amp)
{
    int pos;
    double below, span;

    if (amp < 0.0) return -ss_arcsin(-amp);

    pos = 0;
    while (pos < SS_TABLE_N && amp > SS_SINE[pos] / SS_R)
        pos++;

    below = (pos > 0) ? SS_SINE[pos - 1] / SS_R : 0.0;
    span = SS_SINE[pos] / SS_R - below;
    if (span <= 0.0)
        return SS_STEP_DEG * pos;

    return SS_STEP_DEG * ((pos - 1) + (amp - below) / span);
}

/* ---------------------------------------------------------------------------
 * Positions
 * ---------------------------------------------------------------------------
 */

/* Convert JD (UT) to the Surya Siddhanta's time argument: days since the Kali
 * Yuga epoch, in local time at the observer's meridian.
 *
 * The longitude term is not cosmetic -- at Delhi it shifts the argument by
 * 5.15 hours, moving the sun by ~0.2 degrees. */
static double ss_days_since_epoch(double jd_ut, const Location *loc)
{
    double rd_local = (jd_ut - SS_JD_TO_RD) + loc->longitude / 360.0;
    return rd_local - SS_EPOCH_RD;
}

/* Fractional part, always in [0, 1). */
static double ss_frac(double x)
{
    double f = fmod(x, 1.0);
    return (f < 0.0) ? f + 1.0 : f;
}

/*
 * ss_true_position - Mean position corrected by the manda equation.
 *
 *   days:        days since the Kali Yuga epoch
 *   period:      mean period of the body, in days
 *   anom_period: period of the anomaly, in days
 *   anom_epoch:  mean anomaly at the epoch, in turns
 *   size:        epicycle radius as a fraction of the deferent
 *   contract:    maximum fractional decrease in epicycle size
 */
static double ss_true_position(double days, double period,
                               double anom_period, double anom_epoch,
                               double size, double contract)
{
    double mean = 360.0 * ss_frac(days / period);
    double anomaly = 360.0 * ss_frac(days / anom_period + anom_epoch);
    double offset = ss_sine(anomaly);
    double shrunk = size - fabs(offset) * contract * size;
    double equation = ss_arcsin(offset * shrunk);
    double lon = fmod(mean - equation, 360.0);
    return (lon < 0.0) ? lon + 360.0 : lon;
}

double surya_solar_longitude(double jd_ut, const Location *loc)
{
    return ss_true_position(ss_days_since_epoch(jd_ut, loc),
                            SS_SIDEREAL_YEAR,
                            SS_ANOMALISTIC_YEAR, SS_SOLAR_ANOM_EPOCH,
                            SS_SOLAR_EPICYCLE, SS_SOLAR_CONTRACT);
}

double surya_lunar_longitude(double jd_ut, const Location *loc)
{
    return ss_true_position(ss_days_since_epoch(jd_ut, loc),
                            SS_SIDEREAL_MONTH,
                            SS_ANOMALISTIC_MONTH, SS_LUNAR_ANOM_EPOCH,
                            SS_LUNAR_EPICYCLE, SS_LUNAR_CONTRACT);
}

double surya_lunar_phase(double jd_ut, const Location *loc)
{
    double phase = surya_lunar_longitude(jd_ut, loc) -
                   surya_solar_longitude(jd_ut, loc);
    phase = fmod(phase, 360.0);
    return (phase < 0.0) ? phase + 360.0 : phase;
}

int surya_tithi_at(double jd_ut, const Location *loc)
{
    return (int)floor(surya_lunar_phase(jd_ut, loc) / 12.0) + 1;
}

/* Signed angular difference a - b, in (-180, 180]. */
static double ss_ang_diff(double a, double b)
{
    double d = fmod(a - b, 360.0);
    if (d < 0.0) d += 360.0;
    return (d > 180.0) ? d - 360.0 : d;
}

double surya_tithi_end(double jd_ut, const Location *loc)
{
    double phase = surya_lunar_phase(jd_ut, loc);
    double target = 12.0 * (floor(phase / 12.0) + 1.0);
    double lo, hi;

    if (target >= 360.0) target = 0.0;

    /* The moon gains ~12 degrees on the sun per day; bracket generously. */
    lo = jd_ut;
    hi = jd_ut + 2.0;
    while (ss_ang_diff(surya_lunar_phase(hi, loc), target) < 0.0 &&
           hi - jd_ut < 5.0)
        hi += 0.5;

    for (int i = 0; i < 60; i++) {
        double mid = (lo + hi) / 2.0;
        if (ss_ang_diff(surya_lunar_phase(mid, loc), target) < 0.0)
            lo = mid;
        else
            hi = mid;
        if (hi - lo < 1e-9) break;   /* ~0.1 ms */
    }
    return (lo + hi) / 2.0;
}

double surya_sankranti(double jd_approx, double target_longitude,
                       const Location *loc)
{
    /* Bracket the crossing AROUND the estimate, matching sankranti_jd() in
     * solar.c.  This is deliberately not a forward-only "next crossing at or
     * after" search: callers in solar.c pass an estimate that may sit slightly
     * past the sankranti, and a forward-only search would then skip a whole
     * year to the next occurrence of the same sign boundary. */
    double lo = jd_approx - 20.0;
    double hi = jd_approx + 20.0;

    /* If lo is already past the target, widen the bracket backwards. */
    if (ss_ang_diff(surya_solar_longitude(lo, loc), target_longitude) >= 0.0)
        lo -= 30.0;

    for (int i = 0; i < 60; i++) {
        double mid = (lo + hi) / 2.0;
        if (ss_ang_diff(surya_solar_longitude(mid, loc), target_longitude) < 0.0)
            lo = mid;
        else
            hi = mid;
        if (hi - lo < 1e-9) break;   /* ~0.1 ms */
    }
    return (lo + hi) / 2.0;
}
