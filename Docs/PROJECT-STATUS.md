# Project Status

## Current Version: 0.1.0 (Initial Implementation)

## Phase Completion

| Phase | Description | Status | Notes |
|-------|-------------|--------|-------|
| 1 | Project setup + Swiss Ephemeris integration | Done | Vendored from github.com/aloistr/swisseph |
| 2 | Tithi calculation + sunrise | Done | Bisection-based boundary finding |
| 3 | Month (masa) determination | Done | Amanta scheme, adhika detection working |
| 4 | Full month panchang CLI | Done | Single-day and full-month views |
| 5 | Validation against drikpanchang.com | Partial | 26 dates across 2012-2025 verified |

## Test Results

157 tests passing across 4 test suites:

| Suite | Tests | Coverage |
|-------|-------|----------|
| test_astro | 11 | JD conversion, day-of-week, solar longitude, ayanamsa, sunrise |
| test_tithi | 18 | Tithi at known dates, kshaya/adhika detection, lunar phase |
| test_masa | 24 | Month name, adhika months, solar rashi, Saka/Vikram years |
| test_validation | 104 | 26 reference dates x 4 checks (tithi, masa, adhika, saka year) |

## Validated Against drikpanchang.com

Dates spanning 2012-2025 with confirmed matches:

- Tithi at sunrise: 100% match
- Month (masa) name: 100% match
- Adhika month detection: 100% match (tested 2012 Adhika Bhadrapada, 2015 Adhika Ashadha, 2023 Adhika Shravana)
- Saka/Vikram year: 100% match
- Sunrise time: within ~1 minute of drikpanchang.com (Moshier ephemeris vs Swiss files)

## Known Limitations

- Uses Moshier ephemeris (built-in, ~1 arcminute precision) instead of Swiss Ephemeris data files (~0.001 arcsecond)
- Amanta scheme only (no Purnimanta support)
- No nakshatra, yoga, or karana calculations
- No kshaya masa detection (extremely rare edge case)
- UTC offset is manual (no IANA timezone / DST support)
- Location defaults to New Delhi; no city database

## Source Statistics

- Application code: ~880 lines across 12 files (6 .c + 6 .h)
- Test code: ~480 lines across 4 files
- Vendored Swiss Ephemeris: ~34,600 lines (11 .c + 12 .h)
