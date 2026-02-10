#include "tithi.h"
#include "astro.h"
#include "date_utils.h"
#include <math.h>

double lunar_phase(double jd_ut)
{
    double moon = lunar_longitude(jd_ut);
    double sun = solar_longitude(jd_ut);
    double phase = fmod(moon - sun, 360.0);
    if (phase < 0) phase += 360.0;
    return phase;
}

int tithi_at_moment(double jd_ut)
{
    double phase = lunar_phase(jd_ut);
    int t = (int)(phase / 12.0) + 1;
    if (t > 30) t = 30;
    return t;
}

double find_tithi_boundary(double jd_start, double jd_end, int target_tithi)
{
    /* The boundary for target_tithi is at phase = (target_tithi - 1) * 12 degrees.
     * We search for the moment when lunar_phase crosses this value.
     * Use bisection on the phase difference. */
    double target_phase = (target_tithi - 1) * 12.0;

    /* Bisection: find JD where lunar_phase == target_phase */
    double lo = jd_start;
    double hi = jd_end;

    for (int i = 0; i < 50; i++) {
        double mid = (lo + hi) / 2.0;
        double phase = lunar_phase(mid);

        /* Handle wraparound at 0/360 boundary.
         * Compute signed angular distance from target_phase to phase. */
        double diff = phase - target_phase;
        if (diff > 180.0) diff -= 360.0;
        if (diff < -180.0) diff += 360.0;

        if (diff >= 0)
            hi = mid;
        else
            lo = mid;
    }

    return (lo + hi) / 2.0;
}

TithiInfo tithi_at_sunrise(int year, int month, int day, const Location *loc)
{
    TithiInfo info = {0};

    double jd = gregorian_to_jd(year, month, day);
    double jd_rise = sunrise_jd(jd, loc);

    if (jd_rise <= 0) {
        /* Fallback: use local noon if sunrise fails */
        jd_rise = jd + 0.5 - loc->utc_offset / 24.0;
    }

    /* Tithi at sunrise (convert to UT for calculation) */
    double rise_ut = jd_rise;
    int t = tithi_at_moment(rise_ut);

    info.tithi_num = t;
    info.paksha = (t <= 15) ? SHUKLA_PAKSHA : KRISHNA_PAKSHA;
    info.paksha_tithi = (t <= 15) ? t : t - 15;

    /* Find tithi start: search backwards from sunrise.
     * The current tithi started when phase crossed (t-1)*12.
     * Search within about 2 days before sunrise. */
    info.jd_start = find_tithi_boundary(rise_ut - 2.0, rise_ut, t);

    /* Find tithi end: search forwards from sunrise.
     * The current tithi ends when phase crosses t*12.
     * Search within about 2 days after sunrise.
     * For tithi 30, the next boundary is at 0 degrees = tithi 1. */
    int next_tithi = (t % 30) + 1;
    info.jd_end = find_tithi_boundary(rise_ut, rise_ut + 2.0, next_tithi);

    /* Check for kshaya tithi: if next sunrise has a tithi that
     * is more than 1 ahead of today's tithi */
    double jd_tomorrow = jd + 1.0;
    double jd_rise_tmrw = sunrise_jd(jd_tomorrow, loc);
    if (jd_rise_tmrw > 0) {
        int t_tmrw = tithi_at_moment(jd_rise_tmrw);
        int diff = (t_tmrw - t + 30) % 30;
        info.is_kshaya = (diff > 1) ? 1 : 0;
    }

    return info;
}
