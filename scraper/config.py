"""Shared constants, mappings, and paths for the drikpanchang scraper."""

import os

# --- Paths ---
SCRAPER_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(SCRAPER_DIR, "data")
RAW_DIR = os.path.join(DATA_DIR, "raw")
RAW_MONTH_DIR = os.path.join(RAW_DIR, "month")
RAW_DAY_DIR = os.path.join(RAW_DIR, "day")
PARSED_DIR = os.path.join(DATA_DIR, "parsed")
PARSED_CSV = os.path.join(PARSED_DIR, "drikpanchang_lunisolar.csv")
COMPARISON_REPORT = os.path.join(DATA_DIR, "comparison_report.txt")

# Reference CSV from our C code
PROJECT_DIR = os.path.dirname(SCRAPER_DIR)
REF_CSV = os.path.join(PROJECT_DIR, "validation", "moshier", "ref_1900_2050.csv")

# --- URLs ---
BASE_URL_MONTH = "https://www.drikpanchang.com/panchang/month-panchang.html"
BASE_URL_DAY = "https://www.drikpanchang.com/panchang/day-panchang.html"

# --- Fetch settings ---
DEFAULT_DELAY = 20  # seconds between requests
DEFAULT_START_YEAR = 1900
DEFAULT_END_YEAR = 2050

# --- Cookies for Amanta scheme + New Delhi ---
COOKIES = {
    "drik-school-name": "amanta",
    "drik-geoname-id": "1261481",       # New Delhi
    "drik-language": "en",
    "drik-time-format": "12hour",
    "drik-ayanamsha-type": "chitra-paksha",  # Lahiri
}

HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/120.0.0.0 Safari/537.36"
    ),
}

# --- Tithi name → number mapping ---
# Shukla Pratipada = 1 ... Purnima = 15
# Krishna Pratipada = 16 ... Amavasya = 30
TITHI_NAMES_TO_NUM = {
    "Pratipada": 1,
    "Dwitiya": 2,
    "Tritiya": 3,
    "Chaturthi": 4,
    "Panchami": 5,
    "Shashthi": 6,
    "Saptami": 7,
    "Ashtami": 8,
    "Navami": 9,
    "Dashami": 10,
    "Ekadashi": 11,
    "Dwadashi": 12,
    "Trayodashi": 13,
    "Chaturdashi": 14,
    "Purnima": 15,
    "Amavasya": 30,  # Special: always 30 regardless of paksha
}

# --- Masa name → number mapping (Amanta) ---
MASA_NAMES_TO_NUM = {
    "Chaitra": 1,
    "Vaishakha": 2,
    "Jyeshtha": 3,
    "Ashadha": 4,
    "Shravana": 5,
    "Bhadrapada": 6,
    "Ashvina": 7,
    "Kartika": 8,
    "Margashirsha": 9,
    "Pausha": 10,
    "Magha": 11,
    "Phalguna": 12,
}

# Reverse mapping: number → name
MASA_NUM_TO_NAME = {v: k for k, v in MASA_NAMES_TO_NUM.items()}


def tithi_to_number(tithi_name, paksha):
    """Convert tithi name + paksha string to tithi number (1-30).

    Args:
        tithi_name: e.g. "Pratipada", "Amavasya", "Purnima"
        paksha: "Shukla" or "Krishna"

    Returns:
        int 1-30
    """
    base = TITHI_NAMES_TO_NUM.get(tithi_name)
    if base is None:
        raise ValueError(f"Unknown tithi name: {tithi_name!r}")

    # Amavasya is always 30, Purnima is always 15
    if tithi_name == "Amavasya":
        return 30
    if tithi_name == "Purnima":
        return 15

    # Shukla: 1-14, Krishna: 16-29
    if paksha == "Krishna":
        return base + 15
    return base
