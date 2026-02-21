package com.hindu.calendar.ephemeris;

import com.hindu.calendar.model.Location;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class EphemerisTest {

    private static Ephemeris eph;

    @BeforeAll
    static void setUp() {
        eph = new Ephemeris();
    }

    // ===== Julian Day Tests =====

    @Test
    void testJulianDayJ2000() {
        double jd = eph.gregorianToJd(2000, 1, 1);
        assertEquals(2451544.5, jd, 0.0001, "J2000 epoch");
    }

    @Test
    void testJulianDayUnixEpoch() {
        double jd = eph.gregorianToJd(1970, 1, 1);
        assertEquals(2440587.5, jd, 0.0001, "Unix epoch");
    }

    @Test
    void testJulianDayRoundTrip() {
        int[][] dates = {
                {2000, 1, 1}, {2025, 3, 15}, {1900, 6, 21},
                {2050, 12, 31}, {1950, 1, 1}, {1976, 7, 4}
        };
        for (int[] d : dates) {
            double jd = eph.gregorianToJd(d[0], d[1], d[2]);
            int[] ymd = eph.jdToGregorian(jd);
            assertEquals(d[0], ymd[0], "Round-trip year for " + d[0] + "-" + d[1] + "-" + d[2]);
            assertEquals(d[1], ymd[1], "Round-trip month");
            assertEquals(d[2], ymd[2], "Round-trip day");
        }
    }

    @Test
    void testDayOfWeek() {
        // 2025-01-13 is a Monday (0)
        double jd = eph.gregorianToJd(2025, 1, 13);
        assertEquals(0, eph.dayOfWeek(jd), "2025-01-13 should be Monday");

        // 2025-01-19 is a Sunday (6)
        jd = eph.gregorianToJd(2025, 1, 19);
        assertEquals(6, eph.dayOfWeek(jd), "2025-01-19 should be Sunday");
    }

    // ===== Solar Longitude Tests =====

    @Test
    void testSolarLongitudeRange() {
        double jd = eph.gregorianToJd(2025, 3, 20);
        double lon = eph.solarLongitude(jd);
        assertTrue(lon >= 0 && lon < 360, "Solar longitude should be [0, 360)");
    }

    @Test
    void testSolarLongitudeVernalEquinox() {
        // Near vernal equinox 2025: solar longitude ~0 or ~360
        double jd = eph.gregorianToJd(2025, 3, 20);
        double lon = eph.solarLongitude(jd + 0.5);
        // Should be near 0/360
        assertTrue(lon < 5 || lon > 355, "Solar longitude near equinox should be ~0, got " + lon);
    }

    // ===== Lunar Longitude Tests =====

    @Test
    void testLunarLongitudeRange() {
        double jd = eph.gregorianToJd(2025, 1, 13);
        double lon = eph.lunarLongitude(jd);
        assertTrue(lon >= 0 && lon < 360, "Lunar longitude should be [0, 360)");
    }

    // ===== Ayanamsa Tests =====

    @Test
    void testAyanamsaRange() {
        double jd = eph.gregorianToJd(2025, 1, 1);
        double ayan = eph.getAyanamsa(jd);
        // Lahiri ayanamsa in 2025 should be ~24 degrees
        assertTrue(ayan > 23 && ayan < 25, "Ayanamsa should be ~24 for 2025, got " + ayan);
    }

    @Test
    void testAyanamsaAtReferenceEpoch() {
        // At the Lahiri reference epoch (1956 Sep 22.0 TT), ayanamsa ~23.245
        double jd = eph.gregorianToJd(1956, 9, 22);
        double ayan = eph.getAyanamsa(jd);
        assertEquals(23.245, ayan, 0.1, "Ayanamsa at reference epoch");
    }

    // ===== Sunrise/Sunset Tests =====

    @Test
    void testSunriseDelhi() {
        Location delhi = Location.NEW_DELHI;
        double jd = eph.gregorianToJd(2025, 1, 15);
        double rise = eph.sunriseJd(jd, delhi);
        assertTrue(rise > 0, "Sunrise should be found");

        // Sunrise in Delhi mid-January is around 7:10-7:15 IST
        // JD to local: add 0.5 + 5.5/24
        double localJd = rise + 0.5 + 5.5 / 24.0;
        double frac = localJd - Math.floor(localJd);
        double hours = frac * 24.0;
        assertTrue(hours > 7.0 && hours < 7.5, "Delhi sunrise in Jan should be ~7:10-7:15 IST, got " + hours);
    }

    @Test
    void testSunsetDelhi() {
        Location delhi = Location.NEW_DELHI;
        double jd = eph.gregorianToJd(2025, 1, 15);
        double set = eph.sunsetJd(jd, delhi);
        assertTrue(set > 0, "Sunset should be found");

        double localJd = set + 0.5 + 5.5 / 24.0;
        double frac = localJd - Math.floor(localJd);
        double hours = frac * 24.0;
        assertTrue(hours > 17.0 && hours < 18.0, "Delhi sunset in Jan should be ~17:30-18:00 IST, got " + hours);
    }

    // ===== Sidereal Solar Longitude Tests =====

    @Test
    void testSiderealSolarLongitude() {
        double jd = eph.gregorianToJd(2025, 4, 14);
        double sid = eph.solarLongitudeSidereal(jd);
        // Near Mesha Sankranti, sidereal longitude should be near 0 or 360
        assertTrue(sid < 5 || sid > 355, "Sidereal longitude near Mesha Sankranti should be ~0, got " + sid);
    }
}
