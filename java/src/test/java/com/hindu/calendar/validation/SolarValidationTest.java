package com.hindu.calendar.validation;

import com.hindu.calendar.core.Solar;
import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.SolarCalendarType;
import com.hindu.calendar.model.SolarDate;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Solar calendar external validation against drikpanchang.com.
 * 109 dates: Tamil new years + 12-month cycles, Bengali, Odia, Malayalam.
 * Ported from tests/test_solar_validation.c.
 */
class SolarValidationTest {

    private static Solar solar;
    private static final Location LOC = Location.NEW_DELHI;

    @BeforeAll
    static void setUp() {
        Ephemeris eph = new Ephemeris();
        solar = new Solar(eph);
    }

    private void check(int gy, int gm, int gd, SolarCalendarType type,
                        int expMonth, int expDay, int expYear) {
        SolarDate sd = solar.gregorianToSolar(gy, gm, gd, LOC, type);
        String label = type + " " + gy + "-" + gm + "-" + gd;
        assertEquals(expMonth, sd.month(), "month " + label);
        assertEquals(expDay, sd.day(), "day " + label);
        assertEquals(expYear, sd.year(), "year " + label);
    }

    // ===== Tamil: Chithirai 1 (month 1) across 1950-2050, every 5 years =====

    @Test
    void testTamilNewYears() {
        check(1950, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1872);
        check(1955, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1877);
        check(1960, 4, 13, SolarCalendarType.TAMIL, 1, 1, 1882);
        check(1965, 4, 13, SolarCalendarType.TAMIL, 1, 1, 1887);
        check(1970, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1892);
        check(1975, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1897);
        check(1980, 4, 13, SolarCalendarType.TAMIL, 1, 1, 1902);
        check(1985, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1907);
        check(1990, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1912);
        check(1995, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1917);
        check(2000, 4, 13, SolarCalendarType.TAMIL, 1, 1, 1922);
        check(2005, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1927);
        check(2010, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1932);
        check(2015, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1937);
        check(2020, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1942);
        check(2025, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1947);
        check(2030, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1952);
        check(2035, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1957);
        check(2040, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1962);
        check(2045, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1967);
        check(2050, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1972);
    }

    // ===== Tamil: all 12 months of 2025 =====

    @Test
    void testTamil2025Months() {
        check(2025, 1, 14, SolarCalendarType.TAMIL, 10, 1, 1946);
        check(2025, 2, 13, SolarCalendarType.TAMIL, 11, 1, 1946);
        check(2025, 3, 15, SolarCalendarType.TAMIL, 12, 1, 1946);
        check(2025, 4, 14, SolarCalendarType.TAMIL, 1, 1, 1947);
        check(2025, 5, 15, SolarCalendarType.TAMIL, 2, 1, 1947);
        check(2025, 6, 15, SolarCalendarType.TAMIL, 3, 1, 1947);
        check(2025, 7, 16, SolarCalendarType.TAMIL, 4, 1, 1947);
        check(2025, 8, 17, SolarCalendarType.TAMIL, 5, 1, 1947);
        check(2025, 9, 17, SolarCalendarType.TAMIL, 6, 1, 1947);
        check(2025, 10, 17, SolarCalendarType.TAMIL, 7, 1, 1947);
        check(2025, 11, 16, SolarCalendarType.TAMIL, 8, 1, 1947);
        check(2025, 12, 16, SolarCalendarType.TAMIL, 9, 1, 1947);
    }

    // ===== Bengali: Boishakh 1 across 1950-2050 =====

    @Test
    void testBengaliNewYears() {
        check(1950, 4, 14, SolarCalendarType.BENGALI, 1, 1, 1357);
        check(1960, 4, 14, SolarCalendarType.BENGALI, 1, 1, 1367);
        check(1970, 4, 15, SolarCalendarType.BENGALI, 1, 1, 1377);
        check(1980, 4, 14, SolarCalendarType.BENGALI, 1, 1, 1387);
        check(1990, 4, 15, SolarCalendarType.BENGALI, 1, 1, 1397);
        check(2000, 4, 14, SolarCalendarType.BENGALI, 1, 1, 1407);
        check(2010, 4, 15, SolarCalendarType.BENGALI, 1, 1, 1417);
        check(2015, 4, 15, SolarCalendarType.BENGALI, 1, 1, 1422);
        check(2025, 4, 15, SolarCalendarType.BENGALI, 1, 1, 1432);
        check(2030, 4, 15, SolarCalendarType.BENGALI, 1, 1, 1437);
        check(2040, 4, 14, SolarCalendarType.BENGALI, 1, 1, 1447);
        check(2050, 4, 15, SolarCalendarType.BENGALI, 1, 1, 1457);
    }

    // ===== Bengali: all 12 months of 2025 =====

    @Test
    void testBengali2025Months() {
        check(2025, 1, 15, SolarCalendarType.BENGALI, 10, 1, 1431);
        check(2025, 2, 13, SolarCalendarType.BENGALI, 11, 1, 1431);
        check(2025, 3, 15, SolarCalendarType.BENGALI, 12, 1, 1431);
        check(2025, 4, 15, SolarCalendarType.BENGALI, 1, 1, 1432);
        check(2025, 5, 15, SolarCalendarType.BENGALI, 2, 1, 1432);
        check(2025, 6, 16, SolarCalendarType.BENGALI, 3, 1, 1432);
        check(2025, 7, 17, SolarCalendarType.BENGALI, 4, 1, 1432);
        check(2025, 8, 18, SolarCalendarType.BENGALI, 5, 1, 1432);
        check(2025, 9, 18, SolarCalendarType.BENGALI, 6, 1, 1432);
        check(2025, 10, 18, SolarCalendarType.BENGALI, 7, 1, 1432);
        check(2025, 11, 17, SolarCalendarType.BENGALI, 8, 1, 1432);
        check(2025, 12, 17, SolarCalendarType.BENGALI, 9, 1, 1432);
    }

    // ===== Odia: all 12 months of 2025 =====

    @Test
    void testOdia2025Months() {
        check(2025, 1, 14, SolarCalendarType.ODIA, 10, 1, 1432);
        check(2025, 2, 12, SolarCalendarType.ODIA, 11, 1, 1432);
        check(2025, 3, 14, SolarCalendarType.ODIA, 12, 1, 1432);
        check(2025, 4, 14, SolarCalendarType.ODIA, 1, 1, 1432);
        check(2025, 5, 15, SolarCalendarType.ODIA, 2, 1, 1432);
        check(2025, 6, 15, SolarCalendarType.ODIA, 3, 1, 1432);
        check(2025, 7, 16, SolarCalendarType.ODIA, 4, 1, 1432);
        check(2025, 8, 17, SolarCalendarType.ODIA, 5, 1, 1432);
        check(2025, 9, 17, SolarCalendarType.ODIA, 6, 1, 1433);
        check(2025, 10, 17, SolarCalendarType.ODIA, 7, 1, 1433);
        check(2025, 11, 16, SolarCalendarType.ODIA, 8, 1, 1433);
        check(2025, 12, 16, SolarCalendarType.ODIA, 9, 1, 1433);
    }

    // ===== Odia: all 12 months of 2030 =====

    @Test
    void testOdia2030Months() {
        check(2030, 1, 14, SolarCalendarType.ODIA, 10, 1, 1437);
        check(2030, 2, 13, SolarCalendarType.ODIA, 11, 1, 1437);
        check(2030, 3, 15, SolarCalendarType.ODIA, 12, 1, 1437);
        check(2030, 4, 14, SolarCalendarType.ODIA, 1, 1, 1437);
        check(2030, 5, 15, SolarCalendarType.ODIA, 2, 1, 1437);
        check(2030, 6, 15, SolarCalendarType.ODIA, 3, 1, 1437);
        check(2030, 7, 17, SolarCalendarType.ODIA, 4, 1, 1437);
        check(2030, 8, 17, SolarCalendarType.ODIA, 5, 1, 1437);
        check(2030, 9, 17, SolarCalendarType.ODIA, 6, 1, 1438);
        check(2030, 10, 17, SolarCalendarType.ODIA, 7, 1, 1438);
        check(2030, 11, 16, SolarCalendarType.ODIA, 8, 1, 1438);
        check(2030, 12, 16, SolarCalendarType.ODIA, 9, 1, 1438);
    }

    // ===== Malayalam: Chingam 1 across 1950-2030, every 5 years =====

    @Test
    void testMalayalamNewYears() {
        check(1950, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1126);
        check(1955, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1131);
        check(1960, 8, 16, SolarCalendarType.MALAYALAM, 1, 1, 1136);
        check(1965, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1141);
        check(1970, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1146);
        check(1975, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1151);
        check(1985, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1161);
        check(1990, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1166);
        check(1995, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1171);
        check(2000, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1176);
        check(2005, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1181);
        check(2010, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1186);
        check(2015, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1191);
        check(2020, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1196);
        check(2025, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1201);
        check(2030, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1206);
    }

    // ===== Malayalam: all 12 months of 2025 =====

    @Test
    void testMalayalam2025Months() {
        check(2025, 1, 14, SolarCalendarType.MALAYALAM, 6, 1, 1200);
        check(2025, 2, 13, SolarCalendarType.MALAYALAM, 7, 1, 1200);
        check(2025, 3, 15, SolarCalendarType.MALAYALAM, 8, 1, 1200);
        check(2025, 4, 14, SolarCalendarType.MALAYALAM, 9, 1, 1200);
        check(2025, 5, 15, SolarCalendarType.MALAYALAM, 10, 1, 1200);
        check(2025, 6, 15, SolarCalendarType.MALAYALAM, 11, 1, 1200);
        check(2025, 7, 17, SolarCalendarType.MALAYALAM, 12, 1, 1200);
        check(2025, 8, 17, SolarCalendarType.MALAYALAM, 1, 1, 1201);
        check(2025, 9, 17, SolarCalendarType.MALAYALAM, 2, 1, 1201);
        check(2025, 10, 18, SolarCalendarType.MALAYALAM, 3, 1, 1201);
        check(2025, 11, 17, SolarCalendarType.MALAYALAM, 4, 1, 1201);
        check(2025, 12, 16, SolarCalendarType.MALAYALAM, 5, 1, 1201);
    }
}
