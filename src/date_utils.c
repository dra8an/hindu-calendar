#include "date_utils.h"
#include "swephexp.h"
#include <math.h>

double gregorian_to_jd(int year, int month, int day)
{
    return swe_julday(year, month, day, 0.0, SE_GREG_CAL);
}

void jd_to_gregorian(double jd, int *year, int *month, int *day)
{
    double ut;
    swe_revjul(jd, SE_GREG_CAL, year, month, day, &ut);
}

int day_of_week(double jd)
{
    /* swe_day_of_week returns 0=Monday..6=Sunday */
    return swe_day_of_week(jd);
}

static const char *DOW_NAMES[] = {
    "Monday", "Tuesday", "Wednesday", "Thursday",
    "Friday", "Saturday", "Sunday"
};

static const char *DOW_SHORT[] = {
    "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

const char *day_of_week_name(int dow)
{
    if (dow < 0 || dow > 6) return "???";
    return DOW_NAMES[dow];
}

const char *day_of_week_short(int dow)
{
    if (dow < 0 || dow > 6) return "???";
    return DOW_SHORT[dow];
}
