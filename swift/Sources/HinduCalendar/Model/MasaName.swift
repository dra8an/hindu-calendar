public enum MasaName: Int, CaseIterable {
    case chaitra = 1
    case vaishakha = 2
    case jyeshtha = 3
    case ashadha = 4
    case shravana = 5
    case bhadrapada = 6
    case ashvina = 7
    case kartika = 8
    case margashirsha = 9
    case pausha = 10
    case magha = 11
    case phalguna = 12

    public var displayName: String {
        switch self {
        case .chaitra: return "Chaitra"
        case .vaishakha: return "Vaishakha"
        case .jyeshtha: return "Jyeshtha"
        case .ashadha: return "Ashadha"
        case .shravana: return "Shravana"
        case .bhadrapada: return "Bhadrapada"
        case .ashvina: return "Ashvina"
        case .kartika: return "Kartika"
        case .margashirsha: return "Margashirsha"
        case .pausha: return "Pausha"
        case .magha: return "Magha"
        case .phalguna: return "Phalguna"
        }
    }

    public static func fromNumber(_ num: Int) -> MasaName {
        guard let masa = MasaName(rawValue: num) else {
            fatalError("Invalid masa number: \(num)")
        }
        return masa
    }
}
