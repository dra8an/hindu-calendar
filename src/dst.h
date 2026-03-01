#ifndef DST_H
#define DST_H

/* US Eastern time UTC offset for a given Gregorian date.
 * Returns -5.0 (EST) or -4.0 (EDT) based on historical DST rules.
 * Covers 1900-2050. */
double us_eastern_offset(int y, int m, int d);

#endif /* DST_H */
