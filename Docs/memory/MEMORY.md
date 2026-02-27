# Hindu Calendar Project Memory

## Project Overview
- Hindu lunisolar calendar (panchang) implementation in C
- Must match drikpanchang.com output (Drik Siddhanta approach)
- Dual backend: self-contained Moshier (default, 1,265 lines) or Swiss Ephemeris (USE_SWISSEPH=1, 51K lines)
- Lahiri ayanamsa (SE_SIDM_LAHIRI = 1)
- NOT Surya Siddhanta (traditional method from Reingold/Dershowitz book)
- Initial scope: Tithi + Month (masa) + adhika months + Gregorian-to-Hindu conversion
- Amanta scheme (new moon to new moon) first, Purnimanta later

## Key References
- **Book**: Docs/calendrical-calculations.pdf, Chapter 20 (pages 548-606) - mathematical foundations
- **Lisp code**: github.com/EdReingold/calendar-code2/calendar.l - 72 Hindu calendar functions (Surya Siddhanta)
- **Python reference**: github.com/webresh/drik-panchanga - uses Swiss Ephemeris, same approach we're taking
- **Validation**: drikpanchang.com/panchang/month-panchang.html - HTML only, no API
- **Plan**: Docs/IMPLEMENTATION_PLAN.md

## Technical Decisions
- Default location: New Delhi (28.6139°N, 77.2090°E, UTC+5:30)
- Ephemeris: Self-contained Moshier (Meeus algorithms), Swiss Ephemeris optional via USE_SWISSEPH=1
- Build: Simple Makefile
- Tithi formula: floor(((lunar_long - solar_long) mod 360) / 12) + 1
- Month naming: Solar rashi at new moon → next rashi = month name
- New moon finding: Bisection on lunar phase

## Important Calendar Concepts
- Tithi: lunar day (1-30), determined by 12° segments of moon-sun elongation
- Paksha: Shukla (bright, 1-15) and Krishna (dark, 16-30/Amavasya)
- Adhika masa: leap month (two new moons in same solar sign, ~every 32.5 months)
- Kshaya masa: lost month (very rare, 19-141 year gap)
- Adhika tithi: repeated tithi (same tithi at two consecutive sunrises)
- Kshaya tithi: skipped tithi (~every 2 months)

## Phase Status
- [x] Phase 1: Project setup + Swiss Ephemeris integration
- [x] Phase 2: Tithi calculation + sunrise
- [x] Phase 3: Month (masa) determination
- [x] Phase 4: Full month panchang CLI
- [x] Phase 5: Validation against drikpanchang.com — 186 dates verified, 100% match
- [x] Phase 6: Validation web page for manual month-by-month comparison
- [x] Phase 7: Reingold diff overlay on validation web page
- [x] Phase 8: Hindu solar calendars — Bengali, Tamil, Odia, Malayalam
- [x] Phase 9: Self-contained Moshier ephemeris library (SE replacement)
- [x] Phase 10: VSOP87 solar longitude upgrade — 120→29 failures, all solar tests 100%
- [x] Phase 11: DE404 moon pipeline + sunrise improvements — 29→2 failures (99.996%)
- [x] Phase 13: Java 21 port — 227 tests, identical to C
- [x] Phase 14: Rust port — 275,396 assertions, identical to C
- [x] Phase 15: Drikpanchang.com full scrape validation — 55,117/55,152 tithi match (99.937%)
- [x] Phase 16: Solar calendar full scrape — Tamil 100%, Bengali 99.558% (8), Odia 100%, Malayalam 100%

## Validation Web Page
- `tools/csv_to_json.py --backend {se,moshier}` — converts ref CSV → 1,812 per-month JSON files
- Also loads `validation/reingold/reingold_1900_2050.csv` and embeds HL diffs into JSON
- `validation/web/index.html` — self-contained HTML page (inline CSS/JS, no deps)
- `validation/web/serve.sh` — no-cache Python HTTP server on port 8082
- Backend selector dropdown (SE | Moshier) in header, persists across navigation
- `validation/web/data/{se,moshier}/YYYY-MM.json` — 1,812 lunisolar files per backend
- Layout: transposed grid (7 rows = Sun-Sat, columns = weeks), matches drikpanchang.com
- Each cell: Gregorian day, tithi (S/K prefix + name), masa, Saka year
- Visual indicators: adhika tithi (blue), kshaya tithi (red), purnima/amavasya highlights, adhika masa tint
- Reingold diff overlay: orange background + red "R/D:" text for days where Reingold HL disagrees
- Toggle: "R/D overlay" checkbox in header (default on) shows/hides Reingold diffs
- Navigation: year/month dropdowns, prev/next buttons, arrow keys
- URL hash: `#backend/calendar/YYYY-MM` (e.g., `#se/lunisolar/2025-03`), backward compat with bare `#2025-03`
- Includes direct link to drikpanchang.com for same month for side-by-side comparison
- JSON includes pre-computed adhika_tithi/kshaya_tithi flags (cross-month boundaries handled)
- JSON includes hl_diff/hl_tithi/hl_masa/hl_adhika fields only when Reingold differs (89.2% match)
- `tools/extract_adhika_kshaya.py INPUT_CSV OUTPUT_CSV` — derives adhika/kshaya from ref CSV
- `tools/generate_all_validation.sh` — master script regenerating all data for both backends
- Validation CSV structure: `validation/{se,moshier}/ref_1900_2050.csv`, `adhika_kshaya_tithis.csv`, `solar/*.csv`
- Old dirs `validation/drikpanchang_data/` and `validation/solar/` removed (moved to `validation/se/`)
- Makefile targets: `gen-ref` (build generators + produce CSVs), `gen-json` (produce JSON from CSVs)
- Manual month-by-month validation superseded by full automated scrape (Phase 15)

## Solar Calendar Module
- `src/solar.h` + `src/solar.c` — 4 regional variants: Tamil, Bengali, Odia, Malayalam
- Sankranti finding: bisection on sidereal solar longitude (50 iterations, ~3ns precision)
- Critical time rules (which civil day "owns" a sankranti):
  - **Tamil**: sunset − 8.0 min (ayanamsa buffer) — splits 7.7–8.7 min danger zone, fixes 6 boundary dates
  - **Bengali**: midnight + 24min buffer + tithi-based rule (Sewell & Dikshit 1896): Karkata→before midnight, Makara→after midnight, others→tithi at Hindu sunrise determines assignment. Full scrape: 1,803/1,811 (99.558%), 8 irreducible mismatches
  - **Odia**: fixed 22:12 IST cutoff — 100/100 edge cases correct, no ayanamsa adjustment needed
  - **Malayalam**: end of madhyahna − 9.5 min (ayanamsa buffer) = sunrise + 3/5 × (sunset − sunrise) − 9.5 min — splits 9.3–10.0 min danger zone, fixes 15 boundary dates
- Era offsets (from Gregorian year):
  - Tamil/Odia (Saka): gy - 78 on/after Mesha, gy - 79 before
  - Bengali (Bangabda): gy - 593 on/after Mesha, gy - 594 before
  - Malayalam (Kollam): gy - 824 on/after Simha, gy - 825 before
- Malayalam unique: year starts at Simha (rashi 5), months rotated accordingly
- CLI: `./hindu-calendar -s tamil|bengali|odia|malayalam [-y Y] [-m M] [-d D]`
- 351 assertions in test_solar.c (35 Odia + 17 Malayalam boundary cases), all verified against drikpanchang.com

## Moshier Ephemeris Library
- `lib/moshier/` — ~2,000 lines total, replaces 51,493-line Swiss Ephemeris (26x reduction)
- `moshier_jd.c` (56 lines): Meeus Ch.7 JD↔Gregorian, day of week
- `moshier_sun.c` (726 lines): VSOP87 solar longitude (135 harmonic terms from SE), EMB→Earth correction, nutation (13 terms), obliquity, delta-T, RA/Dec helpers
- `moshier_moon.c` (~760 lines): Full DE404 Moshier moon pipeline (ported from SE swemmoon.c)
- `moshier_ayanamsa.c` (147 lines): IAU 1976 3D equatorial precession (matches SE Lahiri)
- `moshier_rise.c` (176 lines): Meeus Ch.15 iterative sunrise/sunset, Sinclair refraction, GAST
- Build: `make` = moshier (default), `make USE_SWISSEPH=1` = Swiss Ephemeris
- Precision vs SE: solar ±1" (VSOP87), ayanamsa ±0.3", sidereal ±0.5", lunar ±0.07" (DE404), sunrise ±2s
- All 1,071 drikpanchang.com validation assertions pass with both backends
- 53,143/53,143 tests pass with both backends
- Full drikpanchang.com scrape: Moshier 55,117/55,152 (99.937%), SE 55,115/55,152 (99.933%)
- SE has 2 extra mismatches vs drikpanchang: 1965-05-30, 2001-09-20 (drikpanchang confirms Moshier correct)
- All solar calendar tests 100% pass
- Key bugs found during development: transit formula sign (lon_east not +lon), Meeus correction `m += dm` not `m -= dm`
- SE Lahiri constants: t0=JD 2435553.5, ayan_t0=23.245524743°, prec=IAU_1976
- Day-of-week formula (matches SE): `(((int)floor(jd - 2433282 - 1.5) % 7) + 7) % 7`
- **CRITICAL**: `swe_get_ayanamsa_ut()` returns MEAN ayanamsa (WITHOUT nutation).
  Our `moshier_ayanamsa()` must NOT add nutation. Nutation cancels in sidereal position:
  sid = (trop + dpsi) - (ayan + dpsi) = trop - ayan. Adding nutation to ayanamsa causes
  ~17" oscillating error with 18.6-year period.
- VSOP87 implementation: ported from SE's swemplan.c/swemptab.h
  - 460 doubles (eartabl), 819 signed chars (earargs), 9 frequencies + 9 phases
  - Pipeline: VSOP87 J2000 → general precession IAU 1976 → EMB→Earth correction → geocentric → nutation → aberration(-20.496")
  - EMB→Earth correction: simplified 6-term Moon series from SE embofs_mosh(), first-order ecliptic approx
- Previous Meeus EoC approach (120 failures) had beneficial error cancellation:
  - Meeus p.164 combined formula (-0.00569° - 0.00478°×sin(Ω)) masked ~13" solar longitude error
  - With VSOP87, must use standard aberration (-20.496") — combined formula makes results worse
- DE404 moon pipeline (Phase 11): ported from SE swemmoon.c (longitude only, ~760 lines)
  - Pipeline: mean_elements → mean_elements_pl → moon1 → moon2 → moon3 → moon4
  - Data: z[26], LR[118×8], LRT[38×8], LRT2[25×6] tables, plus moon1/moon2 explicit terms
  - Helpers: mods3600, sscc (Chebyshev sin/cos multiples), chewm (table accumulation)
  - Variable light-time correction using 5 LR radius terms (distance-dependent)
  - SE delta-T yearly lookup table embedded (replaces Meeus polynomial for 1620-2025)
  - Precision: 0.018" RMS, 0.065" max vs SE (was 10" RMS with 60 Meeus terms)
- Sunrise improvements (Phase 11):
  - Sinclair refraction formula (matches SE's calc_astronomical_refr): h0=-0.612° vs old -0.5667°
  - Apparent sidereal time (GAST = GMST + Δψ×cos(ε)) vs old GMST
  - Midnight UT wrap-around fix for Delhi May sunrise (~00:00 UT)
  - Net: sunrise precision ~2s vs SE (was ~14s), fixed 27 of 29 failures
  - Previous "2 Moshier failures" (1965-05-30, 2001-09-20) were actually SE being wrong — drikpanchang.com full scrape confirms Moshier values
- Previous 118 SE lunar terms (just the main table, no moon1/moon2) gave WORSE results (179 vs 120 failures)
  - Full pipeline with ALL corrections (moon1/moon2 + z[] + LRT/LRT2) is essential for accuracy

## Implementation Notes
- Swiss Ephemeris vendored from github.com/aloistr/swisseph into lib/swisseph/
- Tithi uses **tropical** longitudes (ayanamsa cancels in moon-sun diff)
- Rashi uses **sidereal** (manually subtract ayanamsa from tropical)
- New moon finding: 17-point inverse Lagrange interpolation (matches Python ref)
- Sunrise: SE_BIT_DISC_CENTER (center of disc, with refraction) — matches webresh fork
- swe_day_of_week returns 0=Monday..6=Sunday
- JD is noon-based: add 0.5 to get midnight-based when converting to local time
- 53,143 assertions pass across 10 suites (186 external + regression + 351 solar + 327 solar validation + 1,200 solar edge)
- Bengali tithi-based rule: when sankranti in midnight zone, check tithi at previous day's sunrise — if tithi extends past sankranti → "before midnight" (day 1), else → "after midnight" (last day). Karkata always C, Makara always W. Source: Sewell & Dikshit "The Indian Calendar" (1896). Full investigation in Docs/BENGALI_INVESTIGATION.md
- Bengali diagnostic tools: `tools/bengali_tithi_rule.c`, `bengali_diag.c`, `bengali_analysis.c`, `bengali_eot.c`, `bengali_ayanamsa.c`, `bengali_shifted.c`, `bengali_weekday.c`
- 186 dates manually verified against drikpanchang.com: tithi, masa, adhika, saka all 100% match
- 132 of those are adhika/kshaya tithi edge cases (the hardest dates to get right)
- Full automated scrape: 55,152 days (1900-2050) verified against drikpanchang.com, 99.937% match
- Malayalam critical time is end of madhyahna = sunrise + 3/5 × daytime (NOT apparent noon)
- Discovered via 33 boundary cases verified against drikpanchang.com
- ~24 arcsecond Lahiri ayanamsa difference between SE_SIDM_LAHIRI and drikpanchang → ~8-10 min sankranti time offset
- Resolved in v0.3.2 by subtracting empirical buffers from critical times (Tamil −8.0 min, Malayalam −9.5 min)
- Edge case scanner: `tools/solar_boundary_scan.c` finds 100 closest sankrantis per calendar for verification
- Edge case tests: `tests/test_solar_edge.c` — 1,200 assertions (400 entries × 3 checks), 21 corrected from drikpanchang
- All verified data points saved in `validation/malayalam_boundary_cases.csv`
- drikpanchang.com festivals page shows sankramam dates, not month start dates (misleading for validation)
- Odia critical time is fixed 22:12 IST, NOT apparent-midnight-based — discovered via 23 boundary cases
- Initial hypothesis (5 ghati before apparent midnight) failed: cases at same before_am distance got different assignments
- IST time cleanly separates all cases: ≤22:11:33 = current day, ≥22:12:24 = next day
- Odia diagnostic tools: `tools/odia_diag.c`, `tools/odia_boundary.c`, `tools/odia_midnight_scan.c`, `tools/odia_nishita.c`
- Validation web page now supports solar calendars (Tamil, Bengali, Odia, Malayalam) via dropdown
- `tools/csv_to_solar_json.py --backend {se,moshier}` generates 7,248 solar JSON files per backend
- Solar JSON in `validation/web/data/{se,moshier}/{tamil,bengali,odia,malayalam}/YYYY-MM.json`

## Drikpanchang.com Full Scrape Validation (Phases 15-16)
- **Lunisolar**: 55,152 days (1900-2050), 99.937% match (35 boundary edge cases)
- **Solar**: Tamil 100%, Bengali 99.558% (8 irreducible), Odia 100%, Malayalam 100%
- CAPTCHA: hard 200-request per-IP limit; 2s delay works; VPN rotation required
- `scraper/lunisolar/` and `scraper/solar/` — fetch, parse, compare scripts
- Bengali 8 mismatches: midnight boundary cases, no rule change fixes all without regressions
- Disc center retained (h0=-0.612°); optimal h0=-0.817° gives 8 lunisolar mismatches but is overfitting
- Detailed notes: `memory/scrape_validation.md`, docs: `Docs/DRIKPANCHANG_VALIDATION.md`

## Rust Port
- `rust/` — Moshier-only, 2,784 production + 409 test lines, zero external deps
- 9 tests, 275,396 assertions, 0 failures, CLI output byte-identical to C
- Docs: `Docs/RUST_PORT.md`
