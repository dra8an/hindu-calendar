# Plan: Create Comprehensive Hindu Calendar Guide Document

## Context

We have built a complete Hindu calendar system (lunisolar panchang + 4 regional solar calendars) in C, validated against drikpanchang.com with 51,943+ assertions. Along the way we discovered critical edge cases (Odia 22:12 IST cutoff, Malayalam 3/5-of-daytime rule, Bengali midnight buffer, etc.) that are not documented anywhere in a single place. The user wants a comprehensive, language-agnostic guide that captures all of this knowledge — astronomical background, algorithms in pseudocode, per-calendar step-by-step conversion procedures, and all gotchas — so that anyone could reimplement these calendars from scratch.

**Output file:** `Docs/HINDU_CALENDAR_GUIDE.md`

## Document Structure

The document will have 7 major sections plus appendices, estimated ~2,500-3,000 lines.

### Section 1: Introduction (~80 lines)
- What this guide covers: Hindu lunisolar (panchang) + 4 regional solar calendars
- Drik Siddhanta vs Surya Siddhanta — why modern ephemeris-based approach
- Prerequisites: a planetary ephemeris library (Swiss Ephemeris recommended)
- Scope: Gregorian ↔ Hindu date conversion, month generation, tithi/masa computation
- Key terminology table (tithi, paksha, masa, rashi, sankranti, ayanamsa, etc.)

### Section 2: Astronomical Background (~250 lines)
- **2.1 The Ecliptic and Celestial Coordinates** — ecliptic longitude, tropical vs sidereal
- **2.2 Ayanamsa** — precession of equinoxes, Lahiri ayanamsa, why ~24° offset matters, SE_SIDM_LAHIRI
- **2.3 Tropical vs Sidereal: When to Use Which** — tithi uses tropical (ayanamsa cancels), rashi uses sidereal
- **2.4 Solar Motion** — ~1°/day, sidereal year = 365.25636 days, rashi = 30° segments
- **2.5 Lunar Motion** — ~13°/day, synodic month = 29.53059 days, lunar phase = moon - sun
- **2.6 Sunrise and Sunset** — disc center vs upper limb, refraction, SE_BIT_DISC_CENTER
- **2.7 Julian Day Numbers** — noon-based convention, converting to/from Gregorian, midnight pitfall
- **2.8 The Hindu Five-Fold Day Division** — pratahkala, sangava, madhyahna, aparahna, sayahna (each = 1/5 of sunrise-to-sunset)

### Section 3: The Hindu Lunisolar Calendar (Panchang) (~700 lines)
- **3.1 Overview** — Amanta (new moon to new moon) vs Purnimanta scheme
- **3.2 Tithi (Lunar Day)**
  - Definition: 12° segments of moon-sun elongation
  - Formula: `tithi = floor(lunar_phase / 12) + 1` (1-30)
  - Paksha: Shukla (1-15, waxing) and Krishna (16-30, waning)
  - Special tithis: Purnima (15), Amavasya (30)
  - Tithi names (Pratipada through Amavasya)
  - Pseudocode: `tithi_at_moment(jd)`
- **3.3 Tithi at Sunrise**
  - The civil-day rule: tithi at sunrise governs the entire day
  - Pseudocode: `tithi_at_sunrise(date, location)`
  - Finding tithi boundaries via bisection (50 iterations, ~3ns precision)
  - Pseudocode: `find_tithi_boundary(jd_start, jd_end, target_tithi)`
  - Handling the 0°/360° wraparound in bisection
- **3.4 Adhika Tithi (Repeated Tithi)**
  - Definition: same tithi at two consecutive sunrises
  - Detection: compare today's and yesterday's tithi at sunrise
  - Pseudocode and example
- **3.5 Kshaya Tithi (Skipped Tithi)**
  - Definition: a tithi that completes entirely between two sunrises
  - Detection: tomorrow's tithi - today's tithi > 1
  - Frequency: ~every 2 months
  - Pseudocode and example
- **3.6 Finding New Moons**
  - Why needed: new moons bracket lunar months
  - 17-point inverse Lagrange interpolation algorithm
  - Step-by-step: sample lunar phase at 17 points (0.25-day spacing), unwrap angles, interpolate
  - Pseudocode: `new_moon_before(jd, tithi_hint)`, `new_moon_after(jd, tithi_hint)`
  - Angle unwrapping helper: `unwrap_angles(angles[])`
  - The inverse Lagrange formula explained
- **3.7 Masa (Lunar Month)**
  - Definition: period between two new moons (Amanta scheme)
  - Naming rule: solar rashi at new moon + 1 = month name
  - The 12 masa names (Chaitra through Phalguna)
  - Rashi-to-masa mapping table
  - Pseudocode: `masa_for_date(date, location)`
- **3.8 Adhika Masa (Leap Month)**
  - Definition: when sun stays in same rashi across two consecutive new moons
  - Detection: `rashi_at_prev_new_moon == rashi_at_next_new_moon`
  - Frequency: ~every 32.5 months (11-day shortfall accumulation)
  - The adhika month takes the same name as the regular month
  - Pseudocode and example
- **3.9 Kshaya Masa (Lost Month)**
  - Definition: sun transits two rashis in one lunar month (extremely rare)
  - Frequency: 19-141 year gaps
  - Not implemented in our system (noted for completeness)
- **3.10 Year Numbering**
  - Saka era: `kali = (ahar + (4 - masa_num) * 30) / sidereal_year; saka = kali - 3179`
  - Where `ahar = jd - 588465.5` (days since Kali epoch)
  - Vikram Samvat: `saka + 135`
  - Pseudocode: `hindu_year_saka(jd, masa_num)`
- **3.11 Complete Gregorian-to-Hindu Conversion**
  - Step-by-step walkthrough combining all above algorithms
  - Pseudocode: `gregorian_to_hindu(year, month, day, location)`
  - Worked example: convert a specific date showing every intermediate value

### Section 4: Hindu Solar Calendars (~600 lines)
- **4.1 Overview** — Solar months defined by sankranti (sun entering new rashi)
  - 12 solar months, each 29-32 days
  - 4 regional variants: Tamil, Bengali, Odia, Malayalam
  - Common structure, different critical time rules and eras
- **4.2 Sankranti Finding**
  - Definition: exact moment sun's sidereal longitude crosses a multiple of 30°
  - Bisection algorithm on `solar_longitude_sidereal(jd)` (50 iterations)
  - Pseudocode: `sankranti_jd(jd_approx, target_longitude)`
  - Finding the most recent sankranti: `sankranti_before(jd)`
  - Bracket verification and 0°/360° wraparound handling
- **4.3 Critical Time Rules**
  - The key question: when a sankranti falls during a civil day, does that day start the new month or end the old month?
  - Each calendar has its own rule — detailed per-calendar subsections below
  - Generic pseudocode: `sankranti_to_civil_day(jd_sankranti, location, calendar_type)`
- **4.4 Tamil Solar Calendar (Tamizh Varudam)**
  - Critical time: **sunset** — if sankranti before sunset, that day is day 1 of new month
  - Era: Saka (Gregorian year - 78 on/after Mesha, - 79 before)
  - Year starts: Mesha (mid-April)
  - Month names: Chithirai, Vaikaasi, Aani, Aadi, Aavani, Purattaasi, Aippasi, Karthikai, Maargazhi, Thai, Maasi, Panguni
  - Step-by-step Gregorian → Tamil conversion with pseudocode
  - Worked example
- **4.5 Bengali Solar Calendar (Bangabda)**
  - Critical time: **midnight + 24-minute buffer** — sankrantis in 11:36 PM - 12:24 AM zone assigned to current day (from Reingold/Dershowitz)
  - Era: Bangabda (Gregorian year - 593 on/after Mesha, - 594 before)
  - Year starts: Mesha (mid-April)
  - Month names: Boishakh, Joishtho, Asharh, Srabon, Bhadro, Ashshin, Kartik, Ogrohaeon, Poush, Magh, Falgun, Choitro
  - Step-by-step conversion + pseudocode
  - Worked example
- **4.6 Odia Solar Calendar (Saka)**
  - Critical time: **fixed 22:12 IST** — sankranti at/before 22:11 = current day, at/after 22:12 = next day
  - Discovery story: midnight → apparent midnight → fixed offset → fixed IST (4 stages, 35 boundary cases)
  - Era: Saka (same as Tamil)
  - Year starts: Mesha (mid-April)
  - Month names: Baisakha, Jyeshtha, Ashadha, Shravana, Bhadrapada, Ashvina, Kartika, Margashirsha, Pausha, Magha, Phalguna, Chaitra
  - Step-by-step conversion + pseudocode
  - Worked example
  - Boundary case table (tightest cases within 3 min of cutoff)
- **4.7 Malayalam Solar Calendar (Kollam)**
  - Critical time: **end of madhyahna = sunrise + 3/5 × (sunset - sunrise)**
  - Discovery story: apparent noon → fixed 13:12 IST → 3/5 of daytime (3 stages, 33 boundary cases)
  - The Hindu five-fold day division connection
  - Era: Kollam (Gregorian year - 824 on/after Simha, - 825 before)
  - Year starts: **Simha** (mid-August) — unique among the four calendars
  - Month numbering rotation: month 1 = Chingam (Simha), month 9 = Medam (Mesha)
  - Month names: Chingam, Kanni, Thulam, Vrishchikam, Dhanu, Makaram, Kumbham, Meenam, Medam, Edavam, Mithunam, Karkadakam
  - Step-by-step conversion + pseudocode
  - Worked example
  - Boundary zone analysis (fraction 0.586-0.600, ~10 min ayanamsa offset)
- **4.8 Generic Solar Date Conversion**
  - Unified pseudocode: `gregorian_to_solar(date, location, calendar_type)`
  - Configuration table: start_rashi, era_offset_on, era_offset_before, era_name, critical_time_function
  - Inverse conversion: `solar_to_gregorian(solar_date, calendar_type, location)`

### Section 5: Implementation Gotchas (~400 lines)
- **5.1 Tropical vs Sidereal Longitude** — the #1 mistake: using sidereal for tithi or tropical for rashi
- **5.2 Julian Day Noon Convention** — JD 2451545.0 = 2000-01-01 12:00 UT (not midnight!)
- **5.3 Ayanamsa Differences** — SE_SIDM_LAHIRI vs Indian Astronomical Ephemeris (~24 arcseconds, ~10 min offset in sankranti times)
- **5.4 Sunrise Definition** — disc center vs upper limb, with vs without refraction (must use center + refraction to match drikpanchang)
- **5.5 Day-of-Week Conventions** — Swiss Ephemeris: 0=Monday (ISO), some systems: 0=Sunday
- **5.6 New Moon Near Midnight** — when new moon falls very close to midnight, rashi at new moon can be ambiguous
- **5.7 Angle Wraparound** — 359° to 1° is only 2° apart, not 358°. Must handle in all bisection and interpolation routines
- **5.8 Month Naming Off-by-One** — masa = rashi + 1, not rashi. Common source of bugs
- **5.9 Adhika/Kshaya Tithi Cross-Day Boundaries** — detecting these requires looking at adjacent days, which may be in different Gregorian months
- **5.10 Solar Month Lengths** — vary from 29 to 32 days (not fixed like Gregorian). Don't assume 30
- **5.11 Malayalam Year Start** — Simha (rashi 5), not Mesha (rashi 1). Month numbering rotation is easy to get wrong
- **5.12 Bengali Midnight Buffer Zone** — the 48-minute zone (23:36-00:24) requires special handling per Reingold/Dershowitz
- **5.13 Odia Fixed vs Astronomical Rule** — surprising that a fixed clock time (22:12 IST) works better than any astronomical rule
- **5.14 Malayalam Boundary Zone** — no single fraction perfectly separates all cases due to ayanamsa offset. Accept ~15 ambiguous cases in 150 years

### Section 6: Validation Strategy (~200 lines)
- **6.1 Reference Source** — drikpanchang.com as ground truth (manual verification, no API)
- **6.2 Validation Approach** — hand-verify representative dates, then generate bulk regression data
- **6.3 Edge Case Priority** — adhika/kshaya tithis are hardest (132 of 186 verified dates)
- **6.4 Boundary Case Investigation Method** — systematic scanning + manual verification batches
- **6.5 Regression Testing** — generate CSV for full date range, test all dates programmatically
- **6.6 Known Limitations** — ayanamsa offset boundary zones, kshaya masa not implemented

### Section 7: Reference Tables (~200 lines)
- **7.1 Tithi Names** — 30 tithis with numbers, paksha, Sanskrit names
- **7.2 Masa Names** — 12 months with rashi correspondence
- **7.3 Rashi (Zodiac Signs)** — 12 rashis with degree ranges and Western equivalents
- **7.4 Solar Month Names** — all 4 regional calendars side by side
- **7.5 Era Offsets** — Saka, Bangabda, Kollam formulas
- **7.6 Critical Time Rules Summary** — all 4 calendars in one comparison table

### Appendix A: Complete Pseudocode Reference (~300 lines)
All algorithms collected in one place with consistent naming:
- `lunar_phase(jd)`, `tithi_at_moment(jd)`, `tithi_at_sunrise(date, loc)`
- `find_tithi_boundary(jd_lo, jd_hi, target)`
- `inverse_lagrange(x[], y[], n, target)`, `unwrap_angles(angles[])`
- `new_moon_before(jd, hint)`, `new_moon_after(jd, hint)`
- `solar_rashi(jd)`, `masa_for_date(date, loc)`
- `hindu_year_saka(jd, masa)`, `gregorian_to_hindu(date, loc)`
- `sankranti_jd(jd_approx, target_long)`, `sankranti_before(jd)`
- `critical_time_jd(date, loc, cal_type)`
- `gregorian_to_solar(date, loc, cal_type)`, `solar_to_gregorian(solar_date, cal_type, loc)`

### Appendix B: Worked Examples (~200 lines)
- **B.1 Lunisolar**: Convert 2024-04-09 → Hindu date (full intermediate values)
- **B.2 Tamil**: Convert 2025-04-14 → Chithirai 1, Saka 1947
- **B.3 Bengali**: Convert 2025-04-14 → Boishakh 1, Bangabda 1432
- **B.4 Odia**: Convert a date near 22:12 boundary
- **B.5 Malayalam**: Convert 2025-08-17 → Chingam 1, Kollam 1201

## Source Files Referenced

| File | Purpose |
|------|---------|
| `src/astro.c` | Ephemeris wrapper — solar/lunar longitude, sunrise/sunset |
| `src/tithi.c` | Tithi calculation — lunar phase, bisection, kshaya detection |
| `src/masa.c` | Masa determination — Lagrange interpolation, new moon finding, adhika detection |
| `src/panchang.c` | High-level API — combines tithi + masa |
| `src/solar.c` | Solar calendars — sankranti finding, 4 critical time rules |
| `src/types.h` | All data structures and constants |
| `Docs/MALAYALAM_ADJUSTMENTS.md` | Malayalam critical time investigation |
| `Docs/ODIA_ADJUSTMENTS.md` | Odia critical time investigation |
| `Docs/IMPLEMENTATION_PLAN.md` | Project roadmap and technical decisions |

## Verification

1. Review: read through the entire document for accuracy and completeness
2. Cross-check all pseudocode against actual C implementations in `src/`
3. Verify all constant values (ayanamsa ID, epoch JD, era offsets) match the code
4. Ensure worked examples produce correct results when traced manually
5. Check that all gotchas from MALAYALAM_ADJUSTMENTS.md and ODIA_ADJUSTMENTS.md are captured
