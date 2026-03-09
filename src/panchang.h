/*
 * panchang.h - Daily panchang (Hindu almanac) generation
 *
 * High-level API for converting Gregorian dates to Hindu dates and
 * generating monthly panchang tables.  Combines tithi, masa, and
 * sunrise into a single HinduDate or PanchangDay output.
 *
 * This is typically the main entry point for applications that need
 * Hindu calendar information for display.
 */
#ifndef PANCHANG_H
#define PANCHANG_H

#include "types.h"

/*
 * gregorian_to_hindu - Convert a Gregorian date to a Hindu date.
 *
 *   year, month, day: Gregorian date.
 *   loc: Observer location (for sunrise computation).
 *   Returns: HinduDate with Saka/Vikram year, masa, paksha, tithi,
 *            and adhika flags.
 *
 * The Hindu date is determined at sunrise of the given civil day.
 */
HinduDate gregorian_to_hindu(int year, int month, int day, const Location *loc);

/*
 * generate_month_panchang - Generate panchang for a full Gregorian month.
 *
 *   year, month: Gregorian year and month (1-12).
 *   loc:   Observer location.
 *   days:  Output array, must have space for at least 31 PanchangDay entries.
 *   count: Set to the number of days filled (28-31).
 *
 * Fills one PanchangDay per civil day with Gregorian date, sunrise JD,
 * Hindu date, and full tithi details.
 */
void generate_month_panchang(int year, int month, const Location *loc,
                             PanchangDay *days, int *count);

/*
 * print_month_panchang - Print a monthly panchang table to stdout.
 *
 *   days:       Array of PanchangDay entries from generate_month_panchang().
 *   count:      Number of entries.
 *   utc_offset: Hours east of UTC, for displaying sunrise in local time.
 */
void print_month_panchang(const PanchangDay *days, int count, double utc_offset);

/*
 * print_day_panchang - Print detailed panchang for a single day to stdout.
 *
 *   day:        Single PanchangDay entry.
 *   utc_offset: Hours east of UTC, for displaying sunrise in local time.
 */
void print_day_panchang(const PanchangDay *day, double utc_offset);

#endif /* PANCHANG_H */
