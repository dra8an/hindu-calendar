# Project Status

## Current Version: 0.1.0 (Initial Implementation)

## Phase Completion

| Phase | Description | Status | Notes |
|-------|-------------|--------|-------|
| 1 | Project setup + Swiss Ephemeris integration | Done | Vendored from github.com/aloistr/swisseph |
| 2 | Tithi calculation + sunrise | Done | Bisection-based boundary finding |
| 3 | Month (masa) determination | Done | Amanta scheme, adhika detection working |
| 4 | Full month panchang CLI | Done | Single-day and full-month views |
| 5 | Validation against drikpanchang.com | Done | 54 dates spot-checked + 55K-day CSV regression |

## Test Results

21,761 assertions passing across 6 test suites:

| Suite | Assertions | Coverage |
|-------|------------|----------|
| test_astro | 11 | JD conversion, day-of-week, solar longitude, ayanamsa, sunrise |
| test_tithi | 18 | Tithi at known dates, kshaya/adhika detection, lunar phase |
| test_masa | 24 | Month name, adhika months, solar rashi, Saka/Vikram years |
| test_validation | 216 | 54 drikpanchang-verified dates x 4 checks (tithi, masa, adhika, saka) |
| test_csv_regression | 4,416 | 1,104 sampled days from 55K-day CSV (1900-2050) |
| test_adhika_kshaya | 17,076 | All 4,269 adhika/kshaya tithi days (1900-2050) x 4 checks |

## Validated Against drikpanchang.com

54 dates spanning 1950-2045 spot-checked with confirmed matches, plus 55,152-day CSV (1900-2050):

- Tithi at sunrise: 100% match
- Month (masa) name: 100% match
- Adhika month detection: 100% match (tested 2004 Shravana, 2007 Jyeshtha, 2012 Bhadrapada, 2015 Ashadha, 2018 Jyeshtha, 2020 Ashvina, 2023 Shravana)
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
- Test code: ~600 lines across 6 files
- Vendored Swiss Ephemeris: ~34,600 lines (11 .c + 12 .h)
