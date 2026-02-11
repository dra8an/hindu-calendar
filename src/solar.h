#ifndef SOLAR_H
#define SOLAR_H

#include "types.h"

/* Find exact JD when sidereal solar longitude crosses a target (0, 30, 60...).
 * jd_approx should be a rough estimate within ~15 days of the crossing. */
double sankranti_jd(double jd_approx, double target_longitude);

/* Find the sankranti (rashi entry) on or before a given JD.
 * Returns the JD of when the sun entered its current sidereal sign. */
double sankranti_before(double jd_ut);

/* Convert Gregorian date to solar calendar date for a given regional variant. */
SolarDate gregorian_to_solar(int year, int month, int day,
                             const Location *loc, SolarCalendarType type);

/* Convert solar calendar date back to Gregorian (inverse). */
void solar_to_gregorian(const SolarDate *sd, SolarCalendarType type,
                        const Location *loc, int *year, int *month, int *day);

/* Regional month name string (1-indexed). */
const char *solar_month_name(int month, SolarCalendarType type);

/* Regional era name string. */
const char *solar_era_name(SolarCalendarType type);

#endif /* SOLAR_H */
