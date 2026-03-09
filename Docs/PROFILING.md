# Profiling Results

Profiled with macOS `sample` tool (1ms sampling interval, 3 seconds) on the
full benchmark: 275,760 calendar conversions (55,152 days × 5 calendars).

Benchmark timing: Lunisolar 26.6 μs, Tamil 18.9 μs, Bengali 4.4 μs, Odia 4.0 μs, Malayalam 32.9 μs.

## Top-level split

| Subtree | Samples | % | What |
|---------|---------|---|------|
| `gregorian_to_solar` | 1,751 | 73.9% | All 4 solar calendars combined |
| `gregorian_to_hindu` | 616 | 26.0% | Lunisolar calendar |

## Where time is actually spent

The dominant cost is `solar_position()` (the VSOP87 pipeline):

| Function | Inclusive samples | % | Role |
|----------|-----------------|---|------|
| `solar_position` | ~1,500 | ~63% | VSOP87 harmonics + nutation + aberration |
| `nutation` | ~300 | ~13% | 13-term nutation series (within solar_position) |
| `rise_set` / `rise_set_for_date` | ~700 | ~30% | Sunrise/sunset via Meeus iteration (each iter calls solar_position) |
| `solar_longitude_sidereal` | ~540 | ~23% | 1 solar_position + ayanamsa per call |
| `moshier_solar_ra_dec` | ~500 | ~21% | Combined RA+Dec (iteration loop in sunrise) |
| `moshier_solar_ra_dec_nut` | ~160 | ~7% | Initial noon estimate (1 per sunrise) |
| `lunar_phase` / `moshier_lunar_longitude` | ~56 | ~2.4% | Moon pipeline (DE404) |
| `delta_t_seconds` | ~44 | ~1.9% | Delta-T lookup (per solar_position call) |
| `moshier_ayanamsa` | ~20 | ~0.8% | Lahiri ayanamsa (precession) |

Note: percentages overlap because the call tree is nested (e.g., `rise_set` calls
`solar_position` which calls `nutation`).

## Key takeaways

1. **VSOP87 harmonics dominate** — `solar_position()` is ~63% of total time.
   The trig evaluations (`sincos`, `sin`, `cos`, `fmod`) inside the 135-term
   VSOP87 loop are the irreducible core.

2. **Sunrise is the single most expensive operation** — `rise_set` accounts for
   ~30% of total time, because each sunrise/sunset requires ~3-4 Meeus iterations,
   each calling `solar_position()`.

3. **Moon is cheap** — `lunar_phase` + `moshier_lunar_longitude` is only 2.4%.
   The new moon cache means the DE404 pipeline rarely runs.

4. **The Phase 2 optimizations hit the right targets** — combining RA+Dec
   eliminated ~50% of sunrise cost, and the caches eliminated most lunar/rashi
   redundancy.

5. **Further gains would require** reducing the number of `solar_position()` calls
   (fewer Meeus iterations) or using a faster solar longitude approximation. Both
   trade accuracy for speed — diminishing returns territory given the sub-35 μs
   per-day cost.

## Leaf (self-time) profile

Functions where the CPU was actually executing (not just calling children):

| Samples | % | Function |
|---------|---|----------|
| 238 | 10.1% | `solar_position` (VSOP87 arithmetic) |
| 101 | 4.3% | `__sincos_stret` (libm sin/cos) |
| 81 | 3.4% | `nutation` (13-term series) |
| 67 | 2.8% | `fmod` (libm modular arithmetic) |
| 44 | 1.9% | `delta_t_seconds` (Delta-T lookup) |
| 43 | 1.8% | `sin` (libm) |
| 43 | 1.8% | `moshier_revjul` (JD→Gregorian) |
| 34 | 1.4% | `rise_set_for_date` (Meeus iteration overhead) |
| 29 | 1.2% | `lunar_phase` (moon-sun elongation) |
| 27 | 1.1% | `moshier_lunar_longitude` (DE404 pipeline) |
| 26 | 1.1% | `accum_series` (DE404 table accumulation) |

The top 3 leaf functions (`solar_position` + `sincos` + `nutation`) account for
~18% of self-time — all inside the VSOP87 solar longitude pipeline.

## Full call tree

Raw output from `sample` (2,368 samples at 1ms intervals). Numbers are inclusive
sample counts. The `+ ! : |` characters are tree connectors showing nesting depth.

```
2368 Thread_12741959   DispatchQueue_1: com.apple.main-thread  (serial)
  2368 start  (in dyld)
    1751 main  (in test_perf)  test_perf.c:55                    ← solar calendars loop
    + 642 gregorian_to_solar  solar.c:498                        ← critical_time (sunrise/sunset)
    + ! 350 rise_set+276  moshier_rise.c:169                     ← rise_set_for_date (1st attempt)
    + ! : 248 rise_set_for_date+565  moshier_rise.c:113          ← Meeus iteration loop
    + ! : | 240 moshier_solar_ra_dec  moshier_sun.c:773          ← combined RA+Dec per iter
    + ! : | + 140 solar_position  moshier_sun.c:682              ← VSOP87 harmonics
    + ! : | + 30 solar_position  moshier_sun.c:700               ← nutation call
    + ! : | + ! 23 nutation+502  moshier_sun.c:630
    + ! : | + ! : 23 __sincos_stret  (libm)
    + ! : | + ! 2 nutation  moshier_sun.c:628
    + ! : | + ! 2 nutation  moshier_sun.c:635
    + ! : | + ! 1 nutation  moshier_sun.c:591
    + ! : | + ! 1 nutation  moshier_sun.c:629
    + ! : | + ! 1 nutation  moshier_sun.c:631
    + ! : | + 19 solar_position+260  moshier_sun.c:682
    + ! : | + ! 19 __sincos_stret  (libm)
    + ! : | + 8 solar_position  moshier_sun.c:692                ← VSOP87 term arithmetic
    + ! : | + 7 solar_position+51  moshier_sun.c:678             ← delta-T
    + ! : | + ! 6 delta_t_seconds  moshier_sun.c:545
    + ! : | + ! : 2 moshier_revjul  moshier_jd.c:34
    + ! : | + ! : 1 moshier_revjul  moshier_jd.c:35
    + ! : | + ! : 1 moshier_revjul  moshier_jd.c:39
    + ! : | + ! : 1 moshier_revjul  moshier_jd.c:41
    + ! : | + ! 1 delta_t_seconds  moshier_sun.c:565
    + ! : | + 4 asin  (libm)
    + ! : | + 3 solar_position  moshier_sun.c:692  → sincos
    + ! : | + 3 solar_position  moshier_sun.c:692  → sin
    + ! : | + 3 solar_position  moshier_sun.c:712
    + ! : | + 2 solar_position  moshier_sun.c:692  → fmod (×3)
    + ! : | + 2 solar_position  moshier_sun.c:692  → sincos
    + ! : | + 2 solar_position  moshier_sun.c:692  → sin
    + ! : | + 2 solar_position  moshier_sun.c:717  → sin (×2)
    + ! : | + 5 solar_position  (leaf, various lines)
    + ! : | 5 moshier_solar_ra_dec+82  → atan2  (libm)
    + ! : | 1 moshier_solar_ra_dec  → sincos  (libm)
    + ! : | 1 moshier_solar_ra_dec  → fmod  (libm)
    + ! : | 1 moshier_solar_ra_dec  (leaf)
    + ! : 91 rise_set_for_date+191  moshier_rise.c:77            ← initial noon estimate
    + ! : | 83 moshier_solar_ra_dec_nut  moshier_sun.c:759
    + ! : | + 58 solar_position  moshier_sun.c:682               ← VSOP87 harmonics
    + ! : | + 14 solar_position  moshier_sun.c:700               ← nutation
    + ! : | + ! 7 nutation+502  → sincos
    + ! : | + ! 3 nutation  moshier_sun.c:628
    + ! : | + ! 1 nutation  moshier_sun.c:596
    + ! : | + ! 1 nutation  moshier_sun.c:627
    + ! : | + ! 1 nutation  moshier_sun.c:629
    + ! : | + ! 1 nutation  moshier_sun.c:631
    + ! : | + 3 solar_position  moshier_sun.c:692  → sin
    + ! : | + 2 solar_position+260  → sincos
    + ! : | + 2 solar_position  moshier_sun.c:692  (leaf)
    + ! : | + 1 solar_position  → fmod (×2)
    + ! : | + 1 solar_position  → sincos (×2)
    + ! : | + 1 solar_position  moshier_sun.c:712
    + ! : | 5 moshier_solar_ra_dec_nut+205  moshier_sun.c:768   ← mean_obliquity (delta-T)
    + ! : | + 2 delta_t_seconds  → moshier_revjul
    + ! : | + 2 delta_t_seconds  moshier_sun.c:548
    + ! : | + 1 delta_t_seconds  moshier_sun.c:546
    + ! : | 1 moshier_solar_ra_dec_nut  → cos
    + ! : | 1 moshier_solar_ra_dec_nut  → atan2
    + ! : | 1 moshier_solar_ra_dec_nut  → fmod
    + ! : | 1 moshier_solar_ra_dec_nut  (leaf)
    + ! : 3 rise_set_for_date+775  → sin                        ← Meeus correction arithmetic
    + ! : 3 rise_set_for_date+140  → fmod
    + ! : 1 asin  (libm)
    + ! : 1 rise_set_for_date  → fmod
    + ! : 1 rise_set_for_date  → cos
    + ! : 1 rise_set_for_date  (leaf, ×3)
    + ! 291 rise_set+339  moshier_rise.c:175                     ← rise_set_for_date (2nd/next day)
    + ! : 211 rise_set_for_date+565  moshier_rise.c:113          ← Meeus iteration loop
    + ! : | 204 moshier_solar_ra_dec  moshier_sun.c:773
    + ! : | + 140 solar_position  moshier_sun.c:682              ← VSOP87
    + ! : | + 22 solar_position  moshier_sun.c:700               ← nutation
    + ! : | + ! 15 nutation  → sincos
    + ! : | + ! 3 nutation  moshier_sun.c:628
    + ! : | + ! 1 nutation  moshier_sun.c:627
    + ! : | + ! 1 nutation  moshier_sun.c:629
    + ! : | + ! 1 nutation  moshier_sun.c:631
    + ! : | + ! 1 nutation  moshier_sun.c:635
    + ! : | + 10 solar_position+260  → sincos
    + ! : | + 5 solar_position  moshier_sun.c:692  → sin
    + ! : | + 4 solar_position  moshier_sun.c:692  (leaf)
    + ! : | + 3 solar_position+51  → delta_t_seconds → moshier_revjul
    + ! : | + 3 solar_position  moshier_sun.c:692  → sincos
    + ! : | + 2 asin  (libm)
    + ! : | + 2 solar_position  → fmod (×2)
    + ! : | + 2 solar_position  → sincos
    + ! : | + 2 solar_position  → sin
    + ! : | + 6 solar_position  (leaf, various lines)
    + ! : | 4 moshier_solar_ra_dec  → atan2
    + ! : | 2 moshier_solar_ra_dec  (leaf)
    + ! : | 1 moshier_solar_ra_dec  (leaf)
    + ! : 72 rise_set_for_date+191  moshier_rise.c:77            ← initial noon estimate
    + ! : | 67 moshier_solar_ra_dec_nut  moshier_sun.c:759
    + ! : | + 45 solar_position  moshier_sun.c:682
    + ! : | + 8 solar_position  moshier_sun.c:700  → nutation
    + ! : | + ! 6 nutation  → sincos
    + ! : | + ! 1 nutation  moshier_sun.c:629
    + ! : | + ! 1 nutation  moshier_sun.c:631
    + ! : | + 4 solar_position+51  → delta_t_seconds → moshier_revjul
    + ! : | + 2 solar_position+260  → sincos
    + ! : | + 2 solar_position  → fmod
    + ! : | + 2 solar_position  (leaf)
    + ! : | + 1 solar_position  → fmod (×2)
    + ! : | + 1 solar_position  → sincos (×2)
    + ! : | + 1 solar_position  → sin
    + ! : | 1 moshier_solar_ra_dec_nut  → sincos
    + ! : | 1 moshier_solar_ra_dec_nut  → atan2
    + ! : | 1 moshier_solar_ra_dec_nut  → fmod
    + ! : | 1 moshier_solar_ra_dec_nut  → delta_t_seconds → moshier_revjul
    + ! : | 1 moshier_solar_ra_dec_nut  (leaf)
    + ! : 1 asin  (libm)
    + ! : 1 fmod  (libm)
    + ! : 1 rise_set_for_date  → fmod (×2)
    + ! : 1 rise_set_for_date  → cos
    + ! : 1 rise_set_for_date  → sin
    + ! : 1 rise_set_for_date  (leaf, ×2)
    + ! 1 rise_set+232  → moshier_revjul
    + 290 gregorian_to_solar+823  solar.c:532                    ← sankranti rashi check
    + ! 270 solar_longitude_sidereal  astro.c:136
    + ! : 165 solar_position  moshier_sun.c:682                  ← VSOP87
    + ! : 43 solar_position  moshier_sun.c:700                   ← nutation
    + ! : | 27 nutation  → sincos
    + ! : | 6 nutation  moshier_sun.c:628
    + ! : | 5 nutation  moshier_sun.c:631
    + ! : | 2 nutation  moshier_sun.c:627
    + ! : | 2 nutation  moshier_sun.c:629
    + ! : | 1 nutation  moshier_sun.c:594
    + ! : 24 solar_position+260  → sincos
    + ! : 6 solar_position+51  → delta_t_seconds → moshier_revjul
    + ! : 6 solar_position  moshier_sun.c:692  (leaf)
    + ! : 5 solar_position  moshier_sun.c:692  → sin
    + ! : 5 solar_position  moshier_sun.c:692  → fmod
    + ! : 3 solar_position  → fmod (×3)
    + ! : 2 solar_position  → fmod (×2)
    + ! : 2 solar_position  → sincos
    + ! : 2 solar_position  → sin
    + ! : 3 solar_position  (leaf, various)
    + ! 19 solar_longitude_sidereal+33  astro.c:137              ← ayanamsa
    + ! : 8 moshier_ayanamsa  moshier_ayanamsa.c:121
    + ! : | 7 moshier_delta_t  → delta_t_seconds → moshier_revjul
    + ! : | 1 moshier_delta_t  (leaf)
    + ! : 4 moshier_ayanamsa  moshier_ayanamsa.c:127  (leaf)
    + ! : 3 moshier_ayanamsa  → sincos
    + ! : 3 moshier_ayanamsa  → atan2
    + ! : 1 moshier_ayanamsa  (leaf)
    + ! 1 solar_longitude_sidereal  → fmod
    + 276 gregorian_to_solar+231  solar.c:501                    ← main rashi check
    + ! 256 solar_longitude_sidereal  astro.c:136
    + ! : 163 solar_position  moshier_sun.c:682                  ← VSOP87
    + ! : 36 solar_position  moshier_sun.c:700                   ← nutation
    + ! : | 26 nutation  → sincos
    + ! : | 3 nutation  moshier_sun.c:631
    + ! : | 2 nutation  moshier_sun.c:627
    + ! : | 2 nutation  moshier_sun.c:628
    + ! : | 2 nutation  moshier_sun.c:629
    + ! : | 1 nutation  moshier_sun.c:594
    + ! : 22 solar_position+260  → sincos
    + ! : 13 solar_position+51  → delta_t_seconds → moshier_revjul
    + ! : 4 solar_position  moshier_sun.c:692  (leaf)
    + ! : 3 solar_position  → sincos
    + ! : 2 solar_position  → fmod (×3)
    + ! : 2 solar_position  → sin
    + ! : 5 solar_position  moshier_sun.c:692  → sin/fmod
    + ! : 3 solar_position  (leaf, various)
    + ! 19 solar_longitude_sidereal+33  astro.c:137              ← ayanamsa
    + ! : 8 moshier_ayanamsa  → moshier_delta_t → delta_t_seconds
    + ! : 3 moshier_ayanamsa  (leaf)
    + ! : 3 moshier_ayanamsa  → atan2
    + ! : 2 moshier_ayanamsa  → sincos
    + ! : 1 moshier_ayanamsa  → sincos
    + ! : 1 moshier_ayanamsa  (leaf)
    + ! : 1 moshier_ayanamsa  (leaf)
    + ! 1 solar_longitude_sidereal  → fmod
    + 254 gregorian_to_solar+138  solar.c:498                    ← another critical_time branch
    + ! 135 rise_set+276  moshier_rise.c:169
    + ! : 97 rise_set_for_date+565  moshier_rise.c:113           ← Meeus iteration
    + ! : | 97 moshier_solar_ra_dec  moshier_sun.c:773
    + ! : |   70 solar_position  moshier_sun.c:682
    + ! : |   6 solar_position+260  → sincos
    + ! : |   4 solar_position  → fmod
    + ! : |   4 solar_position  moshier_sun.c:700  → nutation → sincos
    + ! : |   3 solar_position  → fmod
    + ! : |   2 solar_position+51  → delta_t_seconds
    + ! : |   2 solar_position  → sincos
    + ! : |   5 solar_position  (leaf/fmod/sin, various)
    + ! : 37 rise_set_for_date+191  moshier_rise.c:77            ← initial noon
    + ! : | 32 moshier_solar_ra_dec_nut  moshier_sun.c:759
    + ! : | + 21 solar_position  moshier_sun.c:682
    + ! : | + 4 solar_position  → nutation → sincos
    + ! : | + 2 solar_position+51  → delta_t_seconds
    + ! : | + 3 solar_position  (leaf/fmod/sincos/sin, various)
    + ! : | 2 moshier_solar_ra_dec_nut  → delta_t_seconds → moshier_revjul
    + ! : | 2 moshier_solar_ra_dec_nut  (leaf)
    + ! : | 1 moshier_solar_ra_dec_nut  → atan2
    + ! : 1 rise_set_for_date  (leaf)
    + ! 118 rise_set+339  moshier_rise.c:175                     ← 2nd attempt (next day)
    + !   (same structure as above, ~80 iteration + ~37 initial)
    + 69 gregorian_to_solar  (leaf/overhead, various)
    + 10 gregorian_to_solar  (misc)
    616 main  (in test_perf)  test_perf.c:57                     ← lunisolar calendar
    + 329 gregorian_to_hindu  panchang.c:32
    + ! 166 rise_set+339  moshier_rise.c:175                     ← today's sunrise
    + ! : (same rise_set_for_date structure as solar)
    + ! 75 masa_compute  masa.c                                  ← month determination
    + ! : (tithi_at_moment, new_moon cache, solar_rashi)
    + ! 57 rise_set+276  moshier_rise.c:169
    + ! : (sunrise for yesterday — cache miss path)
    + ! 20 tithi_num_at_jd  tithi.c                              ← lightweight tithi
    + ! 11 lunar_phase  astro.c                                  ← moon-sun elongation
    + 287 gregorian_to_hindu  (continued)
    +   (remaining: prev-day cache hits, masa overhead, year calc)
    1 main  test_perf.c:56                                       ← timing overhead
```
