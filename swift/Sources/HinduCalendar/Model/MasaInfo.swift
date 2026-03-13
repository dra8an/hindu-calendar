public struct MasaInfo {
    public let name: MasaName
    public let isAdhika: Bool
    public let yearSaka: Int
    public let yearVikram: Int
    public let jdStart: Double
    public let jdEnd: Double

    public init(name: MasaName, isAdhika: Bool, yearSaka: Int, yearVikram: Int,
                jdStart: Double, jdEnd: Double) {
        self.name = name
        self.isAdhika = isAdhika
        self.yearSaka = yearSaka
        self.yearVikram = yearVikram
        self.jdStart = jdStart
        self.jdEnd = jdEnd
    }
}
