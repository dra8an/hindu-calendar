# Plan: Self-contained Moshier Ephemeris Library (Swiss Ephemeris Replacement)

## Context

All our astronomical calculations currently use Swiss Ephemeris (SE), a 34,600-line vendored C library. However, we only use 8 SE functions through 2 wrapper files (`src/astro.c` and `src/date_utils.c`), and we already use the `SEFLG_MOSEPH` flag (Moshier's built-in analytical ephemeris, not the high-precision Swiss files). This means we're carrying 34K lines of code but only exercising the Moshier code path.

**Goal**: Create a self-contained `lib/moshier/` library (~800-1000 lines) implementing the same 8 functions using Meeus/VSOP87 algorithms. Keep both backends selectable via compile flag (`make` = moshier, `make USE_SWISSEPH=1` = SE).

**Key benefit**: With our own ayanamsa implementation, we can potentially match drikpanchang.com's exact Lahiri model, eliminating the ~24 arcsecond difference that currently requires empirical buffers for Tamil (-8.0 min) and Malayalam (-9.5 min) solar calendars.

## SE Functions to Replace

From `src/astro.c`:
| Function | Usage | Replacement Algorithm |
|----------|-------|----------------------|
| `swe_set_ephe_path()` | Init ephemeris path | No-op (no external files) |
| `swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0)` | Set ayanamsa mode | No-op (hardcoded Lahiri) |
| `swe_close()` | Cleanup | No-op |
| `swe_calc_ut(jd, SE_SUN, ...)` | Tropical solar longitude | VSOP87 truncated (Meeus Ch.25) |
| `swe_calc_ut(jd, SE_MOON, ...)` | Tropical lunar longitude | ELP2000-82 truncated (Meeus Ch.47) |
| `swe_get_ayanamsa_ut(jd)` | Lahiri ayanamsa (degrees) | Newcomb precession + Lahiri epoch |
| `swe_rise_trans(jd, SE_SUN, ..., SE_CALC_RISE\|SE_BIT_DISC_CENTER, ...)` | Sunrise JD | Meeus Ch.15 (hour angle + iteration) |
| `swe_rise_trans(jd, SE_SUN, ..., SE_CALC_SET\|SE_BIT_DISC_CENTER, ...)` | Sunset JD | Meeus Ch.15 (hour angle + iteration) |

From `src/date_utils.c`:
| Function | Usage | Replacement Algorithm |
|----------|-------|----------------------|
| `swe_julday(y, m, d, h, SE_GREG_CAL)` | Gregorian → JD | Meeus Ch.7 formula |
| `swe_revjul(jd, SE_GREG_CAL, ...)` | JD → Gregorian | Meeus Ch.7 inverse |
| `swe_day_of_week(jd)` | Day of week (0=Mon..6=Sun) | `floor(jd + 1.5) % 7` |

## Files to Create

```
lib/moshier/
├── moshier.h           # Public API — 8 replacement functions (~40 lines)
├── moshier_jd.c        # JD ↔ Gregorian, day of week (~80 lines)
├── moshier_sun.c       # Solar longitude + declination (~200 lines)
├── moshier_moon.c      # Lunar longitude (~300 lines)
├── moshier_ayanamsa.c  # Lahiri ayanamsa (~60 lines)
└── moshier_rise.c      # Sunrise/sunset (~200 lines)
```

## Files to Modify

| File | Change |
|------|--------|
| `src/astro.c` | `#ifdef USE_SWISSEPH` to select SE or moshier backend |
| `src/date_utils.c` | `#ifdef USE_SWISSEPH` to select SE or moshier backend |
| `Makefile` | Add moshier build rules, default to moshier, `USE_SWISSEPH=1` for SE |

## Implementation Steps

### Step 1: `lib/moshier/moshier.h` — Public header

```c
#ifndef MOSHIER_H
#define MOSHIER_H

// JD ↔ Gregorian (Meeus Ch.7)
double moshier_julday(int year, int month, int day, double hour);
void   moshier_revjul(double jd, int *year, int *month, int *day, double *hour);
int    moshier_day_of_week(double jd);  // 0=Mon..6=Sun (matching SE convention)

// Planetary positions — tropical longitude in degrees [0,360)
double moshier_solar_longitude(double jd_ut);
double moshier_lunar_longitude(double jd_ut);

// Solar declination in degrees (needed for sunrise/sunset)
double moshier_solar_declination(double jd_ut);

// Lahiri ayanamsa in degrees
double moshier_ayanamsa(double jd_ut);

// Sunrise/sunset — returns JD (UT), disc center with refraction
double moshier_sunrise(double jd_ut, double lon, double lat, double alt);
double moshier_sunset(double jd_ut, double lon, double lat, double alt);

#endif
```

### Step 2: `lib/moshier/moshier_jd.c` (~80 lines)

- **`moshier_julday`**: Meeus Ch.7 formula. Standard algorithm handling Jan/Feb → month+12/year-1, A = floor(year/100), B = 2-A+floor(A/4), JD = floor(365.25*(year+4716)) + floor(30.6001*(month+1)) + day + hour/24 + B - 1524.5.
- **`moshier_revjul`**: Inverse of the above. Standard Meeus algorithm.
- **`moshier_day_of_week`**: `((int)floor(jd + 1.5)) % 7` gives 0=Mon..6=Sun (must verify this matches SE convention).

### Step 3: `lib/moshier/moshier_sun.c` (~200 lines)

- **Solar longitude**: Meeus Ch.25 — truncated VSOP87 series.
  - Compute centuries T from J2000.0: `T = (jd - 2451545.0) / 36525.0`
  - Geometric mean longitude: `L0 = 280.46646 + 36000.76983*T + 0.0003032*T²`
  - Mean anomaly: `M = 357.52911 + 35999.05029*T - 0.0001537*T²`
  - Equation of center: `C = (1.914602 - 0.004817*T)*sin(M) + (0.019993 - 0.000101*T)*sin(2M) + 0.000289*sin(3M)`
  - Sun's true longitude: `Θ = L0 + C`
  - Apply nutation and aberration corrections (~20" aberration)
  - Precision: ~1 arcsecond (1900-2100)

- **Solar declination** (for sunrise/sunset):
  - Obliquity: `ε = 23.439291 - 0.013004*T`
  - Right ascension and declination from ecliptic coordinates
  - `sin(δ) = sin(ε) × sin(λ)`

- **Nutation in longitude** (shared helper): Meeus Ch.22 — 5-term truncation of the IAU 1980 nutation model (~0.5" precision, sufficient for our needs). Uses mean elongation of Moon, solar anomaly, lunar anomaly, Moon's argument of latitude, and longitude of ascending node.

### Step 4: `lib/moshier/moshier_moon.c` (~300 lines)

- **Lunar longitude**: Meeus Ch.47 — truncated ELP-2000/82.
  - 4 fundamental arguments: L' (mean longitude), D (mean elongation), M (sun mean anomaly), M' (moon mean anomaly), F (argument of latitude)
  - ~60 terms for longitude (Σl), using sin of linear combinations of D, M, M', F
  - Apply eccentricity correction for terms involving M (E = 1 - 0.002516*T - 0.0000074*T²)
  - Add 3958*sin(A1) + 1962*sin(L'-F) + 318*sin(A2) for Venus/Jupiter/flattening
  - Longitude = L' + Σl/1000000 (converted to degrees)
  - Apply nutation correction
  - Precision: ~10 arcseconds (more than sufficient for tithi calculation)

### Step 5: `lib/moshier/moshier_ayanamsa.c` (~60 lines)

- **Lahiri ayanamsa**: Based on precession from a reference epoch.
  - Traditional Lahiri: ayanamsa was 23°15'00" at the spring equinox of 1950 (mean equinox)
  - Alternative (Chitrapaksha): The star Spica (Chitra) is fixed at 0° Libra (180° sidereal)
  - SE_SIDM_LAHIRI uses: epoch J1900 (B1900.0), ayanamsa = 22°27'37.7" at epoch, with Newcomb precession
  - Formula: `ayan = 22.460864 + precession_rate * (T - T_epoch)`
  - Newcomb precession: `p = 5029.0966" * T + 1.1120" * T² - 0.000006" * T³` per Julian century from J2000
  - **Key opportunity**: We can calibrate this to match drikpanchang.com's exact ayanamsa, potentially eliminating the ~24" difference

### Step 6: `lib/moshier/moshier_rise.c` (~200 lines)

- **Sunrise/sunset**: Meeus Ch.15 iterative algorithm.
  - **SE_BIT_DISC_CENTER** means: center of disc at horizon, with atmospheric refraction
  - Standard refraction at horizon ≈ 34 arcminutes
  - Depression angle h₀ = -34/60 = -0.5667° (center of disc, with refraction)
  - Algorithm:
    1. Compute apparent sidereal time at 0h UT of the date
    2. Compute sun's RA and declination for the date
    3. Hour angle: `cos(H₀) = (sin(h₀) - sin(φ)sin(δ)) / (cos(φ)cos(δ))`
    4. Transit time: `m₀ = (α - θ₀ - L) / 360`
    5. Rise: `m₁ = m₀ - H₀/360`, Set: `m₂ = m₀ + H₀/360`
    6. Iterate 2-3 times: recompute δ and α at each trial time, correct m₁/m₂
    7. Convert fractional day to JD

  - Sidereal time: `θ₀ = 280.46061837 + 360.98564736629*(JD-2451545.0)` (Meeus Ch.12)
  - Need: solar RA + declination (from moshier_sun.c), sidereal time, observer location

### Step 7: Modify `src/astro.c` — dual backend

```c
#ifdef USE_SWISSEPH
  #include "swephexp.h"
#else
  #include "moshier.h"
#endif

void astro_init(const char *ephe_path) {
#ifdef USE_SWISSEPH
    if (ephe_path) swe_set_ephe_path((char *)ephe_path);
    swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
#else
    (void)ephe_path;  // moshier needs no init
#endif
}

// ... same pattern for all functions
```

### Step 8: Modify `src/date_utils.c` — dual backend

Same `#ifdef USE_SWISSEPH` pattern for `gregorian_to_jd`, `jd_to_gregorian`, `day_of_week`.

### Step 9: Modify `Makefile`

```makefile
ifdef USE_SWISSEPH
  CFLAGS += -DUSE_SWISSEPH
  EPHLIB_SRCS = $(wildcard lib/swisseph/*.c)
  EPHLIB_OBJS = $(patsubst lib/swisseph/%.c,build/swe/%.o,$(EPHLIB_SRCS))
  INCLUDES = -Ilib/swisseph -Isrc
else
  EPHLIB_SRCS = $(wildcard lib/moshier/*.c)
  EPHLIB_OBJS = $(patsubst lib/moshier/%.c,build/moshier/%.o,$(EPHLIB_SRCS))
  INCLUDES = -Ilib/moshier -Isrc
endif
```

### Step 10: Verification

1. Build with moshier (default): `make clean && make && make test` — all 53,143 assertions must pass
2. Build with SE: `make clean && make USE_SWISSEPH=1 && make test USE_SWISSEPH=1` — all 53,143 assertions must pass
3. Compare outputs: run `./hindu-calendar -y 2025 -m 1` with both backends, verify identical tithi/masa/saka
4. Spot-check sunrise times: compare both backends for Delhi, 2025-01-14 — should agree within ~1 minute
5. Compare ayanamsa: print `get_ayanamsa()` at J2000 for both backends — should be very close

## Precision Requirements

| Quantity | Need | Meeus delivers | OK? |
|----------|------|----------------|-----|
| Solar longitude | ~1 arcminute (for sankranti within ~1.5 min) | ~1 arcsecond | Yes |
| Lunar longitude | ~1 arcminute (for tithi within ~2 min) | ~10 arcseconds | Yes |
| Ayanamsa | ~1 arcsecond (to minimize buffer need) | Exact formula | Yes |
| Sunrise/sunset | ~1 minute | ~1 minute | Yes |
| JD conversion | Exact | Exact | Yes |

## Dependency Order

```
moshier_jd.c      (no deps)
moshier_sun.c     (no deps — self-contained nutation + solar position)
moshier_moon.c    (no deps — self-contained nutation + lunar position)
moshier_ayanamsa.c (no deps — just precession formula)
moshier_rise.c    (uses solar position + declination from moshier_sun)
moshier.h         (after all .c files are written)
astro.c changes   (after moshier.h)
date_utils.c changes (after moshier.h)
Makefile changes  (last)
```
