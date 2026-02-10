#ifndef PANCHANG_H
#define PANCHANG_H

#include "types.h"

/* Convert a Gregorian date to a Hindu date at sunrise */
HinduDate gregorian_to_hindu(int year, int month, int day, const Location *loc);

/* Generate panchang for every day in a Gregorian month.
 * days must point to an array of at least 31 PanchangDay elements.
 * count is set to the number of days filled. */
void generate_month_panchang(int year, int month, const Location *loc,
                             PanchangDay *days, int *count);

/* Print a month panchang in tabular format to stdout. */
void print_month_panchang(const PanchangDay *days, int count, double utc_offset);

/* Print a single day's panchang in detail. */
void print_day_panchang(const PanchangDay *day, double utc_offset);

#endif /* PANCHANG_H */
