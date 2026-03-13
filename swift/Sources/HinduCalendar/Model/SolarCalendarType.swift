public enum SolarCalendarType {
    case tamil
    case bengali
    case odia
    case malayalam

    public var firstRashi: Int {
        switch self {
        case .tamil: return 1
        case .bengali: return 1
        case .odia: return 1
        case .malayalam: return 5
        }
    }

    public var yearStartRashi: Int {
        switch self {
        case .tamil: return 1
        case .bengali: return 1
        case .odia: return 6
        case .malayalam: return 5
        }
    }

    public var gyOffsetOn: Int {
        switch self {
        case .tamil: return 78
        case .bengali: return 593
        case .odia: return 592
        case .malayalam: return 824
        }
    }

    public var gyOffsetBefore: Int {
        switch self {
        case .tamil: return 79
        case .bengali: return 594
        case .odia: return 593
        case .malayalam: return 825
        }
    }

    public var eraName: String {
        switch self {
        case .tamil: return "Saka"
        case .bengali: return "Bangabda"
        case .odia: return "Amli"
        case .malayalam: return "Kollam"
        }
    }

    private static let tamilMonths = [
        "", "Chithirai", "Vaikaasi", "Aani", "Aadi", "Aavani", "Purattaasi",
        "Aippasi", "Karthikai", "Maargazhi", "Thai", "Maasi", "Panguni"
    ]

    private static let bengaliMonths = [
        "", "Boishakh", "Joishtho", "Asharh", "Srabon", "Bhadro", "Ashshin",
        "Kartik", "Ogrohaeon", "Poush", "Magh", "Falgun", "Choitro"
    ]

    private static let odiaMonths = [
        "", "Baisakha", "Jyeshtha", "Ashadha", "Shravana", "Bhadrapada", "Ashvina",
        "Kartika", "Margashirsha", "Pausha", "Magha", "Phalguna", "Chaitra"
    ]

    private static let malayalamMonths = [
        "", "Chingam", "Kanni", "Thulam", "Vrishchikam", "Dhanu", "Makaram",
        "Kumbham", "Meenam", "Medam", "Edavam", "Mithunam", "Karkadakam"
    ]

    public func monthName(_ month: Int) -> String {
        guard month >= 1, month <= 12 else { return "???" }
        switch self {
        case .tamil: return Self.tamilMonths[month]
        case .bengali: return Self.bengaliMonths[month]
        case .odia: return Self.odiaMonths[month]
        case .malayalam: return Self.malayalamMonths[month]
        }
    }

    public static func fromString(_ name: String) -> SolarCalendarType? {
        switch name.lowercased() {
        case "tamil": return .tamil
        case "bengali": return .bengali
        case "odia": return .odia
        case "malayalam": return .malayalam
        default: return nil
        }
    }
}
