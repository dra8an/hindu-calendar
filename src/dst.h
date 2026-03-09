/*
 * dst.h - US Eastern timezone DST rules
 *
 * Determines the UTC offset for US Eastern time on a given date,
 * accounting for historical changes in DST rules (1900-2050).
 * Used for NYC panchang validation.
 */
#ifndef DST_H
#define DST_H

/*
 * us_eastern_offset - UTC offset for US Eastern time.
 *
 *   y, m, d: Gregorian date.
 *   Returns: -5.0 (EST) or -4.0 (EDT).
 *
 * Handles all historical US DST rule changes:
 *   Pre-1918:  no DST (always EST)
 *   1918-1919: first US DST
 *   1942-1945: War Time (year-round DST)
 *   1966+:     Uniform Time Act
 *   2007+:     Energy Policy Act (extended DST)
 */
double us_eastern_offset(int y, int m, int d);

#endif /* DST_H */
