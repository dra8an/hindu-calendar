package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.LunisolarScheme;
import com.hindu.calendar.model.MasaInfo;
import com.hindu.calendar.model.MasaName;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.io.BufferedReader;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Lunisolar month start/length tests.
 * Ported from tests/test_lunisolar_month.c.
 */
class LunisolarMonthTest {

    private static Ephemeris eph;
    private static Tithi tithi;
    private static Masa masa;
    private static final Location LOC = Location.NEW_DELHI;

    @BeforeAll
    static void setUp() {
        eph = new Ephemeris();
        tithi = new Tithi(eph);
        masa = new Masa(eph, tithi);
    }

    // ===== Known month starts (16 spot checks) =====

    @Test
    void testKnownMonthStarts() {
        record Case(MasaName masa, int sakaYear, boolean isAdhika,
                     int expY, int expM, int expD) {}

        Case[] cases = {
            // 2025 months
            new Case(MasaName.CHAITRA, 1947, false, 2025, 3, 30),
            new Case(MasaName.VAISHAKHA, 1947, false, 2025, 4, 28),
            new Case(MasaName.JYESHTHA, 1947, false, 2025, 5, 28),
            new Case(MasaName.ASHADHA, 1947, false, 2025, 6, 26),
            new Case(MasaName.SHRAVANA, 1947, false, 2025, 7, 25),
            new Case(MasaName.BHADRAPADA, 1947, false, 2025, 8, 24),
            new Case(MasaName.ASHVINA, 1947, false, 2025, 9, 22),
            new Case(MasaName.KARTIKA, 1947, false, 2025, 10, 22),
            new Case(MasaName.MARGASHIRSHA, 1947, false, 2025, 11, 21),
            new Case(MasaName.PAUSHA, 1947, false, 2025, 12, 21),
            // Adhika Bhadrapada 2012 (Saka 1934)
            new Case(MasaName.BHADRAPADA, 1934, true, 2012, 8, 18),
            new Case(MasaName.BHADRAPADA, 1934, false, 2012, 9, 17),
            // Adhika Ashadha 2015 (Saka 1937)
            new Case(MasaName.ASHADHA, 1937, true, 2015, 6, 17),
            new Case(MasaName.ASHADHA, 1937, false, 2015, 7, 17),
            // Year boundary
            new Case(MasaName.PHALGUNA, 1946, false, 2025, 2, 28),
            new Case(MasaName.CHAITRA, 1946, false, 2024, 4, 9),
        };

        for (Case c : cases) {
            double jd = masa.lunisolarMonthStart(c.masa, c.sakaYear, c.isAdhika,
                    LunisolarScheme.AMANTA, LOC);
            String label = c.masa + " " + c.sakaYear + (c.isAdhika ? " adhika" : "");
            assertTrue(jd > 0, label + " should be found");

            int[] ymd = eph.jdToGregorian(jd);
            assertEquals(c.expY, ymd[0], label + " year");
            assertEquals(c.expM, ymd[1], label + " month");
            assertEquals(c.expD, ymd[2], label + " day");

            // Verify masa_for_date roundtrip
            MasaInfo mi = masa.masaForDate(c.expY, c.expM, c.expD, LOC);
            assertEquals(c.masa, mi.name(), label + " masa roundtrip");
            assertEquals(c.isAdhika, mi.isAdhika(), label + " adhika roundtrip");
            assertEquals(c.sakaYear, mi.yearSaka(), label + " saka roundtrip");

            // Verify day before belongs to different month
            double jdPrev = jd - 1;
            int[] prevYmd = eph.jdToGregorian(jdPrev);
            MasaInfo prevMi = masa.masaForDate(prevYmd[0], prevYmd[1], prevYmd[2], LOC);
            assertTrue(prevMi.name() != c.masa || prevMi.isAdhika() != c.isAdhika,
                    label + " prev day should be different month");
        }
    }

    // ===== Month lengths =====

    @Test
    void testMonthLengths() {
        // All 12 months of 1947 should be 29 or 30
        for (MasaName m : MasaName.values()) {
            int len = masa.lunisolarMonthLength(m, 1947, false,
                    LunisolarScheme.AMANTA, LOC);
            assertTrue(len == 29 || len == 30,
                    m + " 1947 length should be 29 or 30, got " + len);
        }

        // Adhika month length
        int adhikaLen = masa.lunisolarMonthLength(MasaName.BHADRAPADA, 1934, true,
                LunisolarScheme.AMANTA, LOC);
        assertTrue(adhikaLen == 29 || adhikaLen == 30,
                "Adhika Bhadrapada 1934 length should be 29 or 30, got " + adhikaLen);

        // start + length = next month start
        double jdStart = masa.lunisolarMonthStart(MasaName.VAISHAKHA, 1947, false,
                LunisolarScheme.AMANTA, LOC);
        int len = masa.lunisolarMonthLength(MasaName.VAISHAKHA, 1947, false,
                LunisolarScheme.AMANTA, LOC);
        double jdNext = masa.lunisolarMonthStart(MasaName.JYESHTHA, 1947, false,
                LunisolarScheme.AMANTA, LOC);
        assertEquals((int) (jdNext - jdStart), len, "Vaishakha+length = Jyeshtha start");
    }

    // ===== Roundtrip test =====

    @Test
    void testRoundtrip() {
        // Walk every day in sampled years, detect masa transitions,
        // verify lunisolarMonthStart returns the transition date.
        // Sample every 10 years to keep runtime reasonable.
        int[] years = {1900, 1910, 1920, 1930, 1940, 1950, 1960, 1970,
                       1980, 1990, 2000, 2010, 2020, 2030, 2040, 2050};

        int monthsTested = 0;
        int failures = 0;
        StringBuilder errors = new StringBuilder();

        for (int y : years) {
            int prevMasa = -1, prevAdhika = -1, prevSaka = -1;
            boolean firstTransition = true;

            for (int m = 1; m <= 12; m++) {
                int dim = daysInMonth(y, m);
                for (int d = 1; d <= dim; d++) {
                    MasaInfo mi = masa.masaForDate(y, m, d, LOC);
                    int masaNum = mi.name().number();
                    int adhika = mi.isAdhika() ? 1 : 0;
                    int saka = mi.yearSaka();

                    if (masaNum != prevMasa || adhika != prevAdhika || saka != prevSaka) {
                        if (firstTransition) {
                            firstTransition = false;
                            prevMasa = masaNum;
                            prevAdhika = adhika;
                            prevSaka = saka;
                            continue;
                        }

                        double jd = masa.lunisolarMonthStart(mi.name(), saka,
                                mi.isAdhika(), LunisolarScheme.AMANTA, LOC);
                        if (jd > 0) {
                            int[] ymd = eph.jdToGregorian(jd);
                            if (ymd[0] != y || ymd[1] != m || ymd[2] != d) {
                                failures++;
                                errors.append(String.format(
                                        "%s%s %d: expected %04d-%02d-%02d, got %04d-%02d-%02d%n",
                                        mi.isAdhika() ? "Adhika " : "",
                                        mi.name(), saka, y, m, d,
                                        ymd[0], ymd[1], ymd[2]));
                            }
                        } else {
                            failures++;
                            errors.append(String.format("%s%s %d: not found%n",
                                    mi.isAdhika() ? "Adhika " : "",
                                    mi.name(), saka));
                        }
                        monthsTested++;
                        prevMasa = masaNum;
                        prevAdhika = adhika;
                        prevSaka = saka;
                    }
                }
            }
        }

        System.out.printf("Roundtrip: %d months tested, %d failures%n", monthsTested, failures);
        if (failures > 0) {
            fail(String.format("Roundtrip: %d failures:%n%s", failures,
                    errors.toString().substring(0, Math.min(errors.length(), 5000))));
        }
        assertTrue(monthsTested > 150, "Expected 150+ months tested, got " + monthsTested);
    }

    // ===== CSV regression test =====

    @Test
    void testLunisolarMonthsCsvRegression() throws IOException {
        Path csv = Path.of(System.getProperty("user.dir"))
                .getParent().resolve("validation").resolve("moshier")
                .resolve("lunisolar_months.csv");
        if (!Files.exists(csv)) {
            System.out.println("Skipping: " + csv + " not found");
            return;
        }

        int checked = 0;
        int failures = 0;
        StringBuilder errors = new StringBuilder();

        try (BufferedReader br = Files.newBufferedReader(csv)) {
            String header = br.readLine();
            assertNotNull(header);

            String line;
            while ((line = br.readLine()) != null) {
                String[] parts = line.split(",");
                if (parts.length < 7) continue;

                int masaNum = Integer.parseInt(parts[0]);
                int isAdhika = Integer.parseInt(parts[1]);
                int sakaYear = Integer.parseInt(parts[2]);
                int length = Integer.parseInt(parts[3]);
                int gy = Integer.parseInt(parts[4]);
                int gm = Integer.parseInt(parts[5]);
                int gd = Integer.parseInt(parts[6]);

                MasaName masaName = MasaName.fromNumber(masaNum);

                // Check start date
                double jd = masa.lunisolarMonthStart(masaName, sakaYear,
                        isAdhika != 0, LunisolarScheme.AMANTA, LOC);
                checked++;
                if (jd > 0) {
                    int[] ymd = eph.jdToGregorian(jd);
                    if (ymd[0] != gy || ymd[1] != gm || ymd[2] != gd) {
                        failures++;
                        errors.append(String.format("%s%s %d start: expected %04d-%02d-%02d, got %04d-%02d-%02d%n",
                                isAdhika != 0 ? "Adhika " : "", masaName, sakaYear,
                                gy, gm, gd, ymd[0], ymd[1], ymd[2]));
                    }
                } else {
                    failures++;
                    errors.append(String.format("%s%s %d: not found%n",
                            isAdhika != 0 ? "Adhika " : "", masaName, sakaYear));
                }

                // Check length
                if (length > 0) {
                    int calcLen = masa.lunisolarMonthLength(masaName, sakaYear,
                            isAdhika != 0, LunisolarScheme.AMANTA, LOC);
                    checked++;
                    if (calcLen != length) {
                        failures++;
                        errors.append(String.format("%s%s %d length: expected %d, got %d%n",
                                isAdhika != 0 ? "Adhika " : "", masaName, sakaYear,
                                length, calcLen));
                    }
                }
            }
        }

        System.out.printf("CSV regression: %d checks, %d failures%n", checked, failures);
        if (failures > 0) {
            fail(String.format("CSV regression: %d failures:%n%s", failures,
                    errors.toString().substring(0, Math.min(errors.length(), 5000))));
        }
        assertTrue(checked > 1000, "Expected 1,000+ checks, got " + checked);
    }

    // ===== Purnimanta month starts =====

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
            int t = tithi.tithiAtMoment(jr);
            assertTrue(t >= 16 && t <= 30,
                    "Purnimanta " + m + " 1947 tithi " + t + " should be in Krishna paksha");
        }
    }

    // ===== Purnimanta month lengths =====

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

    // ===== Helpers =====

    private static int daysInMonth(int y, int m) {
        int[] mdays = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int d = mdays[m];
        if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
            d = 29;
        return d;
    }
}
