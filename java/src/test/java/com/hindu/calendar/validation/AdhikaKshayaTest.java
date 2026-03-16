package com.hindu.calendar.validation;

import com.hindu.calendar.core.Masa;
import com.hindu.calendar.core.Tithi;
import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.MasaInfo;
import com.hindu.calendar.model.TithiInfo;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.io.BufferedReader;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Reads validation/moshier/adhika_kshaya_tithis.csv and verifies each row.
 * CSV format: year,month,day,tithi,masa,adhika,saka,type
 * Ported from tests/test_adhika_kshaya.c.
 */
class AdhikaKshayaTest {

    private static final Path CSV = Path.of(System.getProperty("user.dir"))
            .getParent().resolve("validation").resolve("moshier")
            .resolve("adhika_kshaya_tithis.csv");
    private static final Location LOC = Location.NEW_DELHI;

    private static Tithi tithi;
    private static Masa masa;

    @BeforeAll
    static void setUp() {
        Ephemeris eph = new Ephemeris();
        tithi = new Tithi(eph);
        masa = new Masa(eph, tithi);
    }

    @Test
    void testAdhikaKshayaTithis() throws IOException {
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
                String[] parts = line.split(",");
                if (parts.length < 8) continue;

                int y = Integer.parseInt(parts[0]);
                int m = Integer.parseInt(parts[1]);
                int d = Integer.parseInt(parts[2]);
                int expTithi = Integer.parseInt(parts[3]);
                int expMasa = Integer.parseInt(parts[4]);
                int expAdhika = Integer.parseInt(parts[5]);
                int expSaka = Integer.parseInt(parts[6]);
                // parts[7] is type (adhika/kshaya) — not needed for verification

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

        System.out.printf("AdhikaKshaya: %d rows checked, %d failures%n", checked, failures);
        if (failures > 0) {
            fail(String.format("AdhikaKshaya: %d/%d failures:%n%s",
                    failures, checked, errors.toString().substring(0, Math.min(errors.length(), 5000))));
        }
        assertTrue(checked > 4000, "Expected 4,000+ rows, got " + checked);
    }
}
