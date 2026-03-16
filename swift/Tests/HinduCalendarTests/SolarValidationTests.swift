import XCTest
@testable import HinduCalendar

/// Port of test_solar_validation.c -- 109 drikpanchang.com verified dates.
final class SolarValidationTests: XCTestCase {

    static let eph = Ephemeris()
    static let solar = Solar(ephemeris: eph)
    static let loc = Location.newDelhi

    private func check(_ gy: Int, _ gm: Int, _ gd: Int, _ type: SolarCalendarType,
                       _ expMonth: Int, _ expDay: Int, _ expYear: Int,
                       file: StaticString = #filePath, line: UInt = #line) {
        let sd = Self.solar.gregorianToSolar(year: gy, month: gm, day: gd, loc: Self.loc, type: type)
        let label = "\(type) \(gy)-\(gm)-\(gd)"
        XCTAssertEqual(sd.month, expMonth, "\(label) month", file: file, line: line)
        XCTAssertEqual(sd.day, expDay, "\(label) day", file: file, line: line)
        XCTAssertEqual(sd.year, expYear, "\(label) year", file: file, line: line)
    }

    // ===== Tamil: Chithirai 1 across 1950-2050, every 5 years (21 entries) =====

    func testTamilChithiraiAcrossYears() {
        check(1950,  4, 14, .tamil,  1, 1, 1872)
        check(1955,  4, 14, .tamil,  1, 1, 1877)
        check(1960,  4, 13, .tamil,  1, 1, 1882)
        check(1965,  4, 13, .tamil,  1, 1, 1887)
        check(1970,  4, 14, .tamil,  1, 1, 1892)
        check(1975,  4, 14, .tamil,  1, 1, 1897)
        check(1980,  4, 13, .tamil,  1, 1, 1902)
        check(1985,  4, 14, .tamil,  1, 1, 1907)
        check(1990,  4, 14, .tamil,  1, 1, 1912)
        check(1995,  4, 14, .tamil,  1, 1, 1917)
        check(2000,  4, 13, .tamil,  1, 1, 1922)
        check(2005,  4, 14, .tamil,  1, 1, 1927)
        check(2010,  4, 14, .tamil,  1, 1, 1932)
        check(2015,  4, 14, .tamil,  1, 1, 1937)
        check(2020,  4, 14, .tamil,  1, 1, 1942)
        check(2025,  4, 14, .tamil,  1, 1, 1947)
        check(2030,  4, 14, .tamil,  1, 1, 1952)
        check(2035,  4, 14, .tamil,  1, 1, 1957)
        check(2040,  4, 14, .tamil,  1, 1, 1962)
        check(2045,  4, 14, .tamil,  1, 1, 1967)
        check(2050,  4, 14, .tamil,  1, 1, 1972)
    }

    // ===== Tamil: all 12 months of 2025 =====

    func testTamil2025Months() {
        check(2025,  1, 14, .tamil, 10, 1, 1946)
        check(2025,  2, 13, .tamil, 11, 1, 1946)
        check(2025,  3, 15, .tamil, 12, 1, 1946)
        check(2025,  4, 14, .tamil,  1, 1, 1947)
        check(2025,  5, 15, .tamil,  2, 1, 1947)
        check(2025,  6, 15, .tamil,  3, 1, 1947)
        check(2025,  7, 16, .tamil,  4, 1, 1947)
        check(2025,  8, 17, .tamil,  5, 1, 1947)
        check(2025,  9, 17, .tamil,  6, 1, 1947)
        check(2025, 10, 17, .tamil,  7, 1, 1947)
        check(2025, 11, 16, .tamil,  8, 1, 1947)
        check(2025, 12, 16, .tamil,  9, 1, 1947)
    }

    // ===== Bengali: Boishakh 1 across years (12 entries) =====

    func testBengaliBoishakhAcrossYears() {
        check(1950,  4, 14, .bengali,  1, 1, 1357)
        check(1960,  4, 14, .bengali,  1, 1, 1367)
        check(1970,  4, 15, .bengali,  1, 1, 1377)
        check(1980,  4, 14, .bengali,  1, 1, 1387)
        check(1990,  4, 15, .bengali,  1, 1, 1397)
        check(2000,  4, 14, .bengali,  1, 1, 1407)
        check(2010,  4, 15, .bengali,  1, 1, 1417)
        check(2015,  4, 15, .bengali,  1, 1, 1422)
        check(2025,  4, 15, .bengali,  1, 1, 1432)
        check(2030,  4, 15, .bengali,  1, 1, 1437)
        check(2040,  4, 14, .bengali,  1, 1, 1447)
        check(2050,  4, 15, .bengali,  1, 1, 1457)
    }

    // ===== Bengali: all 12 months of 2025 =====

    func testBengali2025Months() {
        check(2025,  1, 15, .bengali, 10, 1, 1431)
        check(2025,  2, 13, .bengali, 11, 1, 1431)
        check(2025,  3, 15, .bengali, 12, 1, 1431)
        check(2025,  4, 15, .bengali,  1, 1, 1432)
        check(2025,  5, 15, .bengali,  2, 1, 1432)
        check(2025,  6, 16, .bengali,  3, 1, 1432)
        check(2025,  7, 17, .bengali,  4, 1, 1432)
        check(2025,  8, 18, .bengali,  5, 1, 1432)
        check(2025,  9, 18, .bengali,  6, 1, 1432)
        check(2025, 10, 18, .bengali,  7, 1, 1432)
        check(2025, 11, 17, .bengali,  8, 1, 1432)
        check(2025, 12, 17, .bengali,  9, 1, 1432)
    }

    // ===== Odia: all 12 months of 2025 =====

    func testOdia2025Months() {
        check(2025,  1, 14, .odia, 10, 1, 1432)
        check(2025,  2, 12, .odia, 11, 1, 1432)
        check(2025,  3, 14, .odia, 12, 1, 1432)
        check(2025,  4, 14, .odia,  1, 1, 1432)
        check(2025,  5, 15, .odia,  2, 1, 1432)
        check(2025,  6, 15, .odia,  3, 1, 1432)
        check(2025,  7, 16, .odia,  4, 1, 1432)
        check(2025,  8, 17, .odia,  5, 1, 1432)
        check(2025,  9, 17, .odia,  6, 1, 1433)
        check(2025, 10, 17, .odia,  7, 1, 1433)
        check(2025, 11, 16, .odia,  8, 1, 1433)
        check(2025, 12, 16, .odia,  9, 1, 1433)
    }

    // ===== Odia: all 12 months of 2030 =====

    func testOdia2030Months() {
        check(2030,  1, 14, .odia, 10, 1, 1437)
        check(2030,  2, 13, .odia, 11, 1, 1437)
        check(2030,  3, 15, .odia, 12, 1, 1437)
        check(2030,  4, 14, .odia,  1, 1, 1437)
        check(2030,  5, 15, .odia,  2, 1, 1437)
        check(2030,  6, 15, .odia,  3, 1, 1437)
        check(2030,  7, 17, .odia,  4, 1, 1437)
        check(2030,  8, 17, .odia,  5, 1, 1437)
        check(2030,  9, 17, .odia,  6, 1, 1438)
        check(2030, 10, 17, .odia,  7, 1, 1438)
        check(2030, 11, 16, .odia,  8, 1, 1438)
        check(2030, 12, 16, .odia,  9, 1, 1438)
    }

    // ===== Malayalam: Chingam 1 across 1950-2030, every 5 years (16 entries) =====

    func testMalayalamChingamAcrossYears() {
        check(1950,  8, 17, .malayalam,  1, 1, 1126)
        check(1955,  8, 17, .malayalam,  1, 1, 1131)
        check(1960,  8, 16, .malayalam,  1, 1, 1136)
        check(1965,  8, 17, .malayalam,  1, 1, 1141)
        check(1970,  8, 17, .malayalam,  1, 1, 1146)
        check(1975,  8, 17, .malayalam,  1, 1, 1151)
        check(1985,  8, 17, .malayalam,  1, 1, 1161)
        check(1990,  8, 17, .malayalam,  1, 1, 1166)
        check(1995,  8, 17, .malayalam,  1, 1, 1171)
        check(2000,  8, 17, .malayalam,  1, 1, 1176)
        check(2005,  8, 17, .malayalam,  1, 1, 1181)
        check(2010,  8, 17, .malayalam,  1, 1, 1186)
        check(2015,  8, 17, .malayalam,  1, 1, 1191)
        check(2020,  8, 17, .malayalam,  1, 1, 1196)
        check(2025,  8, 17, .malayalam,  1, 1, 1201)
        check(2030,  8, 17, .malayalam,  1, 1, 1206)
    }

    // ===== Malayalam: all 12 months of 2025 =====

    func testMalayalam2025Months() {
        check(2025,  1, 14, .malayalam,  6, 1, 1200)
        check(2025,  2, 13, .malayalam,  7, 1, 1200)
        check(2025,  3, 15, .malayalam,  8, 1, 1200)
        check(2025,  4, 14, .malayalam,  9, 1, 1200)
        check(2025,  5, 15, .malayalam, 10, 1, 1200)
        check(2025,  6, 15, .malayalam, 11, 1, 1200)
        check(2025,  7, 17, .malayalam, 12, 1, 1200)
        check(2025,  8, 17, .malayalam,  1, 1, 1201)
        check(2025,  9, 17, .malayalam,  2, 1, 1201)
        check(2025, 10, 18, .malayalam,  3, 1, 1201)
        check(2025, 11, 17, .malayalam,  4, 1, 1201)
        check(2025, 12, 16, .malayalam,  5, 1, 1201)
    }
}
