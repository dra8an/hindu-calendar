#ifndef TITHI_H
#define TITHI_H

#include "types.h"

/* Lunar phase angle (0-360 degrees): (moon_long - sun_long) mod 360
 * 0 = new moon, 180 = full moon */
double lunar_phase(double jd_ut);

/* Tithi number (1-30) at a given moment.
 * 1 = Shukla Pratipada, 15 = Purnima, 30 = Amavasya */
int tithi_at_moment(double jd_ut);

/* Tithi at sunrise for a given Gregorian date and location.
 * Fills in TithiInfo including tithi boundaries and kshaya detection. */
TithiInfo tithi_at_sunrise(int year, int month, int day, const Location *loc);

/* Find the exact Julian Day when a tithi boundary occurs.
 * Searches between jd_start and jd_end for the moment when
 * the lunar phase crosses the boundary for target_tithi.
 * Returns the JD of the boundary. */
double find_tithi_boundary(double jd_start, double jd_end, int target_tithi);

#endif /* TITHI_H */
