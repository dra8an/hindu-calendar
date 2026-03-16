import XCTest
@testable import HinduCalendar

/// Port of test_various_locations.c -- validates tithi and solar calendars
/// for multiple locations (Ujjain, NYC, LA) from various_locations.csv.
final class VariousLocationsTests: XCTestCase {

    static let eph = Ephemeris()
    static let tithi = Tithi(ephemeris: eph)
    static let solar = Solar(ephemeris: eph)
    static let loc = Location.newDelhi

    private static func findCsv() -> String? {
        let testDir = URL(fileURLWithPath: #filePath).deletingLastPathComponent()
        let projectRoot = testDir
            .deletingLastPathComponent()  // Tests/
            .deletingLastPathComponent()  // swift/
            .deletingLastPathComponent()  // hindu-calendar/
        let path = projectRoot.appendingPathComponent("validation/moshier/various_locations.csv").path
        return FileManager.default.fileExists(atPath: path) ? path : nil
    }

    private static func parseSolarType(_ cal: String) -> SolarCalendarType? {
        switch cal {
        case "tamil": return .tamil
        case "bengali": return .bengali
        case "odia": return .odia
        case "malayalam": return .malayalam
        default: return nil
        }
    }

    func testVariousLocations() throws {
        guard let path = Self.findCsv() else {
            throw XCTSkip("various_locations.csv not found")
        }

        let content = try String(contentsOfFile: path, encoding: .utf8)
        let lines = content.components(separatedBy: .newlines)

        var checked = 0
        var failures = 0

        // Skip header (line 0)
        for i in 1..<lines.count {
            let line = lines[i]
            if line.isEmpty { continue }

            // Split by commas, handling empty trailing fields
            let parts = line.split(separator: ",", omittingEmptySubsequences: false)
            guard parts.count >= 8 else { continue }

            let calendar = String(parts[0])
            let location = String(parts[1])
            guard let lat = Double(parts[2]),
                  let lon = Double(parts[3]),
                  let utcOffset = Double(parts[4]),
                  let gy = Int(parts[5]),
                  let gm = Int(parts[6]),
                  let gd = Int(parts[7]) else { continue }

            let loc = Location(latitude: lat, longitude: lon,
                               altitude: 0.0, utcOffset: utcOffset)

            if calendar == "lunisolar" {
                // Lunisolar: verify tithi
                guard parts.count > 8, let expTithi = Int(parts[8]), expTithi > 0 else {
                    continue
                }

                let jd = Self.eph.gregorianToJd(year: gy, month: gm, day: gd)
                var jdRise = Self.eph.sunriseJd(jd, loc)
                if jdRise <= 0 {
                    jdRise = jd + 0.5 - utcOffset / 24.0
                }
                let actualTithi = Self.tithi.tithiAtMoment(jdRise)

                if actualTithi != expTithi {
                    failures += 1
                    XCTFail("lunisolar \(location) \(gy)-\(gm)-\(gd): "
                        + "tithi expected \(expTithi), got \(actualTithi)")
                }
                checked += 1
            } else {
                // Solar calendar: verify month, day, year
                guard parts.count > 11,
                      let expMonth = Int(parts[9]), expMonth > 0,
                      let expDay = Int(parts[10]),
                      let expYear = Int(parts[11]) else {
                    continue
                }

                guard let type = Self.parseSolarType(calendar) else { continue }

                let sd = Self.solar.gregorianToSolar(year: gy, month: gm, day: gd,
                                                      loc: loc, type: type)

                if sd.month != expMonth || sd.day != expDay || sd.year != expYear {
                    failures += 1
                    XCTFail("\(calendar) \(location) \(gy)-\(gm)-\(gd): "
                        + "expected m\(expMonth) d\(expDay) y\(expYear), "
                        + "got m\(sd.month) d\(sd.day) y\(sd.year)")
                }
                checked += 1
            }
        }

        print("Various locations: \(checked) entries checked, \(failures) failures")
        XCTAssertEqual(failures, 0)
        XCTAssertTrue(checked > 100, "Should check >100 entries, got \(checked)")
    }
}
