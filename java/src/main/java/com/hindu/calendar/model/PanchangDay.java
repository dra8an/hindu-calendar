package com.hindu.calendar.model;

public record PanchangDay(
        int gregYear,
        int gregMonth,
        int gregDay,
        double jdSunrise,
        HinduDate hinduDate,
        TithiInfo tithi
) {}
