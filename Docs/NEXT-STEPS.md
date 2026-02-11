# Next Steps

## Priority 1: Expanded Validation (Phase 5+6+7+8 — Largely Complete)

186 lunisolar dates verified against drikpanchang.com with 100% match. 143 solar calendar assertions verified across 4 regional variants. Validation web page enables manual month-by-month comparison across all 1,812 months (1900-2050). Reingold diff overlay highlights where Surya Siddhanta disagrees (89.2% match).

- [x] 186 dates verified against drikpanchang.com (tithi, masa, adhika, saka all 100%)
- [x] Validation web page for visual month-by-month comparison (`bash validation/web/serve.sh`)
- [x] Reingold/Dershowitz diff overlay on validation web page
- [x] Hindu solar calendars: Tamil, Bengali, Odia, Malayalam (143 unit + 327 validation + 28,976 regression assertions)
- [x] Generate bulk regression CSV for solar calendars (4 CSVs, 1,811 months each, 1900-2050)
- [x] Malayalam critical time rule verified: apparent noon confirmed correct (see `Docs/MALAYALAM_NOON_FIX.md`)
- [ ] Continue manual validation of remaining lunisolar months via web page against drikpanchang.com
- [ ] Extend validation web page for solar calendar comparison (or create separate page)
- [ ] Document any edge-case discrepancies (midnight new moons, polar regions)

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
