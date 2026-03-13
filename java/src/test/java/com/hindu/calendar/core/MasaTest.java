package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.LunisolarScheme;
import com.hindu.calendar.model.MasaInfo;
import com.hindu.calendar.model.MasaName;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class MasaTest {

    private static Masa masa;
    private static Ephemeris eph;
    private static final Location LOC = Location.NEW_DELHI;

    @BeforeAll
    static void setUp() {
        eph = new Ephemeris();
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

    // ===== Amanta month start tests =====

    @Test
    void testAmantaMonthStarts() {
        // Known Amanta month starts for Saka 1947 (verified against drikpanchang.com)
        record MonthCase(MasaName masa, int sakaYear, boolean isAdhika,
                         int expY, int expM, int expD) {}
        MonthCase[] cases = {
                new MonthCase(MasaName.CHAITRA, 1947, false, 2025, 3, 30),
                new MonthCase(MasaName.VAISHAKHA, 1947, false, 2025, 4, 28),
                new MonthCase(MasaName.JYESHTHA, 1947, false, 2025, 5, 28),
                new MonthCase(MasaName.ASHADHA, 1947, false, 2025, 6, 26),
                new MonthCase(MasaName.SHRAVANA, 1947, false, 2025, 7, 25),
                new MonthCase(MasaName.BHADRAPADA, 1947, false, 2025, 8, 24),
                new MonthCase(MasaName.ASHVINA, 1947, false, 2025, 9, 22),
                new MonthCase(MasaName.KARTIKA, 1947, false, 2025, 10, 22),
                new MonthCase(MasaName.MARGASHIRSHA, 1947, false, 2025, 11, 21),
                new MonthCase(MasaName.PAUSHA, 1947, false, 2025, 12, 21),

                // Adhika Bhadrapada 2012 (Saka 1934)
                new MonthCase(MasaName.BHADRAPADA, 1934, true, 2012, 8, 18),
                new MonthCase(MasaName.BHADRAPADA, 1934, false, 2012, 9, 17),

                // Year boundary
                new MonthCase(MasaName.PHALGUNA, 1946, false, 2025, 2, 28),
                new MonthCase(MasaName.CHAITRA, 1946, false, 2024, 4, 9),
        };

        for (MonthCase mc : cases) {
            double jd = masa.lunisolarMonthStart(mc.masa, mc.sakaYear, mc.isAdhika,
                    LunisolarScheme.AMANTA, LOC);
            assertTrue(jd > 0, mc.masa + " " + mc.sakaYear + " should be found");
            int[] ymd = eph.jdToGregorian(jd);
            String label = mc.masa + " " + mc.sakaYear + (mc.isAdhika ? " adhika" : "");
            assertEquals(mc.expY, ymd[0], label + " year");
            assertEquals(mc.expM, ymd[1], label + " month");
            assertEquals(mc.expD, ymd[2], label + " day");
        }
    }

    // ===== Amanta month length tests =====

    @Test
    void testAmantaMonthLengths() {
        for (MasaName m : MasaName.values()) {
            int len = masa.lunisolarMonthLength(m, 1947, false,
                    LunisolarScheme.AMANTA, LOC);
            assertTrue(len == 29 || len == 30,
                    m + " 1947 length should be 29 or 30, got " + len);
        }

        // start + length = next month start
        double jdStart = masa.lunisolarMonthStart(MasaName.VAISHAKHA, 1947, false,
                LunisolarScheme.AMANTA, LOC);
        int len = masa.lunisolarMonthLength(MasaName.VAISHAKHA, 1947, false,
                LunisolarScheme.AMANTA, LOC);
        double jdNext = masa.lunisolarMonthStart(MasaName.JYESHTHA, 1947, false,
                LunisolarScheme.AMANTA, LOC);
        assertEquals((int) (jdNext - jdStart), len, "Vaishakha+length = Jyeshtha start");
    }

    // ===== Purnimanta month start tests =====

    @Test
    void testPurnimantaMonthStarts() {
        for (MasaName m : MasaName.values()) {
            double jdAmanta = masa.lunisolarMonthStart(m, 1947, false,
                    LunisolarScheme.AMANTA, LOC);
            double jdPurni = masa.lunisolarMonthStart(m, 1947, false,
                    LunisolarScheme.PURNIMANTA, LOC);

            assertTrue(jdPurni > 0, "Purnimanta " + m + " 1947 should be found");

            // Purnimanta start should be ~13-17 days before Amanta start
            double diff = jdAmanta - jdPurni;
            assertTrue(diff >= 13 && diff <= 17,
                    "Purnimanta " + m + " 1947 offset " + diff + " should be in [13,17]");

            // First day should have Krishna paksha tithi (>=16)
            double jr = eph.sunriseJd(jdPurni, LOC);
            if (jr <= 0) jr = jdPurni + 0.5 - LOC.utcOffset() / 24.0;
            Tithi tithi = new Tithi(eph);
            int t = tithi.tithiAtMoment(jr);
            assertTrue(t >= 16 && t <= 30,
                    "Purnimanta " + m + " 1947 tithi " + t + " should be in Krishna paksha");
        }
    }

    // ===== Purnimanta month length tests =====

    @Test
    void testPurnimantaMonthLengths() {
        for (MasaName m : MasaName.values()) {
            int len = masa.lunisolarMonthLength(m, 1947, false,
                    LunisolarScheme.PURNIMANTA, LOC);
            assertTrue(len == 29 || len == 30,
                    "Purnimanta " + m + " 1947 length should be 29 or 30, got " + len);
        }

        // start + length = next month start
        double jdStart = masa.lunisolarMonthStart(MasaName.VAISHAKHA, 1947, false,
                LunisolarScheme.PURNIMANTA, LOC);
        int len = masa.lunisolarMonthLength(MasaName.VAISHAKHA, 1947, false,
                LunisolarScheme.PURNIMANTA, LOC);
        double jdNext = masa.lunisolarMonthStart(MasaName.JYESHTHA, 1947, false,
                LunisolarScheme.PURNIMANTA, LOC);
        assertEquals((int) (jdNext - jdStart), len,
                "Purnimanta Vaishakha+length = Jyeshtha start");
    }
}
