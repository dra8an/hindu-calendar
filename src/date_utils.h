#ifndef DATE_UTILS_H
#define DATE_UTILS_H

/* Convert Gregorian date to Julian Day number */
double gregorian_to_jd(int year, int month, int day);

/* Convert Julian Day number to Gregorian date */
void jd_to_gregorian(double jd, int *year, int *month, int *day);

/* Day of week (0=Monday, 6=Sunday) from Julian Day */
int day_of_week(double jd);

/* Day of week name */
const char *day_of_week_name(int dow);

/* Short day of week name (3 chars) */
const char *day_of_week_short(int dow);

#endif /* DATE_UTILS_H */
