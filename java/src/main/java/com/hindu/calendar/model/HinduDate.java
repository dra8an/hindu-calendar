package com.hindu.calendar.model;

public record HinduDate(
        int yearSaka,
        int yearVikram,
        MasaName masa,
        boolean isAdhikaMasa,
        Paksha paksha,
        int tithi,            // Tithi number within paksha (1-15)
        boolean isAdhikaTithi // Same tithi as previous day
) {}
