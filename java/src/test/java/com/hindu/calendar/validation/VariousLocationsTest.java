package com.hindu.calendar.validation;

import com.hindu.calendar.core.Solar;
import com.hindu.calendar.core.Tithi;
import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.SolarCalendarType;
import com.hindu.calendar.model.SolarDate;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.io.BufferedReader;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Reads validation/moshier/various_locations.csv and verifies lunisolar tithi
 * and solar calendar calculations for multiple locations (Ujjain, NYC, LA).
 * Ported from tests/test_various_locations.c.
 */
class VariousLocationsTest {

    private static final Path CSV = Path.of(System.getProperty("user.dir"))
            .getParent().resolve("validation").resolve("moshier")
            .resolve("various_locations.csv");

    private static Ephemeris eph;
    private static Tithi tithi;
    private static Solar solar;

    @BeforeAll
    static void setUp() {
        eph = new Ephemeris();
        tithi = new Tithi(eph);
        solar = new Solar(eph);
    }

    @Test
    void testVariousLocations() throws IOException {
        if (!Files.exists(CSV)) {
            System.out.println("Skipping: " + CSV + " not found");
            return;
        }

        int checked = 0;
        int failures = 0;
        StringBuilder errors = new StringBuilder();

        try (BufferedReader br = Files.newBufferedReader(CSV)) {
            String header = br.readLine(); // skip header
            assertNotNull(header);

            String line;
            while ((line = br.readLine()) != null) {
                // Parse CSV manually to handle empty fields
                String[] fields = line.split(",", -1);
                if (fields.length < 8) continue;

                String calendar = fields[0].trim();
                String location = fields[1].trim();
                double lat = Double.parseDouble(fields[2]);
                double lon = Double.parseDouble(fields[3]);
                double utcOffset = Double.parseDouble(fields[4]);
                int gy = Integer.parseInt(fields[5]);
                int gm = Integer.parseInt(fields[6]);
                int gd = Integer.parseInt(fields[7]);

                String tithiStr = fields.length > 8 ? fields[8].trim() : "";
                String solMonthStr = fields.length > 9 ? fields[9].trim() : "";
                String solDayStr = fields.length > 10 ? fields[10].trim() : "";
                String solYearStr = fields.length > 11 ? fields[11].trim() : "";

                Location loc = new Location(lat, lon, 0.0, utcOffset);

                if ("lunisolar".equals(calendar)) {
                    if (tithiStr.isEmpty()) continue;
                    int expectedTithi = Integer.parseInt(tithiStr);
                    if (expectedTithi == 0) continue;

                    double jd = eph.gregorianToJd(gy, gm, gd);
                    double jdRise = eph.sunriseJd(jd, loc);
                    if (jdRise <= 0)
                        jdRise = jd + 0.5 - utcOffset / 24.0;
                    int actualTithi = tithi.tithiAtMoment(jdRise);

                    checked++;
                    if (actualTithi != expectedTithi) {
                        failures++;
                        errors.append(String.format("lunisolar %s %04d-%02d-%02d: tithi expected %d, got %d%n",
                                location, gy, gm, gd, expectedTithi, actualTithi));
                    }
                } else {
                    // Solar calendar
                    if (solMonthStr.isEmpty()) continue;
                    int expectedMonth = Integer.parseInt(solMonthStr);
                    int expectedDay = Integer.parseInt(solDayStr);
                    int expectedYear = Integer.parseInt(solYearStr);
                    if (expectedMonth == 0) continue;

                    SolarCalendarType type;
                    try {
                        type = SolarCalendarType.fromString(calendar);
                    } catch (IllegalArgumentException e) {
                        continue;
                    }

                    SolarDate sd = solar.gregorianToSolar(gy, gm, gd, loc, type);

                    checked++;
                    if (sd.month() != expectedMonth || sd.day() != expectedDay || sd.year() != expectedYear) {
                        failures++;
                        errors.append(String.format("%s %s %04d-%02d-%02d: expected m%d d%d y%d, got m%d d%d y%d%n",
                                calendar, location, gy, gm, gd,
                                expectedMonth, expectedDay, expectedYear,
                                sd.month(), sd.day(), sd.year()));
                    }
                }
            }
        }

        System.out.printf("VariousLocations: %d rows checked, %d failures%n", checked, failures);
        if (failures > 0) {
            fail(String.format("VariousLocations: %d/%d failures:%n%s",
                    failures, checked, errors.toString().substring(0, Math.min(errors.length(), 5000))));
        }
        assertTrue(checked > 100, "Expected 100+ rows, got " + checked);
    }
}
