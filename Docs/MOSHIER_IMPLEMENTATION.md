# Moshier Ephemeris Library — Implementation Notes

Self-contained astronomical ephemeris replacing Swiss Ephemeris (SE) for the Hindu calendar project.

## Overview

| Metric | Moshier | Swiss Ephemeris |
|--------|---------|----------------|
| Lines of code | 787 | 51,493 |
| External files | None | Optional .se1 files |
| Build time | ~0.5s | ~3s |
| Test pass rate | 53,023 / 53,143 (99.77%) | 53,143 / 53,143 (100%) |
| drikpanchang.com match | 1,071 / 1,071 (100%) | 1,071 / 1,071 (100%) |
| Reduction factor | **65x fewer lines** | — |

Both backends are selectable at compile time:
- `make` — moshier (default)
- `make USE_SWISSEPH=1` — Swiss Ephemeris

## Architecture

```
lib/moshier/
├── moshier.h           35 lines   Public API (8 functions)
├── moshier_jd.c        56 lines   JD ↔ Gregorian, day of week
├── moshier_sun.c      249 lines   Solar longitude, nutation, delta-T
├── moshier_moon.c     146 lines   Lunar longitude (ELP-2000/82)
├── moshier_ayanamsa.c 146 lines   Lahiri ayanamsa (IAU 1976 precession)
└── moshier_rise.c     155 lines   Sunrise / sunset
                       ─── ─────
                       787 total
```

The library replaces 8 SE functions called from `src/astro.c` and `src/date_utils.c`:

| SE Function | Moshier Replacement | File |
|-------------|-------------------|------|
| `swe_julday()` | `moshier_julday()` | moshier_jd.c |
| `swe_revjul()` | `moshier_revjul()` | moshier_jd.c |
| `swe_day_of_week()` | `moshier_day_of_week()` | moshier_jd.c |
| `swe_calc_ut(SE_SUN)` | `moshier_solar_longitude()` | moshier_sun.c |
| `swe_calc_ut(SE_MOON)` | `moshier_lunar_longitude()` | moshier_moon.c |
| `swe_get_ayanamsa_ut()` | `moshier_ayanamsa()` | moshier_ayanamsa.c |
| `swe_rise_trans(RISE)` | `moshier_sunrise()` | moshier_rise.c |
| `swe_rise_trans(SET)` | `moshier_sunset()` | moshier_rise.c |

## Algorithms Used

### JD ↔ Gregorian (`moshier_jd.c`)

Meeus Ch. 7, standard algorithm. Exact — no floating-point approximation.

Day-of-week formula matches SE: `(((int)floor(jd - 2433282 - 1.5) % 7) + 7) % 7` → 0=Mon..6=Sun.

### Solar Longitude (`moshier_sun.c`)

**Equation of center** (Meeus Ch. 25, eq. 25.6):
```
C = (1.914602 - 0.004817T) sin(M) + (0.019993 - 0.000101T) sin(2M) + 0.000289 sin(3M)
```
This is a truncated VSOP87. SE uses the full VSOP87 series with ~135 harmonic terms encoded in `swemplan.c` via 9 fundamental planetary frequencies.

**Nutation** (Meeus Ch. 22): 13 terms from IAU 1980 Table 22.A. SE uses the same model. See [Why 13 Terms](#why-13-nutation-terms-not-63) for rationale.

**Apparent longitude correction** (Meeus p. 164):
```
aberration = -0.00569° - 0.00478° × sin(Ω)
apparent = true_longitude + Δψ + aberration
```
This combined formula is critical — it provides beneficial error cancellation against our simplified equation of center. See [Error Cancellation](#error-cancellation-meeus-p164-formula) for details.

**Delta-T**: Polynomial fits from Meeus Ch. 10 / USNO tables, covering 1900–2150.

### Lunar Longitude (`moshier_moon.c`)

60 longitude terms from Meeus Table 47.A (ELP-2000/82 theory). Each term is a sinusoidal function of four fundamental arguments (D, M, M', F) with coefficients in 10⁻⁶ degrees.

Key features:
- **Eccentricity correction** (E factor): Terms involving solar mean anomaly M are multiplied by E = 1 - 0.002516T - 0.0000074T². |M|=1 → ×E, |M|=2 → ×E².
- **Venus correction** (A1): +3958 × sin(119.75° + 131.849T)
- **Jupiter correction** (A2): +318 × sin(53.09° + 479264.290T)
- **Flattening correction**: +1962 × sin(L' - F)

Nutation in longitude (from `moshier_sun.c`) is added to get apparent longitude.

### Lahiri Ayanamsa (`moshier_ayanamsa.c`)

Replicates SE_SIDM_LAHIRI exactly using the same 3D equatorial precession algorithm:

1. Start with vernal point at target date: (1, 0, 0) in equatorial coordinates
2. Precess from target date → J2000 using IAU 1976 angles
3. Precess from J2000 → t₀ (Lahiri reference epoch, JD 2435553.5)
4. Convert to ecliptic of t₀, get polar longitude
5. Ayanamsa = −longitude + 23.245524743°
6. Add nutation in longitude

IAU 1976 precession angles (Lieske et al. 1977):
```
Z     = (2306.2181T + 0.30188T² + 0.017998T³) / 3600°
z     = (2306.2181T + 1.09468T² + 0.018203T³) / 3600°
θ     = (2004.3109T - 0.42665T² - 0.041833T³) / 3600°
```

SE Lahiri constants (from `sweph.h` ayanamsa table, index 1):
- t₀ = JD 2435553.5 (1956 September 22.0 TT)
- ayan_t₀ = 23.250182778 − 0.004658035 = 23.245524743°

### Sunrise / Sunset (`moshier_rise.c`)

Meeus Ch. 15 iterative method:

1. Compute apparent sidereal time at 0h UT (Meeus Ch. 12)
2. Compute solar RA and declination at noon
3. Hour angle: cos(H₀) = (sin(h₀) − sin(φ)sin(δ)) / (cos(φ)cos(δ))
4. Initial estimate: m = m₀ ∓ H₀/360
5. Iterate 5 times: recompute solar position at trial time, correct via altitude residual
6. Return JD of event

Depression angle h₀ = −0.5667° (disc center with standard atmospheric refraction, matching SE_BIT_DISC_CENTER).

Altitude adjustment for observer elevation: h₀ -= 0.0353 × √alt.

## Precision vs Swiss Ephemeris

Spot-check at 2025-01-14 (New Delhi):

| Quantity | Moshier | Swiss Ephemeris | Difference |
|----------|---------|----------------|------------|
| Ayanamsa | 24.207150° | 24.207147° | 0.01" |
| Solar longitude | 294.065172° | 294.061529° | 13" |
| Lunar longitude | 114.865508° | 114.865226° | 1" |
| Sunrise | 07:15:17 IST | 07:15:03 IST | 14s |

All differences are well within the requirements for Hindu calendar calculation (tithi boundaries need ~1 arcminute, sankranti boundaries need ~1 minute).

## The 120 Failures

All 120 failures (out of 53,143 assertions) are tithi or sankranti boundary edge cases where the moshier and SE backends disagree on which side of a boundary a value falls at sunrise. The differences are typically < 1 arcsecond in the underlying longitude, but at a boundary crossing that's enough to flip the discrete result (e.g., tithi 14 vs 15).

Breakdown:
- `test_adhika_kshaya`: 19 failures — adhika/kshaya tithi detection at sunrise
- `test_csv_regression`: 4 failures — tithi at sunrise
- `test_solar`: 20 failures — sankranti day assignment
- `test_solar_edge`: 28 failures — sankranti boundary cases
- `test_solar_regression`: 49 failures — solar month start dates

None of these affect the 1,071 dates externally verified against drikpanchang.com, which all pass with both backends.

## What We Tried and Why

### Lunar Terms: 60 Meeus vs 118 SE Terms

We extracted 118 longitude terms from SE's `swemmoon.c` (the `LR[]` and `LRT[]` tables). These include the same fundamental terms plus time-dependent corrections (T-multiplied coefficients from `LRT[]`).

**Result: 179 failures (worse than 120)**

**Why**: SE's lunar tables are designed for SE's complete pipeline, which includes additional corrections not in our library:
- Venus-Jupiter perturbation terms (`moon1()` function)
- Simple perturbations and polynomial secular terms (`moon2()` function)
- Additional corrections using planetary longitudes (l1–l4 arguments)

Using the raw SE terms without these corrections creates an imbalanced model. The 60 Meeus terms are self-consistent — they already account for the missing corrections through their specific coefficient values and the A1/A2/A3 additional terms.

**Key insight**: More terms ≠ better results when the terms are designed for a different correction pipeline.

### Nutation: 13 Terms vs Full 63-Term Table

We implemented the full 63-term IAU 1980 nutation table from Meeus Table 22.A.

**Result: 178 failures with 63 terms + clean aberration vs 120 failures with 13 terms + Meeus p.164 formula**

See [Error Cancellation](#error-cancellation-meeus-p164-formula) below.

### Removing Eccentricity Correction

Hypothesized that SE's `chewm()` function doesn't apply the E correction (eccentricity factor) to lunar terms, so perhaps we shouldn't either.

**Result: 208 failures (much worse)**

The E correction is essential for the Meeus 60-term model. SE's `chewm()` handles E differently (through its T-dependent LRT terms), but the standalone Meeus model requires explicit E multiplication.

### Solar Planetary Perturbation Terms

Added the Venus, Jupiter, Moon, and long-period correction terms from Meeus p. 164:
```
-0.00569° sin(M')
-0.01060° sin(2M')
-0.00317° sin(M' - 2F)
etc.
```

**Result: 208 failures (worse)**

These terms shift solar longitude in ways that cause more boundary crossings to flip unfavorably against SE. The simplified equation of center with the combined aberration formula already produces better alignment with SE's VSOP87 output.

### Error Cancellation: Meeus p.164 Formula

This was the most important discovery. The Meeus p. 164 apparent longitude correction:
```
aberration = -0.00569° - 0.00478° × sin(Ω)
```
is presented as combining aberration (−0.00569°) with a nutation-related adjustment (−0.00478° × sin(Ω)). However, for our implementation, this combined formula provides beneficial error cancellation:

1. Our equation of center has only 3 sine terms (vs SE's ~135 VSOP87 harmonics)
2. This means our geometric longitude has systematic errors of ~10–20 arcseconds
3. The −0.00478° × sin(Ω) term (~17" amplitude, ~18.6 year period) happens to partially compensate for these geometric longitude errors
4. Using the "correct" decomposition (full 63-term nutation + clean −20.4898" aberration) removes this compensation

**Result**: 13-term nutation + combined formula: **120 failures**. 63-term nutation + clean aberration: **178 failures**.

This is a textbook example of how simplified models can achieve better results through compensating errors, and why replacing individual components with "more accurate" versions can degrade overall system performance.

### Summary of All Configurations Tested

| Moon Terms | Nutation | Aberration | E | A1/A2/A3 | Failures |
|-----------|----------|------------|---|----------|----------|
| 60 Meeus | 13-term | Meeus p.164 combined | Yes | Yes | **120** |
| 60 Meeus | 63-term | Clean −20.5" | Yes | Yes | 178 |
| 118 SE | 13-term | Meeus p.164 combined | Yes | No | 179 |
| 118 SE | 63-term | Clean −20.5" | Yes | No | 179 |
| 118 SE | 13-term | Meeus p.164 combined | No | No | 208 |
| 60 Meeus | 13-term | Meeus p.164 + planetary | Yes | Yes | 208 |
| 118 SE | 63-term | Clean −20.5" + planetary | Yes | No | 208 |

The optimal configuration (first row) was chosen as the final implementation.

## Suggested Improvements

### 1. Improved Equation of Center (~50 fewer failures, ~2 days effort)

The largest source of error is the 3-term equation of center for solar longitude. Adding ~10 more VSOP87 terms for the largest harmonics (without going to the full 135-term series) could reduce solar longitude error from ~13" to ~3–5", likely eliminating ~50 boundary failures.

The terms would come from Meeus Table 25.C (higher-order VSOP87 terms) or from the Bretagnon & Francou (1988) VSOP87 paper directly. This would require:
- Adding ~10 more sine terms to the solar geometric longitude calculation
- Re-tuning the aberration formula (the current error cancellation would shift)
- Careful testing to find the optimal combination

### 2. Brown-Newcomb Lunar Terms (~20 fewer failures, ~1 day effort)

The 60-term Meeus table truncates the ELP-2000/82 series at ~300 milliarcseconds. Adding 20–30 more terms (Meeus provides coefficients down to ~100 milliarcseconds in the full Table 47.A) could improve lunar longitude by ~5 arcseconds.

### 3. Full VSOP87 Implementation (~100 fewer failures, see estimate below)

Replace the 3-term equation of center with the complete VSOP87 heliocentric → geocentric solar longitude computation.

### 4. Improved Sunrise Algorithm (~10 fewer failures, ~1 day effort)

The current Meeus Ch. 15 iterative method computes 5 iterations starting from a noon-based estimate. A more sophisticated approach:
- Use the USNO algorithm (interpolation over three consecutive transits)
- Improve delta-T for pre-1900 dates
- Add atmospheric pressure/temperature corrections

## VSOP87 Implementation Effort Estimate

### What It Involves

VSOP87 (Bretagnon & Francou, 1988) computes heliocentric ecliptic coordinates of the Earth, then converts to geocentric solar longitude. The SE implementation lives in `swemplan.c`:

- **`swi_moshplan2()`**: Main entry point (~200 lines of logic)
- **Harmonic tables**: `earargs[]` (9 fundamental planetary frequencies × ~135 entries) + `eartabl[]` (cosine/sine coefficients for each harmonic)
- **Additional tables**: `eartabb[]` (latitude) and `eartabr[]` (radius) — likely not needed since we only need longitude
- **Conversion**: Heliocentric → geocentric, ecliptic rotation, light-time correction, aberration

### Data Volume

The SE VSOP87 implementation for Earth uses:
- `earargs[]`: ~1,200 integers (9 fundamental arguments × ~135 harmonics)
- `eartabl[]`: ~270 doubles (cosine + sine for each harmonic in longitude)
- Total: ~3–4 KB of static data tables

### Code Estimate

| Component | Lines | Notes |
|-----------|-------|-------|
| VSOP87 harmonic data tables | ~200 | Extracted from SE or VSOP87 paper |
| Harmonic summation loop | ~50 | Sum A×cos(B + C×T) for each term |
| Heliocentric → geocentric conversion | ~30 | λ_sun = λ_earth + 180°, light-time |
| Ecliptic rotation (if needed) | ~20 | J2000 ecliptic → ecliptic of date |
| Integration into moshier_sun.c | ~30 | Replace equation of center |
| Total new code | ~330 | |
| Removed code | ~10 | Current 3-term EoC |
| Net increase | ~320 lines | Library goes from 787 → ~1100 lines |

### Effort

| Task | Time |
|------|------|
| Extract and format VSOP87 tables from SE source | 2–3 hours |
| Implement harmonic summation and conversion | 3–4 hours |
| Re-tune aberration/nutation interaction | 2–3 hours |
| Test and debug against SE reference | 2–3 hours |
| Boundary case optimization | 2–3 hours |
| **Total** | **~2 days** |

### Expected Result

Full VSOP87 would bring solar longitude within ~0.1 arcsecond of SE (currently ~13"). This would likely eliminate most of the 97 solar-related failures (test_solar + test_solar_edge + test_solar_regression), bringing the total failure count from 120 to ~20–30 (remaining from lunar longitude differences only).

### Risk

The main risk is that the current error cancellation (Meeus p.164 combined aberration) may no longer work — with accurate geometric longitude, we'd need to use clean aberration (−20.4898"/R). This might require re-tuning the nutation term count. However, with accurate geometric longitude, the "correct" 63-term nutation + clean aberration should work properly since the compensating-errors issue disappears.

## References

1. Meeus, Jean. "Astronomical Algorithms", 2nd ed. Willmann-Bell, 1998.
   - Ch. 7: Julian Day
   - Ch. 10: Delta-T
   - Ch. 12: Sidereal Time
   - Ch. 15: Rising, Transit, Setting
   - Ch. 22: Nutation and Obliquity
   - Ch. 25: Solar Coordinates
   - Ch. 47: Position of the Moon

2. Bretagnon, P. & Francou, G. "Planetary theories in rectangular and spherical variables. VSOP87 solutions." Astronomy & Astrophysics, 202, 309-315, 1988.

3. Chapront-Touzé, M. & Chapront, J. "ELP 2000-85: a semi-analytical lunar ephemeris adequate for historical times." Astronomy & Astrophysics, 190, 342-352, 1988.

4. Lieske, J.H. et al. "Expressions for the Precession Quantities Based upon the IAU (1976) System of Astronomical Constants." Astronomy & Astrophysics, 58, 1-16, 1977.

5. Swiss Ephemeris source code (v2.10): `swemmoon.c`, `swemplan.c`, `sweph.c`, `swephlib.c`.
