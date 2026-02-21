package com.hindu.calendar.model;

public record MasaInfo(
        MasaName name,
        boolean isAdhika,
        int yearSaka,
        int yearVikram,
        double jdStart,
        double jdEnd
) {}
