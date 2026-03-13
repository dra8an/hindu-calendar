public struct HinduDate {
    public let yearSaka: Int
    public let yearVikram: Int
    public let masa: MasaName
    public let isAdhikaMasa: Bool
    public let paksha: Paksha
    public let tithi: Int            // Tithi number within paksha (1-15)
    public let isAdhikaTithi: Bool   // Same tithi as previous day

    public init(yearSaka: Int, yearVikram: Int, masa: MasaName, isAdhikaMasa: Bool,
                paksha: Paksha, tithi: Int, isAdhikaTithi: Bool) {
        self.yearSaka = yearSaka
        self.yearVikram = yearVikram
        self.masa = masa
        self.isAdhikaMasa = isAdhikaMasa
        self.paksha = paksha
        self.tithi = tithi
        self.isAdhikaTithi = isAdhikaTithi
    }
}
