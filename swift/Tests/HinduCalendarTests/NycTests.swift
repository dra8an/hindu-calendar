import XCTest
@testable import HinduCalendar

/// Port of test_nyc.c -- US Eastern DST rules and NYC tithi/masa validation.
final class NycTests: XCTestCase {

    static let eph = Ephemeris()
    static let tithi = Tithi(ephemeris: eph)
    static let masa = Masa(ephemeris: eph, tithi: tithi)

    // MARK: - US Eastern DST offset helper (port of src/dst.c)

    /// Day of week: 0=Sun, 1=Mon, ..., 6=Sat (Tomohiko Sakamoto's algorithm)
    private static func dow(_ y: Int, _ m: Int, _ d: Int) -> Int {
        var y = y
        let t = [0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4]
        if m < 3 { y -= 1 }
        return (y + y/4 - y/100 + y/400 + t[m - 1] + d) % 7
    }

    /// Find nth occurrence of weekday (0=Sun) in month m of year y.
    /// nth > 0: 1st, 2nd, ... occurrence. nth == -1: last occurrence.
    private static func nthWeekday(_ y: Int, _ m: Int, _ nth: Int, _ wday: Int) -> Int {
        if nth > 0 {
            let d1Dow = dow(y, m, 1)
            return 1 + ((wday - d1Dow + 7) % 7) + (nth - 1) * 7
        }
        // Last occurrence
        let mdays = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
        var last = mdays[m]
        if m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
            last = 29
        }
        let lastDow = dow(y, m, last)
        return last - ((lastDow - wday + 7) % 7)
    }

    /// Day of year (1-based)
    private static func dayOfYear(_ y: Int, _ m: Int, _ d: Int) -> Int {
        let cum = [0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334]
        var doy = cum[m] + d
        if m > 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
            doy += 1
        }
        return doy
    }

    /// Check if date is within DST period [start, end)
    private static func inDst(_ y: Int, _ m: Int, _ d: Int,
                              _ sm: Int, _ sd: Int, _ em: Int, _ ed: Int) -> Bool {
        let doy = dayOfYear(y, m, d)
        let start = dayOfYear(y, sm, sd)
        let end = dayOfYear(y, em, ed)
        return doy >= start && doy < end
    }

    /// US Eastern time zone offset for a given date.
    /// Returns -5.0 (EST) or -4.0 (EDT).
    static func usEasternOffset(_ y: Int, _ m: Int, _ d: Int) -> Double {
        // 1900-1917: No DST
        if y < 1918 { return -5.0 }

        // 1918-1919: Last Sun March - Last Sun October
        if y <= 1919 {
            let sd = nthWeekday(y, 3, -1, 0)
            let ed = nthWeekday(y, 10, -1, 0)
            return inDst(y, m, d, 3, sd, 10, ed) ? -4.0 : -5.0
        }

        // 1920-1941: No federal DST
        if y <= 1941 { return -5.0 }

        // 1942-Feb-09 to 1945-Sep-30: Year-round War Time (EDT)
        if y >= 1942 && y <= 1945 {
            if y == 1942 {
                return (m > 2 || (m == 2 && d >= 9)) ? -4.0 : -5.0
            }
            if y <= 1944 { return -4.0 }
            // 1945: War Time ended Sep 30
            return (m < 10 || (m == 9 && d <= 30)) ? -4.0 : -5.0
        }

        // 1946-1966: Last Sun April - Last Sun September
        if y <= 1966 {
            let sd = nthWeekday(y, 4, -1, 0)
            let ed = nthWeekday(y, 9, -1, 0)
            return inDst(y, m, d, 4, sd, 9, ed) ? -4.0 : -5.0
        }

        // 1967-1973: Last Sun April - Last Sun October
        if y <= 1973 {
            let sd = nthWeekday(y, 4, -1, 0)
            let ed = nthWeekday(y, 10, -1, 0)
            return inDst(y, m, d, 4, sd, 10, ed) ? -4.0 : -5.0
        }

        // 1974: Jan 6 - Last Sun October (energy crisis)
        if y == 1974 {
            let ed = nthWeekday(y, 10, -1, 0)
            return inDst(y, m, d, 1, 6, 10, ed) ? -4.0 : -5.0
        }

        // 1975: Last Sun Feb - Last Sun October
        if y == 1975 {
            let sd = nthWeekday(y, 2, -1, 0)
            let ed = nthWeekday(y, 10, -1, 0)
            return inDst(y, m, d, 2, sd, 10, ed) ? -4.0 : -5.0
        }

        // 1976-1986: Last Sun April - Last Sun October
        if y <= 1986 {
            let sd = nthWeekday(y, 4, -1, 0)
            let ed = nthWeekday(y, 10, -1, 0)
            return inDst(y, m, d, 4, sd, 10, ed) ? -4.0 : -5.0
        }

        // 1987-2006: First Sun April - Last Sun October
        if y <= 2006 {
            let sd = nthWeekday(y, 4, 1, 0)
            let ed = nthWeekday(y, 10, -1, 0)
            return inDst(y, m, d, 4, sd, 10, ed) ? -4.0 : -5.0
        }

        // 2007+: Second Sun March - First Sun November
        let sd = nthWeekday(y, 3, 2, 0)
        let ed = nthWeekday(y, 11, 1, 0)
        return inDst(y, m, d, 3, sd, 11, ed) ? -4.0 : -5.0
    }

    /// Create NYC Location for a given date (adjusts UTC offset for DST).
    private static func nycLoc(_ y: Int, _ m: Int, _ d: Int) -> Location {
        Location(latitude: 40.7128, longitude: -74.0060,
                 altitude: 0.0, utcOffset: usEasternOffset(y, m, d))
    }

    // MARK: - DST rule tests

    func testDstRules() {
        // 1900-1917: No DST
        XCTAssertEqual(Self.usEasternOffset(1910, 6, 15), -5.0, "1910-06-15 = EST")

        // 1918: Last Sun March (Mar 31) - Last Sun Oct (Oct 27)
        XCTAssertEqual(Self.usEasternOffset(1918, 3, 30), -5.0, "1918-03-30 = EST")
        XCTAssertEqual(Self.usEasternOffset(1918, 3, 31), -4.0, "1918-03-31 = EDT")
        XCTAssertEqual(Self.usEasternOffset(1918, 10, 26), -4.0, "1918-10-26 = EDT")
        XCTAssertEqual(Self.usEasternOffset(1918, 10, 27), -5.0, "1918-10-27 = EST")

        // 1920-1941: No federal DST
        XCTAssertEqual(Self.usEasternOffset(1935, 7, 1), -5.0, "1935-07-01 = EST")

        // 1942-1945: War Time
        XCTAssertEqual(Self.usEasternOffset(1942, 2, 8), -5.0, "1942-02-08 = EST")
        XCTAssertEqual(Self.usEasternOffset(1942, 2, 9), -4.0, "1942-02-09 = EDT War Time")
        XCTAssertEqual(Self.usEasternOffset(1943, 1, 1), -4.0, "1943-01-01 = EDT War Time")
        XCTAssertEqual(Self.usEasternOffset(1944, 12, 31), -4.0, "1944-12-31 = EDT War Time")
        XCTAssertEqual(Self.usEasternOffset(1945, 9, 30), -4.0, "1945-09-30 = EDT War Time")
        XCTAssertEqual(Self.usEasternOffset(1945, 10, 1), -5.0, "1945-10-01 = EST")

        // 1946-1966: Last Sun April - Last Sun September
        // 1960: Last Sun April = Apr 24, Last Sun Sep = Sep 25
        XCTAssertEqual(Self.usEasternOffset(1960, 4, 23), -5.0, "1960-04-23 = EST")
        XCTAssertEqual(Self.usEasternOffset(1960, 4, 24), -4.0, "1960-04-24 = EDT")
        XCTAssertEqual(Self.usEasternOffset(1960, 7, 15), -4.0, "1960-07-15 = EDT")
        XCTAssertEqual(Self.usEasternOffset(1960, 9, 24), -4.0, "1960-09-24 = EDT")
        XCTAssertEqual(Self.usEasternOffset(1960, 9, 25), -5.0, "1960-09-25 = EST")

        // 1967-1973: Last Sun April - Last Sun October
        // 1970: Last Sun April = Apr 26, Last Sun Oct = Oct 25
        XCTAssertEqual(Self.usEasternOffset(1970, 4, 25), -5.0, "1970-04-25 = EST")
        XCTAssertEqual(Self.usEasternOffset(1970, 4, 26), -4.0, "1970-04-26 = EDT")
        XCTAssertEqual(Self.usEasternOffset(1970, 10, 24), -4.0, "1970-10-24 = EDT")
        XCTAssertEqual(Self.usEasternOffset(1970, 10, 25), -5.0, "1970-10-25 = EST")

        // 1974: Energy crisis - Jan 6 start
        XCTAssertEqual(Self.usEasternOffset(1974, 1, 5), -5.0, "1974-01-05 = EST")
        XCTAssertEqual(Self.usEasternOffset(1974, 1, 6), -4.0, "1974-01-06 = EDT energy crisis")

        // 1975: Last Sun Feb (Feb 23)
        XCTAssertEqual(Self.usEasternOffset(1975, 2, 22), -5.0, "1975-02-22 = EST")
        XCTAssertEqual(Self.usEasternOffset(1975, 2, 23), -4.0, "1975-02-23 = EDT")

        // 1976-1986: Last Sun April - Last Sun October
        // 1980: Last Sun April = Apr 27, Last Sun Oct = Oct 26
        XCTAssertEqual(Self.usEasternOffset(1980, 4, 26), -5.0, "1980-04-26 = EST")
        XCTAssertEqual(Self.usEasternOffset(1980, 4, 27), -4.0, "1980-04-27 = EDT")
        XCTAssertEqual(Self.usEasternOffset(1980, 10, 25), -4.0, "1980-10-25 = EDT")
        XCTAssertEqual(Self.usEasternOffset(1980, 10, 26), -5.0, "1980-10-26 = EST")

        // 1987-2006: First Sun April - Last Sun October
        // 2000: First Sun April = Apr 2, Last Sun Oct = Oct 29
        XCTAssertEqual(Self.usEasternOffset(2000, 4, 1), -5.0, "2000-04-01 = EST")
        XCTAssertEqual(Self.usEasternOffset(2000, 4, 2), -4.0, "2000-04-02 = EDT")
        XCTAssertEqual(Self.usEasternOffset(2000, 10, 28), -4.0, "2000-10-28 = EDT")
        XCTAssertEqual(Self.usEasternOffset(2000, 10, 29), -5.0, "2000-10-29 = EST")

        // 2007+: Second Sun March - First Sun November
        // 2025: DST starts Mar 9, ends Nov 2
        XCTAssertEqual(Self.usEasternOffset(2025, 3, 8), -5.0, "2025-03-08 = EST")
        XCTAssertEqual(Self.usEasternOffset(2025, 3, 9), -4.0, "2025-03-09 = EDT")
        XCTAssertEqual(Self.usEasternOffset(2025, 7, 4), -4.0, "2025-07-04 = EDT")
        XCTAssertEqual(Self.usEasternOffset(2025, 11, 1), -4.0, "2025-11-01 = EDT")
        XCTAssertEqual(Self.usEasternOffset(2025, 11, 2), -5.0, "2025-11-02 = EST")
        XCTAssertEqual(Self.usEasternOffset(2025, 12, 25), -5.0, "2025-12-25 = EST")
    }

    // MARK: - NYC tithi/masa validation

    func testNycValidation() {
        let cases: [(Int, Int, Int, Int, Int, Bool, Int, String)] = [
            // Winter (EST)
            (2025,  1,  1,  2, 10, false, 1946, "Jan 1 Pausha S-2"),
            (2025,  1, 13, 15, 10, false, 1946, "Pausha Purnima"),
            (2025,  1, 29, 30, 10, false, 1946, "Pausha Amavasya"),
            (2025,  2, 12, 15, 11, false, 1946, "Magha Purnima"),
            (2025,  2, 28,  1, 12, false, 1946, "Phalguna S-1"),

            // Around DST transition: Mar 8 (EST) -> Mar 9 (EDT)
            (2025,  3,  8, 10, 12, false, 1946, "Mar 8 last EST day"),
            (2025,  3,  9, 11, 12, false, 1946, "Mar 9 first EDT day"),
            (2025,  3, 10, 12, 12, false, 1946, "Mar 10 EDT"),

            // Hindu New Year
            (2025,  3, 29, 30, 12, false, 1946, "Phalguna Amavasya"),
            (2025,  3, 30,  2,  1, false, 1947, "Chaitra S-2 (NYC)"),

            // Summer (EDT)
            (2025,  6, 21, 26,  3, false, 1947, "Jun 21 summer solstice"),
            (2025,  7,  4,  9,  4, false, 1947, "Jul 4"),
            (2025,  8, 15, 22,  5, false, 1947, "Aug 15"),

            // Around DST end: Nov 1 (EDT) -> Nov 2 (EST)
            (2025, 11,  1, 11,  8, false, 1947, "Nov 1 last EDT day"),
            (2025, 11,  2, 12,  8, false, 1947, "Nov 2 first EST day"),
            (2025, 11,  3, 13,  8, false, 1947, "Nov 3 EST"),

            // Winter again
            (2025, 12, 25,  6, 10, false, 1947, "Dec 25"),
            (2025, 12, 31, 12, 10, false, 1947, "Dec 31"),
        ]

        for (y, m, d, expTithi, expMasa, expAdhika, expSaka, note) in cases {
            let loc = Self.nycLoc(y, m, d)
            let ti = Self.tithi.tithiAtSunrise(year: y, month: m, day: d, loc: loc)
            let mi = Self.masa.masaForDate(year: y, month: m, day: d, loc: loc)

            let label = "\(y)-\(m)-\(d) [\(note)]"
            XCTAssertEqual(ti.tithiNum, expTithi, "\(label) tithi")
            XCTAssertEqual(mi.name.rawValue, expMasa, "\(label) masa")
            XCTAssertEqual(mi.isAdhika, expAdhika, "\(label) adhika")
            XCTAssertEqual(mi.yearSaka, expSaka, "\(label) saka")
        }
    }
}
