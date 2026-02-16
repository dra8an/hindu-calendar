#include "solar.h"
#include "astro.h"
#include "masa.h"
#include "tithi.h"
#include "date_utils.h"
#include <math.h>
#include <string.h>

/* ---- Regional month names ---- */

static const char *BENGALI_MONTHS[] = {
    "", /* 0 unused */
    "Boishakh",    /* Mesha */
    "Joishtho",    /* Vrishabha */
    "Asharh",      /* Mithuna */
    "Srabon",      /* Karka */
    "Bhadro",      /* Simha */
    "Ashshin",     /* Kanya */
    "Kartik",      /* Tula */
    "Ogrohaeon",   /* Vrishchika */
    "Poush",       /* Dhanu */
    "Magh",        /* Makara */
    "Falgun",      /* Kumbha */
    "Choitro",     /* Meena */
};

static const char *TAMIL_MONTHS[] = {
    "",
    "Chithirai",   /* Mesha */
    "Vaikaasi",    /* Vrishabha */
    "Aani",        /* Mithuna */
    "Aadi",        /* Karka */
    "Aavani",      /* Simha */
    "Purattaasi",  /* Kanya */
    "Aippasi",     /* Tula */
    "Karthikai",   /* Vrishchika */
    "Maargazhi",   /* Dhanu */
    "Thai",        /* Makara */
    "Maasi",       /* Kumbha */
    "Panguni",     /* Meena */
};

static const char *ODIA_MONTHS[] = {
    "",
    "Baisakha",    /* Mesha */
    "Jyeshtha",    /* Vrishabha */
    "Ashadha",     /* Mithuna */
    "Shravana",    /* Karka */
    "Bhadrapada",  /* Simha */
    "Ashvina",     /* Kanya */
    "Kartika",     /* Tula */
    "Margashirsha",/* Vrishchika */
    "Pausha",      /* Dhanu */
    "Magha",       /* Makara */
    "Phalguna",    /* Kumbha */
    "Chaitra",     /* Meena */
};

/* Malayalam: year starts at Simha (rashi 5), so month 1 = Simha.
 * Indexed by regional month number (1-12). */
static const char *MALAYALAM_MONTHS[] = {
    "",
    "Chingam",     /* Simha (rashi 5) */
    "Kanni",       /* Kanya (rashi 6) */
    "Thulam",      /* Tula (rashi 7) */
    "Vrishchikam", /* Vrishchika (rashi 8) */
    "Dhanu",       /* Dhanu (rashi 9) */
    "Makaram",     /* Makara (rashi 10) */
    "Kumbham",     /* Kumbha (rashi 11) */
    "Meenam",      /* Meena (rashi 12) */
    "Medam",       /* Mesha (rashi 1) */
    "Edavam",      /* Vrishabha (rashi 2) */
    "Mithunam",    /* Mithuna (rashi 3) */
    "Karkadakam",  /* Karka (rashi 4) */
};

/* ---- Regional configuration ---- */

typedef struct {
    SolarCalendarType type;
    int first_rashi;             /* Rashi that starts month 1 (1=Mesha, 5=Simha) */
    int gy_offset_on;            /* regional_year = gy - gy_offset_on (on/after year start) */
    int gy_offset_before;        /* regional_year = gy - gy_offset_before (before year start) */
    const char *const *months;   /* Regional month names array */
    const char *era_name;
} SolarCalendarConfig;

/*
 * Era offsets from Gregorian year:
 *   Tamil (Saka):     gy - 78  on/after Mesha, gy - 79 before
 *   Bengali (Bangabda): gy - 593 on/after Mesha, gy - 594 before
 *   Odia (Saka):      gy - 78  on/after Mesha, gy - 79 before
 *   Malayalam (Kollam): gy - 824 on/after Simha, gy - 825 before
 */
static const SolarCalendarConfig SOLAR_CONFIGS[] = {
    { SOLAR_CAL_TAMIL,     1,  78,   79,  TAMIL_MONTHS,     "Saka"     },
    { SOLAR_CAL_BENGALI,   1,  593,  594, BENGALI_MONTHS,   "Bangabda" },
    { SOLAR_CAL_ODIA,      1,  78,   79,  ODIA_MONTHS,      "Saka"     },
    { SOLAR_CAL_MALAYALAM, 5,  824,  825, MALAYALAM_MONTHS,  "Kollam"   },
};

static const SolarCalendarConfig *get_config(SolarCalendarType type)
{
    for (int i = 0; i < 4; i++) {
        if (SOLAR_CONFIGS[i].type == type)
            return &SOLAR_CONFIGS[i];
    }
    return &SOLAR_CONFIGS[0]; /* fallback to Tamil */
}

/* ---- Critical time computation ----
 *
 * Each calendar uses a different "critical time" to determine which
 * civil day a sankranti belongs to:
 *   Tamil:     sunset of the current day
 *   Bengali:   midnight (start of the civil day, i.e. 0h local)
 *   Odia:      22:12 IST (fixed cutoff, empirically determined from drikpanchang.com)
 *   Malayalam:  end of madhyahna = sunrise + 3/5 × (sunset − sunrise)
 */

static double critical_time_jd(double jd_midnight_ut, const Location *loc,
                                SolarCalendarType type)
{
    switch (type) {
    case SOLAR_CAL_TAMIL:
        /* Subtract 8.0 min buffer to account for ~24 arcsecond ayanamsa
         * difference between SE_SIDM_LAHIRI and drikpanchang.com's Lahiri.
         * This shifts sankranti times by ~8 min; verified against 100
         * boundary cases — splits the 7.7–8.7 min gap cleanly. */
        return sunset_jd(jd_midnight_ut, loc) - 8.0 / (24.0 * 60.0);

    case SOLAR_CAL_BENGALI:
        /* Bengali midnight zone upper bound (00:24 IST = midnight + 24 min).
         * This is the outer boundary of the "zone" described by
         * Reingold/Dershowitz (11:36 PM to 12:24 AM).  Within the zone,
         * the actual assignment uses a tithi-based rule applied in
         * sankranti_to_civil_day(). */
        return jd_midnight_ut - loc->utc_offset / 24.0 + 24.0 / (24.0 * 60.0);

    case SOLAR_CAL_ODIA:
        /* Fixed IST cutoff at 22:12 (10:12 PM IST).
         * Empirically determined from 23 boundary cases on drikpanchang.com:
         * all sankrantis at <=22:11 IST are assigned to the current day,
         * all sankrantis at >=22:12 IST are assigned to the next day.
         * 22:12 IST = 16:42 UTC = 16.7h UTC. */
        return jd_midnight_ut + 16.7 / 24.0;

    case SOLAR_CAL_MALAYALAM:
        {
            /* End of madhyahna = sunrise + 3/5 of daytime.
             * The Hindu day is divided into 5 equal parts (pratahkala, sangava,
             * madhyahna, aparahna, sayahna). The critical time for Malayalam
             * sankranti assignment is the end of the 3rd part (madhyahna).
             * Verified against drikpanchang.com boundary cases. */
            double sr = sunrise_jd(jd_midnight_ut, loc);
            double ss = sunset_jd(jd_midnight_ut, loc);
            /* Subtract 9.5 min buffer for ayanamsa difference (same
             * reason as Tamil). Splits the 9.3–10.0 min gap cleanly.
             * Verified against 100 boundary cases. */
            return sr + 0.6 * (ss - sr) - 9.5 / (24.0 * 60.0);
        }
    }
    return jd_midnight_ut; /* fallback */
}

/* ---- Sankranti finding ----
 *
 * Find the exact JD when sidereal solar longitude crosses target_longitude.
 * Uses bisection on the angular difference, handling 360/0 wraparound.
 */

double sankranti_jd(double jd_approx, double target_longitude)
{
    /* Bracket: target should be within this range */
    double lo = jd_approx - 20.0;
    double hi = jd_approx + 20.0;

    /* Verify bracket: longitude at lo should be before target,
     * longitude at hi should be after target. */
    double lon_lo = solar_longitude_sidereal(lo);
    double diff_lo = lon_lo - target_longitude;
    if (diff_lo > 180.0) diff_lo -= 360.0;
    if (diff_lo < -180.0) diff_lo += 360.0;

    /* If lo is already past the target, widen bracket */
    if (diff_lo >= 0) lo -= 30.0;

    /* 50 iterations of bisection gives precision ~40 days / 2^50 ~ 3e-14 days ~ 3 ns */
    for (int i = 0; i < 50; i++) {
        double mid = (lo + hi) / 2.0;
        double lon = solar_longitude_sidereal(mid);

        double diff = lon - target_longitude;
        if (diff > 180.0) diff -= 360.0;
        if (diff < -180.0) diff += 360.0;

        if (diff >= 0)
            hi = mid;
        else
            lo = mid;
    }

    return (lo + hi) / 2.0;
}

double sankranti_before(double jd_ut)
{
    /* Get current sidereal longitude and determine current rashi */
    double lon = solar_longitude_sidereal(jd_ut);
    int rashi = (int)floor(lon / 30.0) + 1;
    if (rashi > 12) rashi = 12;
    if (rashi < 1) rashi = 1;

    /* The sankranti that started this sign = entry into rashi */
    double target = (rashi - 1) * 30.0;

    /* Approximate: sun moves ~1 deg/day, so we're at most 30 days past target */
    double degrees_past = lon - target;
    if (degrees_past < 0) degrees_past += 360.0;
    double jd_est = jd_ut - degrees_past; /* ~1 deg/day */

    return sankranti_jd(jd_est, target);
}

/* ---- Determine civil day of a sankranti ----
 *
 * Given a sankranti JD, calendar type, and rashi being entered, determine
 * which Gregorian day "owns" that sankranti (i.e., the first day of the
 * new solar month).
 *
 * For Bengali, a tithi-based rule applies when the sankranti falls in the
 * "midnight zone" (~24 min around midnight).  See Sewell & Dikshit,
 * "The Indian Calendar" (1896), pp. 12-13.
 */

static void sankranti_to_civil_day(double jd_sankranti, const Location *loc,
                                    SolarCalendarType type, int rashi,
                                    int *gy, int *gm, int *gd)
{
    /* Convert sankranti JD (UT) to local date */
    double local_jd = jd_sankranti + loc->utc_offset / 24.0 + 0.5;
    int sy, sm, sd;
    jd_to_gregorian(floor(local_jd), &sy, &sm, &sd);

    /* Does the sankranti fall before or after the critical time of this day?
     * If before: this day starts the new month.
     * If after: the next day starts the new month. */
    double jd_day = gregorian_to_jd(sy, sm, sd);
    double crit = critical_time_jd(jd_day, loc, type);

    if (jd_sankranti <= crit) {
        /* Bengali tithi-based override for sankrantis in the midnight zone.
         *
         * Traditional Bengali rule (Sewell & Dikshit, 1896):
         *   Karkata (rashi 4): always "before midnight" → this day
         *   Makara (rashi 10): always "after midnight" → next day
         *   Others: if the tithi at the Hindu day's sunrise (= previous
         *           civil day's sunrise) extends past the sankranti moment,
         *           treat as "before midnight" → this day; otherwise → next day.
         *
         * Verified against 37 drikpanchang.com edge cases: 36/37 correct.
         * (1 failure: 1976-10-17 Tula — tithi extends 134 min past sankranti
         * but drikpanchang assigns "after midnight".) */
        if (type == SOLAR_CAL_BENGALI && rashi != 4) {
            int push_next = 0;
            if (rashi == 10) {
                push_next = 1;
            } else {
                /* Tithi at previous day's sunrise (= start of the Hindu day
                 * containing this post-midnight sankranti). */
                int py, pm, pd;
                jd_to_gregorian(jd_day - 1.0, &py, &pm, &pd);
                TithiInfo ti = tithi_at_sunrise(py, pm, pd, loc);
                push_next = (ti.jd_end <= jd_sankranti) ? 1 : 0;
            }
            if (push_next) {
                jd_to_gregorian(jd_day + 1.0, gy, gm, gd);
                return;
            }
        }
        *gy = sy; *gm = sm; *gd = sd;
    } else {
        jd_to_gregorian(jd_day + 1.0, gy, gm, gd);
    }
}

/* ---- Rashi to regional month number ---- */

static int rashi_to_regional_month(int rashi, SolarCalendarType type)
{
    const SolarCalendarConfig *cfg = get_config(type);
    int m = rashi - cfg->first_rashi + 1;
    if (m <= 0) m += 12;
    return m;
}

/* ---- Solar year computation ----
 *
 * Each calendar has its own era, derived directly from the Gregorian year:
 *   regional_year = gy - offset
 * where the offset differs depending on whether we're before or on/after
 * the year-start sankranti in the current Gregorian year.
 */

static int solar_year(double jd_ut, const Location *loc,
                      double jd_greg_date, SolarCalendarType type)
{
    const SolarCalendarConfig *cfg = get_config(type);
    int gy, gm, gd;
    jd_to_gregorian(jd_ut, &gy, &gm, &gd);

    /* Find the year-start sankranti for this Gregorian year.
     * first_rashi 1 (Mesha) ~ April 14, first_rashi 5 (Simha) ~ August 17. */
    double target_long = (double)(cfg->first_rashi - 1) * 30.0;
    int approx_greg_month = 3 + cfg->first_rashi; /* Mesha=4, Simha=8 */
    if (approx_greg_month > 12) approx_greg_month -= 12;

    double jd_year_start_est = gregorian_to_jd(gy, approx_greg_month, 14);
    double jd_year_start = sankranti_jd(jd_year_start_est, target_long);

    /* Determine which civil day "owns" the year-start sankranti,
     * using the same critical-time rule as the calendar. */
    int ysy, ysm, ysd;
    sankranti_to_civil_day(jd_year_start, loc, type, cfg->first_rashi,
                           &ysy, &ysm, &ysd);
    double jd_year_civil = gregorian_to_jd(ysy, ysm, ysd);

    if (jd_greg_date >= jd_year_civil) {
        return gy - cfg->gy_offset_on;
    } else {
        return gy - cfg->gy_offset_before;
    }
}

/* ---- Public API ---- */

SolarDate gregorian_to_solar(int year, int month, int day,
                             const Location *loc, SolarCalendarType type)
{
    SolarDate sd = {0};

    double jd = gregorian_to_jd(year, month, day);
    double jd_crit = critical_time_jd(jd, loc, type);

    /* Sidereal solar longitude at critical time */
    double lon = solar_longitude_sidereal(jd_crit);

    /* Current rashi (1-12) */
    int rashi = (int)floor(lon / 30.0) + 1;
    if (rashi > 12) rashi = 12;
    if (rashi < 1) rashi = 1;
    sd.rashi = rashi;

    /* Find the sankranti that started this month */
    double target = (rashi - 1) * 30.0;
    double degrees_past = lon - target;
    if (degrees_past < 0) degrees_past += 360.0;
    double jd_est = jd_crit - degrees_past;
    sd.jd_sankranti = sankranti_jd(jd_est, target);

    /* Find the civil day of that sankranti */
    int sy, sm, s_day;
    sankranti_to_civil_day(sd.jd_sankranti, loc, type, rashi, &sy, &sm, &s_day);

    /* Day within solar month = days since month start + 1 */
    double jd_month_start = gregorian_to_jd(sy, sm, s_day);
    sd.day = (int)(jd - jd_month_start) + 1;

    /* Bengali tithi-based rule may push the month start past our date.
     * In that case, this date belongs to the previous solar month. */
    if (sd.day <= 0) {
        rashi = (rashi == 1) ? 12 : rashi - 1;
        sd.rashi = rashi;
        double prev_target = (double)(rashi - 1) * 30.0;
        sd.jd_sankranti = sankranti_jd(sd.jd_sankranti - 28.0, prev_target);
        sankranti_to_civil_day(sd.jd_sankranti, loc, type, rashi,
                               &sy, &sm, &s_day);
        jd_month_start = gregorian_to_jd(sy, sm, s_day);
        sd.day = (int)(jd - jd_month_start) + 1;
    }

    /* Regional month number */
    sd.month = rashi_to_regional_month(rashi, type);

    /* Year */
    sd.year = solar_year(jd_crit, loc, jd, type);

    return sd;
}

void solar_to_gregorian(const SolarDate *sd, SolarCalendarType type,
                        const Location *loc, int *year, int *month, int *day)
{
    const SolarCalendarConfig *cfg = get_config(type);

    /* Convert regional month back to rashi */
    int rashi = sd->month + cfg->first_rashi - 1;
    if (rashi > 12) rashi -= 12;

    /* Convert regional year back to Gregorian year.
     * The year-start sankranti falls in approximately:
     *   Mesha (first_rashi=1): April of gy
     *   Simha (first_rashi=5): August of gy
     * Months after the year start that wrap past December fall in gy+1. */
    int gy = sd->year + cfg->gy_offset_on;

    /* Approximate Gregorian month for this rashi: rashi 1=Apr, 2=May, ..., 9=Dec, 10=Jan+1, etc. */
    int rashi_greg_month = 3 + rashi;  /* Mesha(1)=Apr(4), ..., Dhanu(9)=Dec(12) */
    int start_greg_month = 3 + cfg->first_rashi;  /* Year-start approx greg month */

    if (rashi_greg_month > 12 && start_greg_month <= 12) {
        /* This rashi falls in Jan-Mar of the next Gregorian year */
        gy++;
    } else if (rashi_greg_month <= 12 && start_greg_month <= 12 &&
               rashi_greg_month < start_greg_month) {
        /* This rashi falls before the year-start month in the same year,
         * meaning it's actually in the next Gregorian year
         * (e.g., Malayalam: Mesha(Apr) in a year starting Simha(Aug)) */
        gy++;
    }

    /* Find the sankranti for this rashi in this Gregorian year */
    double target = (rashi - 1) * 30.0;
    int est_month = rashi_greg_month;
    if (est_month > 12) est_month -= 12;
    double jd_est = gregorian_to_jd(gy, est_month, 14);
    double jd_sank = sankranti_jd(jd_est, target);

    /* Find the civil day of the sankranti */
    int sy, sm, s_day;
    sankranti_to_civil_day(jd_sank, loc, type, rashi, &sy, &sm, &s_day);

    /* Add (day - 1) days */
    double jd_result = gregorian_to_jd(sy, sm, s_day) + (sd->day - 1);
    jd_to_gregorian(jd_result, year, month, day);
}

const char *solar_month_name(int month, SolarCalendarType type)
{
    if (month < 1 || month > 12) return "???";
    const SolarCalendarConfig *cfg = get_config(type);
    return cfg->months[month];
}

const char *solar_era_name(SolarCalendarType type)
{
    return get_config(type)->era_name;
}
