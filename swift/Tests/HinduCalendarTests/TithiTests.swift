import XCTest
@testable import HinduCalendar

final class TithiTests: XCTestCase {

    static let eph = Ephemeris()
    static let tithi = Tithi(ephemeris: eph)
    static let loc = Location.newDelhi

    func testTithiKnownDates() {
        let cases: [(Int, Int, Int, Int, Paksha)] = [
            (2013, 1, 18, 7, .shukla),
            (2012, 8, 18, 1, .shukla),
            (2025, 1, 13, 15, .shukla),
            (2025, 1, 29, 30, .krishna),
            (2025, 1, 1, 2, .shukla),
            (2025, 1, 14, 16, .krishna),
            (2025, 1, 30, 1, .shukla),
        ]

        for (y, m, d, expTithi, expPaksha) in cases {
            let ti = Self.tithi.tithiAtSunrise(year: y, month: m, day: d, loc: Self.loc)
            XCTAssertEqual(ti.tithiNum, expTithi, "\(y)-\(m)-\(d) tithi")
            XCTAssertEqual(ti.paksha, expPaksha, "\(y)-\(m)-\(d) paksha")
        }
    }

    func testKshayaTithi() {
        let ti1 = Self.tithi.tithiAtSunrise(year: 2025, month: 1, day: 11, loc: Self.loc)
        XCTAssertTrue(ti1.isKshaya, "2025-01-11 should be kshaya")

        let ti2 = Self.tithi.tithiAtSunrise(year: 2025, month: 1, day: 12, loc: Self.loc)
        XCTAssertFalse(ti2.isKshaya, "2025-01-12 should not be kshaya")
    }

    func testAdhikaTithi() {
        let ti18 = Self.tithi.tithiAtSunrise(year: 2025, month: 1, day: 18, loc: Self.loc)
        let ti19 = Self.tithi.tithiAtSunrise(year: 2025, month: 1, day: 19, loc: Self.loc)
        XCTAssertEqual(ti18.tithiNum, ti19.tithiNum, "Same tithi 18-19")
    }

    func testLunarPhaseAtPurnima() {
        let jd = Self.eph.gregorianToJd(year: 2025, month: 1, day: 13)
        let rise = Self.eph.sunriseJd(jd, Self.loc)
        let phase = Self.tithi.lunarPhase(rise)
        XCTAssertTrue(phase > 156.0 && phase < 192.0, "Phase at Purnima: \(phase)")
    }
}
