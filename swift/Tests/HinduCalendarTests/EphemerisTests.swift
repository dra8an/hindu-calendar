import XCTest
@testable import HinduCalendar

final class EphemerisTests: XCTestCase {

    static let eph = Ephemeris()

    // ===== Julian Day =====

    func testJulianDayJ2000() {
        let jd = Self.eph.gregorianToJd(year: 2000, month: 1, day: 1)
        XCTAssertEqual(jd, 2451544.5, accuracy: 0.0001)
    }

    func testJulianDayUnixEpoch() {
        let jd = Self.eph.gregorianToJd(year: 1970, month: 1, day: 1)
        XCTAssertEqual(jd, 2440587.5, accuracy: 0.0001)
    }

    func testJulianDayRoundTrip() {
        let dates = [
            (2000, 1, 1), (2025, 3, 15), (1900, 6, 21),
            (2050, 12, 31), (1950, 1, 1), (1976, 7, 4)
        ]
        for d in dates {
            let jd = Self.eph.gregorianToJd(year: d.0, month: d.1, day: d.2)
            let ymd = Self.eph.jdToGregorian(jd)
            XCTAssertEqual(ymd.year, d.0, "Round-trip year for \(d)")
            XCTAssertEqual(ymd.month, d.1, "Round-trip month for \(d)")
            XCTAssertEqual(ymd.day, d.2, "Round-trip day for \(d)")
        }
    }

    func testDayOfWeek() {
        var jd = Self.eph.gregorianToJd(year: 2025, month: 1, day: 13)
        XCTAssertEqual(Self.eph.dayOfWeek(jd), 0, "Monday")
        jd = Self.eph.gregorianToJd(year: 2025, month: 1, day: 19)
        XCTAssertEqual(Self.eph.dayOfWeek(jd), 6, "Sunday")
    }

    // ===== Solar Longitude =====

    func testSolarLongitudeRange() {
        let jd = Self.eph.gregorianToJd(year: 2025, month: 3, day: 20)
        let lon = Self.eph.solarLongitude(jd)
        XCTAssertTrue(lon >= 0 && lon < 360)
    }

    func testSolarLongitudeVernalEquinox() {
        let jd = Self.eph.gregorianToJd(year: 2025, month: 3, day: 20)
        let lon = Self.eph.solarLongitude(jd + 0.5)
        XCTAssertTrue(lon < 5 || lon > 355, "Near equinox: \(lon)")
    }

    // ===== Lunar Longitude =====

    func testLunarLongitudeRange() {
        let jd = Self.eph.gregorianToJd(year: 2025, month: 1, day: 13)
        let lon = Self.eph.lunarLongitude(jd)
        XCTAssertTrue(lon >= 0 && lon < 360)
    }

    // ===== Ayanamsa =====

    func testAyanamsaRange() {
        let jd = Self.eph.gregorianToJd(year: 2025, month: 1, day: 1)
        let ayan = Self.eph.getAyanamsa(jd)
        XCTAssertTrue(ayan > 23 && ayan < 25, "Ayanamsa ~24: \(ayan)")
    }

    func testAyanamsaAtReferenceEpoch() {
        let jd = Self.eph.gregorianToJd(year: 1956, month: 9, day: 22)
        let ayan = Self.eph.getAyanamsa(jd)
        XCTAssertEqual(ayan, 23.245, accuracy: 0.1)
    }

    // ===== Sunrise/Sunset =====

    func testSunriseDelhi() {
        let loc = Location.newDelhi
        let jd = Self.eph.gregorianToJd(year: 2025, month: 1, day: 15)
        let rise = Self.eph.sunriseJd(jd, loc)
        XCTAssertTrue(rise > 0)

        let localJd = rise + 0.5 + 5.5 / 24.0
        let frac = localJd - floor(localJd)
        let hours = frac * 24.0
        XCTAssertTrue(hours > 7.0 && hours < 7.5, "Delhi sunrise Jan: \(hours)")
    }

    func testSunsetDelhi() {
        let loc = Location.newDelhi
        let jd = Self.eph.gregorianToJd(year: 2025, month: 1, day: 15)
        let set = Self.eph.sunsetJd(jd, loc)
        XCTAssertTrue(set > 0)

        let localJd = set + 0.5 + 5.5 / 24.0
        let frac = localJd - floor(localJd)
        let hours = frac * 24.0
        XCTAssertTrue(hours > 17.0 && hours < 18.0, "Delhi sunset Jan: \(hours)")
    }

    // ===== Sidereal Solar Longitude =====

    func testSiderealSolarLongitude() {
        let jd = Self.eph.gregorianToJd(year: 2025, month: 4, day: 14)
        let sid = Self.eph.solarLongitudeSidereal(jd)
        XCTAssertTrue(sid < 5 || sid > 355, "Near Mesha Sankranti: \(sid)")
    }
}
