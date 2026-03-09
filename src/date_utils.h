/*
 * date_utils.h - Gregorian date / Julian Day conversions
 *
 * Utility functions for converting between Gregorian calendar dates
 * and Julian Day numbers, plus day-of-week helpers.
 *
 * Julian Day (JD) is a continuous count of days since 4713 BCE Jan 1
 * at noon UT.  JD noon-based: gregorian_to_jd(2000, 1, 1) = 2451544.5
 * (midnight Jan 1, 2000).
 */
#ifndef DATE_UTILS_H
#define DATE_UTILS_H

/*
 * gregorian_to_jd - Convert a Gregorian date to Julian Day number.
 *
 *   year, month, day: Gregorian date (year can be negative for BCE).
 *   Returns: JD at 0h UT (midnight) of the given date.
 *
 * Uses Meeus Ch. 7 algorithm.  Valid for all dates in the Gregorian
 * calendar (1582-10-15 onward, extended proleptic for earlier dates).
 */
double gregorian_to_jd(int year, int month, int day);

/*
 * jd_to_gregorian - Convert a Julian Day number to Gregorian date.
 *
 *   jd:   Julian Day number (any fractional value; date is taken at noon).
 *   year, month, day: output pointers for the Gregorian date.
 */
void jd_to_gregorian(double jd, int *year, int *month, int *day);

/*
 * day_of_week - Day of week from a Julian Day.
 *
 *   jd: Julian Day number.
 *   Returns: 0 = Monday, 1 = Tuesday, ..., 6 = Sunday (ISO convention).
 */
int day_of_week(double jd);

/*
 * day_of_week_name - Full English name for a day of week.
 *
 *   dow: 0-6 (Monday-Sunday), as returned by day_of_week().
 *   Returns: "Monday", "Tuesday", etc.
 */
const char *day_of_week_name(int dow);

/*
 * day_of_week_short - Three-letter abbreviation for a day of week.
 *
 *   dow: 0-6 (Monday-Sunday).
 *   Returns: "Mon", "Tue", etc.
 */
const char *day_of_week_short(int dow);

#endif /* DATE_UTILS_H */
