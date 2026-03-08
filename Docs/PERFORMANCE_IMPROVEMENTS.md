# Performance Improvements

## Results

**10.1x overall speedup** (123.6s → 12.2s for 275,760 calendar conversions):

| Calendar | Before (us/day) | After (us/day) | Speedup |
|----------|-----------------|----------------|---------|
| Lunisolar | 1,044 | 90 | 11.6x |
| Tamil | 301 | 40 | 7.6x |
| Bengali | 219 | 5.0 | 43.6x |
| Odia | 205 | 4.5 | 45.3x |
| Malayalam | 471 | 83 | 5.7x |
| **Total** | **123.6s** | **12.2s** | **10.1x** |

Zero regressions: all 53,286 test assertions pass, all 7,244 solar month starts and
55,152 lunisolar days byte-identical to pre-optimization reference data.

## Optimizations

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

## Files Changed

| File | Change |
|------|--------|
| `src/tithi.h` | Added `tithi_num_at_jd()` declaration |
| `src/tithi.c` | Added `tithi_num_at_jd()`, early exit in `find_tithi_boundary()` |
| `src/panchang.c` | Rewrote `gregorian_to_hindu()`: cached sunrise, lightweight tithi, `masa_for_date_at()` |
| `src/masa.h` | Added `masa_for_date_at()` declaration |
| `src/masa.c` | Added `masa_for_date_at()`, static new moon cache |
| `src/solar.c` | Static sankranti + solar year caches, early exit in `sankranti_jd()` |

## Why Bengali/Odia are fastest

Bengali and Odia critical times are pure arithmetic (midnight + offset and fixed 22:12
IST respectively) — no sunrise/sunset computation needed. With the sankranti cache
eliminating ~97% of bisections, nearly all ephemeris computation is skipped, leaving
only one `solar_longitude_sidereal()` call per day to determine the rashi.

Tamil and Malayalam are slower because they compute sunrise/sunset for every day's
critical time (sunset for Tamil, sunrise + 3/5 × daytime for Malayalam).
