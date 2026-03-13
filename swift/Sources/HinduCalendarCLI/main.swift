import Foundation
import HinduCalendar

func printUsage() {
    let msg = """
    Usage: hindu-calendar [options]
      -y YEAR      Gregorian year (default: current)
      -m MONTH     Gregorian month 1-12 (default: current)
      -d DAY       Specific day (if omitted, shows full month)
      -s TYPE      Solar calendar: tamil, bengali, odia, malayalam
                   (if omitted, shows lunisolar panchang)
      -l LAT,LON   Location (default: New Delhi 28.6139,77.2090)
      -u OFFSET    UTC offset in hours (default: 5.5)
      -h           Show this help
    """
    FileHandle.standardError.write(Data(msg.utf8))
}

func printSolarDay(_ ephemeris: Ephemeris, _ solar: Solar, _ year: Int, _ month: Int,
                    _ day: Int, _ loc: Location, _ type: SolarCalendarType) {
    let sd = solar.gregorianToSolar(year: year, month: month, day: day, loc: loc, type: type)
    let jd = ephemeris.gregorianToJd(year: year, month: month, day: day)
    let dow = ephemeris.dayOfWeek(jd)
    let calName: String
    switch type {
    case .tamil: calName = "Tamil"
    case .bengali: calName = "Bengali"
    case .odia: calName = "Odia"
    case .malayalam: calName = "Malayalam"
    }

    print(String(format: "Date:         %04d-%02d-%02d (%@)", year, month, day,
                DateUtils.dayOfWeekName(dow)))
    print(String(format: "Calendar:     %@ Solar", calName))
    print(String(format: "Solar Date:   %@ %d, %d (%@)",
                type.monthName(sd.month), sd.day, sd.year, type.eraName))
    let rashiName = (sd.rashi >= 1 && sd.rashi <= 12) ? Solar.rashiNames[sd.rashi] : "???"
    print(String(format: "Rashi:        %@", rashiName))
}

func printSolarMonth(_ ephemeris: Ephemeris, _ solar: Solar, _ year: Int, _ month: Int,
                      _ loc: Location, _ type: SolarCalendarType) {
    let ndays = DateUtils.daysInMonth(year: year, month: month)
    let sd1 = solar.gregorianToSolar(year: year, month: month, day: 1, loc: loc, type: type)
    let calName: String
    switch type {
    case .tamil: calName = "Tamil"
    case .bengali: calName = "Bengali"
    case .odia: calName = "Odia"
    case .malayalam: calName = "Malayalam"
    }

    print(String(format: "%@ Solar Calendar — %@ %d (%@)",
                calName, type.monthName(sd1.month), sd1.year, type.eraName))
    print(String(format: "Gregorian %04d-%02d\n", year, month))

    print(String(format: "%-12s %-5s %-20s %@", "Date", "Day", "Solar Date", ""))
    print(String(format: "%-12s %-5s %-20s", "----------", "---", "--------------------"))

    for d in 1...ndays {
        let sd = solar.gregorianToSolar(year: year, month: month, day: d, loc: loc, type: type)
        let jd = ephemeris.gregorianToJd(year: year, month: month, day: d)
        let dow = ephemeris.dayOfWeek(jd)

        print(String(format: "%04d-%02d-%02d   %-5s %@ %d, %d",
                    year, month, d,
                    DateUtils.dayOfWeekShort(dow),
                    type.monthName(sd.month),
                    sd.day, sd.year))
    }
}

// ===== Main =====

let calendar = Calendar.current
let now = Date()
var year = calendar.component(.year, from: now)
var month = calendar.component(.month, from: now)
var day = 0
var loc = Location.newDelhi
var solarMode = false
var solarType = SolarCalendarType.tamil

var args = CommandLine.arguments
args.removeFirst() // program name
var i = 0

while i < args.count {
    switch args[i] {
    case "-y":
        i += 1
        year = Int(args[i])!
    case "-m":
        i += 1
        month = Int(args[i])!
    case "-d":
        i += 1
        day = Int(args[i])!
    case "-s":
        i += 1
        guard let st = SolarCalendarType.fromString(args[i]) else {
            FileHandle.standardError.write(Data("Unknown solar calendar: \(args[i])\n".utf8))
            exit(1)
        }
        solarType = st
        solarMode = true
    case "-l":
        i += 1
        let parts = args[i].split(separator: ",")
        loc = Location(latitude: Double(parts[0])!, longitude: Double(parts[1])!,
                       altitude: loc.altitude, utcOffset: loc.utcOffset)
    case "-u":
        i += 1
        let offset = Double(args[i])!
        loc = Location(latitude: loc.latitude, longitude: loc.longitude,
                       altitude: loc.altitude, utcOffset: offset)
    case "-h":
        printUsage()
        exit(0)
    default:
        FileHandle.standardError.write(Data("Unknown option: \(args[i])\n".utf8))
        printUsage()
        exit(1)
    }
    i += 1
}

if month < 1 || month > 12 {
    FileHandle.standardError.write(Data("Error: month must be 1-12\n".utf8))
    exit(1)
}

let ephemeris = Ephemeris()

if solarMode {
    let solar = Solar(ephemeris: ephemeris)
    if day > 0 {
        printSolarDay(ephemeris, solar, year, month, day, loc, solarType)
    } else {
        printSolarMonth(ephemeris, solar, year, month, loc, solarType)
    }
} else {
    let panchang = Panchang(ephemeris: ephemeris)
    if day > 0 {
        let jd = ephemeris.gregorianToJd(year: year, month: month, day: day)
        let jdSunrise = ephemeris.sunriseJd(jd, loc)
        let ti = panchang.tithi.tithiAtSunrise(year: year, month: month, day: day, loc: loc)
        let hd = panchang.gregorianToHindu(year: year, month: month, day: day, loc: loc)
        let pd = PanchangDay(gregYear: year, gregMonth: month, gregDay: day,
                            jdSunrise: jdSunrise, hinduDate: hd, tithi: ti)
        print(panchang.formatDayPanchang(pd, loc.utcOffset), terminator: "")
    } else {
        print(String(format: "Hindu Calendar — %04d-%02d (%.4f°N, %.4f°E, UTC%+.1f)\n",
                    year, month, loc.latitude, loc.longitude, loc.utcOffset))
        let days = panchang.generateMonthPanchang(year: year, month: month, loc: loc)
        print(panchang.formatMonthPanchang(days, loc.utcOffset), terminator: "")
    }
}
