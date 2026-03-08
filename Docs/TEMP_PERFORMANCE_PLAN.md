# Plan: Performance Optimizations

## Context

Benchmark results show significant room for improvement:
```
Lunisolar  : 55152 days in 57.596s  (1044.3 us/day)
Tamil      : 55152 days in 16.570s  (300.5 us/day)
Bengali    : 55152 days in 12.076s  (219.0 us/day)
Odia       : 55152 days in 11.315s  (205.2 us/day)
Malayalam  : 55152 days in 26.000s  (471.4 us/day)
Total      : 275760 calls in 123.557s
```

### Root cause analysis

**Lunisolar (1044 us/day)** — dominated by redundant ephemeris computation:
- `tithi_at_sunrise()` always computes tithi boundaries via 2×50-iteration bisection
  (100 `lunar_phase()` calls each = 200 total), even though `gregorian_to_hindu()`
  only uses the tithi *number*, not jd_start/jd_end
- Same sunrise computed 3× independently: `tithi_at_sunrise(today)`, `masa_for_date()`,
  and `tithi_at_sunrise(yesterday)` for the adhika check
- `masa_for_date()` independently searches for new moons via 2×17 Lagrange interpolation
  points, even when consecutive days bracket the same pair of new moons

**Solar (205–471 us/day)** — dominated by redundant sankranti bisection:
- `gregorian_to_solar()` runs `sankranti_jd()` (50 VSOP87 iterations) to find the current
  month's sankranti, even when consecutive days are in the same solar month (~30/31 times)
- `solar_year()` runs *another* `sankranti_jd()` for the year-start sankranti, same for
  all days in same Gregorian year
- Malayalam additionally computes both `sunrise_jd()` + `sunset_jd()` for critical time

## Implementation

### 1. Add lightweight tithi query (`src/tithi.c`, `src/tithi.h`)

Add `tithi_num_at_sunrise()` — returns only tithi number at sunrise (no boundary
finding, no kshaya check). This avoids 200 `lunar_phase()` calls per invocation.

```c
int tithi_num_at_sunrise(double jd_sunrise);
// Just: tithi_at_moment(jd_sunrise)
```

Modify `gregorian_to_hindu()` in `src/panchang.c` to use this instead of full
`tithi_at_sunrise()`. The adhika check (previous day) also only needs the tithi
number. The kshaya check also only needs tithi numbers.

**Expected savings**: ~400 lunar_phase calls/day → ~60% of lunisolar time.

### 2. Cache sunrise in `gregorian_to_hindu()` (`src/panchang.c`)

Compute sunrise once and pass it through. Currently 3 independent `sunrise_jd()`
calls per `gregorian_to_hindu()`:
- `tithi_at_sunrise(today)` — line 25
- `masa_for_date()` — line 28 (via its own `sunrise_jd()`)
- `tithi_at_sunrise(yesterday)` for adhika check — line 40

Inline the logic in `gregorian_to_hindu()`: compute sunrise once for today,
call `tithi_at_moment()` directly. For masa, add `masa_for_date_at()` that
takes a pre-computed sunrise JD.

For the adhika check (previous day), we still need yesterday's sunrise — one
additional sunrise call. But we save 2 of the 3 redundant ones for today.

**Expected savings**: ~2 sunrise computations/day → ~5-10%.

### 3. Static new moon cache (`src/masa.c`)

Add a simple static cache for the last (last_nm, next_nm) pair. When consecutive
days fall between the same new moons (~29/30 times), skip the Lagrange search.

```c
static double cached_last_nm = 0, cached_next_nm = 0;
// In masa_for_date: if jd_rise > cached_last_nm && jd_rise < cached_next_nm → reuse
```

**Expected savings**: ~90% of masa_for_date new moon searches eliminated.

### 4. Static sankranti cache (`src/solar.c`)

Cache the last (rashi, sankranti_jd) and (year, solar_year) results. When
consecutive days share the same rashi (~30/31 times), skip the 50-iteration
bisection.

```c
static int cached_rashi = 0;
static double cached_sank_jd = 0;
// In gregorian_to_solar: if current rashi == cached_rashi → reuse cached_sank_jd
```

Same for `solar_year()`: cache (gregorian_year, calendar_type, result).

**Expected savings**: ~90% of sankranti bisections eliminated → ~50-60% of solar time.

### 5. Early termination in bisection (`src/tithi.c`, `src/solar.c`)

For `find_tithi_boundary()` and `sankranti_jd()`, add early exit when
`|hi - lo|` converges below a threshold (1 second = ~1.16e-5 days). The
current 50 iterations give nanosecond precision; sub-second is sufficient.

Tithi boundaries: ~25 iterations sufficient (1 second precision).
Sankranti: ~30 iterations sufficient (millisecond precision for critical time comparison).

This only helps the full `tithi_at_sunrise()` path (used by `generate_month_panchang()`),
since `gregorian_to_hindu()` will skip boundaries after optimization #1.

**Expected savings**: ~40-50% of remaining bisection time.

## Files

| File | Change |
|------|--------|
| `src/tithi.h` | Add `tithi_num_at_sunrise()` declaration |
| `src/tithi.c` | Add `tithi_num_at_sunrise()`, early exit in `find_tithi_boundary()` |
| `src/panchang.c` | Rewrite `gregorian_to_hindu()` to use lightweight tithi + cached sunrise |
| `src/masa.h` | Add `masa_for_date_at()` taking pre-computed sunrise JD |
| `src/masa.c` | Add `masa_for_date_at()`, static new moon cache |
| `src/solar.c` | Static sankranti + solar_year caches, early exit in `sankranti_jd()` |

## Verification

1. `make test` — all existing 53,143 assertions must still pass (correctness unchanged)
2. `make bench` — compare before/after timings
3. Target: lunisolar < 300 us/day (~3.5×), solar < 100 us/day (~2-4×), total < 40s (~3×)
