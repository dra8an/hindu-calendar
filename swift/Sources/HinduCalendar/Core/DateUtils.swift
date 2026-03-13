import Foundation

public enum DateUtils {

    private static let dowNames = [
        "Monday", "Tuesday", "Wednesday", "Thursday",
        "Friday", "Saturday", "Sunday"
    ]

    private static let dowShort = [
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    ]

    private static let mdays = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

    public static func daysInMonth(year: Int, month: Int) -> Int {
        if month == 2 {
            if (year % 4 == 0 && year % 100 != 0) || year % 400 == 0 {
                return 29
            }
        }
        return mdays[month]
    }

    public static func dayOfWeekName(_ dow: Int) -> String {
        guard dow >= 0, dow <= 6 else { return "???" }
        return dowNames[dow]
    }

    public static func dayOfWeekShort(_ dow: Int) -> String {
        guard dow >= 0, dow <= 6 else { return "???" }
        return dowShort[dow]
    }
}
