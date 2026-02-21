package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.MasaInfo;
import com.hindu.calendar.model.MasaName;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class MasaTest {

    private static Masa masa;
    private static final Location LOC = Location.NEW_DELHI;

    @BeforeAll
    static void setUp() {
        Ephemeris eph = new Ephemeris();
        Tithi tithi = new Tithi(eph);
        masa = new Masa(eph, tithi);
    }

    @Test
    void testMasaKnownDates() {
        record MasaCase(int y, int m, int d, int expectedMasa, boolean expectedAdhika) {}
        MasaCase[] cases = {
                new MasaCase(2013, 1, 18, 10, false),   // Pausha
                new MasaCase(2013, 2, 10, 10, false),   // Pausha
                new MasaCase(2012, 8, 17, 5, false),    // Shravana (Amavasya)
                new MasaCase(2012, 8, 18, 6, true),     // Adhika Bhadrapada
                new MasaCase(2012, 9, 18, 6, false),    // Nija Bhadrapada
                new MasaCase(2025, 1, 1, 10, false),    // Pausha
                new MasaCase(2025, 1, 30, 11, false),   // Magha
                new MasaCase(2025, 3, 30, 1, false),    // Chaitra
                new MasaCase(2025, 4, 30, 2, false),    // Vaishakha
        };

        for (MasaCase mc : cases) {
            MasaInfo mi = masa.masaForDate(mc.y, mc.m, mc.d, LOC);
            assertEquals(mc.expectedMasa, mi.name().number(),
                    "Masa for " + mc.y + "-" + mc.m + "-" + mc.d);
            assertEquals(mc.expectedAdhika, mi.isAdhika(),
                    "Adhika for " + mc.y + "-" + mc.m + "-" + mc.d);
        }
    }

    @Test
    void testYearDetermination() {
        MasaInfo mi = masa.masaForDate(2025, 1, 18, LOC);
        assertEquals(1946, mi.yearSaka(), "Saka year for 2025-01-18");
        assertEquals(2081, mi.yearVikram(), "Vikram year for 2025-01-18");

        mi = masa.masaForDate(2012, 8, 18, LOC);
        assertEquals(1934, mi.yearSaka(), "Saka year for 2012-08-18");
        assertEquals(2069, mi.yearVikram(), "Vikram year for 2012-08-18");
    }
}
