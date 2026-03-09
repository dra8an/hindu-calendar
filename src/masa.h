/*
 * masa.h - Lunisolar month (masa) determination
 *
 * Determines which Hindu lunisolar month a given date belongs to,
 * using the Amanta (new-moon-to-new-moon) scheme.
 *
 * Month determination algorithm:
 *   1. Find the new moons bracketing the given sunrise
 *   2. Compute the sidereal solar rashi at each new moon
 *   3. Month name = rashi at the preceding new moon + 1
 *   4. If both new moons have the same rashi, this is an adhika
 *      (intercalary/leap) month
 *
 * Also provides inverse lookups: from (masa, saka_year, is_adhika)
 * to the first civil day and month length.
 *
 * New moon finding uses 17-point inverse Lagrange interpolation on
 * the lunar phase function (matching the Python drik-panchanga reference).
 */
#ifndef MASA_H
#define MASA_H

#include "types.h"

/*
 * new_moon_before - Find the new moon preceding a given moment.
 *
 *   jd_ut:      Julian Day in UT.
 *   tithi_hint: Current tithi number (1-30), used to estimate how
 *               far back to search.  Use tithi_at_moment(jd_ut).
 *   Returns: JD (UT) of the new moon.
 */
double new_moon_before(double jd_ut, int tithi_hint);

/*
 * new_moon_after - Find the new moon following a given moment.
 *
 *   jd_ut:      Julian Day in UT.
 *   tithi_hint: Current tithi number (1-30).
 *   Returns: JD (UT) of the new moon.
 */
double new_moon_after(double jd_ut, int tithi_hint);

/*
 * solar_rashi - Sidereal zodiac sign of the sun.
 *
 *   jd_ut: Julian Day in UT.
 *   Returns: 1-12 (1 = Mesha/Aries, ..., 12 = Meena/Pisces).
 *
 * Based on sidereal (Nirayana) solar longitude with Lahiri ayanamsa.
 * Rashi = ceil(sidereal_longitude / 30).
 */
int solar_rashi(double jd_ut);

/*
 * masa_for_date - Determine the lunisolar month for a Gregorian date.
 *
 *   year, month, day: Gregorian date.
 *   loc: Observer location (needed for sunrise computation).
 *   Returns: MasaInfo with month name, adhika flag, Saka/Vikram years,
 *            and the new moon JDs bracketing this month.
 *
 * This is the primary forward lookup: Gregorian -> Hindu month.
 * The Hindu date is determined at sunrise of the given civil day.
 */
MasaInfo masa_for_date(int year, int month, int day, const Location *loc);

/*
 * masa_for_date_at - Same as masa_for_date() with pre-computed sunrise.
 *
 *   jd_rise: JD (UT) of sunrise (previously obtained from sunrise_jd()).
 *   loc:     Observer location (currently unused but reserved).
 *   Returns: MasaInfo.
 *
 * Use this when you already have the sunrise JD to avoid redundant
 * sunrise computation.
 */
MasaInfo masa_for_date_at(double jd_rise, const Location *loc);

/*
 * hindu_year_saka - Saka era year for a given date and month.
 *
 *   jd_ut:    Julian Day in UT.
 *   masa_num: Masa number (1-12).
 *   Returns: Saka era year.
 *
 * Uses the Kali Ahargana method (from the Python drik-panchanga
 * reference).  The Hindu new year begins with Chaitra (masa 1).
 */
int hindu_year_saka(double jd_ut, int masa_num);

/*
 * hindu_year_vikram - Vikram Samvat year from Saka year.
 *
 *   saka_year: Saka era year.
 *   Returns: saka_year + 135.
 */
int hindu_year_vikram(int saka_year);

/*
 * lunisolar_month_start - First civil day of a lunisolar month.
 *
 *   masa:      Month name (CHAITRA..PHALGUNA).
 *   saka_year: Saka era year.
 *   is_adhika: 1 for adhika (leap) month, 0 for nija (regular).
 *   loc:       Observer location.
 *   Returns: JD at 0h UT of the first civil day (Shukla Pratipada),
 *            or 0 if the month is not found (invalid combination).
 *
 * Inverse lookup: (masa, year, adhika) -> Gregorian date.
 * To get the Gregorian date, pass the result to jd_to_gregorian().
 *
 * Algorithm: estimates the Gregorian month from the masa, then uses
 * masa_for_date() to verify and navigates via new moon boundaries
 * until the target month is found.  Tested for all 1,868 months
 * in 1900-2050.
 *
 * Example:
 *   double jd = lunisolar_month_start(CHAITRA, 1947, 0, &loc);
 *   int y, m, d;
 *   jd_to_gregorian(jd, &y, &m, &d);  // -> 2025-03-30
 */
double lunisolar_month_start(MasaName masa, int saka_year, int is_adhika,
                             const Location *loc);

/*
 * lunisolar_month_length - Number of days in a lunisolar month.
 *
 *   masa, saka_year, is_adhika, loc: Same as lunisolar_month_start().
 *   Returns: 29 or 30 (the number of civil days in the month),
 *            or 0 if the month is not found.
 *
 * Computed by finding the first day where masa_for_date() returns
 * a different month, starting from day 28 of the month.
 */
int lunisolar_month_length(MasaName masa, int saka_year, int is_adhika,
                           const Location *loc);

#endif /* MASA_H */
