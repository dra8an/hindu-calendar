/*
 * solar.h - Regional Hindu solar calendars
 *
 * Four South/East Indian solar calendar variants, each based on
 * sidereal (Nirayana) solar longitude with Lahiri ayanamsa:
 *
 *   Tamil     - Saka era, year starts at Mesha (rashi 1, ~Apr 14)
 *   Bengali   - Bangabda era, year starts at Mesha
 *   Odia      - Amli era, year starts at Kanya (rashi 6, ~Sep)
 *   Malayalam - Kollam era, year starts at Simha (rashi 5, ~Aug 17)
 *
 * A solar month begins when the sun enters a new sidereal zodiac
 * sign (sankranti).  Each regional calendar has its own rule for
 * determining which civil day "owns" a sankranti that falls near
 * a day boundary (see SolarCalendarType in types.h for details).
 *
 * All four calendars are 100% validated against drikpanchang.com
 * for 1,811 months each (1900-2050).
 */
#ifndef SOLAR_H
#define SOLAR_H

#include "types.h"

/*
 * sankranti_jd - Exact JD when the sun enters a sidereal zodiac sign.
 *
 *   jd_approx:        Rough estimate within ~15 days of the crossing.
 *   target_longitude:  Target sidereal longitude (0, 30, 60, ..., 330).
 *   Returns: JD (UT) of the crossing, found by bisection (50 iterations,
 *            ~3 nanosecond precision).
 *
 * Example: sankranti_jd(jd_approx, 0.0) finds Mesha Sankranti.
 */
double sankranti_jd(double jd_approx, double target_longitude);

/*
 * sankranti_before - Find the most recent sankranti on or before a JD.
 *
 *   jd_ut: Julian Day in UT.
 *   Returns: JD (UT) of when the sun entered its current sidereal sign.
 */
double sankranti_before(double jd_ut);

/*
 * gregorian_to_solar - Convert a Gregorian date to a regional solar date.
 *
 *   year, month, day: Gregorian date.
 *   loc:  Observer location (for sunrise/sunset in critical time rules).
 *   type: Which regional calendar (SOLAR_CAL_TAMIL, etc.).
 *   Returns: SolarDate with regional era year, month, day, rashi,
 *            and the JD of the sankranti that started the solar month.
 */
SolarDate gregorian_to_solar(int year, int month, int day,
                             const Location *loc, SolarCalendarType type);

/*
 * solar_to_gregorian - Convert a regional solar date back to Gregorian.
 *
 *   sd:   Solar date (year, month, day in regional convention).
 *   type: Which regional calendar.
 *   loc:  Observer location.
 *   year, month, day: Output Gregorian date.
 *
 * Inverse of gregorian_to_solar().  Roundtrips exactly for all dates
 * in the supported range.
 */
void solar_to_gregorian(const SolarDate *sd, SolarCalendarType type,
                        const Location *loc, int *year, int *month, int *day);

/*
 * solar_month_name - Regional month name string.
 *
 *   month: 1-12 (regional month number).
 *   type:  Which regional calendar.
 *   Returns: Month name in the regional language (e.g., "Chittirai" for
 *            Tamil month 1, "Baishakh" for Bengali month 1).
 */
const char *solar_month_name(int month, SolarCalendarType type);

/*
 * solar_era_name - Regional era name string.
 *
 *   type: Which regional calendar.
 *   Returns: "Saka", "Bangabda", "Amli", or "Kollam".
 */
const char *solar_era_name(SolarCalendarType type);

/*
 * solar_month_start - JD of the first civil day of a solar month.
 *
 *   month: 1-12 (regional month number).
 *   year:  Regional era year.
 *   type:  Which regional calendar.
 *   loc:   Observer location.
 *   Returns: JD at 0h UT of the first civil day of the month.
 *
 * Uses the forward sankranti pipeline: finds the sankranti, applies
 * the regional critical-time rule, and returns the owning civil day.
 * Verified against drikpanchang.com for 7,248 months (1,812 per calendar).
 */
double solar_month_start(int month, int year, SolarCalendarType type,
                         const Location *loc);

/*
 * solar_month_length - Number of days in a solar month.
 *
 *   month, year, type, loc: Same as solar_month_start().
 *   Returns: 29-32 days.
 *
 * Defined as solar_month_start(next_month) - solar_month_start(this_month),
 * with proper year wrapping at the calendar's year boundary.
 */
int solar_month_length(int month, int year, SolarCalendarType type,
                       const Location *loc);

#endif /* SOLAR_H */
