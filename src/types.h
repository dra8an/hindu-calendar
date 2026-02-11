#ifndef TYPES_H
#define TYPES_H

typedef struct {
    double latitude;       /* degrees N */
    double longitude;      /* degrees E */
    double altitude;       /* meters */
    double utc_offset;     /* hours (e.g., 5.5 for IST) */
} Location;

/* Default location: New Delhi */
#define DEFAULT_LOCATION { 28.6139, 77.2090, 0.0, 5.5 }

typedef enum {
    SHUKLA_PAKSHA = 0,     /* Bright half (waxing, tithis 1-15) */
    KRISHNA_PAKSHA = 1     /* Dark half (waning, tithis 1-15) */
} Paksha;

typedef struct {
    int tithi_num;         /* 1-30 (1-15 Shukla, 16-30 Krishna) */
    Paksha paksha;
    int paksha_tithi;      /* 1-15 within the paksha */
    double jd_start;       /* Julian day when this tithi starts */
    double jd_end;         /* Julian day when this tithi ends */
    int is_kshaya;         /* 1 if tithi is skipped (no sunrise during it) */
} TithiInfo;

typedef enum {
    CHAITRA = 1,
    VAISHAKHA = 2,
    JYESHTHA = 3,
    ASHADHA = 4,
    SHRAVANA = 5,
    BHADRAPADA = 6,
    ASHVINA = 7,
    KARTIKA = 8,
    MARGASHIRSHA = 9,
    PAUSHA = 10,
    MAGHA = 11,
    PHALGUNA = 12
} MasaName;

typedef struct {
    MasaName name;         /* Month name (1-12) */
    int is_adhika;         /* 1 if this is a leap (adhika) month */
    int year_saka;         /* Saka era year */
    int year_vikram;       /* Vikram Samvat year */
    double jd_start;       /* Julian day of month start (new moon) */
    double jd_end;         /* Julian day of month end (next new moon) */
} MasaInfo;

typedef struct {
    int year_saka;         /* Saka era year */
    int year_vikram;       /* Vikram Samvat year */
    MasaName masa;         /* Month name */
    int is_adhika_masa;    /* Leap month flag */
    Paksha paksha;         /* Bright/dark half */
    int tithi;             /* Tithi number within paksha (1-15) */
    int is_adhika_tithi;   /* Leap day flag (same tithi as previous day) */
} HinduDate;

typedef struct {
    int greg_year, greg_month, greg_day;  /* Gregorian date */
    double jd_sunrise;                     /* Julian day of sunrise */
    HinduDate hindu_date;                  /* Hindu date at sunrise */
    TithiInfo tithi;                       /* Tithi details */
} PanchangDay;

/* Tithi names (Sanskrit) */
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

/* Solar calendar types */
typedef enum {
    SOLAR_CAL_TAMIL = 0,
    SOLAR_CAL_BENGALI,
    SOLAR_CAL_ODIA,
    SOLAR_CAL_MALAYALAM,
} SolarCalendarType;

typedef struct {
    int year;              /* Regional era year */
    int month;             /* 1-12 (regional month number) */
    int day;               /* Day within solar month (1-32) */
    int rashi;             /* Sidereal zodiac sign 1-12 at critical time */
    double jd_sankranti;   /* JD of the sankranti that started this month */
} SolarDate;

/* Month names */
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
