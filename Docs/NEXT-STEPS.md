# Next Steps

## Priority 1: Expanded Validation (Phase 5+6+7+8 — Largely Complete)

186 lunisolar dates verified against drikpanchang.com with 100% match. 30,854 solar calendar assertions across 4 regional variants. Validation web page enables manual month-by-month comparison across all 1,812 months (1900-2050). Reingold diff overlay highlights where Surya Siddhanta disagrees (89.2% match).

- [x] 186 dates verified against drikpanchang.com (tithi, masa, adhika, saka all 100%)
- [x] Validation web page for visual month-by-month comparison (`bash validation/web/serve.sh`)
- [x] Reingold/Dershowitz diff overlay on validation web page
- [x] Hindu solar calendars: Tamil, Bengali, Odia, Malayalam (351 unit + 327 validation + 28,976 regression + 1,200 edge case assertions)
- [x] Generate bulk regression CSV for solar calendars (4 CSVs, 1,811 months each, 1900-2050)
- [x] Malayalam critical time rule verified: end of madhyahna (3/5 of daytime) confirmed correct (see `Docs/MALAYALAM_NOON_FIX.md`)
- [x] Solar edge case scanner: 100 closest sankrantis per calendar scanned and verified against drikpanchang.com
- [x] Ayanamsa buffer adjustments: Tamil −8.0 min, Malayalam −9.5 min (fixes 6 + 15 boundary dates)
- [x] Validation web page extended for solar calendar comparison (dropdown for all 4 calendars)
- [x] Resolve Bengali solar calendar edge cases — implemented tithi-based rule from Sewell & Dikshit (1896): 36/37 verified edge cases correct (1 known failure: 1976-10-17)
- [x] Dual-backend validation data: SE + Moshier reference CSVs, JSON, web backend selector, `make gen-ref`/`gen-json` targets, `tools/generate_all_validation.sh` master script
- [ ] Continue manual validation of remaining lunisolar months via web page against drikpanchang.com
- [ ] Document any edge-case discrepancies (midnight new moons, polar regions)

## Priority 1.5: Self-Contained Ephemeris (Phase 9+10+11 — Complete)

Replaced the 51,493-line Swiss Ephemeris with a self-contained 1,265-line Moshier library as the default backend. Both backends remain available via compile-time selection.

- [x] Self-contained Moshier ephemeris library (`lib/moshier/`, 1,265 lines)
- [x] VSOP87 solar longitude (135-term harmonic summation, ±1″ vs SE)
- [x] ELP-2000/82 lunar longitude (60 Meeus Ch.47 terms, ±10″ vs SE)
- [x] Lahiri ayanamsa via IAU 1976 3D equatorial precession (±0.3″ vs SE)
- [x] Iterative sunrise/sunset (Meeus Ch.15, ±14 seconds vs SE)
- [x] Dual backend: `make` = moshier, `make USE_SWISSEPH=1` = SE
- [x] All solar calendar tests 100% pass with moshier (91 failures eliminated by VSOP87)
- [x] 53,114/53,143 (99.95%) pass with moshier; 29 remaining failures are tithi boundary edge cases from ~10″ lunar precision
- [ ] Improve lunar longitude precision to eliminate remaining 29 tithi boundary failures
- [ ] Calibrate Lahiri ayanamsa to match drikpanchang.com (close the ~24″ gap with SE_SIDM_LAHIRI), eliminating the need for Tamil (−8.0 min) and Malayalam (−9.5 min) empirical buffers

## Priority 2: Sunrise Accuracy

- [ ] Compare `SE_BIT_DISC_CENTER` (current, with refraction) vs `SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION` (geometric) against drikpanchang.com
- [ ] Try `SE_BIT_HINDU_RISING` flag (disc center + no refraction + geocentric)
- [ ] Optionally support Swiss Ephemeris data files (in `ephe/`) for higher precision
- [ ] Add sunrise/sunset unit tests with tighter tolerances

## Priority 3: Additional Panchang Elements

- [ ] **Nakshatra** (lunar mansion, 1-27): sidereal lunar longitude / (360/27)
- [ ] **Yoga** (1-27): (sidereal moon + sidereal sun) mod 360 / (360/27)
- [ ] **Karana** (1-60): half-tithi, lunar_phase / 6
- [ ] Add these to PanchangDay struct and CLI output

## Priority 4: Purnimanta Scheme

- [ ] Add Purnimanta month calculation (full-moon-to-full-moon boundaries)
- [ ] CLI flag to switch between Amanta and Purnimanta
- [ ] Purnimanta is used in North India; Amanta in South India

## Priority 5: Kshaya Masa Detection

- [ ] Detect when two solar sign transits occur between consecutive new moons
- [ ] Extremely rare (~19-141 year gap between occurrences)
- [ ] The month corresponding to the skipped rashi is "lost"
- [ ] Need historical test cases to validate

## Priority 6: Usability Improvements

- [ ] IANA timezone support (replace manual UTC offset with e.g. "Asia/Kolkata")
- [ ] Built-in city database (at least major Indian cities)
- [ ] Hindu-to-Gregorian reverse conversion
- [ ] Date range output (e.g., "show all Ekadashis in 2025")
- [ ] JSON output format for programmatic use
- [ ] Festival/observance calendar overlay

## Future Considerations

- Rashi (zodiac sign of moon) for daily horoscope-style output
- Muhurta (auspicious time windows)
- Eclipse calculations (Swiss Ephemeris supports this)
- Web interface or API wrapper
- Performance optimization (caching ephemeris calls for month generation)
