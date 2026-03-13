public struct SolarDate {
    public let year: Int           // Regional era year
    public let month: Int          // 1-12 (regional month number)
    public let day: Int            // Day within solar month (1-32)
    public let rashi: Int          // Sidereal zodiac sign 1-12 at critical time
    public let jdSankranti: Double // JD of the sankranti that started this month

    public init(year: Int, month: Int, day: Int, rashi: Int, jdSankranti: Double) {
        self.year = year
        self.month = month
        self.day = day
        self.rashi = rashi
        self.jdSankranti = jdSankranti
    }
}
