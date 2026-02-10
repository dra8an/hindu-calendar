#ifndef MASA_H
#define MASA_H

#include "types.h"

/* Find JD of new moon before the given JD.
 * Uses the tithi number at jd_ut as a hint for search range. */
double new_moon_before(double jd_ut, int tithi_hint);

/* Find JD of new moon after the given JD.
 * Uses the tithi number at jd_ut as a hint for search range. */
double new_moon_after(double jd_ut, int tithi_hint);

/* Solar rashi (zodiac sign 1-12) at a given moment.
 * 1 = Mesha, 2 = Vrishabha, ..., 12 = Meena.
 * Based on sidereal (Nirayana) solar longitude. */
int solar_rashi(double jd_ut);

/* Determine the lunar month (masa) for a given Gregorian date. */
MasaInfo masa_for_date(int year, int month, int day, const Location *loc);

/* Saka year for a given date and masa number. */
int hindu_year_saka(double jd_ut, int masa_num);

/* Vikram Samvat year for a given Saka year. */
int hindu_year_vikram(int saka_year);

#endif /* MASA_H */
