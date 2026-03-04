# Plan: Investigate NYC Future Mismatch Skew (Delta-T)

## Problem

34 NYC mismatches against drikpanchang.com are heavily skewed toward the future:
- 1900–1999: 11 mismatches (0.11/yr)
- 2000–2026: 5 mismatches (0.19/yr)
- 2027–2050: 18 mismatches (0.75/yr) — 4-7x higher rate

All mismatches are +1 (our tithi is 1 behind drikpanchang), meaning our sunrise
is systematically slightly later than drikpanchang's for future dates.

## Hypothesis

Delta-T (TT-UT) extrapolation divergence post-2025. Our Moshier library uses an
SE-derived yearly lookup table ending around 2025, then falls back to a polynomial
extrapolation. If drikpanchang uses different delta-T values (e.g., updated IERS
bulletins or a different polynomial), sunrise times diverge increasingly with time.

A delta-T error of 1 second shifts sunrise by ~1 second. By 2050, polynomial
extrapolations can diverge by 10-30 seconds depending on the model used.

## Investigation Steps

### 1. Audit our delta-T implementation
- Read `lib/moshier/moshier_sun.c` (or wherever delta_t() lives)
- Document what polynomial/table we use and where the cutoff is
- Print delta-T values for 2025, 2030, 2040, 2050

### 2. Check what Swiss Ephemeris uses
- Compare SE's delta-T for the same years (build with USE_SWISSEPH=1)
- Check if SE has been updated with newer delta-T tables since we vendored it

### 3. Check drikpanchang's likely delta-T source
- Drikpanchang uses Swiss Ephemeris internally
- Check latest SE release notes for delta-T updates
- Compare scraped sunrise times for future dates vs our computed values

### 4. Build diagnostic tool
- `tools/nyc_mismatch_diag.c` — for each of the 34 mismatch dates:
  - Print sunrise time (HH:MM:SS)
  - Print tithi boundary crossing time
  - Print margin (seconds between sunrise and boundary)
  - Print delta-T value used
- Confirm the margin correlates with date (larger margins for further future dates
  would disprove delta-T hypothesis; smaller margins would confirm it)

### 5. Compare with Delhi future mismatches
- Delhi has 10 mismatches — how many are post-2025?
- If Delhi also skews future, it's definitely a systematic delta-T issue
- If Delhi doesn't skew, it might be a latitude-dependent refraction issue instead

### 6. Possible fixes
- **Option A**: Update delta-T table with latest IERS data through 2025, use
  Morrison & Stephenson (2004) or Espenak & Meeus (2006) extrapolation beyond
- **Option B**: Match whatever polynomial SE's latest version uses
- **Option C**: Accept the skew as inherent uncertainty for future dates (delta-T
  is genuinely unpredictable beyond a few years)

## Expected Outcome

Either:
1. Identify a fixable delta-T divergence and reduce future mismatches, or
2. Confirm these are irreducible uncertainties for future dates and document accordingly

## Files Involved

- `lib/moshier/moshier_sun.c` — delta-T implementation
- `tools/nyc_mismatch_diag.c` — new diagnostic tool
- `Docs/NYC_VALIDATION.md` — update with findings
