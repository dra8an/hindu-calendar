public struct Location {
    public let latitude: Double
    public let longitude: Double
    public let altitude: Double
    public let utcOffset: Double

    public init(latitude: Double, longitude: Double, altitude: Double, utcOffset: Double) {
        self.latitude = latitude
        self.longitude = longitude
        self.altitude = altitude
        self.utcOffset = utcOffset
    }

    public static let newDelhi = Location(latitude: 28.6139, longitude: 77.2090, altitude: 0.0, utcOffset: 5.5)
}
