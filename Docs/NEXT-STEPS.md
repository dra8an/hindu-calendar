# Next Steps

## Priority 1: Expanded Validation (Phase 5 Completion)

The current 26 reference dates provide good coverage but Phase 5 targets 12+ years of bulk validation.

- [ ] Scrape drikpanchang.com month panchang pages for years 2015-2027
- [ ] Store reference data as CSV in `validation/drikpanchang_data/`
- [ ] Expand test_validation.c to load and check CSV data automatically
- [ ] Target: every day across 12 years (4,380+ dates), 100% tithi match
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
