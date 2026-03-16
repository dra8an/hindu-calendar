package com.hindu.calendar.validation;

import com.hindu.calendar.core.Masa;
import com.hindu.calendar.core.Tithi;
import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.MasaInfo;
import com.hindu.calendar.model.TithiInfo;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * NYC validation: US Eastern DST rules + lunisolar tithi/masa for New York City.
 * Ported from tests/test_nyc.c and src/dst.c.
 */
class NycTest {

    private static Tithi tithi;
    private static Masa masa;

    @BeforeAll
    static void setUp() {
        Ephemeris eph = new Ephemeris();
        tithi = new Tithi(eph);
        masa = new Masa(eph, tithi);
    }

    // ===== US Eastern DST helper (ported from src/dst.c) =====

    /**
     * Day of week for Gregorian date: 0=Sun, 1=Mon, ..., 6=Sat.
     * Tomohiko Sakamoto's algorithm.
     */
    private static int dow(int y, int m, int d) {
        int[] t = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        if (m < 3) y--;
        return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
    }

    /**
     * Find nth occurrence of weekday (0=Sun) in month m of year y.
     * nth > 0: 1st, 2nd, etc. nth == -1: last occurrence.
     */
    private static int nthWeekday(int y, int m, int nth, int wday) {
        if (nth > 0) {
            int d1Dow = dow(y, m, 1);
            return 1 + ((wday - d1Dow + 7) % 7) + (nth - 1) * 7;
        }
        // Last occurrence
        int[] mdays = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int last = mdays[m];
        if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
            last = 29;
        int lastDow = dow(y, m, last);
        return last - ((lastDow - wday + 7) % 7);
    }

    /**
     * Day of year (1-based).
     */
    private static int dayOfYear(int y, int m, int d) {
        int[] cum = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
        int doy = cum[m] + d;
        if (m > 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
            doy++;
        return doy;
    }

    /**
     * Check if date is in DST period [start, end).
     */
    private static boolean inDst(int y, int m, int d, int sm, int sd, int em, int ed) {
        int doy = dayOfYear(y, m, d);
        int start = dayOfYear(y, sm, sd);
        int end = dayOfYear(y, em, ed);
        return doy >= start && doy < end;
    }

    /**
     * US Eastern timezone offset for a given date.
     * Returns -5.0 (EST) or -4.0 (EDT).
     */
    static double usEasternOffset(int y, int m, int d) {
        // 1900-1917: No DST
        if (y < 1918) return -5.0;

        // 1918-1919: Last Sun March - Last Sun October
        if (y <= 1919) {
            int sd = nthWeekday(y, 3, -1, 0);
            int ed = nthWeekday(y, 10, -1, 0);
            return inDst(y, m, d, 3, sd, 10, ed) ? -4.0 : -5.0;
        }

        // 1920-1941: No federal DST
        if (y <= 1941) return -5.0;

        // 1942-1945: War Time
        if (y >= 1942 && y <= 1945) {
            if (y == 1942) {
                return (m > 2 || (m == 2 && d >= 9)) ? -4.0 : -5.0;
            }
            if (y <= 1944) return -4.0;
            // 1945: War Time ended Sep 30
            return (m < 10 || (m == 9 && d <= 30)) ? -4.0 : -5.0;
        }

        // 1946-1966: Last Sun April - Last Sun September
        if (y <= 1966) {
            int sd = nthWeekday(y, 4, -1, 0);
            int ed = nthWeekday(y, 9, -1, 0);
            return inDst(y, m, d, 4, sd, 9, ed) ? -4.0 : -5.0;
        }

        // 1967-1973: Last Sun April - Last Sun October
        if (y <= 1973) {
            int sd = nthWeekday(y, 4, -1, 0);
            int ed = nthWeekday(y, 10, -1, 0);
            return inDst(y, m, d, 4, sd, 10, ed) ? -4.0 : -5.0;
        }

        // 1974: Jan 6 - Last Sun October (energy crisis)
        if (y == 1974) {
            int ed = nthWeekday(y, 10, -1, 0);
            return inDst(y, m, d, 1, 6, 10, ed) ? -4.0 : -5.0;
        }

        // 1975: Last Sun Feb - Last Sun October
        if (y == 1975) {
            int sd = nthWeekday(y, 2, -1, 0);
            int ed = nthWeekday(y, 10, -1, 0);
            return inDst(y, m, d, 2, sd, 10, ed) ? -4.0 : -5.0;
        }

        // 1976-1986: Last Sun April - Last Sun October
        if (y <= 1986) {
            int sd = nthWeekday(y, 4, -1, 0);
            int ed = nthWeekday(y, 10, -1, 0);
            return inDst(y, m, d, 4, sd, 10, ed) ? -4.0 : -5.0;
        }

        // 1987-2006: First Sun April - Last Sun October
        if (y <= 2006) {
            int sd = nthWeekday(y, 4, 1, 0);
            int ed = nthWeekday(y, 10, -1, 0);
            return inDst(y, m, d, 4, sd, 10, ed) ? -4.0 : -5.0;
        }

        // 2007+: Second Sun March - First Sun November
        {
            int sd = nthWeekday(y, 3, 2, 0);
            int ed = nthWeekday(y, 11, 1, 0);
            return inDst(y, m, d, 3, sd, 11, ed) ? -4.0 : -5.0;
        }
    }

    private static Location nycLoc(int y, int m, int d) {
        return new Location(40.7128, -74.0060, 0.0, usEasternOffset(y, m, d));
    }

    // ===== DST Rule Tests =====

    @Test
    void testDstRules() {
        // 1900-1917: No DST
        assertEquals(-5.0, usEasternOffset(1910, 6, 15), "1910-06-15 = EST");

        // 1918: Last Sun March (Mar 31) - Last Sun Oct (Oct 27)
        assertEquals(-5.0, usEasternOffset(1918, 3, 30), "1918-03-30 = EST");
        assertEquals(-4.0, usEasternOffset(1918, 3, 31), "1918-03-31 = EDT");
        assertEquals(-4.0, usEasternOffset(1918, 10, 26), "1918-10-26 = EDT");
        assertEquals(-5.0, usEasternOffset(1918, 10, 27), "1918-10-27 = EST");

        // 1920-1941: No federal DST
        assertEquals(-5.0, usEasternOffset(1935, 7, 1), "1935-07-01 = EST");

        // 1942-1945: War Time
        assertEquals(-5.0, usEasternOffset(1942, 2, 8), "1942-02-08 = EST");
        assertEquals(-4.0, usEasternOffset(1942, 2, 9), "1942-02-09 = EDT War Time");
        assertEquals(-4.0, usEasternOffset(1943, 1, 1), "1943-01-01 = EDT War Time");
        assertEquals(-4.0, usEasternOffset(1944, 12, 31), "1944-12-31 = EDT War Time");
        assertEquals(-4.0, usEasternOffset(1945, 9, 30), "1945-09-30 = EDT War Time");
        assertEquals(-5.0, usEasternOffset(1945, 10, 1), "1945-10-01 = EST");

        // 1946-1966: Last Sun April - Last Sun September
        // 1960: Last Sun April = Apr 24, Last Sun Sep = Sep 25
        assertEquals(-5.0, usEasternOffset(1960, 4, 23), "1960-04-23 = EST");
        assertEquals(-4.0, usEasternOffset(1960, 4, 24), "1960-04-24 = EDT");
        assertEquals(-4.0, usEasternOffset(1960, 7, 15), "1960-07-15 = EDT");
        assertEquals(-4.0, usEasternOffset(1960, 9, 24), "1960-09-24 = EDT");
        assertEquals(-5.0, usEasternOffset(1960, 9, 25), "1960-09-25 = EST");

        // 1967-1973: Last Sun April - Last Sun October
        // 1970: Last Sun April = Apr 26, Last Sun Oct = Oct 25
        assertEquals(-5.0, usEasternOffset(1970, 4, 25), "1970-04-25 = EST");
        assertEquals(-4.0, usEasternOffset(1970, 4, 26), "1970-04-26 = EDT");
        assertEquals(-4.0, usEasternOffset(1970, 10, 24), "1970-10-24 = EDT");
        assertEquals(-5.0, usEasternOffset(1970, 10, 25), "1970-10-25 = EST");

        // 1974: Energy crisis - Jan 6 start
        assertEquals(-5.0, usEasternOffset(1974, 1, 5), "1974-01-05 = EST");
        assertEquals(-4.0, usEasternOffset(1974, 1, 6), "1974-01-06 = EDT energy crisis");

        // 1975: Last Sun Feb (Feb 23)
        assertEquals(-5.0, usEasternOffset(1975, 2, 22), "1975-02-22 = EST");
        assertEquals(-4.0, usEasternOffset(1975, 2, 23), "1975-02-23 = EDT");

        // 1976-1986: Last Sun April - Last Sun October
        // 1980: Last Sun April = Apr 27, Last Sun Oct = Oct 26
        assertEquals(-5.0, usEasternOffset(1980, 4, 26), "1980-04-26 = EST");
        assertEquals(-4.0, usEasternOffset(1980, 4, 27), "1980-04-27 = EDT");
        assertEquals(-4.0, usEasternOffset(1980, 10, 25), "1980-10-25 = EDT");
        assertEquals(-5.0, usEasternOffset(1980, 10, 26), "1980-10-26 = EST");

        // 1987-2006: First Sun April - Last Sun October
        // 2000: First Sun April = Apr 2, Last Sun Oct = Oct 29
        assertEquals(-5.0, usEasternOffset(2000, 4, 1), "2000-04-01 = EST");
        assertEquals(-4.0, usEasternOffset(2000, 4, 2), "2000-04-02 = EDT");
        assertEquals(-4.0, usEasternOffset(2000, 10, 28), "2000-10-28 = EDT");
        assertEquals(-5.0, usEasternOffset(2000, 10, 29), "2000-10-29 = EST");

        // 2007+: Second Sun March - First Sun November
        // 2025: DST starts Mar 9, ends Nov 2
        assertEquals(-5.0, usEasternOffset(2025, 3, 8), "2025-03-08 = EST");
        assertEquals(-4.0, usEasternOffset(2025, 3, 9), "2025-03-09 = EDT");
        assertEquals(-4.0, usEasternOffset(2025, 7, 4), "2025-07-04 = EDT");
        assertEquals(-4.0, usEasternOffset(2025, 11, 1), "2025-11-01 = EDT");
        assertEquals(-5.0, usEasternOffset(2025, 11, 2), "2025-11-02 = EST");
        assertEquals(-5.0, usEasternOffset(2025, 12, 25), "2025-12-25 = EST");
    }

    // ===== NYC Tithi/Masa Validation =====

    private void checkNyc(int y, int m, int d, int expTithi, int expMasa, int expAdhika, int expSaka, String note) {
        Location loc = nycLoc(y, m, d);
        TithiInfo ti = tithi.tithiAtSunrise(y, m, d, loc);
        MasaInfo mi = masa.masaForDate(y, m, d, loc);
        String label = y + "-" + m + "-" + d + " [" + note + "]";
        assertEquals(expTithi, ti.tithiNum(), "tithi " + label);
        assertEquals(expMasa, mi.name().number(), "masa " + label);
        assertEquals(expAdhika != 0, mi.isAdhika(), "adhika " + label);
        assertEquals(expSaka, mi.yearSaka(), "saka " + label);
    }

    @Test
    void testNycValidation() {
        // Winter (EST)
        checkNyc(2025, 1, 1, 2, 10, 0, 1946, "Jan 1 Pausha S-2");
        checkNyc(2025, 1, 13, 15, 10, 0, 1946, "Pausha Purnima");
        checkNyc(2025, 1, 29, 30, 10, 0, 1946, "Pausha Amavasya");
        checkNyc(2025, 2, 12, 15, 11, 0, 1946, "Magha Purnima");
        checkNyc(2025, 2, 28, 1, 12, 0, 1946, "Phalguna S-1");

        // Around DST transition: Mar 8 (EST) -> Mar 9 (EDT)
        checkNyc(2025, 3, 8, 10, 12, 0, 1946, "Mar 8 last EST day");
        checkNyc(2025, 3, 9, 11, 12, 0, 1946, "Mar 9 first EDT day");
        checkNyc(2025, 3, 10, 12, 12, 0, 1946, "Mar 10 EDT");

        // Hindu New Year
        checkNyc(2025, 3, 29, 30, 12, 0, 1946, "Phalguna Amavasya");
        checkNyc(2025, 3, 30, 2, 1, 0, 1947, "Chaitra S-2 (NYC)");

        // Summer (EDT)
        checkNyc(2025, 6, 21, 26, 3, 0, 1947, "Jun 21 summer solstice");
        checkNyc(2025, 7, 4, 9, 4, 0, 1947, "Jul 4");
        checkNyc(2025, 8, 15, 22, 5, 0, 1947, "Aug 15");

        // Around DST end: Nov 1 (EDT) -> Nov 2 (EST)
        checkNyc(2025, 11, 1, 11, 8, 0, 1947, "Nov 1 last EDT day");
        checkNyc(2025, 11, 2, 12, 8, 0, 1947, "Nov 2 first EST day");
        checkNyc(2025, 11, 3, 13, 8, 0, 1947, "Nov 3 EST");

        // Winter again
        checkNyc(2025, 12, 25, 6, 10, 0, 1947, "Dec 25");
        checkNyc(2025, 12, 31, 12, 10, 0, 1947, "Dec 31");
    }
}
