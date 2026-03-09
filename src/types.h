/*
 * types.h - Core data types for the Hindu calendar library
 *
 * Shared type definitions used across all modules: lunisolar calendar
 * (tithi, masa, panchang), solar calendars (Tamil, Bengali, Odia,
 * Malayalam), and the astronomical backend.
 *
 * Calendar conventions:
 *   - Lunisolar: Amanta (new-moon-to-new-moon) scheme
 *   - Ayanamsa:  Lahiri (Chitrapaksha)
 *   - Ephemeris: Drik Siddhanta (modern astronomical), NOT Surya Siddhanta
 *   - Era:       Saka (lunisolar), regional eras for solar calendars
 *
 * Julian Day (JD) is used throughout as the internal time representation.
 * JD is noon-based: add 0.5 to get midnight-based when converting to local
 * time.  All JD values are in Universal Time (UT).
 */
#ifndef TYPES_H
#define TYPES_H

/* ---------------------------------------------------------------------------
 * Location
 * ---------------------------------------------------------------------------
 * Geographic position and timezone for all location-dependent calculations
 * (sunrise, sunset, critical times for solar calendar boundaries).
 *
 * IMPORTANT: All four fields must be set.  Using a three-field initializer
 * like {lat, lon, utc_offset} silently sets altitude = utc_offset and
 * utc_offset = 0, producing wrong results.
 *
 * Example:
 *   Location delhi = DEFAULT_LOCATION;                    // New Delhi
 *   Location nyc   = { 40.7128, -74.0060, 0.0, -5.0 };   // NYC (EST)
 */
typedef struct {
    double latitude;       /* degrees N (negative for S) */
    double longitude;      /* degrees E (negative for W) */
    double altitude;       /* meters above sea level */
    double utc_offset;     /* hours east of UTC (e.g., 5.5 for IST, -5.0 for EST) */
} Location;

/* Default location: New Delhi (28.6139 N, 77.2090 E, IST = UTC+5:30) */
#define DEFAULT_LOCATION { 28.6139, 77.2090, 0.0, 5.5 }

/* ---------------------------------------------------------------------------
 * Paksha - Lunar fortnight
 * ---------------------------------------------------------------------------
 * Each lunisolar month is divided into two halves (pakshas):
 *   SHUKLA (bright/waxing): new moon -> full moon, tithis 1-15
 *   KRISHNA (dark/waning):  full moon -> new moon, tithis 1-15 (or 16-30)
 */
typedef enum {
    SHUKLA_PAKSHA = 0,     /* Bright half (waxing, tithis 1-15) */
    KRISHNA_PAKSHA = 1     /* Dark half (waning, tithis 1-15) */
} Paksha;

/* ---------------------------------------------------------------------------
 * TithiInfo - Lunar day details
 * ---------------------------------------------------------------------------
 * A tithi is one of 30 lunar days in a lunisolar month, each spanning
 * exactly 12 degrees of moon-sun elongation.  A tithi is not tied to
 * sunrise — it can start and end at any time.  The tithi "at" a civil
 * day is the one prevailing at sunrise.
 *
 * Special cases:
 *   - Adhika (extra) tithi: same tithi at two consecutive sunrises
 *   - Kshaya (lost) tithi:  tithi that passes entirely between two
 *     consecutive sunrises, so no civil day "owns" it (~every 2 months)
 */
typedef struct {
    int tithi_num;         /* 1-30 (1-15 Shukla, 16-30 Krishna) */
    Paksha paksha;         /* Which half of the month */
    int paksha_tithi;      /* 1-15 within the paksha */
    double jd_start;       /* JD (UT) when this tithi begins */
    double jd_end;         /* JD (UT) when this tithi ends */
    int is_kshaya;         /* 1 if the *next* tithi is skipped (kshaya) */
} TithiInfo;

/* ---------------------------------------------------------------------------
 * MasaName - Lunisolar month names
 * ---------------------------------------------------------------------------
 * The 12 months of the Hindu lunisolar calendar, beginning with Chaitra
 * (March/April).  Month numbering follows the standard North Indian
 * (Amanta) convention.
 */
typedef enum {
    CHAITRA = 1,           /* Mar-Apr */
    VAISHAKHA = 2,         /* Apr-May */
    JYESHTHA = 3,          /* May-Jun */
    ASHADHA = 4,           /* Jun-Jul */
    SHRAVANA = 5,          /* Jul-Aug */
    BHADRAPADA = 6,        /* Aug-Sep */
    ASHVINA = 7,           /* Sep-Oct */
    KARTIKA = 8,           /* Oct-Nov */
    MARGASHIRSHA = 9,      /* Nov-Dec */
    PAUSHA = 10,           /* Dec-Jan */
    MAGHA = 11,            /* Jan-Feb */
    PHALGUNA = 12          /* Feb-Mar */
} MasaName;

/* ---------------------------------------------------------------------------
 * MasaInfo - Lunisolar month details
 * ---------------------------------------------------------------------------
 * Returned by masa_for_date() and related functions.
 *
 * An adhika (intercalary/leap) month occurs when two consecutive new moons
 * fall in the same solar rashi (~every 32.5 months).  The adhika month
 * precedes its nija (regular) counterpart and shares the same name.
 *
 * jd_start and jd_end are the new moon JDs bracketing this month (not the
 * first/last civil days — use lunisolar_month_start() for civil dates).
 */
typedef struct {
    MasaName name;         /* Month name (1-12) */
    int is_adhika;         /* 1 if this is a leap (adhika/intercalary) month */
    int year_saka;         /* Saka era year */
    int year_vikram;       /* Vikram Samvat year (= saka + 135) */
    double jd_start;       /* JD (UT) of the new moon that starts this month */
    double jd_end;         /* JD (UT) of the new moon that ends this month */
} MasaInfo;

/* ---------------------------------------------------------------------------
 * HinduDate - Complete Hindu date at sunrise
 * ---------------------------------------------------------------------------
 * The full Hindu calendar date for a given civil day, determined at sunrise.
 *
 * An adhika tithi means the same tithi prevails at sunrise on two
 * consecutive civil days (the second is the adhika).
 */
typedef struct {
    int year_saka;         /* Saka era year */
    int year_vikram;       /* Vikram Samvat year */
    MasaName masa;         /* Lunisolar month name */
    int is_adhika_masa;    /* 1 if this is a leap month */
    Paksha paksha;         /* Bright (Shukla) or dark (Krishna) half */
    int tithi;             /* Tithi number within paksha (1-15) */
    int is_adhika_tithi;   /* 1 if same tithi as previous civil day */
} HinduDate;

/* ---------------------------------------------------------------------------
 * PanchangDay - Daily panchang entry
 * ---------------------------------------------------------------------------
 * One row of a monthly panchang table, combining the Gregorian date,
 * sunrise time, and the full Hindu date with tithi details.
 */
typedef struct {
    int greg_year, greg_month, greg_day;  /* Gregorian date */
    double jd_sunrise;                     /* JD (UT) of sunrise */
    HinduDate hindu_date;                  /* Hindu date at sunrise */
    TithiInfo tithi;                       /* Tithi details (boundaries, kshaya) */
} PanchangDay;

/* ---------------------------------------------------------------------------
 * Name lookup tables
 * ---------------------------------------------------------------------------
 * TITHI_NAMES[1..15] - Sanskrit tithi names within a paksha.
 *   Index 15 = "Purnima" (full moon, end of Shukla paksha).
 *   For Krishna paksha, index 15 corresponds to "Amavasya" (new moon)
 *   but the table does not include that name — use tithi_num == 30
 *   to detect Amavasya.
 *
 * MASA_NAMES[1..12] - Lunisolar month names.
 *   Indexed by MasaName enum value.
 */
static const char *TITHI_NAMES[] = {
    "",             /* 0 - unused */
    "Pratipada",    /* 1 */
    "Dwitiya",      /* 2 */
    "Tritiya",      /* 3 */
    "Chaturthi",    /* 4 */
    "Panchami",     /* 5 */
    "Shashthi",     /* 6 */
    "Saptami",      /* 7 */
    "Ashtami",      /* 8 */
    "Navami",       /* 9 */
    "Dashami",      /* 10 */
    "Ekadashi",     /* 11 */
    "Dwadashi",     /* 12 */
    "Trayodashi",   /* 13 */
    "Chaturdashi",  /* 14 */
    "Purnima",      /* 15 - full moon (end of Shukla) */
};

/* ---------------------------------------------------------------------------
 * SolarCalendarType - Regional solar calendar variants
 * ---------------------------------------------------------------------------
 * Four supported South/East Indian solar calendars.  Each has its own
 * era, month names, year-start month, and critical-time rule for
 * determining which civil day "owns" a sankranti.
 *
 * Critical time rules (when a sankranti falls near a day boundary):
 *   Tamil:     sunset - 9.5 min (upper limb)
 *   Bengali:   midnight + 24 min buffer + tithi-based rule (Sewell & Dikshit)
 *   Odia:      fixed 22:12 IST cutoff
 *   Malayalam:  end of madhyahna - 9.5 min (sunrise + 3/5 * daytime)
 */
typedef enum {
    SOLAR_CAL_TAMIL = 0,
    SOLAR_CAL_BENGALI,
    SOLAR_CAL_ODIA,
    SOLAR_CAL_MALAYALAM,
} SolarCalendarType;

/* ---------------------------------------------------------------------------
 * SolarDate - Regional solar calendar date
 * ---------------------------------------------------------------------------
 * Returned by gregorian_to_solar().  The year uses the regional era
 * (Saka for Tamil, Bangabda for Bengali, Amli for Odia, Kollam for
 * Malayalam).
 */
typedef struct {
    int year;              /* Regional era year */
    int month;             /* 1-12 (regional month number) */
    int day;               /* Day within solar month (1-32) */
    int rashi;             /* Sidereal zodiac sign 1-12 at critical time */
    double jd_sankranti;   /* JD (UT) of the sankranti that started this month */
} SolarDate;

static const char *MASA_NAMES[] = {
    "",              /* 0 - unused */
    "Chaitra",       /* 1 */
    "Vaishakha",     /* 2 */
    "Jyeshtha",      /* 3 */
    "Ashadha",       /* 4 */
    "Shravana",      /* 5 */
    "Bhadrapada",    /* 6 */
    "Ashvina",       /* 7 */
    "Kartika",       /* 8 */
    "Margashirsha",  /* 9 */
    "Pausha",        /* 10 */
    "Magha",         /* 11 */
    "Phalguna",      /* 12 */
};

#endif /* TYPES_H */
