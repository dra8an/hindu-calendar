public struct PanchangDay {
    public let gregYear: Int
    public let gregMonth: Int
    public let gregDay: Int
    public let jdSunrise: Double
    public let hinduDate: HinduDate
    public let tithi: TithiInfo

    public init(gregYear: Int, gregMonth: Int, gregDay: Int, jdSunrise: Double,
                hinduDate: HinduDate, tithi: TithiInfo) {
        self.gregYear = gregYear
        self.gregMonth = gregMonth
        self.gregDay = gregDay
        self.jdSunrise = jdSunrise
        self.hinduDate = hinduDate
        self.tithi = tithi
    }
}
