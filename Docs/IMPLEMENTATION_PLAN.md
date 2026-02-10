# Hindu Lunisolar Calendar - Implementation Plan

## Context

We need a C implementation of a Hindu lunisolar calendar (panchang) that matches [drikpanchang.com](https://www.drikpanchang.com/panchang/month-panchang.html) output. The site uses modern astronomical calculations (Drik Siddhanta) with the Swiss Ephemeris and Lahiri ayanamsa, so we'll use the same approach rather than the traditional Surya Siddhanta method from the Reingold/Dershowitz book.

**Initial scope**: Tithi (lunar day) + lunar month names (masa) + adhika (leap) months + Gregorian-to-Hindu date conversion, following the Amanta (new moon to new moon) scheme.

---

## Project Structure

```
hindu-calendar/
├── src/
│   ├── main.c                # CLI entry point - month panchang display
│   ├── astro.h/.c            # Swiss Ephemeris wrapper (sun/moon longitude, sunrise)
│   ├── tithi.h/.c            # Tithi calculation (lunar day 1-30, paksha)
│   ├── masa.h/.c             # Month determination (adhika/kshaya detection)
│   ├── panchang.h/.c         # High-level panchang: date → Hindu date
│   ├── date_utils.h/.c       # Julian day ↔ Gregorian conversion helpers
│   └── types.h               # Shared data structures and constants
├── lib/
│   └── swisseph/             # Swiss Ephemeris C source (vendored)
├── ephe/                     # Ephemeris data files (optional, Moshier fallback)
├── tests/
│   ├── test_astro.c          # Test sun/moon longitude against known values
│   ├── test_tithi.c          # Test tithi calculation
│   ├── test_masa.c           # Test month determination + adhika months
│   └── test_validation.c     # Bulk validation against drikpanchang.com data
├── validation/
│   └── drikpanchang_data/    # Scraped reference data from drikpanchang.com
├── Docs/
│   └── calendrical-calculations.pdf
├── Makefile
└── CLAUDE.md
```

---

## Key Data Structures

```c
// types.h

typedef struct {
    double latitude;       // degrees N
    double longitude;      // degrees E
    double altitude;       // meters
    char timezone[64];     // IANA timezone (e.g., "Asia/Kolkata")
    double utc_offset;     // hours (e.g., 5.5 for IST)
} Location;

typedef enum {
    SHUKLA_PAKSHA = 0,     // Bright half (waxing, tithis 1-15)
    KRISHNA_PAKSHA = 1     // Dark half (waning, tithis 1-15, with 15=amavasya=30)
} Paksha;

typedef struct {
    int tithi_num;         // 1-30 (1-15 Shukla, 16-30 Krishna)
    Paksha paksha;
    int paksha_tithi;      // 1-15 within the paksha
    double jd_start;       // Julian day when this tithi starts
    double jd_end;         // Julian day when this tithi ends
    int is_kshaya;         // 1 if tithi is skipped (no sunrise during it)
} TithiInfo;

typedef enum {
    CHAITRA = 1, VAISHAKHA, JYESHTHA, ASHADHA,
    SHRAVANA, BHADRAPADA, ASHVINA, KARTIKA,
    MARGASHIRSHA, PAUSHA, MAGHA, PHALGUNA
} MasaName;

typedef struct {
    MasaName name;         // Month name (1-12)
    int is_adhika;         // 1 if this is a leap (adhika) month
    int year_saka;         // Saka era year
    int year_vikram;       // Vikram Samvat year
    double jd_start;       // Julian day of month start (new moon)
    double jd_end;         // Julian day of month end (next new moon)
} MasaInfo;

typedef struct {
    int year_saka;         // Saka era year
    int year_vikram;       // Vikram Samvat year
    MasaName masa;         // Month name
    int is_adhika_masa;    // Leap month flag
    Paksha paksha;         // Bright/dark half
    int tithi;             // Tithi number within paksha (1-15)
    int is_adhika_tithi;   // Leap day flag (same tithi as previous day)
} HinduDate;

typedef struct {
    int greg_year, greg_month, greg_day;  // Gregorian date
    double jd_sunrise;                     // Julian day of sunrise
    HinduDate hindu_date;                  // Hindu date at sunrise
    TithiInfo tithi;                       // Tithi details
    // Future: nakshatra, yoga, karana, rashi
} PanchangDay;
```

---

## Phases

### Phase 1: Project Setup + Swiss Ephemeris Integration

**Goal**: Get Swiss Ephemeris compiling and returning sun/moon longitudes.

1. **Download Swiss Ephemeris** C source from astro.com (or use the GitHub mirror)
   - Vendor it into `lib/swisseph/`
   - Key files: `swephexp.h`, `swedate.c`, `sweph.c`, `swecl.c`, `swephlib.c`, `swemplan.c`, `swemptab.c`

2. **Create build system** (`Makefile`)
   - Compile swisseph as a static library
   - Compile our src/ files and link against it
   - Start with Moshier ephemeris (built-in, no external files needed)

3. **Implement `astro.c`** - Swiss Ephemeris wrapper:
   ```c
   void astro_init(const char *ephe_path);  // swe_set_ephe_path + swe_set_sid_mode
   void astro_close(void);                   // swe_close
   double solar_longitude_sidereal(double jd_ut);   // swe_calc_ut(SE_SUN) - ayanamsa
   double lunar_longitude_sidereal(double jd_ut);   // swe_calc_ut(SE_MOON) - ayanamsa
   double get_ayanamsa(double jd_ut);               // swe_get_ayanamsa_ut(SE_SIDM_LAHIRI)
   double sunrise_jd(double jd_ut, Location *loc);  // swe_rise_trans
   double sunset_jd(double jd_ut, Location *loc);   // swe_rise_trans
   ```

4. **Implement `date_utils.c`**:
   ```c
   double gregorian_to_jd(int y, int m, int d);     // swe_julday
   void jd_to_gregorian(double jd, int *y, int *m, int *d);  // swe_revjul
   ```

5. **Write `test_astro.c`**: Verify solar/lunar longitudes match known values.

### Phase 2: Tithi Calculation + Sunrise

**Goal**: Given a Gregorian date and location, compute the tithi at sunrise.

1. **Implement `tithi.c`**:
   ```c
   // Lunar phase angle: (moon_long - sun_long) mod 360
   double lunar_phase(double jd_ut);

   // Tithi number (1-30) at a given moment
   int tithi_at_moment(double jd_ut);

   // Tithi at sunrise for a given Gregorian date
   TithiInfo tithi_at_sunrise(int year, int month, int day, Location *loc);

   // Find exact JD when a specific tithi starts/ends (using bisection)
   double find_tithi_boundary(double jd_start, double jd_end, int target_tithi);
   ```

2. **Key algorithm**:
   - Tithi = floor(lunar_phase / 12) + 1
   - lunar_phase = (lunar_long_sidereal - solar_long_sidereal) mod 360
   - Compute at sunrise to determine "current" tithi for that civil day
   - If tithi at today's sunrise == tithi at yesterday's sunrise → adhika (leap) tithi
   - If tithi at today's sunrise > tithi at yesterday's sunrise + 1 → kshaya (skipped) tithi

3. **Paksha mapping**:
   - Tithi 1-15 → Shukla Paksha (waxing, after new moon)
   - Tithi 16-30 → Krishna Paksha (waning, tithi 30 = Amavasya = new moon)
   - Display as: Shukla 1-15, Krishna 1-14, Amavasya (30)

### Phase 3: Month (Masa) Determination

**Goal**: Determine the Hindu lunar month name, detect adhika and kshaya months.

1. **Implement `masa.c`**:
   ```c
   // Find JD of new moon on or before a given JD (bisection on lunar_phase)
   double new_moon_before(double jd_ut);

   // Find JD of new moon on or after a given JD
   double new_moon_after(double jd_ut);

   // Determine solar rashi (zodiac sign 1-12) at a given moment
   int solar_rashi(double jd_ut);

   // Determine masa for a given date
   MasaInfo masa_for_date(int year, int month, int day, Location *loc);

   // Get Hindu year (Saka/Vikram) for a given date
   int hindu_year_saka(double jd_ut);
   int hindu_year_vikram(double jd_ut);
   ```

2. **Month naming algorithm (Amanta scheme)**:
   - Find the new moon **before** the given date's sunrise → month_start
   - Find the new moon **after** → month_end
   - Solar rashi at month_start determines the month name:
     - Rashi at new moon → next rashi name = month name
     - Mesha (Aries, 1) → Chaitra, Vrishabha (Taurus, 2) → Vaishakha, etc.
   - If solar rashi at month_start == solar rashi at month_end → **Adhika** (leap) month
   - If two rashi transitions occur between consecutive new moons → **Kshaya** month (extremely rare)

3. **Solar sign to month mapping**:
   | Solar Sign (entered during month) | Lunar Month Name |
   |---|---|
   | Mesha (0°-30°) | Chaitra |
   | Vrishabha (30°-60°) | Vaishakha |
   | Mithuna (60°-90°) | Jyeshtha |
   | Karka (90°-120°) | Ashadha |
   | Simha (120°-150°) | Shravana |
   | Kanya (150°-180°) | Bhadrapada |
   | Tula (180°-210°) | Ashvina |
   | Vrishchika (210°-240°) | Kartika |
   | Dhanu (240°-270°) | Margashirsha |
   | Makara (270°-300°) | Pausha |
   | Kumbha (300°-330°) | Magha |
   | Meena (330°-360°) | Phalguna |

4. **Year determination**:
   - Hindu New Year (Chaitra Shukla 1) starts the year
   - Saka year = Gregorian year - 78 (approximately)
   - Vikram Samvat = Gregorian year + 57 (approximately)

### Phase 4: Full Month Panchang Generation

**Goal**: Generate a month-panchang view matching drikpanchang.com format.

1. **Implement `panchang.c`**:
   ```c
   // Convert Gregorian date to Hindu date
   HinduDate gregorian_to_hindu(int year, int month, int day, Location *loc);

   // Generate panchang for every day in a Gregorian month
   void generate_month_panchang(int year, int month, Location *loc, PanchangDay *days, int *count);

   // Print month panchang in tabular format
   void print_month_panchang(PanchangDay *days, int count);
   ```

2. **Implement `main.c`** CLI:
   ```
   Usage: hindu-calendar [options]
     -y YEAR   Gregorian year (default: current)
     -m MONTH  Gregorian month (default: current)
     -d DAY    Specific day (if omitted, shows full month)
     -l LAT,LON  Location (default: New Delhi 28.6139,77.2090)
   ```

3. **Output format** (per day):
   ```
   Date       | Day | Sunrise  | Tithi                    | Hindu Date
   2025-01-15 | Wed | 07:12:34 | Purnima (Shukla 15)     | Pausha Purnima, Saka 1946
   2025-01-16 | Thu | 07:12:22 | Krishna Pratipada (K-1)  | Pausha Krishna 1, Saka 1946
   ```

### Phase 5: Validation Against drikpanchang.com

**Goal**: Verify our output matches the reference for 12+ years.

1. **Scrape reference data**: Use drikpanchang.com month panchang for years 2015-2027:
   - For each day: Gregorian date, tithi at sunrise, masa name, adhika status
   - Store as CSV in `validation/drikpanchang_data/`

2. **Implement `test_validation.c`**:
   - Read CSV reference data
   - Run our calculations for same dates
   - Compare tithi, month name, adhika status
   - Report mismatches with details
   - Target: 100% match for tithi, >99% for month boundaries (edge cases around midnight new moons)

3. **Known edge cases to test**:
   - Adhika months (occur roughly every 32.5 months)
   - Kshaya months (extremely rare, ~141 year gap)
   - Kshaya tithis (skipped tithis, ~every 2 months)
   - Adhika tithis (repeated tithis)
   - New moon exactly at sunrise
   - Year boundaries (Chaitra Shukla 1)

---

## Implementation Order

| Step | What | Files | Est. Complexity |
|------|-------|-------|-----------------|
| 1 | Vendor Swiss Ephemeris, create Makefile | lib/swisseph/, Makefile | Setup |
| 2 | astro.c wrapper + date_utils.c | src/astro.c, src/date_utils.c | Medium |
| 3 | Tithi calculation | src/tithi.c | Medium |
| 4 | Sunrise computation | src/astro.c (addition) | Low |
| 5 | New moon finding (bisection) | src/masa.c | Medium-High |
| 6 | Month determination + adhika | src/masa.c | High |
| 7 | Hindu date conversion | src/panchang.c | Medium |
| 8 | CLI + month display | src/main.c | Low |
| 9 | Scrape validation data | validation/ | Separate task |
| 10 | Validation test suite | tests/test_validation.c | Medium |

---

## Key Technical Decisions

1. **Ephemeris mode**: Start with Moshier (built-in, no files). Upgrade to Swiss Ephemeris files later for higher precision if needed.
2. **Location default**: New Delhi (28.6139°N, 77.2090°E, UTC+5:30) to match drikpanchang.com defaults.
3. **Ayanamsa**: Lahiri (SE_SIDM_LAHIRI = 1) - this is what drikpanchang.com uses.
4. **Calendar scheme**: Amanta (new moon to new moon) initially. Purnimanta variant later.
5. **New moon search**: Bisection method on lunar phase, similar to the Reingold/Dershowitz approach but using Swiss Ephemeris positions.
6. **Build**: Simple Makefile, no external dependencies beyond Swiss Ephemeris source.

## Verification

After each phase:
- **Phase 1**: Print solar/lunar longitude for known dates, compare with online ephemeris
- **Phase 2**: Print tithi for a known date, compare with drikpanchang.com
- **Phase 3**: Print month for several dates spanning a year, verify adhika months match
- **Phase 4**: Generate full month view, visual comparison with drikpanchang.com
- **Phase 5**: Automated bulk validation across 12+ years
