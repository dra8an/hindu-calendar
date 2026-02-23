use std::fmt;

#[derive(Debug, Clone, Copy)]
pub struct Location {
    pub latitude: f64,
    pub longitude: f64,
    pub altitude: f64,
    pub utc_offset: f64,
}

impl Location {
    pub const NEW_DELHI: Location = Location {
        latitude: 28.6139,
        longitude: 77.2090,
        altitude: 0.0,
        utc_offset: 5.5,
    };
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Paksha {
    Shukla = 0,
    Krishna = 1,
}

impl fmt::Display for Paksha {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Paksha::Shukla => write!(f, "Shukla"),
            Paksha::Krishna => write!(f, "Krishna"),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum MasaName {
    Chaitra = 1,
    Vaishakha = 2,
    Jyeshtha = 3,
    Ashadha = 4,
    Shravana = 5,
    Bhadrapada = 6,
    Ashvina = 7,
    Kartika = 8,
    Margashirsha = 9,
    Pausha = 10,
    Magha = 11,
    Phalguna = 12,
}

impl MasaName {
    pub fn from_number(n: i32) -> MasaName {
        match n {
            1 => MasaName::Chaitra,
            2 => MasaName::Vaishakha,
            3 => MasaName::Jyeshtha,
            4 => MasaName::Ashadha,
            5 => MasaName::Shravana,
            6 => MasaName::Bhadrapada,
            7 => MasaName::Ashvina,
            8 => MasaName::Kartika,
            9 => MasaName::Margashirsha,
            10 => MasaName::Pausha,
            11 => MasaName::Magha,
            12 => MasaName::Phalguna,
            _ => MasaName::Chaitra,
        }
    }

    pub fn number(self) -> i32 {
        self as i32
    }

    pub fn display_name(self) -> &'static str {
        MASA_NAMES[self.number() as usize]
    }
}

impl fmt::Display for MasaName {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.display_name())
    }
}

pub const MASA_NAMES: [&str; 13] = [
    "",
    "Chaitra",
    "Vaishakha",
    "Jyeshtha",
    "Ashadha",
    "Shravana",
    "Bhadrapada",
    "Ashvina",
    "Kartika",
    "Margashirsha",
    "Pausha",
    "Magha",
    "Phalguna",
];

pub const TITHI_NAMES: [&str; 16] = [
    "",
    "Pratipada",
    "Dwitiya",
    "Tritiya",
    "Chaturthi",
    "Panchami",
    "Shashthi",
    "Saptami",
    "Ashtami",
    "Navami",
    "Dashami",
    "Ekadashi",
    "Dwadashi",
    "Trayodashi",
    "Chaturdashi",
    "Purnima",
];

#[derive(Debug, Clone, Copy)]
pub struct TithiInfo {
    pub tithi_num: i32,
    pub paksha: Paksha,
    pub paksha_tithi: i32,
    pub jd_start: f64,
    pub jd_end: f64,
    pub is_kshaya: bool,
}

#[derive(Debug, Clone, Copy)]
pub struct MasaInfo {
    pub name: MasaName,
    pub is_adhika: bool,
    pub year_saka: i32,
    pub year_vikram: i32,
    pub jd_start: f64,
    pub jd_end: f64,
}

#[derive(Debug, Clone, Copy)]
pub struct HinduDate {
    pub year_saka: i32,
    pub year_vikram: i32,
    pub masa: MasaName,
    pub is_adhika_masa: bool,
    pub paksha: Paksha,
    pub tithi: i32,
    pub is_adhika_tithi: bool,
}

#[derive(Debug, Clone, Copy)]
pub struct PanchangDay {
    pub greg_year: i32,
    pub greg_month: i32,
    pub greg_day: i32,
    pub jd_sunrise: f64,
    pub hindu_date: HinduDate,
    pub tithi: TithiInfo,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SolarCalendarType {
    Tamil,
    Bengali,
    Odia,
    Malayalam,
}

#[derive(Debug, Clone, Copy)]
pub struct SolarDate {
    pub year: i32,
    pub month: i32,
    pub day: i32,
    pub rashi: i32,
    pub jd_sankranti: f64,
}

pub const DOW_NAMES: [&str; 7] = [
    "Monday", "Tuesday", "Wednesday", "Thursday",
    "Friday", "Saturday", "Sunday",
];

pub const DOW_SHORT: [&str; 7] = [
    "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun",
];

pub const RASHI_NAMES: [&str; 13] = [
    "", "Mesha", "Vrishabha", "Mithuna", "Karka", "Simha", "Kanya",
    "Tula", "Vrishchika", "Dhanu", "Makara", "Kumbha", "Meena",
];
