import XCTest
@testable import HinduCalendar

/// Port of test_lunisolar_month.c -- roundtrip, CSV regression, and extra coverage
/// beyond what MasaTests.swift already covers.
final class LunisolarMonthTests: XCTestCase {

    static let eph = Ephemeris()
    static let tithi = Tithi(ephemeris: eph)
    static let masa = Masa(ephemeris: eph, tithi: tithi)
    static let loc = Location.newDelhi

    private static func findFile(_ name: String) -> String? {
        let testDir = URL(fileURLWithPath: #filePath).deletingLastPathComponent()
        let projectRoot = testDir
            .deletingLastPathComponent()
            .deletingLastPathComponent()
            .deletingLastPathComponent()
        let path = projectRoot.appendingPathComponent("validation/moshier/\(name)").path
        return FileManager.default.fileExists(atPath: path) ? path : nil
    }

    // MARK: - Known month starts (with full roundtrip + day-before checks)

    func testKnownMonthStarts() {
        let cases: [(MasaName, Int, Bool, Int, Int, Int)] = [
            (.chaitra,      1947, false, 2025,  3, 30),
            (.vaishakha,    1947, false, 2025,  4, 28),
            (.jyeshtha,     1947, false, 2025,  5, 28),
            (.ashadha,      1947, false, 2025,  6, 26),
            (.shravana,     1947, false, 2025,  7, 25),
            (.bhadrapada,   1947, false, 2025,  8, 24),
            (.ashvina,      1947, false, 2025,  9, 22),
            (.kartika,      1947, false, 2025, 10, 22),
            (.margashirsha, 1947, false, 2025, 11, 21),
            (.pausha,       1947, false, 2025, 12, 21),
            (.bhadrapada,   1934, true,  2012,  8, 18),
            (.bhadrapada,   1934, false, 2012,  9, 17),
            (.ashadha,      1937, true,  2015,  6, 17),
            (.ashadha,      1937, false, 2015,  7, 17),
            (.phalguna,     1946, false, 2025,  2, 28),
            (.chaitra,      1946, false, 2024,  4,  9),
        ]

        for (masaName, saka, adhika, expY, expM, expD) in cases {
            let label = "\(masaName)\(adhika ? " adhika" : "") \(saka)"
            let jd = Self.masa.lunisolarMonthStart(masaName, saka, adhika, .amanta, Self.loc)
            XCTAssertTrue(jd > 0, "\(label) should be found")

            let ymd = Self.eph.jdToGregorian(jd)
            XCTAssertEqual(ymd.year, expY, "\(label) year")
            XCTAssertEqual(ymd.month, expM, "\(label) month")
            XCTAssertEqual(ymd.day, expD, "\(label) day")

            // Verify masa_for_date roundtrip
            let mi = Self.masa.masaForDate(year: expY, month: expM, day: expD, loc: Self.loc)
            XCTAssertEqual(mi.name, masaName, "\(label) masa roundtrip")
            XCTAssertEqual(mi.isAdhika, adhika, "\(label) adhika roundtrip")
            XCTAssertEqual(mi.yearSaka, saka, "\(label) saka roundtrip")

            // Verify day before belongs to different month
            let jdPrev = jd - 1
            let prevYmd = Self.eph.jdToGregorian(jdPrev)
            let miPrev = Self.masa.masaForDate(year: prevYmd.year, month: prevYmd.month,
                                                day: prevYmd.day, loc: Self.loc)
            XCTAssertTrue(miPrev.name != masaName || miPrev.isAdhika != adhika,
                          "\(label) prev day should differ")
        }
    }

    // MARK: - Month lengths

    func testMonthLengths() {
        // All 12 months of Saka 1947 should be 29 or 30
        for m in MasaName.allCases {
            let len = Self.masa.lunisolarMonthLength(m, 1947, false, .amanta, Self.loc)
            XCTAssertTrue(len == 29 || len == 30, "\(m) 1947 length: \(len)")
        }

        // Adhika Bhadrapada 1934
        let adhikaLen = Self.masa.lunisolarMonthLength(.bhadrapada, 1934, true, .amanta, Self.loc)
        XCTAssertTrue(adhikaLen == 29 || adhikaLen == 30,
                      "Adhika Bhadrapada 1934 length: \(adhikaLen)")

        // start + length = next month start
        let jdStart = Self.masa.lunisolarMonthStart(.vaishakha, 1947, false, .amanta, Self.loc)
        let len = Self.masa.lunisolarMonthLength(.vaishakha, 1947, false, .amanta, Self.loc)
        let jdNext = Self.masa.lunisolarMonthStart(.jyeshtha, 1947, false, .amanta, Self.loc)
        XCTAssertEqual(Int(jdNext - jdStart), len, "Vaishakha+length = Jyeshtha start")
    }

    // MARK: - Roundtrip (sampled: every 10th year from 1900-2050)

    func testRoundtrip() {
        let sampleYears = stride(from: 1900, through: 2050, by: 10)
        var monthsTested = 0
        var failures = 0

        for y in sampleYears {
            var prevMasa = -1
            var prevAdhika = -1
            var prevSaka = -1
            var firstTransition = true

            for m in 1...12 {
                let dim = Self.daysInMonth(y, m)
                for d in 1...dim {
                    let mi = Self.masa.masaForDate(year: y, month: m, day: d, loc: Self.loc)
                    if mi.name.rawValue != prevMasa || (mi.isAdhika ? 1 : 0) != prevAdhika ||
                       mi.yearSaka != prevSaka {
                        if firstTransition {
                            firstTransition = false
                            prevMasa = mi.name.rawValue
                            prevAdhika = mi.isAdhika ? 1 : 0
                            prevSaka = mi.yearSaka
                            continue
                        }

                        let jd = Self.masa.lunisolarMonthStart(
                            mi.name, mi.yearSaka, mi.isAdhika, .amanta, Self.loc)
                        if jd > 0 {
                            let ymd = Self.eph.jdToGregorian(jd)
                            if ymd.year != y || ymd.month != m || ymd.day != d {
                                failures += 1
                                XCTFail("\(mi.isAdhika ? "Adhika " : "")\(mi.name) \(mi.yearSaka): "
                                    + "expected \(y)-\(m)-\(d), got \(ymd.year)-\(ymd.month)-\(ymd.day)")
                            }
                        } else {
                            failures += 1
                            XCTFail("\(mi.isAdhika ? "Adhika " : "")\(mi.name) \(mi.yearSaka): not found")
                        }

                        monthsTested += 1
                        prevMasa = mi.name.rawValue
                        prevAdhika = mi.isAdhika ? 1 : 0
                        prevSaka = mi.yearSaka
                    }
                }
            }
        }

        print("Roundtrip: \(monthsTested) months tested, \(failures) failures")
        XCTAssertEqual(failures, 0)
        XCTAssertTrue(monthsTested > 150, "Should test >150 months, got \(monthsTested)")
    }

    // MARK: - CSV regression

    func testLunisolarMonthsCsvRegression() throws {
        guard let path = Self.findFile("lunisolar_months.csv") else {
            throw XCTSkip("lunisolar_months.csv not found")
        }

        let content = try String(contentsOfFile: path, encoding: .utf8)
        let lines = content.components(separatedBy: .newlines)

        var count = 0
        var failures = 0

        // Skip header (line 0)
        for i in 1..<lines.count {
            let line = lines[i]
            if line.isEmpty { continue }

            let parts = line.split(separator: ",")
            // Format: masa,is_adhika,saka_year,length,greg_year,greg_month,greg_day,masa_name
            guard parts.count >= 7,
                  let masaNum = Int(parts[0]), let isAdhika = Int(parts[1]),
                  let sakaYear = Int(parts[2]), let length = Int(parts[3]),
                  let gy = Int(parts[4]), let gm = Int(parts[5]), let gd = Int(parts[6]) else {
                continue
            }

            let masaName = MasaName.fromNumber(masaNum)
            let adhika = isAdhika != 0

            // Verify start date
            let jd = Self.masa.lunisolarMonthStart(masaName, sakaYear, adhika, .amanta, Self.loc)
            if jd > 0 {
                let ymd = Self.eph.jdToGregorian(jd)
                if ymd.year != gy || ymd.month != gm || ymd.day != gd {
                    failures += 1
                    XCTFail("\(adhika ? "Adhika " : "")\(masaName) \(sakaYear): "
                        + "expected \(gy)-\(gm)-\(gd), got \(ymd.year)-\(ymd.month)-\(ymd.day)")
                }
            } else {
                failures += 1
                XCTFail("\(adhika ? "Adhika " : "")\(masaName) \(sakaYear): not found")
            }

            // Verify length
            if length > 0 {
                let calcLen = Self.masa.lunisolarMonthLength(masaName, sakaYear, adhika,
                                                             .amanta, Self.loc)
                if calcLen != length {
                    failures += 1
                    XCTFail("\(adhika ? "Adhika " : "")\(masaName) \(sakaYear) length: "
                        + "expected \(length), got \(calcLen)")
                }
            }

            count += 1
        }

        print("Lunisolar months CSV: \(count) entries, \(failures) failures")
        XCTAssertEqual(failures, 0)
        XCTAssertTrue(count > 1800, "Should test >1800 months, got \(count)")
    }

    // MARK: - Purnimanta month starts (extra checks beyond MasaTests)

    func testPurnimantaMonthStartsExtended() {
        for m in MasaName.allCases {
            let jdAmanta = Self.masa.lunisolarMonthStart(m, 1947, false, .amanta, Self.loc)
            let jdPurni = Self.masa.lunisolarMonthStart(m, 1947, false, .purnimanta, Self.loc)

            XCTAssertTrue(jdPurni > 0, "Purnimanta \(m) found")

            // Offset 13-17 days from Amanta
            let diff = jdAmanta - jdPurni
            XCTAssertTrue(diff >= 13 && diff <= 17, "Purnimanta \(m) offset \(diff)")

            // Tithi at sunrise should be Krishna (16-30)
            var jr = Self.eph.sunriseJd(jdPurni, Self.loc)
            if jr <= 0 { jr = jdPurni + 0.5 - Self.loc.utcOffset / 24.0 }
            let t = Self.tithi.tithiAtMoment(jr)
            XCTAssertTrue(t >= 16 && t <= 30, "Purnimanta \(m) tithi \(t) should be Krishna")
        }
    }

    // MARK: - Purnimanta month lengths

    func testPurnimantaMonthLengths() {
        for m in MasaName.allCases {
            let len = Self.masa.lunisolarMonthLength(m, 1947, false, .purnimanta, Self.loc)
            XCTAssertTrue(len == 29 || len == 30, "Purnimanta \(m) length: \(len)")
        }

        // start + length = next month start
        let jdStart = Self.masa.lunisolarMonthStart(.vaishakha, 1947, false, .purnimanta, Self.loc)
        let len = Self.masa.lunisolarMonthLength(.vaishakha, 1947, false, .purnimanta, Self.loc)
        let jdNext = Self.masa.lunisolarMonthStart(.jyeshtha, 1947, false, .purnimanta, Self.loc)
        XCTAssertEqual(Int(jdNext - jdStart), len, "Purnimanta Vaishakha+length = Jyeshtha start")
    }

    // MARK: - Helpers

    private static func daysInMonth(_ y: Int, _ m: Int) -> Int {
        let mdays = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
        if m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
            return 29
        }
        return mdays[m]
    }
}
