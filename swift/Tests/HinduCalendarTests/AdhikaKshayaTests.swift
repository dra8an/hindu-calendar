import XCTest
@testable import HinduCalendar

/// Tests all adhika (repeated) and kshaya (skipped) tithi days from
/// adhika_kshaya_tithis.csv -- ~4,269 edge-case days across 1900-2050.
final class AdhikaKshayaTests: XCTestCase {

    static let eph = Ephemeris()
    static let tithi = Tithi(ephemeris: eph)
    static let masa = Masa(ephemeris: eph, tithi: tithi)
    static let loc = Location.newDelhi

    private static func findCsv() -> String? {
        let testDir = URL(fileURLWithPath: #filePath).deletingLastPathComponent()
        let projectRoot = testDir
            .deletingLastPathComponent()  // Tests/
            .deletingLastPathComponent()  // swift/
            .deletingLastPathComponent()  // hindu-calendar/
        let path = projectRoot.appendingPathComponent("validation/moshier/adhika_kshaya_tithis.csv").path
        return FileManager.default.fileExists(atPath: path) ? path : nil
    }

    func testAdhikaKshayaCsv() throws {
        guard let path = Self.findCsv() else {
            throw XCTSkip("adhika_kshaya_tithis.csv not found")
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
            // Format: year,month,day,tithi,masa,adhika,saka,type
            guard parts.count == 8,
                  let y = Int(parts[0]), let m = Int(parts[1]), let d = Int(parts[2]),
                  let expTithi = Int(parts[3]), let expMasa = Int(parts[4]),
                  let expAdhika = Int(parts[5]), let expSaka = Int(parts[6]) else {
                continue
            }

            let ti = Self.tithi.tithiAtSunrise(year: y, month: m, day: d, loc: Self.loc)
            let mi = Self.masa.masaForDate(year: y, month: m, day: d, loc: Self.loc)
            let label = "\(y)-\(m)-\(d) [\(parts[7])]"

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

            count += 1
        }

        print("Adhika/Kshaya CSV: \(count) days tested, \(failures) failures")
        XCTAssertEqual(failures, 0)
        XCTAssertTrue(count > 4000, "Should test >4000 days, got \(count)")
    }
}
