package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.Paksha;
import com.hindu.calendar.model.TithiInfo;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class TithiTest {

    private static Ephemeris eph;
    private static Tithi tithi;
    private static final Location LOC = Location.NEW_DELHI;

    @BeforeAll
    static void setUp() {
        eph = new Ephemeris();
        tithi = new Tithi(eph);
    }

    @Test
    void testTithiKnownDates() {
        // Validated against drikpanchang.com
        record TithiCase(int y, int m, int d, int expectedTithi, Paksha expectedPaksha) {}
        TithiCase[] cases = {
                new TithiCase(2013, 1, 18, 7, Paksha.SHUKLA),   // Shukla Saptami
                new TithiCase(2012, 8, 18, 1, Paksha.SHUKLA),   // Shukla Pratipada (Adhika Bhadrapada)
                new TithiCase(2025, 1, 13, 15, Paksha.SHUKLA),  // Purnima
                new TithiCase(2025, 1, 29, 30, Paksha.KRISHNA), // Amavasya (tithi 30)
                new TithiCase(2025, 1, 1, 2, Paksha.SHUKLA),    // Shukla Dwitiya
                new TithiCase(2025, 1, 14, 16, Paksha.KRISHNA), // Krishna Pratipada
                new TithiCase(2025, 1, 30, 1, Paksha.SHUKLA),   // Shukla Pratipada
        };

        for (TithiCase tc : cases) {
            TithiInfo ti = tithi.tithiAtSunrise(tc.y, tc.m, tc.d, LOC);
            assertEquals(tc.expectedTithi, ti.tithiNum(),
                    "Tithi for " + tc.y + "-" + tc.m + "-" + tc.d);
            assertEquals(tc.expectedPaksha, ti.paksha(),
                    "Paksha for " + tc.y + "-" + tc.m + "-" + tc.d);
        }
    }

    @Test
    void testKshayaTithi() {
        // 2025-01-11: kshaya (S-13 skipped between S-12 and S-14)
        TithiInfo ti = tithi.tithiAtSunrise(2025, 1, 11, LOC);
        assertTrue(ti.isKshaya(), "2025-01-11 should be kshaya");

        // 2025-01-12: NOT kshaya
        ti = tithi.tithiAtSunrise(2025, 1, 12, LOC);
        assertFalse(ti.isKshaya(), "2025-01-12 should not be kshaya");
    }

    @Test
    void testAdhikaTithi() {
        // 2025-01-18 and 2025-01-19: same tithi (K-5 repeated)
        TithiInfo ti18 = tithi.tithiAtSunrise(2025, 1, 18, LOC);
        TithiInfo ti19 = tithi.tithiAtSunrise(2025, 1, 19, LOC);
        assertEquals(ti18.tithiNum(), ti19.tithiNum(),
                "2025-01-18 and 2025-01-19 should have same tithi");
    }

    @Test
    void testLunarPhaseAtPurnima() {
        // 2025-01-13 (Purnima): lunar phase should be near 180
        double jd = eph.gregorianToJd(2025, 1, 13);
        double rise = eph.sunriseJd(jd, LOC);
        double phase = tithi.lunarPhase(rise);
        assertTrue(phase > 156.0 && phase < 192.0,
                "Lunar phase at Purnima should be near 180, got " + phase);
    }
}
