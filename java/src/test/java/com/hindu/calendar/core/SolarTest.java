package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.SolarCalendarType;
import com.hindu.calendar.model.SolarDate;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class SolarTest {

    private static Solar solar;
    private static Ephemeris eph;
    private static final Location LOC = Location.NEW_DELHI;

    @BeforeAll
    static void setUp() {
        eph = new Ephemeris();
        solar = new Solar(eph);
    }

    private void checkSolar(int gy, int gm, int gd, SolarCalendarType type,
                            int expMonth, int expDay, int expYear) {
        SolarDate sd = solar.gregorianToSolar(gy, gm, gd, LOC, type);
        String label = gy + "-" + gm + "-" + gd + " " + type;
        assertEquals(expMonth, sd.month(), "month " + label);
        assertEquals(expDay, sd.day(), "day " + label);
        assertEquals(expYear, sd.year(), "year " + label);
    }

    private void checkRoundTrip(int gy, int gm, int gd, SolarCalendarType type) {
        SolarDate sd = solar.gregorianToSolar(gy, gm, gd, LOC, type);
        int[] result = solar.solarToGregorian(sd, type, LOC);
        String label = gy + "-" + gm + "-" + gd + " " + type;
        assertEquals(gy, result[0], "roundtrip year " + label);
        assertEquals(gm, result[1], "roundtrip month " + label);
        assertEquals(gd, result[2], "roundtrip day " + label);
    }

    // ===== Sankranti =====

    @Test
    void testSankrantiJd() {
        // Mesha Sankranti 2025: sidereal solar longitude at 0 deg
        double jdEst = eph.gregorianToJd(2025, 4, 14);
        double jdSank = solar.sankrantiJd(jdEst, 0.0);

        double lon = eph.solarLongitudeSidereal(jdSank);
        assertTrue(Math.abs(lon) < 0.0001 || Math.abs(lon - 360) < 0.0001,
                "Sidereal longitude at Mesha Sankranti should be ~0, got " + lon);

        int[] ymd = eph.jdToGregorian(jdSank);
        assertTrue(ymd[1] == 4 && (ymd[2] == 13 || ymd[2] == 14),
                "Mesha Sankranti 2025 should be April 13-14");
    }

    // ===== Month Names =====

    @Test
    void testMonthNames() {
        assertEquals("Chithirai", SolarCalendarType.TAMIL.monthName(1));
        assertEquals("Boishakh", SolarCalendarType.BENGALI.monthName(1));
        assertEquals("Baisakha", SolarCalendarType.ODIA.monthName(1));
        assertEquals("Chingam", SolarCalendarType.MALAYALAM.monthName(1));
        assertEquals("Karkadakam", SolarCalendarType.MALAYALAM.monthName(12));
    }

    @Test
    void testEraNames() {
        assertEquals("Saka", SolarCalendarType.TAMIL.eraName());
        assertEquals("Bangabda", SolarCalendarType.BENGALI.eraName());
        assertEquals("Saka", SolarCalendarType.ODIA.eraName());
        assertEquals("Kollam", SolarCalendarType.MALAYALAM.eraName());
    }

    // ===== Tamil Calendar =====

    @Test
    void testTamilNewYear2025() {
        checkSolar(2025, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1947);
    }

    @Test
    void testTamilDay2() {
        checkSolar(2025, 4, 15, SolarCalendarType.TAMIL, 1, 2, 1947);
    }

    @Test
    void testTamilLastDayPreviousYear() {
        checkSolar(2025, 4, 13, SolarCalendarType.TAMIL, 12, 30, 1946);
    }

    @Test
    void testTamil2000() {
        checkSolar(2000, 4, 13, SolarCalendarType.TAMIL, 1, 1, 1922);
    }

    // ===== Bengali Calendar =====

    @Test
    void testBengaliNewYear2025() {
        checkSolar(2025, 4, 15, SolarCalendarType.BENGALI, 1, 1, 1432);
    }

    @Test
    void testBengaliMidMonth() {
        checkSolar(2025, 4, 30, SolarCalendarType.BENGALI, 1, 16, 1432);
    }

    @Test
    void testBengaliLastDayPreviousYear() {
        checkSolar(2025, 4, 14, SolarCalendarType.BENGALI, 12, 31, 1431);
    }

    // ===== Odia Calendar =====

    @Test
    void testOdiaNewYear2025() {
        checkSolar(2025, 4, 14, SolarCalendarType.ODIA, 1, 1, 1947);
    }

    @Test
    void testOdiaLastDayPreviousYear() {
        checkSolar(2025, 4, 13, SolarCalendarType.ODIA, 12, 31, 1946);
    }

    // ===== Malayalam Calendar =====

    @Test
    void testMalayalamNewYear2025() {
        checkSolar(2025, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1201);
    }

    @Test
    void testMalayalamLastDayPreviousYear() {
        checkSolar(2025, 8, 16, SolarCalendarType.MALAYALAM, 12, 31, 1200);
    }

    // ===== Round-trip tests =====

    @Test
    void testTamilRoundTrip() {
        checkRoundTrip(2025, 4, 14, SolarCalendarType.TAMIL);
        checkRoundTrip(2025, 4, 15, SolarCalendarType.TAMIL);
        checkRoundTrip(2025, 6, 15, SolarCalendarType.TAMIL);
        checkRoundTrip(2000, 4, 13, SolarCalendarType.TAMIL);
    }

    @Test
    void testBengaliRoundTrip() {
        checkRoundTrip(2025, 4, 15, SolarCalendarType.BENGALI);
        checkRoundTrip(2025, 4, 30, SolarCalendarType.BENGALI);
        checkRoundTrip(2025, 6, 15, SolarCalendarType.BENGALI);
    }

    @Test
    void testOdiaRoundTrip() {
        checkRoundTrip(2025, 4, 14, SolarCalendarType.ODIA);
        checkRoundTrip(2025, 4, 13, SolarCalendarType.ODIA);
    }

    @Test
    void testMalayalamRoundTrip() {
        checkRoundTrip(2025, 8, 17, SolarCalendarType.MALAYALAM);
        checkRoundTrip(2025, 8, 16, SolarCalendarType.MALAYALAM);
    }

    // ===== Additional Tamil/Bengali/Odia/Malayalam dates from C test_solar.c =====

    @Test
    void testTamilAdditional() {
        checkSolar(2025, 4, 30, SolarCalendarType.TAMIL, 1, 17, 1947);
        checkSolar(2025, 4, 1, SolarCalendarType.TAMIL, 12, 18, 1946);
        checkSolar(2025, 1, 1, SolarCalendarType.TAMIL, 9, 17, 1946);
        checkSolar(2025, 7, 15, SolarCalendarType.TAMIL, 3, 31, 1947);
        checkSolar(2000, 4, 14, SolarCalendarType.TAMIL, 1, 2, 1922);
        checkSolar(2000, 1, 1, SolarCalendarType.TAMIL, 9, 17, 1921);
    }

    @Test
    void testBengaliAdditional() {
        checkSolar(2025, 4, 16, SolarCalendarType.BENGALI, 1, 2, 1432);
        checkSolar(2025, 4, 13, SolarCalendarType.BENGALI, 12, 30, 1431);
        checkSolar(2025, 4, 1, SolarCalendarType.BENGALI, 12, 18, 1431);
        checkSolar(2025, 1, 1, SolarCalendarType.BENGALI, 9, 17, 1431);
        checkSolar(2025, 6, 15, SolarCalendarType.BENGALI, 2, 32, 1432);
        checkSolar(2000, 1, 1, SolarCalendarType.BENGALI, 9, 16, 1406);
    }

    @Test
    void testOdiaAdditional() {
        checkSolar(2025, 4, 15, SolarCalendarType.ODIA, 1, 2, 1947);
        checkSolar(2025, 4, 30, SolarCalendarType.ODIA, 1, 17, 1947);
        checkSolar(2025, 4, 13, SolarCalendarType.ODIA, 12, 31, 1946);
        checkSolar(2025, 4, 1, SolarCalendarType.ODIA, 12, 19, 1946);
        checkSolar(2025, 1, 1, SolarCalendarType.ODIA, 9, 18, 1946);
        checkSolar(2025, 7, 15, SolarCalendarType.ODIA, 3, 31, 1947);
    }

    @Test
    void testMalayalamAdditional() {
        checkSolar(2025, 8, 18, SolarCalendarType.MALAYALAM, 1, 2, 1201);
        checkSolar(2025, 8, 31, SolarCalendarType.MALAYALAM, 1, 15, 1201);
        checkSolar(2025, 8, 1, SolarCalendarType.MALAYALAM, 12, 16, 1200);
        checkSolar(2025, 1, 1, SolarCalendarType.MALAYALAM, 5, 17, 1200);
        checkSolar(2025, 4, 15, SolarCalendarType.MALAYALAM, 9, 2, 1200);
    }

    // ===== Odia boundary cases near 22:12 IST =====

    @Test
    void testOdiaBoundary() {
        // sankranti after 22:12 -> next day
        checkSolar(2026, 7, 16, SolarCalendarType.ODIA, 3, 32, 1948);
        checkSolar(2026, 7, 17, SolarCalendarType.ODIA, 4, 1, 1948);
        checkSolar(2022, 7, 16, SolarCalendarType.ODIA, 3, 32, 1944);
        checkSolar(2022, 7, 17, SolarCalendarType.ODIA, 4, 1, 1944);
        checkSolar(2018, 7, 16, SolarCalendarType.ODIA, 3, 32, 1940);
        checkSolar(2018, 7, 17, SolarCalendarType.ODIA, 4, 1, 1940);
        checkSolar(2001, 4, 13, SolarCalendarType.ODIA, 12, 31, 1922);
        checkSolar(2001, 4, 14, SolarCalendarType.ODIA, 1, 1, 1923);
        // sankranti before 22:12 -> current day
        checkSolar(2024, 12, 15, SolarCalendarType.ODIA, 9, 1, 1946);
        checkSolar(2013, 5, 14, SolarCalendarType.ODIA, 1, 31, 1935);
        checkSolar(2003, 11, 16, SolarCalendarType.ODIA, 7, 30, 1925);
    }
}
