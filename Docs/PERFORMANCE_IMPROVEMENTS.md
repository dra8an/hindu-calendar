# Performance Improvements

## Cumulative Results

**27x overall speedup** across two phases (123.6s → 4.5s for 275,760 calendar conversions):

| Calendar | Original | Phase 1 | Phase 2 | Random | Seq Speedup | Rand Speedup |
|----------|----------|---------|---------|--------|-------------|--------------|
| Lunisolar | 1,044 μs | 90 μs | **23 μs** | 112 μs | **45x** | **9.3x** |
| Tamil | 301 μs | 40 μs | **18 μs** | 160 μs | **17x** | **1.9x** |
| Bengali | 219 μs | 5.0 μs | **4.1 μs** | 125 μs | **53x** | **1.8x** |
| Odia | 205 μs | 4.5 μs | **3.7 μs** | 117 μs | **55x** | **1.8x** |
| Malayalam | 471 μs | 83 μs | **32 μs** | 201 μs | **15x** | **2.3x** |

Per-day cost in microseconds (μs). Each column benchmarks 55,152 days × 5 calendars = 275,760 conversions.

Total wall-clock time (55,152 days per calendar):

| Calendar | Original | Phase 1 | Phase 2 | Random |
|----------|----------|---------|---------|--------|
| Lunisolar | 57.6s | 5.0s | 1.3s | 6.2s |
| Tamil | 16.6s | 2.2s | 1.0s | 8.8s |
| Bengali | 12.1s | 0.3s | 0.2s | 6.9s |
| Odia | 11.3s | 0.2s | 0.2s | 6.4s |
| Malayalam | 26.0s | 4.6s | 1.8s | 11.1s |
| **Total** | **123.6s** | **12.2s** | **4.5s** | **39.4s** |

- **Phase 2 (sequential)**: The dominant use case — panchang generation, validation, benchmarks.
  Caches hit ~97% of the time due to consecutive-day iteration.
- **Random access**: Same 55,152 days per calendar, Fisher-Yates shuffled (seed=42).
  All caches miss on nearly every call, isolating the unconditional improvements
  (early bisection termination, combined RA+Dec, reduced Lagrange).

The 3.1x random-access speedup comes entirely from algorithmic improvements that
apply regardless of access pattern. The remaining ~9x gap between random and
sequential is the cache benefit.

Zero regressions: all 58,983 test assertions pass across 12 test suites.

---

## Phase 1: Algorithmic Caching (1,044 → 90 μs lunisolar)

### 1. Lightweight tithi query (`src/tithi.c`, `src/tithi.h`)

Added `tithi_num_at_jd(jd_sunrise)` — returns only the tithi number at a given JD
without computing tithi boundaries. The old `tithi_at_sunrise()` ran two 50-iteration
bisections (100 `lunar_phase()` calls each = 200 total) to find `jd_start`/`jd_end`,
but `gregorian_to_hindu()` only needed the tithi number.

`gregorian_to_hindu()` in `src/panchang.c` now uses `tithi_num_at_jd()` for both
today's and yesterday's tithi (adhika check). The full `tithi_at_sunrise()` is still
used by `generate_month_panchang()` where boundaries and kshaya flags are needed.

**Savings**: ~400 `lunar_phase()` calls eliminated per day.

### 2. Cached sunrise in `gregorian_to_hindu()` (`src/panchang.c`)

Previously, `gregorian_to_hindu()` computed sunrise 3 times independently:
- `tithi_at_sunrise(today)` → `sunrise_jd()`
- `masa_for_date()` → `sunrise_jd()`
- `tithi_at_sunrise(yesterday)` → `sunrise_jd()`

Now computes sunrise once for today, calls `tithi_num_at_jd()` directly, and passes
the pre-computed sunrise to `masa_for_date_at()`. Yesterday's sunrise is still one
additional call, but 2 of the 3 redundant today-sunrise calls are eliminated.

Added `masa_for_date_at(jd_rise, loc)` to `src/masa.h`/`src/masa.c` that accepts a
pre-computed sunrise JD.

**Savings**: 2 sunrise computations per day.

### 3. Static new moon cache (`src/masa.c`)

Consecutive days almost always fall between the same pair of new moons (~29/30 days
per lunation). Added a static cache for `(last_nm, next_nm)`. On cache hit (sunrise JD
is between cached new moons), the 2×17-point Lagrange interpolation search is skipped.

**Savings**: ~96% of new moon searches eliminated (29/30 hit rate).

### 4. Static sankranti cache (`src/solar.c`)

Consecutive days almost always share the same solar rashi (~30/31 days per sign).
Added a static cache keyed on `(calendar_type, rashi)` that stores the sankranti JD
and its civil day. Cache validity check includes a 35-day proximity test to avoid
stale hits across years.

Also added a solar year cache keyed on `(calendar_type, gregorian_year)` that stores
the year-start civil JD. The before/after comparison still runs per call, but the
expensive `sankranti_jd()` computation for the year-start is skipped for all days in
the same Gregorian year.

**Savings**: ~97% of sankranti bisections eliminated. Bengali/Odia benefit most (45x)
because their critical time functions don't need sunrise/sunset computation.

### 5. Early termination in bisection (`src/tithi.c`, `src/solar.c`)

Both `find_tithi_boundary()` and `sankranti_jd()` previously ran a fixed 50 iterations
(nanosecond precision). Added early exit when `|hi - lo|` converges below a threshold:
- Tithi boundaries: 1 second (1/86400 days) — ~25 iterations sufficient
- Sankranti: 1 millisecond (1e-3/86400 days) — ~30 iterations sufficient

**Savings**: ~40-50% reduction in iterations for remaining (non-cached) bisections.

### Phase 1 files changed

| File | Change |
|------|--------|
| `src/tithi.h` | Added `tithi_num_at_jd()` declaration |
| `src/tithi.c` | Added `tithi_num_at_jd()`, early exit in `find_tithi_boundary()` |
| `src/panchang.c` | Rewrote `gregorian_to_hindu()`: cached sunrise, lightweight tithi, `masa_for_date_at()` |
| `src/masa.h` | Added `masa_for_date_at()` declaration |
| `src/masa.c` | Added `masa_for_date_at()`, static new moon cache |
| `src/solar.c` | Static sankranti + solar year caches, early exit in `sankranti_jd()` |

---

## Phase 2: Sunrise Pipeline + Deeper Caching (90 → 25 μs lunisolar)

Phase 1 eliminated redundant high-level calls. Phase 2 targeted the remaining
per-call cost of the Moshier ephemeris pipeline, particularly the sunrise
computation which dominates the per-day cost.

### Profiling: where time was spent after Phase 1

Per `gregorian_to_hindu()` call (~90 μs):
- `sunrise_jd(today)`: ~20 μs (1 call, 10 Meeus iterations)
- `sunrise_jd(yesterday)`: ~20 μs (adhika tithi check — redundant for sequential iteration)
- `tithi_at_moment()` × 2: ~2 μs (duplicate: once in panchang.c, again in masa_compute)
- `solar_rashi()` × 2: ~4 μs (always recomputed, even on new moon cache hit)
- New moon Lagrange (cache miss ~2/month): ~20 μs amortized to ~1.3 μs/day
- Remaining overhead: ~43 μs

The single biggest finding: each Meeus sunrise iteration called `moshier_solar_ra()`
and `moshier_solar_declination()` **independently**, each running the full VSOP87 +
nutation + obliquity pipeline. That's 2 full solar position computations × 10
iterations = **20 redundant VSOP87 evaluations per sunrise**.

### 6. Combined RA+Dec in sunrise (`lib/moshier/moshier_sun.c`, `lib/moshier/moshier_rise.c`)

Added `moshier_solar_ra_dec(jd_ut, &ra, &decl)` which computes both right ascension
and declination from a single `solar_position()` call. Previously each Meeus iteration
called:
- `moshier_solar_ra()` → VSOP87 + nutation + obliquity + aberration
- `moshier_solar_declination()` → VSOP87 + nutation + obliquity + aberration (again)

The combined function runs the VSOP87 + nutation pipeline once and derives both RA and
Dec from the same apparent longitude and true obliquity.

Also added `moshier_solar_ra_dec_nut()` which additionally returns the nutation
longitude (Δψ) and mean obliquity (ε₀), allowing the initial sidereal time computation
in `rise_set_for_date()` to share the same `solar_position()` call as the noon RA/Dec
estimate. This eliminated two additional calls to `moshier_nutation_longitude()` and
`moshier_mean_obliquity()`.

Internal refactoring: `solar_position()` gained an optional `true_obliq_out` parameter
(true obliquity in radians). The `mean_obliquity()` call is only made when `decl` or
`true_obliq_out` is requested, so pure longitude queries (e.g., `moshier_solar_longitude()`)
are not affected.

**Savings**: ~10 VSOP87+nutation evaluations per sunrise (50% of iteration cost).
Benefits all calendars that use sunrise/sunset: lunisolar, Tamil, Malayalam.

### 7. Cached previous-day sunrise+tithi (`src/panchang.c`)

The adhika tithi check in `gregorian_to_hindu()` requires yesterday's sunrise and tithi.
For sequential iteration (the dominant use case: panchang generation, validation, benchmarks),
yesterday's values were already computed on the previous call but discarded.

Added a static cache: `(jd_base, jd_rise, tithi)` keyed on the JD of the base date.
When `gregorian_to_hindu()` is called for day N, it saves today's sunrise and tithi.
On day N+1, the adhika check finds yesterday's data in the cache (O(1) lookup) instead
of recomputing `sunrise_jd()` + `tithi_num_at_jd()`.

For non-sequential access, the cache misses gracefully and falls back to full computation.

**Savings**: 1 sunrise + 1 lunar_phase eliminated per day (~22 μs) for sequential iteration.

### 8. Cached rashi alongside new moon cache (`src/masa.c`)

The new moon cache from Phase 1 already stores `(last_nm, next_nm)`. Phase 2 extends it
to also store `(rashi_last, rashi_next)`. When the cache hits (29/30 days per month),
both `solar_rashi()` calls are skipped — each would otherwise compute
`solar_longitude_sidereal()` (VSOP87 + ayanamsa) at the new moon JD.

**Savings**: 2 × `solar_longitude_sidereal()` eliminated for ~97% of days (~4 μs/day).

### 9. Reduced Lagrange interpolation from 17 to 9 points (`src/masa.c`)

`new_moon_before()` and `new_moon_after()` use inverse Lagrange interpolation on
`lunar_phase()` to find new moons. Phase 1 used 17 sample points at 0.25-day spacing
over a 4-day window. Phase 2 reduces to 9 points at 0.5-day spacing.

The tithi hint already places the estimate within ~1 day of the true new moon, so a
4-day window with 0.5-day spacing is sufficient. Validated by the full 1900-2050 roundtrip
test (1,868 months, all matching).

**Savings**: 8 fewer `lunar_phase()` calls per new moon search (each = `lunar_longitude()`
+ `solar_longitude()`). Only affects cache-miss days (~2/month), but each miss saves ~8 μs.

### Phase 2 files changed

| File | Change |
|------|--------|
| `lib/moshier/moshier_sun.c` | Added `moshier_solar_ra_dec()`, `moshier_solar_ra_dec_nut()`; `solar_position()` gains optional `true_obliq_out`; obliquity computed only when needed |
| `lib/moshier/moshier_rise.c` | `rise_set_for_date()` uses combined `moshier_solar_ra_dec_nut()` for initial estimate and `moshier_solar_ra_dec()` in iteration loop |
| `src/panchang.c` | Static cache for previous day's sunrise + tithi (adhika check) |
| `src/masa.c` | Extended new moon cache with rashi values; reduced Lagrange from 17→9 points |

---

## Remaining Cost Breakdown

After Phase 2, the per-day cost is dominated by irreducible ephemeris computation:

| Calendar | μs/day | Bottleneck |
|----------|--------|-----------|
| Lunisolar | 25 | 1 sunrise (~15 μs) + 1 lunar_phase (~2 μs) + masa overhead |
| Tamil | 19 | 1 sunset per day (critical time) + 1 solar_longitude_sidereal |
| Bengali | 4.2 | 1 solar_longitude_sidereal (no sunrise needed) |
| Odia | 3.8 | 1 solar_longitude_sidereal (no sunrise needed) |
| Malayalam | 34 | 1 sunrise + 1 sunset per day (madhyahna critical time) |

Further gains would require either:
- Caching sunrise/sunset across consecutive days in the solar calendar module
- Reducing the Meeus sunrise iteration count (currently ~3-4 iterations to converge)
- Approximating the VSOP87 solar position with a faster formula (trading accuracy)

These offer diminishing returns given the current sub-40 μs per-day cost.

## Why Bengali/Odia are fastest

Bengali and Odia critical times are pure arithmetic (midnight + offset and fixed 22:12
IST respectively) — no sunrise/sunset computation needed. With the sankranti cache
eliminating ~97% of bisections, nearly all ephemeris computation is skipped, leaving
only one `solar_longitude_sidereal()` call per day to determine the rashi.

Tamil and Malayalam are slower because they compute sunrise/sunset for every day's
critical time (sunset for Tamil, sunrise + 3/5 × daytime for Malayalam).
