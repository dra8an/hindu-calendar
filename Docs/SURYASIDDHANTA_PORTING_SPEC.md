# Porting the Suryasiddhanta Bengali Panjika to another C/C++ project

The target project is C/C++ and under the same ownership, so **copy the code
directly** — this is not a reimplementation exercise. This document says which
files to take, what they depend on, how to hook them up, and how to prove the
port is correct against the same reference data used here.

The specification sections (§5–§7) are kept for debugging and for anyone who
needs to understand *why* the code does what it does, but you should not need
to write any of it from scratch.

**Result to expect:** 1,812 / 1,812 (100.000%) of drikpanchang.com's published
Bengali Suryasiddhanta month starts, 1900–2050.

---

## 1. Files to copy

Everything lives in this repository. Paths are relative to its root.

### 1.1 The engine — copy as-is, no changes needed

| File | Lines | Purpose |
|------|-------|---------|
| `src/surya_siddhanta.h` | 96 | Public API |
| `src/surya_siddhanta.c` | 324 | The whole Surya Siddhanta model |

**Dependency surface is almost nothing.** `surya_siddhanta.c` includes exactly
two things: its own header, and `<math.h>`. The header includes `types.h` but
uses only one type from it — `Location`. So the entire dependency is:

```c
typedef struct {
    double latitude;       /* degrees N */
    double longitude;      /* degrees E  <-- the only field the engine reads */
    double altitude;       /* metres (unused by the engine) */
    double utc_offset;     /* hours east of UTC */
} Location;
```

If the target project already has an equivalent struct, point the header at it
and delete the `types.h` include. The engine reads only `longitude`.

No ephemeris, no data files, no ayanamsa, no tables to generate. It is pure
arithmetic over compile-time constants.

API provided:

```c
double surya_solar_longitude(double jd_ut, const Location *loc);
double surya_lunar_longitude(double jd_ut, const Location *loc);
double surya_lunar_phase   (double jd_ut, const Location *loc);
int    surya_tithi_at      (double jd_ut, const Location *loc);
double surya_tithi_end     (double jd_ut, const Location *loc);
double surya_sankranti     (double jd_approx, double target_longitude,
                            const Location *loc);
```

`jd_ut` is a Julian Day in UT, noon-based (JD *x*.5 is midnight UT).

### 1.2 The day-assignment rule — two ways to take it

The engine gives you sankranti *moments*. Turning those into calendar *days*
is a separate rule, and you have a choice:

**Option A — take the standalone version (recommended to start).**

| File | Lines | Purpose |
|------|-------|---------|
| `tools/surya_bengali_check.c` | 164 | Self-contained rule + validation harness |

This file contains the complete rule in about 30 lines (`predict_start()`)
plus a CSV loader and a comparison loop. It has no dependency on this
project's calendar layer at all — only on the engine, plus Julian Day
conversion and sunrise. It is the fastest path to a working, *verified* port:
copy it, point it at your own JD/sunrise functions, run it, expect 1812/1812.

Lift `predict_start()` into your own calendar layer once it passes.

**Option B — take the integrated version.**

If the target project has a solar-calendar layer resembling this one, the rule
is already factored into named functions in `src/solar.c`:

| Function | Line | What to take |
|---|---|---|
| `is_surya()` / `is_bengali()` | 129, 134 | Type predicates |
| `arith_solar_longitude()` | 139 | Dispatch: Surya vs drik sun |
| `arith_sankranti()` | 149 | Dispatch: Surya vs drik sankranti |
| `critical_time_jd()` | 168 | The `SOLAR_CAL_BENGALI_SURYA` arm: midnight + 40 min |
| `bengali_day_edge_offset()` | 349 | The `is_surya()` branch: per-rashi edges |
| `bengali_tithi_push_next()` | 434 | The `is_surya()` branch: tithi tie-break |
| `sankranti_to_civil_day()` | 462 | The flow that ties them together |

Plus the `SOLAR_CAL_BENGALI_SURYA` enum value in `src/types.h` and its row in
`SOLAR_CONFIGS` in `solar.c`.

Note that both Bengali variants share every rule; they differ only in which
astronomy runs. That difference is deliberately confined to
`arith_solar_longitude()` and `arith_sankranti()`, so the rest of the calendar
layer needed no conditionals.

### 1.3 Verification — copy this too

| File | Purpose |
|------|---------|
| `validation/drikpanchang/bengali_suryasiddhanta.csv` | **The reference data.** 1,812 month starts, 1900–2050, scraped from drikpanchang.com |
| `tools/surya_bengali_check.c` | Standalone checker (see above) |
| `tests/test_surya_bengali.c` | The same check through a calendar-layer API, 3,624 assertions |

The CSV is ~53 KB. **Copy it into the target project** — it took nine VPN
cycles to obtain (drikpanchang rate-limits at 200 requests per IP) and it is
the only thing that proves the port is right.

Format:

```
month,year,length,greg_year,greg_month,greg_day,month_name
4,1433,31,2026,7,18,Srabon
```

`month` is 1–12 Bengali, `year` is Bangabda, `greg_*` is the first civil day of
that month.

---

## 2. What the host project must already provide

Only two things:

| Requirement | Used for |
|---|---|
| **Julian Day ↔ Gregorian** | Everywhere |
| **Sunrise** for a location, returning JD (UT) | The tithi tie-break, §6.3 |

Sunrise must be a real astronomical sunrise, not a Surya Siddhanta mean
sunrise — see §6.3. Ours is upper-limb (Sinclair refraction + 16′ semi-diameter,
h₀ ≈ −0.879°), pressure-adjusted for altitude, no horizon dip. If the target's
sunrise differs by more than ~30 seconds, expect a few midnight-boundary cases
to flip.

---

## 3. Build and verify

Fastest route to a verified port:

```bash
# 1. Copy in
cp src/surya_siddhanta.{c,h}                              <target>/src/
cp tools/surya_bengali_check.c                            <target>/tools/
cp validation/drikpanchang/bengali_suryasiddhanta.csv     <target>/validation/

# 2. Point surya_bengali_check.c at the target's own JD + sunrise headers,
#    and fix CSV path at the top of the file.

# 3. Build (this project's line; adapt include paths)
cc -O2 -std=c99 -Isrc -o surya_bengali_check \
   tools/surya_bengali_check.c src/surya_siddhanta.c \
   <target's jd + sunrise objects> -lm

# 4. Run
./surya_bengali_check
```

Expected output:

```
drikpanchang month starts loaded: 1812

matched 1812 / 1812  (100.000%)   missed 0
```

Any miss prints the rashi and the sankranti timestamp, which tells you
immediately whether it is an astronomy problem (§8) or a rule problem (§6).

**C++ note:** the code is C99, and both `src/surya_siddhanta.c` and
`tools/surya_bengali_check.c` compile clean under `c++ -std=c++17 -Wall
-Wextra` with no changes (verified). Rename to `.cpp` or pass `-x c++` to
silence the "treating 'c' input as 'c++'" warning. No `void*` casts are needed
— the checker only compares `bsearch`'s result against `NULL`.

---

## 4. Verify as a set equality, not a subset test

`tools/surya_bengali_check.c` as written asks "is the predicted start present
in the reference set?" That is a subset test, and it can report a **false
100%**: if two sankrantis collapse onto the same date while another reference
date goes unmatched, the count still comes out right.

We checked this here and the mapping is a genuine bijection — 1,812 predicted,
1,812 reference, zero duplicates, zero unmatched either way. Re-run that check
after porting rather than trusting the match count:

```c
/* after collecting predicted starts, sorted */
assert(n_predicted == n_reference);
for (i = 1; i < n_predicted; i++) assert(pred[i] != pred[i-1]);   /* no dups */
for (i = 0; i < n_predicted; i++) assert(pred[i] == ref[i]);      /* exact */
```

---

## 5. The model, for reference

Only needed if you are debugging or reimplementing rather than copying.

### 5.1 Constants

Everything derives from five integers in the Surya Siddhanta, chapter 1:

```
solar_revs_per_mahayuga        =    4320000
lunar_revs_per_mahayuga        =   57753336
civil_days_per_mahayuga        = 1577917828
solar_apogee_revs_per_kalpa    =        387     (kalpa = 1000 mahayugas)
lunar_apogee_revs_per_mahayuga =     488199

sidereal_year     = civil_days / solar_revs                   = 365.2587564814815
anomalistic_year  = civil_days*1000 / (solar_revs*1000 - 387) = 365.2587892025813
sidereal_month    = civil_days / lunar_revs                   =  27.3216741626839
anomalistic_month = civil_days / (lunar_revs - lunar_apogee)  =  27.5545979746805

epicycles (chapter 2), varying between odd and even quadrant values:
  Sun   14° ↔ 13°40'   ->  size 14/360, contraction 1/42
  Moon  32° ↔ 31°40'   ->  size 32/360, contraction 1/96

epoch = Kali Yuga, Julian -3102 Feb 18 = RD -1132959;  rd = jd - 1721424.5
  mean sun and mean moon are both 0° here, by definition
anomaly at epoch: solar 3143/4000 turn, lunar 3/4 turn
```

Sanity check: `360 × (1 − 3143/4000) = 77.13°` is the implied solar apogee, and
the text states ~77°. Mistyped constants will not land there.

### 5.2 The sine table

Chapter 2, 24 values at 225 arcmin (3.75°) intervals, units of **R = 3438**.
Stated as differences, which sum to exactly 3438 — your transcription check:

```
225 224 222 219 215 210 205 199 191 183 174 164
154 143 131 119 106  93  79  65  51  37  22   7
```

Do **not** substitute `round(3438·sin θ)`. It differs at five entries:

| Index | Angle | Table | `round(3438·sin θ)` |
|---|---|---|---|
| 6 | 22.50° | 1315 | 1316 |
| 7 | 26.25° | 1520 | 1521 |
| 16 | 60.00° | 2978 | 2977 |
| 17 | 63.75° | 3084 | 3083 |
| 18 | 67.50° | 3177 | 3176 |

One unit of 1/3438 is ~1 arcminute, ~2 minutes of solar motion — enough to flip
a midnight-boundary case.

The table covers the first quadrant; the anomaly sweeps all four, so the index
is reflected and the sign carried (`ss_sine_entry()` in the engine).

### 5.3 Positions

```
days   = (jd_ut - 1721424.5 + longitude/360) - (-1132959)

true_position(days, period, anom_period, anom_at_epoch, size, contraction):
    mean     = 360 * frac(days / period)
    anomaly  = 360 * frac(days / anom_period + anom_at_epoch)
    offset   = sine(anomaly)
    shrunk   = size - |offset| * contraction * size
    equation = arcsin(offset * shrunk)
    return (mean - equation) mod 360
```

---

## 6. The day-assignment rule

```
month_start_day(jd_sankranti, rashi, loc):
    local   = jd_sankranti + utc_offset/24 + 0.5      # midnight-based local
    shifted = local + day_edge(rashi)/1440
    D       = floor(shifted)
    crit    = D + 40/1440                             # midnight + 40 min

    if local > crit: return D + 1

    if rashi == 4:    push = false                    # Karkata: always this day
    elif rashi == 10: push = true                     # Makara: always next day
    else:
        sr   = sunrise(D - 1, loc)
        push = (surya_tithi_end(sr, loc) <= jd_sankranti)

    return D + (push ? 1 : 0)
```

### 6.1 Fitted constants

```
critical time = midnight + 40 minutes

day_edge(rashi), minutes BEFORE midnight:
    1 Mesha       7        7 Tula        20
    2 Vrishabha  11        8 Vrishchika  22
    6 Kanya      12        9 Dhanu       19
    all others    0
```

Fitted against all 1,812 month starts. Admissible windows are 8–11 minutes wide
(rashi 7 works anywhere in 15–25, rashi 8 in 17–27), so these are mid-window,
not knife-edge. Out-of-sample — fit on 1900–1975, test on 1976–2050 — the rule
scores **99.667%**; the full-range 100% includes fitting. Do not treat the last
0.3% as physics.

### 6.2 Why it cannot be simplified to a threshold

Four rashis have normal cases *interleaved inside* their exception spans
(minutes from midnight):

```
rashi 11:  +7.8 EXC  +11.2 EXC  +19.2 EXC  +30.7 ok  +38.8 EXC  +50.2 ok
rashi  9: -15.5 EXC   -4.0 ok    +4.0 EXC
```

No cutoff separates those. The tithi tie-break is structurally necessary.

### 6.3 Sunrise is drik, not Surya Siddhanta

The tie-break keys off real astronomical sunrise. This was tested, not assumed:
drikpanchang computes sunrise astronomically regardless of which panjika
arithmetic is selected. Convenient consequence — you do not port the Surya
Siddhanta sunrise formula at all.

### 6.4 Months and era

Month number equals the rashi entered; day = `date - month_start + 1`; month
length = next start − this start (29–32 days).

```
1 Boishakh   2 Joishtho   3 Asharh    4 Srabon
5 Bhadro     6 Ashshin    7 Kartik    8 Ogrohaeon
9 Poush     10 Magh      11 Falgun   12 Choitro
```

Era is **Bangabda**: `gregorian_year - 593` on or after that year's Mesha month
start, `- 594` before it.

---

## 7. Test vectors

Location New Delhi (28.6139 N, 77.2090 E, UTC+5.5). Longitudes at 00:00 UT.

### 7.1 Longitudes

| Date (00:00 UT) | Sun | Moon | Phase | Tithi |
|---|---|---|---|---|
| 1900-01-01 | 257.682788 | 250.319331 | 352.636543 | 30 |
| 1950-06-15 | 60.048762 | 53.018648 | 352.969886 | 30 |
| 2000-01-01 | 255.762164 | 191.978369 | 296.216205 | 25 |
| 2026-07-17 | 89.754797 | 123.559794 | 33.804996 | 3 |
| 2050-12-31 | 254.536962 | 107.828540 | 213.291578 | 18 |

### 7.2 Sankranti moments

| Into rashi | IST | jd_ut |
|---|---|---|
| 1 Mesha | 2026-04-14 11:44:31 | 2461144.760082 |
| 4 Karka | 2026-07-17 11:42:11 | 2461238.758463 |
| 7 Tula | 2026-10-18 10:10:25 | 2461331.694731 |
| 11 Kumbha | 1905-02-12 00:07:50 | 2416888.276275 |
| 8 Vrishchika | 1932-11-15 23:51:50 | 2427027.265165 |

The last two are deliberately midnight-boundary cases. An implementation with
correct astronomy but a broken day rule will match all the longitudes and still
place those two months wrongly.

### 7.3 Month starts

| Month | Year (Bangabda) | Gregorian |
|---|---|---|
| 4 Srabon | 1433 | 2026-07-18 |
| 1 Boishakh | 1433 | 2026-04-15 |
| 7 Kartik | 1311 | 1904-10-17 |
| 9 Poush | 1365 | 1958-12-17 |
| 6 Ashshin | 1420 | 2013-09-18 |

### 7.4 Cross-check against the drik Bengali calendar

If the target project also has the modern (Bisuddhasiddhanta) Bengali calendar,
the two should differ on **26.32%** of month starts (477 of 1,812), never by
more than ±1 day. Identical output for both means the Surya engine is not
actually being called.

---

## 8. Failure modes, in likelihood order

| Symptom | Cause |
|---|---|
| Everything off by ~24° | Subtracted an ayanamsa. These longitudes are already sidereal |
| Everything off by ~0.2° | Used UT instead of local time at the observer's meridian (§5.3) |
| Occasional dates off by ~365 days | Forward-only sankranti search instead of bracketing around the estimate |
| Longitudes right, ~19 month starts wrong | Day rule missing or mis-tuned (§6) |
| ~5 month starts wrong, all near midnight | Per-rashi day edges missing (§6.1) |
| Sporadic single-day errors near midnight | Host sunrise differs from ours by >30 s (§6.3) |
| Reports 100% but a date is duplicated | Subset test instead of set equality (§4) |
| Surya and drik output identical | Dispatch not wired; drik astronomy still running |

On the third row: `surya_sankranti()` brackets the crossing *around* its
estimate rather than searching forward from it. Forward-only is the natural
reading of the classical rule and is what Reingold's
`hindu-solar-longitude-at-or-after` does, but calendar-layer callers pass
estimates that can sit just past the sankranti, and it then jumps a full year.
Here that surfaced as a day-of-month of −667. The shipped code already handles
this; do not "fix" it back.

---

## 9. Licensing

`src/surya_siddhanta.c` was written from the historical constants — the Surya
Siddhanta itself, via Burgess's 1860 English translation, both long in the
public domain — and not ported from any third-party implementation. The
constants and sine table are uncopyrightable facts under 17 USC 102(b), like
tabulated scientific measurements. Copy it freely.

Be aware that Reingold & Dershowitz's `calendar.l` implements the same model
and is **Apache 2.0** — permissive, but with attribution obligations. This
project keeps it out of shipped code deliberately; `validation/reingold/` and
`validation/suryasiddhanta/surya_siddhanta.py` are derived from it and are
tooling only. If the target project cares about a clean dependency surface,
take `src/surya_siddhanta.c` and leave those alone.

The reference CSV is scraped factual data (calendar dates), not creative
expression.

See [LICENSING.md](LICENSING.md) §8 for the full analysis.
