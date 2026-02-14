# Changelog

## 0.3.2 — 2026-02-13

### Added

- **Solar edge case scanner** (`tools/solar_boundary_scan.c`): Scans all 7,248 sankrantis (1,812 per calendar × 4 calendars, 1900–2050) and finds the 100 closest to each calendar's critical time. Outputs CSV for manual verification + auto-generates test file
- **Solar edge case tests** (`tests/test_solar_edge.c`): 1,200 assertions (100 per calendar × 3 checks × 4 calendars) covering the most boundary-sensitive sankrantis. 21 entries corrected after manual verification against drikpanchang.com (6 Tamil + 15 Malayalam)
- **Edge correction helper** (`tools/edge_corrections.c`): Computes corrected expected values for wrong edge case entries
- Total test count: 53,143 assertions across 10 suites (up from 51,735 across 9)

### Fixed

- **Tamil critical time**: Adjusted sunset by −8.0 minutes to compensate for ~24 arcsecond Lahiri ayanamsa difference between Swiss Ephemeris SE_SIDM_LAHIRI and drikpanchang.com. Splits the empirically observed 7.7–8.7 min danger zone cleanly. Fixes 6 boundary dates (1900–2050)
- **Malayalam critical time**: Adjusted end-of-madhyahna by −9.5 minutes for the same ayanamsa reason. Splits the 9.3–10.0 min danger zone cleanly. Fixes 15 boundary dates (1900–2050)
- Regenerated solar reference CSVs and validation web page JSON files to reflect corrected boundaries

### Documentation

- Updated all documentation to reflect ayanamsa buffer adjustments, new test counts, and edge case findings
- Changed validation web server port from 8081 to 8082

## 0.3.1 — 2026-02-11

### Added

- **Malayalam critical time investigation**: Created diagnostic tool (`tools/malayalam_diag.c`) to test noon vs sunset vs midnight rules against all 12 months of 2025. Confirmed apparent noon rule is correct — all 5 edge cases match prokerala.com and drikpanchang.com daily panchangam
- **Expanded solar validation** (`test_solar_validation.c`): 327 assertions (up from 264), now covering all 12 Malayalam months of 2025 + Chingam 1 across 16 years (1950-2030)
- **Solar regression tests** (`test_solar_regression.c`): 28,976 assertions checking all 4 solar calendar CSVs (1,811 months each)
- Total test count: 51,735 assertions across 9 suites (up from 22,432 across 7)

### Fixed

- Corrected inaccurate comment in `src/solar.c`: Odia critical time rule was mislabeled as "sunrise of the next morning" — changed to "end of civil day (midnight)"

### Documentation

- Rewrote `Docs/MALAYALAM_NOON_FIX.md` from a fix plan to an investigation report documenting that the noon rule is correct
- Updated `Docs/PROJECT-STATUS.md` with current test counts (51,735) and expanded solar calendar validation details
- Updated `Docs/NEXT-STEPS.md`: marked solar regression CSVs and Malayalam verification as complete
- Updated `Docs/MASTER.md`: added missing doc entries (SOLAR_TESTING_PLAN, MALAYALAM_NOON_FIX, REINGOLD docs)

## 0.3.0 — 2026-02-11

### Added

- **Hindu solar calendars** (`solar.h`, `solar.c`): Four regional solar calendar variants — Tamil, Bengali, Odia, and Malayalam — all verified against drikpanchang.com
- **Sankranti finding**: Bisection-based algorithm (50 iterations, ~3ns precision) finds the exact moment the sun crosses a sidereal zodiac sign boundary
- **Regional critical time rules**: Each calendar uses a different rule for which civil day "owns" a sankranti — Tamil (sunset), Bengali (midnight + 24min buffer), Odia (end of civil day), Malayalam (apparent noon)
- **Regional eras**: Tamil/Odia (Saka), Bengali (Bangabda), Malayalam (Kollam) — each with correct year-start boundaries
- **Solar calendar CLI** (`-s TYPE`): `./hindu-calendar -s tamil|bengali|odia|malayalam` for month and single-day views
- **Solar test suite** (`test_solar.c`): 143 assertions across all four calendar variants, including month boundaries, year transitions, and roundtrip conversion tests
- **Solar validation suite** (`test_solar_validation.c`): Month-start dates verified against drikpanchang.com for all 4 calendars
- **Solar regression suite** (`test_solar_regression.c`): Reads generated CSVs (1900-2050) for all 4 calendars
- **Solar reference data generator** (`tools/gen_solar_ref.c`): Generates 4 CSVs with month boundaries (1,811 months each)
- Total test count: 22,432 assertions across 7 suites (up from 22,289 across 6)

## 0.2.0 — 2026-02-11

### Added

- **Validation web page** (`validation/web/index.html`): Browser-based month-by-month comparison tool covering all 1,812 months (1900-2050), with transposed calendar grid matching drikpanchang.com layout
- **Reingold diff overlay**: Orange-highlighted cells where Reingold/Dershowitz `hindu-lunar-from-fixed` (Surya Siddhanta) disagrees with our Drik Siddhanta values; 5,943 of 55,152 days differ (89.2% match rate)
- **R/D overlay toggle**: Checkbox in header to show/hide Reingold diffs
- **JSON data pipeline** (`tools/csv_to_json.py`): Converts `ref_1900_2050.csv` + `reingold_1900_2050.csv` into 1,812 per-month JSON files with embedded diff fields (`hl_diff`, `hl_tithi`, `hl_masa`, `hl_adhika`)
- **No-cache dev server** (`validation/web/serve.sh`): Python HTTP server on port 8082 with `Cache-Control: no-store` headers

## 0.1.0 — 2026-02-09

Initial implementation of the Hindu lunisolar calendar (panchang).

### Added

- **Swiss Ephemeris integration**: Vendored C source from github.com/aloistr/swisseph, using Moshier built-in ephemeris with Lahiri ayanamsa (SE_SIDM_LAHIRI)
- **Tithi calculation** (`tithi.c`): Lunar phase computation, tithi at sunrise, tithi boundary finding via bisection, kshaya (skipped) and adhika (repeated) tithi detection
- **Month determination** (`masa.c`): New moon finding via 17-point inverse Lagrange interpolation, solar rashi from sidereal longitude, Amanta month naming, adhika (leap) month detection, Saka and Vikram Samvat year calculation
- **Panchang generation** (`panchang.c`): Gregorian-to-Hindu date conversion, full month panchang generation, tabular and detailed single-day display
- **CLI** (`main.c`): Options for year (`-y`), month (`-m`), day (`-d`), location (`-l LAT,LON`), UTC offset (`-u`), defaults to current month in New Delhi
- **Astronomical wrapper** (`astro.c`): Tropical and sidereal solar/lunar longitude, ayanamsa, sunrise/sunset using disc-center method
- **Test suite**: 22,289 assertions across 6 suites, all passing
- **External validation**: 186 dates spanning 1900-2050 verified against drikpanchang.com — tithi, masa, adhika status, and Saka year all match (includes 132 adhika/kshaya tithi edge-case dates)
- **Regression tests**: 55,152-day CSV (1900-2050) generated by our calculator, with sampled regression tests and dedicated adhika/kshaya tithi coverage (4,269 edge-case days) to catch unintended code changes
- **Build system**: Makefile with `make`, `make test`, `make clean` targets
