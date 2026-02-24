"""Solar calendar-specific constants, mappings, and paths."""

import os

# --- Paths ---
SOLAR_DIR = os.path.dirname(os.path.abspath(__file__))
SCRAPER_DIR = os.path.dirname(SOLAR_DIR)
DATA_DIR = os.path.join(SCRAPER_DIR, "data", "solar")
RAW_DIR = os.path.join(DATA_DIR, "raw")
PARSED_DIR = os.path.join(DATA_DIR, "parsed")
COMPARISON_DIR = os.path.join(DATA_DIR, "comparison")

# Reference CSVs from our C code
PROJECT_DIR = os.path.dirname(SCRAPER_DIR)
REF_DIR = os.path.join(PROJECT_DIR, "validation", "moshier", "solar")

# --- Fetch settings ---
DEFAULT_DELAY = 20
DEFAULT_START_YEAR = 1900
DEFAULT_END_YEAR = 2050

# --- Supported calendars ---
CALENDARS = ["tamil", "bengali", "odia", "malayalam"]

# --- URLs ---
# Each calendar's month panchang page on drikpanchang.com
SOLAR_URLS = {
    "tamil":     "https://www.drikpanchang.com/tamil/tamil-month-panchangam.html",
    "bengali":   "https://www.drikpanchang.com/bengali/bengali-month-panjika.html",
    "odia":      "https://www.drikpanchang.com/oriya/oriya-panji.html",
    "malayalam": "https://www.drikpanchang.com/malayalam/malayalam-month-calendar.html",
}

# --- Month name mappings ---
# Our reference CSVs use these month names (from src/solar.c).
# Drikpanchang may use slightly different spellings — the parser maps
# drikpanchang names to our canonical names for comparison.

TAMIL_MONTHS = {
    1: "Chithirai",    # Mesha
    2: "Vaikaasi",     # Vrishabha
    3: "Aani",         # Mithuna
    4: "Aadi",         # Karka
    5: "Aavani",       # Simha
    6: "Purattaasi",   # Kanya
    7: "Aippasi",      # Tula
    8: "Karthikai",    # Vrishchika
    9: "Maargazhi",    # Dhanu
    10: "Thai",        # Makara
    11: "Maasi",       # Kumbha
    12: "Panguni",     # Meena
}

BENGALI_MONTHS = {
    1: "Boishakh",
    2: "Joishtho",
    3: "Asharh",
    4: "Srabon",
    5: "Bhadro",
    6: "Ashshin",
    7: "Kartik",
    8: "Ogrohaeon",
    9: "Poush",
    10: "Magh",
    11: "Falgun",
    12: "Choitro",
}

ODIA_MONTHS = {
    1: "Baisakha",
    2: "Jyeshtha",
    3: "Ashadha",
    4: "Shravana",
    5: "Bhadrapada",
    6: "Ashvina",
    7: "Kartika",
    8: "Margashirsha",
    9: "Pausha",
    10: "Magha",
    11: "Phalguna",
    12: "Chaitra",
}

MALAYALAM_MONTHS = {
    1: "Chingam",      # Simha (year start)
    2: "Kanni",        # Kanya
    3: "Thulam",       # Tula
    4: "Vrishchikam",  # Vrishchika
    5: "Dhanu",        # Dhanu
    6: "Makaram",      # Makara
    7: "Kumbham",      # Kumbha
    8: "Meenam",       # Meena
    9: "Medam",        # Mesha
    10: "Edavam",      # Vrishabha
    11: "Mithunam",    # Mithuna
    12: "Karkadakam",  # Karka
}

MONTH_NAMES = {
    "tamil": TAMIL_MONTHS,
    "bengali": BENGALI_MONTHS,
    "odia": ODIA_MONTHS,
    "malayalam": MALAYALAM_MONTHS,
}

# Reverse mapping: canonical month name -> month number (per calendar)
MONTH_NAME_TO_NUM = {}
for cal, months in MONTH_NAMES.items():
    MONTH_NAME_TO_NUM[cal] = {name: num for num, name in months.items()}

# Drikpanchang name -> our canonical name, per calendar.
# Keyed by calendar type, then lowercase drikpanchang spelling -> canonical name.
# Add known spelling variants here as discovered during parsing.
_DRIK_ALIASES = {
    "tamil": {
        "chittirai": "Chithirai",
        "chithirai": "Chithirai",
        "vaigasi": "Vaikaasi",
        "vaikasi": "Vaikaasi",
        "vaikaasi": "Vaikaasi",
        "aani": "Aani",
        "aadi": "Aadi",
        "avani": "Aavani",
        "aavani": "Aavani",
        "purattasi": "Purattaasi",
        "purattaasi": "Purattaasi",
        "aippasi": "Aippasi",
        "karthigai": "Karthikai",
        "karthikai": "Karthikai",
        "margazhi": "Maargazhi",
        "maargazhi": "Maargazhi",
        "thai": "Thai",
        "maasi": "Maasi",
        "masi": "Maasi",
        "panguni": "Panguni",
    },
    "bengali": {
        "baishakh": "Boishakh",
        "boishakh": "Boishakh",
        "baisakh": "Boishakh",
        "jyaistha": "Joishtho",
        "joishtho": "Joishtho",
        "jyeshtha": "Joishtho",
        "ashadh": "Asharh",
        "asharh": "Asharh",
        "ashadha": "Asharh",
        "shravan": "Srabon",
        "srabon": "Srabon",
        "shravana": "Srabon",
        "shraban": "Srabon",
        "bhadra": "Bhadro",
        "bhadro": "Bhadro",
        "bhadrapada": "Bhadro",
        "ashwin": "Ashshin",
        "ashshin": "Ashshin",
        "ashvin": "Ashshin",
        "kartik": "Kartik",
        "agrahayana": "Ogrohaeon",
        "ogrohaeon": "Ogrohaeon",
        "agrahayan": "Ogrohaeon",
        "paush": "Poush",
        "poush": "Poush",
        "pausha": "Poush",
        "magh": "Magh",
        "magha": "Magh",
        "maagh": "Magh",
        "phalgun": "Falgun",
        "falgun": "Falgun",
        "phalguna": "Falgun",
        "chaitra": "Choitro",
        "choitro": "Choitro",
    },
    "odia": {
        "baisakha": "Baisakha",
        "baishakha": "Baisakha",
        "byisakha": "Baisakha",
        "jyeshtha": "Jyeshtha",
        "jyosta": "Jyeshtha",
        "ashadha": "Ashadha",
        "asadha": "Ashadha",
        "shravana": "Shravana",
        "srabana": "Shravana",
        "bhadrapada": "Bhadrapada",
        "bhadra": "Bhadrapada",
        "ashvina": "Ashvina",
        "aswin": "Ashvina",
        "kartika": "Kartika",
        "margashirsha": "Margashirsha",
        "margashira": "Margashirsha",
        "pausha": "Pausha",
        "magha": "Magha",
        "phalguna": "Phalguna",
        "phalgun": "Phalguna",
        "chaitra": "Chaitra",
        "chyatra": "Chaitra",
    },
    "malayalam": {
        "chingam": "Chingam",
        "kanni": "Kanni",
        "thulam": "Thulam",
        "vrischikam": "Vrishchikam",
        "vrishchikam": "Vrishchikam",
        "dhanu": "Dhanu",
        "makaram": "Makaram",
        "kumbham": "Kumbham",
        "meenam": "Meenam",
        "medam": "Medam",
        "edavam": "Edavam",
        "mithunam": "Mithunam",
        "karkidakam": "Karkadakam",
        "karkadakam": "Karkadakam",
    },
}


def normalize_month_name(drik_name, calendar_type):
    """Map a drikpanchang month name to our canonical month name.

    Tries exact match first, then case-insensitive per-calendar alias lookup.
    Returns the canonical name or None if unrecognized.
    """
    months = MONTH_NAMES[calendar_type]
    # Exact match against canonical names
    for num, name in months.items():
        if drik_name == name:
            return name
    # Case-insensitive alias (per-calendar)
    aliases = _DRIK_ALIASES.get(calendar_type, {})
    canonical = aliases.get(drik_name.lower())
    if canonical:
        return canonical
    return None


def raw_dir(calendar_type):
    """Return raw HTML directory for a given calendar."""
    return os.path.join(RAW_DIR, calendar_type)


def parsed_csv(calendar_type):
    """Return parsed CSV path for a given calendar."""
    return os.path.join(PARSED_DIR, f"{calendar_type}.csv")


def ref_csv(calendar_type):
    """Return reference CSV path for a given calendar."""
    return os.path.join(REF_DIR, f"{calendar_type}_months_1900_2050.csv")


def comparison_report(calendar_type):
    """Return comparison report path for a given calendar."""
    return os.path.join(COMPARISON_DIR, f"{calendar_type}_report.txt")
