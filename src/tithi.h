/*
 * tithi.h - Tithi (lunar day) computation
 *
 * A tithi is one of 30 lunar days in a lunisolar month.  Each tithi
 * spans exactly 12 degrees of moon-sun elongation (lunar phase).
 * Tithis are not aligned to sunrise — they can start and end at any
 * time of day.
 *
 * Tithi numbering (1-30 continuous):
 *   Shukla (bright) paksha: 1 (Pratipada) to 15 (Purnima)
 *   Krishna (dark) paksha:  16 (Pratipada) to 30 (Amavasya)
 *
 * The tithi "at" a civil day is the one prevailing at sunrise.
 *
 * Formula: tithi = floor(lunar_phase / 12) + 1
 *   where lunar_phase = (moon_longitude - sun_longitude) mod 360
 */
#ifndef TITHI_H
#define TITHI_H

#include "types.h"

/*
 * lunar_phase - Moon-sun elongation angle.
 *
 *   jd_ut: Julian Day in Universal Time.
 *   Returns: degrees [0, 360).
 *            0 = new moon, 180 = full moon.
 *
 * Uses tropical longitudes (ayanamsa cancels in the difference).
 */
double lunar_phase(double jd_ut);

/*
 * tithi_at_moment - Tithi number at an arbitrary moment.
 *
 *   jd_ut: Julian Day in Universal Time.
 *   Returns: 1-30.
 *
 * Lightweight: just computes floor(lunar_phase / 12) + 1.
 * No sunrise, no boundary finding.
 */
int tithi_at_moment(double jd_ut);

/*
 * tithi_at_sunrise - Full tithi details for a civil day.
 *
 *   year, month, day: Gregorian date.
 *   loc: Observer location (needed for sunrise computation).
 *   Returns: TithiInfo with tithi number, paksha, start/end JDs,
 *            and kshaya detection.
 *
 * This is the primary tithi function for panchang generation.
 * It computes sunrise, finds the tithi at that moment, then
 * searches for the exact tithi boundaries (start and end times).
 * Also checks if the next tithi is skipped (kshaya).
 */
TithiInfo tithi_at_sunrise(int year, int month, int day, const Location *loc);

/*
 * tithi_num_at_jd - Tithi number at a pre-computed JD.
 *
 *   jd_sunrise: Julian Day (typically a previously computed sunrise).
 *   Returns: 1-30.
 *
 * Same as tithi_at_moment() but named to clarify intent.  No boundary
 * finding, no kshaya check.  Use when you already have the sunrise JD
 * and only need the tithi number.
 */
int tithi_num_at_jd(double jd_sunrise);

/*
 * find_tithi_boundary - Exact JD when a tithi boundary occurs.
 *
 *   jd_start, jd_end: Search interval (the boundary must lie within).
 *   target_tithi: The tithi number (1-30) whose START to find.
 *   Returns: JD (UT) of the boundary crossing.
 *
 * Uses bisection on lunar phase.  The boundary for tithi N is the
 * moment when lunar_phase crosses (N - 1) * 12 degrees.
 */
double find_tithi_boundary(double jd_start, double jd_end, int target_tithi);

#endif /* TITHI_H */
