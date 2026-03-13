public struct TithiInfo {
    public let tithiNum: Int      // 1-30 (1-15 Shukla, 16-30 Krishna)
    public let paksha: Paksha
    public let pakshaTithi: Int   // 1-15 within the paksha
    public let jdStart: Double    // Julian day when this tithi starts
    public let jdEnd: Double      // Julian day when this tithi ends
    public let isKshaya: Bool     // true if next tithi is skipped

    public init(tithiNum: Int, paksha: Paksha, pakshaTithi: Int,
                jdStart: Double, jdEnd: Double, isKshaya: Bool) {
        self.tithiNum = tithiNum
        self.paksha = paksha
        self.pakshaTithi = pakshaTithi
        self.jdStart = jdStart
        self.jdEnd = jdEnd
        self.isKshaya = isKshaya
    }

    public static let tithiNames: [String] = [
        "",             // 0 - unused
        "Pratipada",    // 1
        "Dwitiya",      // 2
        "Tritiya",      // 3
        "Chaturthi",    // 4
        "Panchami",     // 5
        "Shashthi",     // 6
        "Saptami",      // 7
        "Ashtami",      // 8
        "Navami",       // 9
        "Dashami",      // 10
        "Ekadashi",     // 11
        "Dwadashi",     // 12
        "Trayodashi",   // 13
        "Chaturdashi",  // 14
        "Purnima",      // 15
    ]
}
