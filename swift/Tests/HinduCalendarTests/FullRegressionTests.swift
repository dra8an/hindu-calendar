import XCTest
@testable import HinduCalendar

final class FullRegressionTests: XCTestCase {

    static let eph = Ephemeris()
    static let tithi = Tithi(ephemeris: eph)
    static let masa = Masa(ephemeris: eph, tithi: tithi)
    static let solar = Solar(ephemeris: eph)
    static let loc = Location.newDelhi

    /// Sample step for lunisolar CSV (50 = ~1,100 days out of 55K)
    private static let sampleStep = 50

    private static func findFile(_ name: String) -> String? {
        // Try relative to #filePath (Tests/HinduCalendarTests/)
        let testDir = URL(fileURLWithPath: #filePath).deletingLastPathComponent()
        let projectRoot = testDir
            .deletingLastPathComponent()  // Tests/
            .deletingLastPathComponent()  // swift/
            .deletingLastPathComponent()  // hindu-calendar/

        let candidates = [
            projectRoot.appendingPathComponent("validation/moshier/\(name)").path,
            projectRoot.appendingPathComponent("validation/moshier/solar/\(name)").path,
        ]

        for path in candidates {
            if FileManager.default.fileExists(atPath: path) {
                return path
            }
        }
        return nil
    }

    // ===== Lunisolar CSV regression =====

    func testLunisolarCsvRegression() throws {
        guard let path = Self.findFile("ref_1900_2050.csv") else {
            throw XCTSkip("ref_1900_2050.csv not found")
        }

        let content = try String(contentsOfFile: path, encoding: .utf8)
        let lines = content.components(separatedBy: .newlines)

        var sampled = 0
        var failures = 0

        // Skip header (line 0), then sample every sampleStep-th line
        for i in stride(from: 1, to: lines.count, by: Self.sampleStep) {
            let line = lines[i]
            if line.isEmpty { continue }

            let parts = line.split(separator: ",")
            guard parts.count == 7,
                  let y = Int(parts[0]), let m = Int(parts[1]), let d = Int(parts[2]),
                  let expTithi = Int(parts[3]), let expMasa = Int(parts[4]),
                  let expAdhika = Int(parts[5]), let expSaka = Int(parts[6]) else {
                continue
            }

            let ti = Self.tithi.tithiAtSunrise(year: y, month: m, day: d, loc: Self.loc)
            let mi = Self.masa.masaForDate(year: y, month: m, day: d, loc: Self.loc)

            let label = "\(y)-\(m)-\(d)"
            if ti.tithiNum != expTithi {
                failures += 1
                XCTFail("\(label) tithi: got \(ti.tithiNum), expected \(expTithi)")
            }
            if mi.name.rawValue != expMasa {
                failures += 1
                XCTFail("\(label) masa: got \(mi.name.rawValue), expected \(expMasa)")
            }
            if mi.isAdhika != (expAdhika != 0) {
                failures += 1
                XCTFail("\(label) adhika: got \(mi.isAdhika), expected \(expAdhika != 0)")
            }
            if mi.yearSaka != expSaka {
                failures += 1
                XCTFail("\(label) saka: got \(mi.yearSaka), expected \(expSaka)")
            }
            sampled += 1
        }

        print("Lunisolar CSV: \(sampled) days sampled, \(failures) assertion failures")
        XCTAssertEqual(failures, 0)
        XCTAssertTrue(sampled > 1000, "Should sample >1000 days, got \(sampled)")
    }

    // ===== Solar calendar CSV regression =====

    private func testSolarCalendar(_ type: SolarCalendarType, _ csvName: String) throws {
        guard let path = Self.findFile(csvName) else {
            throw XCTSkip("\(csvName) not found")
        }

        let content = try String(contentsOfFile: path, encoding: .utf8)
        let lines = content.components(separatedBy: .newlines)

        var monthsChecked = 0
        var failures = 0

        for i in 1..<lines.count {
            let line = lines[i]
            if line.isEmpty { continue }

            let parts = line.split(separator: ",")
            guard parts.count == 7,
                  let expMonth = Int(parts[0]), let expYear = Int(parts[1]),
                  let expLength = Int(parts[2]),
                  let gy = Int(parts[3]), let gm = Int(parts[4]), let gd = Int(parts[5]) else {
                continue
            }

            // Check first day of month
            let sd = Self.solar.gregorianToSolar(year: gy, month: gm, day: gd, loc: Self.loc, type: type)
            if sd.month != expMonth {
                failures += 1
                XCTFail("\(type) \(gy)-\(gm)-\(gd): month got \(sd.month), expected \(expMonth)")
            }
            if sd.year != expYear {
                failures += 1
                XCTFail("\(type) \(gy)-\(gm)-\(gd): year got \(sd.year), expected \(expYear)")
            }
            if sd.day != 1 {
                failures += 1
                XCTFail("\(type) \(gy)-\(gm)-\(gd): day got \(sd.day), expected 1")
            }

            // Check last day of month
            let jdFirst = Self.eph.gregorianToJd(year: gy, month: gm, day: gd)
            let jdLast = jdFirst + Double(expLength - 1)
            let lastYmd = Self.eph.jdToGregorian(jdLast)
            let sdLast = Self.solar.gregorianToSolar(
                year: lastYmd.year, month: lastYmd.month, day: lastYmd.day,
                loc: Self.loc, type: type)
            if sdLast.day != expLength {
                failures += 1
                XCTFail("\(type) last day \(lastYmd.year)-\(lastYmd.month)-\(lastYmd.day): day got \(sdLast.day), expected \(expLength)")
            }

            monthsChecked += 1
        }

        print("\(type): checked \(monthsChecked) months, \(failures) failures")
        XCTAssertEqual(failures, 0)
        XCTAssertTrue(monthsChecked > 1800, "Should check >1800 months, got \(monthsChecked)")
    }

    func testTamilCsvRegression() throws {
        try testSolarCalendar(.tamil, "tamil_months_1900_2050.csv")
    }

    func testBengaliCsvRegression() throws {
        try testSolarCalendar(.bengali, "bengali_months_1900_2050.csv")
    }

    func testOdiaCsvRegression() throws {
        try testSolarCalendar(.odia, "odia_months_1900_2050.csv")
    }

    func testMalayalamCsvRegression() throws {
        try testSolarCalendar(.malayalam, "malayalam_months_1900_2050.csv")
    }
}
