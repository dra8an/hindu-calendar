import XCTest
@testable import HinduCalendar

final class MasaTests: XCTestCase {

    static let eph = Ephemeris()
    static let tithi = Tithi(ephemeris: eph)
    static let masa = Masa(ephemeris: eph, tithi: tithi)
    static let loc = Location.newDelhi

    func testMasaKnownDates() {
        let cases: [(Int, Int, Int, Int, Bool)] = [
            (2013, 1, 18, 10, false),
            (2013, 2, 10, 10, false),
            (2012, 8, 17, 5, false),
            (2012, 8, 18, 6, true),
            (2012, 9, 18, 6, false),
            (2025, 1, 1, 10, false),
            (2025, 1, 30, 11, false),
            (2025, 3, 30, 1, false),
            (2025, 4, 30, 2, false),
        ]

        for (y, m, d, expMasa, expAdhika) in cases {
            let mi = Self.masa.masaForDate(year: y, month: m, day: d, loc: Self.loc)
            XCTAssertEqual(mi.name.rawValue, expMasa, "\(y)-\(m)-\(d) masa")
            XCTAssertEqual(mi.isAdhika, expAdhika, "\(y)-\(m)-\(d) adhika")
        }
    }

    func testYearDetermination() {
        var mi = Self.masa.masaForDate(year: 2025, month: 1, day: 18, loc: Self.loc)
        XCTAssertEqual(mi.yearSaka, 1946)
        XCTAssertEqual(mi.yearVikram, 2081)

        mi = Self.masa.masaForDate(year: 2012, month: 8, day: 18, loc: Self.loc)
        XCTAssertEqual(mi.yearSaka, 1934)
        XCTAssertEqual(mi.yearVikram, 2069)
    }

    // ===== Amanta month start =====

    func testAmantaMonthStarts() {
        let cases: [(MasaName, Int, Bool, Int, Int, Int)] = [
            (.chaitra, 1947, false, 2025, 3, 30),
            (.vaishakha, 1947, false, 2025, 4, 28),
            (.jyeshtha, 1947, false, 2025, 5, 28),
            (.ashadha, 1947, false, 2025, 6, 26),
            (.shravana, 1947, false, 2025, 7, 25),
            (.bhadrapada, 1947, false, 2025, 8, 24),
            (.ashvina, 1947, false, 2025, 9, 22),
            (.kartika, 1947, false, 2025, 10, 22),
            (.margashirsha, 1947, false, 2025, 11, 21),
            (.pausha, 1947, false, 2025, 12, 21),
            (.bhadrapada, 1934, true, 2012, 8, 18),
            (.bhadrapada, 1934, false, 2012, 9, 17),
            (.phalguna, 1946, false, 2025, 2, 28),
            (.chaitra, 1946, false, 2024, 4, 9),
        ]

        for (masa, saka, adhika, expY, expM, expD) in cases {
            let jd = Self.masa.lunisolarMonthStart(masa, saka, adhika, .amanta, Self.loc)
            XCTAssertTrue(jd > 0, "\(masa) \(saka) should be found")
            let ymd = Self.eph.jdToGregorian(jd)
            let label = "\(masa) \(saka)\(adhika ? " adhika" : "")"
            XCTAssertEqual(ymd.year, expY, "\(label) year")
            XCTAssertEqual(ymd.month, expM, "\(label) month")
            XCTAssertEqual(ymd.day, expD, "\(label) day")
        }
    }

    func testAmantaMonthLengths() {
        for m in MasaName.allCases {
            let len = Self.masa.lunisolarMonthLength(m, 1947, false, .amanta, Self.loc)
            XCTAssertTrue(len == 29 || len == 30, "\(m) length: \(len)")
        }

        let jdStart = Self.masa.lunisolarMonthStart(.vaishakha, 1947, false, .amanta, Self.loc)
        let len = Self.masa.lunisolarMonthLength(.vaishakha, 1947, false, .amanta, Self.loc)
        let jdNext = Self.masa.lunisolarMonthStart(.jyeshtha, 1947, false, .amanta, Self.loc)
        XCTAssertEqual(Int(jdNext - jdStart), len)
    }

    // ===== Purnimanta =====

    func testPurnimantaMonthStarts() {
        for m in MasaName.allCases {
            let jdAmanta = Self.masa.lunisolarMonthStart(m, 1947, false, .amanta, Self.loc)
            let jdPurni = Self.masa.lunisolarMonthStart(m, 1947, false, .purnimanta, Self.loc)

            XCTAssertTrue(jdPurni > 0, "Purnimanta \(m)")

            let diff = jdAmanta - jdPurni
            XCTAssertTrue(diff >= 13 && diff <= 17, "Purnimanta \(m) offset \(diff)")

            var jr = Self.eph.sunriseJd(jdPurni, Self.loc)
            if jr <= 0 { jr = jdPurni + 0.5 - Self.loc.utcOffset / 24.0 }
            let t = Self.tithi.tithiAtMoment(jr)
            XCTAssertTrue(t >= 16 && t <= 30, "Purnimanta \(m) tithi \(t)")
        }
    }

    func testPurnimantaMonthLengths() {
        for m in MasaName.allCases {
            let len = Self.masa.lunisolarMonthLength(m, 1947, false, .purnimanta, Self.loc)
            XCTAssertTrue(len == 29 || len == 30, "Purnimanta \(m) length: \(len)")
        }

        let jdStart = Self.masa.lunisolarMonthStart(.vaishakha, 1947, false, .purnimanta, Self.loc)
        let len = Self.masa.lunisolarMonthLength(.vaishakha, 1947, false, .purnimanta, Self.loc)
        let jdNext = Self.masa.lunisolarMonthStart(.jyeshtha, 1947, false, .purnimanta, Self.loc)
        XCTAssertEqual(Int(jdNext - jdStart), len)
    }
}
