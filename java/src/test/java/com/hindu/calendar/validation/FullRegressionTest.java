package com.hindu.calendar.validation;

import com.hindu.calendar.core.Masa;
import com.hindu.calendar.core.Solar;
import com.hindu.calendar.core.Tithi;
import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.MasaInfo;
import com.hindu.calendar.model.SolarCalendarType;
import com.hindu.calendar.model.SolarDate;
import com.hindu.calendar.model.TithiInfo;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.io.BufferedReader;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Full regression: every day 1900-2050 for lunisolar + all 4 solar calendars.
 * Reads C-generated CSVs and checks Java output matches exactly.
 *
 * Lunisolar: 55,152 days × 4 checks (tithi, masa, adhika, saka) = 220,608 assertions
 * Solar: 4 calendars × ~55,152 days × 3 checks (month, day, year) = ~661,824 assertions
 * Total: ~882,432 assertions
 */
class FullRegressionTest {

    private static final Path BASE = Path.of(System.getProperty("user.dir"))
            .getParent().resolve("validation").resolve("moshier");
    private static final Location LOC = Location.NEW_DELHI;

    private static Ephemeris eph;
    private static Tithi tithi;
    private static Masa masa;
    private static Solar solar;

    @BeforeAll
    static void setUp() {
        eph = new Ephemeris();
        tithi = new Tithi(eph);
        masa = new Masa(eph, tithi);
        solar = new Solar(eph);
    }

    // ===== Lunisolar: every day 1900-2050 =====

    @Test
    void testLunisolarFullRegression() throws IOException {
        Path csv = BASE.resolve("ref_1900_2050.csv");
        assertTrue(Files.exists(csv), "Missing " + csv);

        int checked = 0;
        int failures = 0;
        StringBuilder errors = new StringBuilder();

        try (BufferedReader br = Files.newBufferedReader(csv)) {
            String header = br.readLine(); // skip header
            assertNotNull(header);

            String line;
            while ((line = br.readLine()) != null) {
                String[] parts = line.split(",");
                int y = Integer.parseInt(parts[0]);
                int m = Integer.parseInt(parts[1]);
                int d = Integer.parseInt(parts[2]);
                int expTithi = Integer.parseInt(parts[3]);
                int expMasa = Integer.parseInt(parts[4]);
                int expAdhika = Integer.parseInt(parts[5]);
                int expSaka = Integer.parseInt(parts[6]);

                TithiInfo ti = tithi.tithiAtSunrise(y, m, d, LOC);
                MasaInfo mi = masa.masaForDate(y, m, d, LOC);

                boolean ok = true;
                if (ti.tithiNum() != expTithi) {
                    errors.append(String.format("%04d-%02d-%02d tithi: got %d, expected %d%n",
                            y, m, d, ti.tithiNum(), expTithi));
                    ok = false;
                }
                if (mi.name().number() != expMasa) {
                    errors.append(String.format("%04d-%02d-%02d masa: got %d, expected %d%n",
                            y, m, d, mi.name().number(), expMasa));
                    ok = false;
                }
                if ((mi.isAdhika() ? 1 : 0) != expAdhika) {
                    errors.append(String.format("%04d-%02d-%02d adhika: got %s, expected %d%n",
                            y, m, d, mi.isAdhika(), expAdhika));
                    ok = false;
                }
                if (mi.yearSaka() != expSaka) {
                    errors.append(String.format("%04d-%02d-%02d saka: got %d, expected %d%n",
                            y, m, d, mi.yearSaka(), expSaka));
                    ok = false;
                }

                if (!ok) failures++;
                checked++;
            }
        }

        System.out.printf("Lunisolar: %d days checked, %d failures%n", checked, failures);
        if (failures > 0) {
            fail(String.format("Lunisolar: %d/%d failures:%n%s",
                    failures, checked, errors.toString().substring(0, Math.min(errors.length(), 5000))));
        }
        assertTrue(checked > 55000, "Expected 55,000+ days, got " + checked);
    }

    // ===== Solar: every day for all 4 calendars =====

    @Test
    void testTamilFullRegression() throws IOException {
        checkSolarCalendar("tamil_months_1900_2050.csv", SolarCalendarType.TAMIL);
    }

    @Test
    void testBengaliFullRegression() throws IOException {
        checkSolarCalendar("bengali_months_1900_2050.csv", SolarCalendarType.BENGALI);
    }

    @Test
    void testOdiaFullRegression() throws IOException {
        checkSolarCalendar("odia_months_1900_2050.csv", SolarCalendarType.ODIA);
    }

    @Test
    void testMalayalamFullRegression() throws IOException {
        checkSolarCalendar("malayalam_months_1900_2050.csv", SolarCalendarType.MALAYALAM);
    }

    private void checkSolarCalendar(String csvName, SolarCalendarType type) throws IOException {
        Path csv = BASE.resolve("solar").resolve(csvName);
        assertTrue(Files.exists(csv), "Missing " + csv);

        int checked = 0;
        int failures = 0;
        StringBuilder errors = new StringBuilder();

        try (BufferedReader br = Files.newBufferedReader(csv)) {
            String header = br.readLine();
            assertNotNull(header);

            String line;
            while ((line = br.readLine()) != null) {
                String[] parts = line.split(",");
                int solarMonth = Integer.parseInt(parts[0]);
                int solarYear = Integer.parseInt(parts[1]);
                int length = Integer.parseInt(parts[2]);
                int gregYear = Integer.parseInt(parts[3]);
                int gregMonth = Integer.parseInt(parts[4]);
                int gregDay = Integer.parseInt(parts[5]);

                // Check every day in this solar month
                for (int dayNum = 1; dayNum <= length; dayNum++) {
                    // Compute Gregorian date: start + (dayNum - 1)
                    double jd = eph.gregorianToJd(gregYear, gregMonth, gregDay) + (dayNum - 1);
                    int[] gdate = eph.jdToGregorian(jd);

                    SolarDate sd = solar.gregorianToSolar(gdate[0], gdate[1], gdate[2], LOC, type);

                    boolean ok = true;
                    if (sd.month() != solarMonth) {
                        errors.append(String.format("%s %04d-%02d-%02d month: got %d, expected %d%n",
                                type, gdate[0], gdate[1], gdate[2], sd.month(), solarMonth));
                        ok = false;
                    }
                    if (sd.day() != dayNum) {
                        errors.append(String.format("%s %04d-%02d-%02d day: got %d, expected %d%n",
                                type, gdate[0], gdate[1], gdate[2], sd.day(), dayNum));
                        ok = false;
                    }
                    if (sd.year() != solarYear) {
                        errors.append(String.format("%s %04d-%02d-%02d year: got %d, expected %d%n",
                                type, gdate[0], gdate[1], gdate[2], sd.year(), solarYear));
                        ok = false;
                    }

                    if (!ok) failures++;
                    checked++;
                }
            }
        }

        System.out.printf("%s: %d days checked, %d failures%n", type, checked, failures);
        if (failures > 0) {
            fail(String.format("%s: %d/%d failures:%n%s",
                    type, failures, checked, errors.toString().substring(0, Math.min(errors.length(), 5000))));
        }
        assertTrue(checked > 54000, type + ": Expected 54,000+ days, got " + checked);
    }
}
