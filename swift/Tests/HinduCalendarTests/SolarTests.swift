import XCTest
@testable import HinduCalendar

final class SolarTests: XCTestCase {

    static let eph = Ephemeris()
    static let solar = Solar(ephemeris: eph)
    static let loc = Location.newDelhi

    private func checkSolar(_ gy: Int, _ gm: Int, _ gd: Int, _ type: SolarCalendarType,
                            _ expMonth: Int, _ expDay: Int, _ expYear: Int,
                            file: StaticString = #filePath, line: UInt = #line) {
        let sd = Self.solar.gregorianToSolar(year: gy, month: gm, day: gd, loc: Self.loc, type: type)
        XCTAssertEqual(sd.month, expMonth, "\(gy)-\(gm)-\(gd) month", file: file, line: line)
        XCTAssertEqual(sd.day, expDay, "\(gy)-\(gm)-\(gd) day", file: file, line: line)
        XCTAssertEqual(sd.year, expYear, "\(gy)-\(gm)-\(gd) year", file: file, line: line)
    }

    // ===== Sankranti =====

    func testSankrantiJd() {
        let jdEst = Self.eph.gregorianToJd(year: 2025, month: 4, day: 14)
        let jdSank = Self.solar.sankrantiJd(jdEst, 0.0)
        let lon = Self.eph.solarLongitudeSidereal(jdSank)
        XCTAssertTrue(abs(lon) < 0.0001 || abs(lon - 360) < 0.0001, "lon at Mesha Sankranti: \(lon)")
        let ymd = Self.eph.jdToGregorian(jdSank)
        XCTAssertTrue(ymd.month == 4 && (ymd.day == 13 || ymd.day == 14))
    }

    // ===== Month names =====

    func testMonthNames() {
        XCTAssertEqual(SolarCalendarType.tamil.monthName(1), "Chithirai")
        XCTAssertEqual(SolarCalendarType.bengali.monthName(1), "Boishakh")
        XCTAssertEqual(SolarCalendarType.odia.monthName(1), "Baisakha")
        XCTAssertEqual(SolarCalendarType.malayalam.monthName(1), "Chingam")
        XCTAssertEqual(SolarCalendarType.malayalam.monthName(12), "Karkadakam")
    }

    func testEraNames() {
        XCTAssertEqual(SolarCalendarType.tamil.eraName, "Saka")
        XCTAssertEqual(SolarCalendarType.bengali.eraName, "Bangabda")
        XCTAssertEqual(SolarCalendarType.odia.eraName, "Amli")
        XCTAssertEqual(SolarCalendarType.malayalam.eraName, "Kollam")
    }

    // ===== Tamil =====

    func testTamil() {
        checkSolar(2025, 4, 14, .tamil, 1, 1, 1947)
        checkSolar(2025, 4, 15, .tamil, 1, 2, 1947)
        checkSolar(2025, 4, 13, .tamil, 12, 30, 1946)
        checkSolar(2000, 4, 13, .tamil, 1, 1, 1922)
        checkSolar(2025, 4, 30, .tamil, 1, 17, 1947)
        checkSolar(2025, 4, 1, .tamil, 12, 18, 1946)
        checkSolar(2025, 1, 1, .tamil, 9, 17, 1946)
        checkSolar(2025, 7, 15, .tamil, 3, 31, 1947)
        checkSolar(2000, 4, 14, .tamil, 1, 2, 1922)
        checkSolar(2000, 1, 1, .tamil, 9, 17, 1921)
    }

    // ===== Bengali =====

    func testBengali() {
        checkSolar(2025, 4, 15, .bengali, 1, 1, 1432)
        checkSolar(2025, 4, 30, .bengali, 1, 16, 1432)
        checkSolar(2025, 4, 14, .bengali, 12, 31, 1431)
        checkSolar(2025, 4, 16, .bengali, 1, 2, 1432)
        checkSolar(2025, 4, 13, .bengali, 12, 30, 1431)
        checkSolar(2025, 4, 1, .bengali, 12, 18, 1431)
        checkSolar(2025, 1, 1, .bengali, 9, 17, 1431)
        checkSolar(2025, 6, 15, .bengali, 2, 32, 1432)
        checkSolar(2000, 1, 1, .bengali, 9, 16, 1406)
    }

    // ===== Odia =====

    func testOdia() {
        checkSolar(2025, 4, 14, .odia, 1, 1, 1432)
        checkSolar(2025, 4, 15, .odia, 1, 2, 1432)
        checkSolar(2025, 4, 13, .odia, 12, 31, 1432)
        checkSolar(2025, 4, 30, .odia, 1, 17, 1432)
        checkSolar(2025, 4, 1, .odia, 12, 19, 1432)
        checkSolar(2025, 1, 1, .odia, 9, 18, 1432)
        checkSolar(2025, 7, 15, .odia, 3, 31, 1432)
    }

    func testOdiaBoundary() {
        checkSolar(2026, 7, 16, .odia, 3, 32, 1433)
        checkSolar(2026, 7, 17, .odia, 4, 1, 1433)
        checkSolar(2022, 7, 16, .odia, 3, 32, 1429)
        checkSolar(2022, 7, 17, .odia, 4, 1, 1429)
        checkSolar(2018, 7, 16, .odia, 3, 32, 1425)
        checkSolar(2018, 7, 17, .odia, 4, 1, 1425)
        checkSolar(2001, 4, 13, .odia, 12, 31, 1408)
        checkSolar(2001, 4, 14, .odia, 1, 1, 1408)
        checkSolar(2024, 12, 15, .odia, 9, 1, 1432)
        checkSolar(2013, 5, 14, .odia, 1, 31, 1420)
        checkSolar(2003, 11, 16, .odia, 7, 30, 1411)
    }

    // ===== Malayalam =====

    func testMalayalam() {
        checkSolar(2025, 8, 17, .malayalam, 1, 1, 1201)
        checkSolar(2025, 8, 16, .malayalam, 12, 31, 1200)
        checkSolar(2025, 8, 18, .malayalam, 1, 2, 1201)
        checkSolar(2025, 8, 31, .malayalam, 1, 15, 1201)
        checkSolar(2025, 8, 1, .malayalam, 12, 16, 1200)
        checkSolar(2025, 1, 1, .malayalam, 5, 17, 1200)
        checkSolar(2025, 4, 15, .malayalam, 9, 2, 1200)
    }

    // ===== Solar month start/length =====

    func testSolarMonthStart() {
        var jd = Self.solar.solarMonthStart(1, 1947, .tamil, Self.loc)
        var ymd = Self.eph.jdToGregorian(jd)
        XCTAssertEqual(ymd.year, 2025)
        XCTAssertEqual(ymd.month, 4)
        XCTAssertEqual(ymd.day, 14)

        jd = Self.solar.solarMonthStart(1, 1432, .bengali, Self.loc)
        ymd = Self.eph.jdToGregorian(jd)
        XCTAssertEqual(ymd.year, 2025)
        XCTAssertEqual(ymd.month, 4)
        XCTAssertEqual(ymd.day, 15)
    }

    func testSolarMonthLength() {
        let len = Self.solar.solarMonthLength(1, 1947, .tamil, Self.loc)
        XCTAssertTrue(len >= 29 && len <= 32, "Tamil month 1 length: \(len)")
    }
}
