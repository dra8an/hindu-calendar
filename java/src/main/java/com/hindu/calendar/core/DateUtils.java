package com.hindu.calendar.core;

public class DateUtils {

    private static final String[] DOW_NAMES = {
            "Monday", "Tuesday", "Wednesday", "Thursday",
            "Friday", "Saturday", "Sunday"
    };

    private static final String[] DOW_SHORT = {
            "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };

    private static final int[] MDAYS = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    public static int daysInMonth(int year, int month) {
        if (month == 2) {
            if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
                return 29;
        }
        return MDAYS[month];
    }

    public static String dayOfWeekName(int dow) {
        if (dow < 0 || dow > 6) return "???";
        return DOW_NAMES[dow];
    }

    public static String dayOfWeekShort(int dow) {
        if (dow < 0 || dow > 6) return "???";
        return DOW_SHORT[dow];
    }
}
