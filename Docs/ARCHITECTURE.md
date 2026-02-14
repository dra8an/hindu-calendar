# Architecture

## Tech Stack

| Component | Technology | Notes |
|-----------|-----------|-------|
| Language | C99 | Chosen for performance and Swiss Ephemeris native compatibility |
| Ephemeris | Swiss Ephemeris (Moshier mode) | Vendored C source, ~34K lines, built-in planetary theory |
| Ayanamsa | Lahiri (SE_SIDM_LAHIRI) | Standard for Indian government calendar, matches drikpanchang.com |
| Build | GNU Make | Simple Makefile, no external dependencies beyond libc + libm |
| Tests | Custom C test harness | Assert macros, no test framework dependency |

## Directory Structure

```
hindu-calendar/
├── src/                    # Application source
│   ├── types.h             # Shared data structures and constants
│   ├── astro.h/.c          # Swiss Ephemeris wrapper
│   ├── date_utils.h/.c     # Julian Day / Gregorian helpers
│   ├── tithi.h/.c          # Tithi (lunar day) calculation
│   ├── masa.h/.c           # Month determination (lunisolar)
│   ├── solar.h/.c          # Solar calendar (Tamil, Bengali, Odia, Malayalam)
│   ├── panchang.h/.c       # High-level panchang and display
│   └── main.c              # CLI entry point
├── lib/swisseph/           # Vendored Swiss Ephemeris C source
├── tests/                  # Test suites
│   ├── test_astro.c
│   ├── test_tithi.c
│   ├── test_masa.c
│   ├── test_solar.c
│   ├── test_solar_validation.c
│   ├── test_solar_regression.c
│   ├── test_solar_edge.c
│   ├── test_validation.c
│   ├── test_csv_regression.c
│   └── test_adhika_kshaya.c
├── tools/                  # Utility programs
│   ├── generate_ref_data.c # Generate lunisolar reference CSV
│   ├── gen_solar_ref.c     # Generate solar calendar CSVs
│   ├── malayalam_diag.c    # Malayalam critical time diagnostic
│   ├── solar_boundary_scan.c  # Solar edge case scanner (100 closest per calendar)
│   ├── edge_corrections.c  # Compute corrected expected values for wrong edge cases
│   ├── csv_to_json.py      # Convert ref CSV + Reingold CSV → per-month JSON
│   └── csv_to_solar_json.py   # Convert solar CSVs → per-month JSON for web page
├── validation/             # Reference data from drikpanchang.com
├── ephe/                   # Swiss Ephemeris data files (optional)
├── Docs/                   # Documentation
├── build/                  # Build artifacts (gitignored)
├── Makefile
└── CHANGELOG.md
```

## Module Dependency Graph

```
main.c
  ├── panchang.h  (generate_month_panchang, print_month_panchang)
  │     ├── tithi.h  (tithi_at_sunrise)
  │     │     └── astro.h  (lunar/solar_longitude, sunrise_jd)
  │     │           └── Swiss Ephemeris (swe_calc_ut, swe_rise_trans)
  │     └── masa.h  (masa_for_date)
  │           ├── tithi.h
  │           └── astro.h  (solar_longitude_sidereal)
  ├── solar.h  (gregorian_to_solar, solar_to_gregorian)
  │     ├── astro.h  (solar_longitude_sidereal, sunrise_jd, sunset_jd)
  │     ├── masa.h   (solar_rashi)
  │     └── date_utils.h
  ├── astro.h  (astro_init, astro_close)
  └── date_utils.h  (gregorian_to_jd)
        └── Swiss Ephemeris (swe_julday, swe_revjul)
```

## Key Algorithms

### Tithi Calculation

Tithi is the fundamental unit of the Hindu lunar calendar. There are 30 tithis in a lunar month, each spanning 12 degrees of moon-sun elongation.

```
lunar_phase = (tropical_moon_longitude - tropical_sun_longitude) mod 360
tithi = floor(lunar_phase / 12) + 1
```

Tropical (Sayana) longitudes are used because the ayanamsa cancels out in the moon-sun difference. The tithi at sunrise determines the Hindu date for that civil day.

**Tithi boundary finding** uses bisection on the lunar phase function to find the exact Julian Day when the phase crosses a 12-degree boundary. 50 iterations give sub-second precision.

**Kshaya tithi** (skipped): When the tithi at tomorrow's sunrise is more than 1 ahead of today's. Occurs roughly every 2 months because the moon's angular velocity varies.

**Adhika tithi** (repeated): When the same tithi appears at two consecutive sunrises. The moon moves slowly enough that the tithi doesn't change overnight.

### New Moon Finding

New moons are found using **17-point inverse Lagrange interpolation**, matching the approach in the Python drik-panchanga reference implementation.

1. Estimate the new moon's position using the current tithi as a hint
2. Sample the lunar phase at 17 points in a ±2 day window (every 0.25 days)
3. Unwrap the phase angles to handle the 0°/360° discontinuity
4. Solve for phase = 360° using inverse Lagrange interpolation

This gives the Julian Day of the new moon to high precision in a single pass without iterative root-finding.

### Month (Masa) Determination

The Amanta (new-moon-to-new-moon) month naming algorithm:

1. Find the new moon **before** the date (start of current lunar month)
2. Find the new moon **after** the date (end of current lunar month)
3. Compute the **sidereal** solar longitude at the first new moon
4. The solar rashi (zodiac sign 1-12) determines the month name: `month = rashi + 1`
5. If the solar rashi is the same at both new moons, the sun did not change signs during the lunar month — this is an **adhika (leap) month**

Sidereal longitude is computed by subtracting the Lahiri ayanamsa (~24.2° in 2025) from the tropical longitude.

### Solar Rashi to Month Mapping

| Sidereal Solar Sign | Degrees | Month Name |
|---------------------|---------|------------|
| Mesha (Aries) | 0°-30° | Chaitra (1) |
| Vrishabha (Taurus) | 30°-60° | Vaishakha (2) |
| Mithuna (Gemini) | 60°-90° | Jyeshtha (3) |
| Karka (Cancer) | 90°-120° | Ashadha (4) |
| Simha (Leo) | 120°-150° | Shravana (5) |
| Kanya (Virgo) | 150°-180° | Bhadrapada (6) |
| Tula (Libra) | 180°-210° | Ashvina (7) |
| Vrishchika (Scorpio) | 210°-240° | Kartika (8) |
| Dhanu (Sagittarius) | 240°-270° | Margashirsha (9) |
| Makara (Capricorn) | 270°-300° | Pausha (10) |
| Kumbha (Aquarius) | 300°-330° | Magha (11) |
| Meena (Pisces) | 330°-360° | Phalguna (12) |

### Year Determination

Uses the Kali Ahargana method:

```
ahargana = jd - 588465.5          (days since Kali epoch)
kali_year = (ahargana + (4 - masa) * 30) / 365.25636
saka_year = kali_year - 3179
vikram_year = saka_year + 135
```

The Saka era began in 78 CE. Vikram Samvat began in 57 BCE. The Hindu new year starts with Chaitra Shukla Pratipada (first day of the bright half of Chaitra), typically in March-April.

### Sunrise Calculation

Uses Swiss Ephemeris `swe_rise_trans()` with `SE_BIT_DISC_CENTER` flag (center of solar disc at the horizon, with atmospheric refraction). The search starts from local midnight UT to find the next sunrise.

```
geopos = {longitude, latitude, altitude}
search_start = jd_midnight_ut - utc_offset/24    (convert local midnight to UT)
swe_rise_trans(search_start, SE_SUN, ..., SE_CALC_RISE | SE_BIT_DISC_CENTER, geopos, ...)
```

The returned Julian Day is in UT. To display local time: add utc_offset/24, then add 0.5 to convert from the noon-based JD epoch to midnight-based.

### Solar Calendar (Regional Variants)

The solar calendar module computes dates for four Indian regional solar traditions: Tamil, Bengali, Odia, and Malayalam. Unlike the lunisolar calendar, solar months are determined purely by the sun's position in the sidereal zodiac — no moon calculations needed.

**Solar month**: When the sun crosses a 30-degree sidereal zodiac sign boundary (sankranti), a new month begins. Days are numbered sequentially from the month start (1, 2, 3...). Solar months range from 29-32 days (the sun moves faster near perihelion in winter). There are always exactly 12 months per year, no leap months.

**Sankranti finding** uses bisection on the sidereal solar longitude, the same pattern as tithi boundary finding:

```
1. Estimate JD based on mean solar motion
2. Bracket: [estimate - 20, estimate + 20] days
3. Bisect 50 iterations on (sidereal_solar_longitude(mid) - target)
4. Handle 360°/0° wraparound in the angular difference
5. Precision: ~3 nanoseconds (40 days / 2^50)
```

**Critical time rules** determine which civil day "owns" a sankranti. This is the only significant difference between the four calendars:

| Calendar | Critical Time | Rule |
|----------|---------------|------|
| **Tamil** | Sunset − 8 min | If sankranti is before (sunset − 8 min), that day starts the new month |
| **Bengali** | Midnight + 24 min | Midnight IST with a 24-minute buffer into the next day (R/D edge-case zone) |
| **Odia** | 22:12 IST (fixed) | Sankranti ≤22:11 IST = current day, ≥22:12 IST = next day |
| **Malayalam** | End of madhyahna − 9.5 min | sunrise + 3/5 × (sunset − sunrise) minus 9.5 min buffer |

The Tamil and Malayalam buffers compensate for ~24 arcsecond difference between Swiss Ephemeris SE_SIDM_LAHIRI and drikpanchang.com's Lahiri ayanamsa (~8–10 min in sankranti time). Verified against 400 boundary cases (100 per calendar).

**Regional eras** are computed directly from the Gregorian year:

| Calendar | Era | On/after year start | Before year start | Year starts at |
|----------|-----|---------------------|-------------------|----------------|
| Tamil | Saka | gy - 78 | gy - 79 | Mesha (~Apr 14) |
| Bengali | Bangabda | gy - 593 | gy - 594 | Mesha (~Apr 14) |
| Odia | Saka | gy - 78 | gy - 79 | Mesha (~Apr 14) |
| Malayalam | Kollam | gy - 824 | gy - 825 | Simha (~Aug 17) |

**Malayalam** is unique: its year starts at Simha (Leo, rashi 5) instead of Mesha (Aries, rashi 1). Month numbering rotates so that Chingam (Simha) = month 1, Kanni (Kanya) = month 2, ..., Karkadakam (Karka) = month 12.

## Swiss Ephemeris

The Swiss Ephemeris is a high-precision astronomical calculation library developed by Astrodienst AG (astro.com). We use it in **Moshier mode** which provides built-in semi-analytical planetary theories without requiring external data files.

**Moshier mode accuracy**: ~1 arcsecond for the Sun, ~3 arcseconds for the Moon. This is more than sufficient for tithi calculation (each tithi spans 12 degrees = 43,200 arcseconds).

**Key functions used**:
- `swe_calc_ut()` — planetary longitude at a given Julian Day
- `swe_rise_trans()` — sunrise/sunset time for a given location
- `swe_get_ayanamsa_ut()` — Lahiri ayanamsa value
- `swe_julday()` / `swe_revjul()` — Julian Day conversions
- `swe_set_sid_mode()` — configure sidereal mode (Lahiri)

## Data Structures

See `src/types.h` for complete definitions. The core types:

- **Location**: latitude, longitude, altitude, UTC offset
- **TithiInfo**: tithi number (1-30), paksha, start/end JD, kshaya flag
- **MasaInfo**: month name, adhika flag, Saka/Vikram year, month boundaries
- **HinduDate**: complete Hindu date (year, month, paksha, tithi, adhika flags)
- **PanchangDay**: combines Gregorian date, sunrise, tithi, and Hindu date

## Reference Implementations

- **Python drik-panchanga** (github.com/webresh/drik-panchanga): Direct reference for our approach. Uses Swiss Ephemeris via pyswisseph. Our new moon finder and masa algorithm match this implementation.
- **Calendrical Calculations** (Reingold/Dershowitz): Mathematical foundations in Chapter 20. Uses Surya Siddhanta (traditional) rather than Drik Siddhanta (observational), so formulas differ, but concepts are the same.
- **drikpanchang.com**: Primary validation target. Uses Drik Siddhanta with Lahiri ayanamsa, same as our approach.
