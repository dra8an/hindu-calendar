package com.hindu.calendar.model;

public record SolarDate(
        int year,           // Regional era year
        int month,          // 1-12 (regional month number)
        int day,            // Day within solar month (1-32)
        int rashi,          // Sidereal zodiac sign 1-12 at critical time
        double jdSankranti  // JD of the sankranti that started this month
) {}
