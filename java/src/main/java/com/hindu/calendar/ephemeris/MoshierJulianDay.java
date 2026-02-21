package com.hindu.calendar.ephemeris;

/**
 * Julian Day <-> Gregorian calendar conversion.
 * Algorithms from Meeus, "Astronomical Algorithms", 2nd ed., Ch. 7.
 */
class MoshierJulianDay {

    static double julday(int year, int month, int day, double hour) {
        int y = year, m = month;
        if (m <= 2) {
            y -= 1;
            m += 12;
        }
        int A = y / 100;
        int B = 2 - A + A / 4;
        return Math.floor(365.25 * (y + 4716)) + Math.floor(30.6001 * (m + 1))
                + day + hour / 24.0 + B - 1524.5;
    }

    /** Returns [year, month, day], hour is fractional part of day */
    static int[] revjul(double jd) {
        double Z, F, A, alpha, B, C, D, E;
        jd += 0.5;
        Z = Math.floor(jd);
        F = jd - Z;
        if (Z < 2299161.0) {
            A = Z;
        } else {
            alpha = Math.floor((Z - 1867216.25) / 36524.25);
            A = Z + 1 + alpha - Math.floor(alpha / 4.0);
        }
        B = A + 1524;
        C = Math.floor((B - 122.1) / 365.25);
        D = Math.floor(365.25 * C);
        E = Math.floor((B - D) / 30.6001);
        double d = B - D - Math.floor(30.6001 * E) + F;
        int day = (int) d;
        int month;
        if (E < 14)
            month = (int) E - 1;
        else
            month = (int) E - 13;
        int year;
        if (month > 2)
            year = (int) C - 4716;
        else
            year = (int) C - 4715;
        return new int[]{year, month, day};
    }

    /** Returns fractional hour part from JD */
    static double revjulHour(double jd) {
        jd += 0.5;
        double Z = Math.floor(jd);
        double F = jd - Z;
        double A, alpha, B, C, D, E;
        if (Z < 2299161.0) {
            A = Z;
        } else {
            alpha = Math.floor((Z - 1867216.25) / 36524.25);
            A = Z + 1 + alpha - Math.floor(alpha / 4.0);
        }
        B = A + 1524;
        C = Math.floor((B - 122.1) / 365.25);
        D = Math.floor(365.25 * C);
        E = Math.floor((B - D) / 30.6001);
        double d = B - D - Math.floor(30.6001 * E) + F;
        int day = (int) d;
        return (d - day) * 24.0;
    }

    /** ISO convention: 0=Mon, 1=Tue, ..., 6=Sun */
    static int dayOfWeek(double jd) {
        return (((int) Math.floor(jd - 2433282 - 1.5) % 7) + 7) % 7;
    }
}
