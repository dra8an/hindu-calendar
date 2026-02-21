package com.hindu.calendar.model;

public record TithiInfo(
        int tithiNum,      // 1-30 (1-15 Shukla, 16-30 Krishna)
        Paksha paksha,
        int pakshaTithi,   // 1-15 within the paksha
        double jdStart,    // Julian day when this tithi starts
        double jdEnd,      // Julian day when this tithi ends
        boolean isKshaya   // true if next tithi is skipped (no sunrise during it)
) {
    public static final String[] TITHI_NAMES = {
            "",             // 0 - unused
            "Pratipada",    // 1
            "Dwitiya",      // 2
            "Tritiya",      // 3
            "Chaturthi",    // 4
            "Panchami",     // 5
            "Shashthi",     // 6
            "Saptami",      // 7
            "Ashtami",      // 8
            "Navami",       // 9
            "Dashami",      // 10
            "Ekadashi",     // 11
            "Dwadashi",     // 12
            "Trayodashi",   // 13
            "Chaturdashi",  // 14
            "Purnima",      // 15 - full moon (end of Shukla)
    };
}
