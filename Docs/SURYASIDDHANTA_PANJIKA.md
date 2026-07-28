# Suryasiddhanta Bengali Panjika

Drikpanchang.com renders the Bengali calendar under **two different schools**,
toggled by a toolbar button. Both are real calendars with different dates:

| School | Cookie | Basis |
|--------|--------|-------|
| **Bisuddhasiddhanta** | `drik-arithmetic=modern` | Drik / modern astronomy. The site default, and what `src/solar.c` targets. |
| **Suryasiddhanta** | `drik-arithmetic=suryasiddhanta` | Traditional Surya Siddhanta arithmetic. |

They disagree on **26.32%** of month starts (477 of 1,812 over 1900–2050).

This document records the scrape of the Suryasiddhanta variant, the
reverse-engineered arithmetic, and the critical-time rule fitted against it.
The result is a shipped implementation reproducing **1,812 / 1,812 (100.000%)**
of drikpanchang's Suryasiddhanta Bengali month starts.

---

## 1. Discovering the toggle

The page's own `<h1>` states which school produced it:

```
Bengali Panjika based on Bisuddhasiddhanta for New Delhi, NCT, India
Bengali Panjika based on Suryasiddhanta   for New Delhi, NCT, India
```

and the toolbar button calls:

```js
dpSettingsToolbar.handlePanchangArithmeticOptionClick('suryasiddhanta', true)
```

which maps to the `drik-arithmetic` cookie. Verified 2026-07-27: same URL, same
month, cookie the only change, Srabon 1433 moves from **2026-07-17** to
**2026-07-18**, and month lengths shift correspondingly. Not cosmetic.

Every scraped page is checked against the expected `<h1>` token
(`scraper/solar/config.py: PROVENANCE_TOKENS`), so the two flavors can never
silently mix into one dataset. `parse` refuses to build a CSV from a mixed
directory rather than producing a plausible-looking blend.

### Scraper usage

```bash
python3 -m scraper.solar.fetch --calendar bengali --arithmetic suryasiddhanta
python3 -m scraper.solar.parse --calendar bengali --arithmetic suryasiddhanta
```

Each flavor gets its own paths; `modern` keeps the original unsuffixed ones:

```
raw/bengali/                    parsed/bengali.csv                 (modern)
raw/bengali_suryasiddhanta/     parsed/bengali_suryasiddhanta.csv  (surya)
```

**Scrape result**: 1,812/1,812 pages, 302 MB, 0 truncated files, provenance
verified on all 1,812. Took 9 VPN cycles — see `scraper/README.md` for the
per-IP rate limit.

---

## 2. The Surya Siddhanta arithmetic

No data tables are needed — the entire model is five integers from the text, two
epicycle sizes per body, and a 24-entry sine table.

**Two implementations exist, with different provenance. This matters:**

| | Source | License |
|---|--------|---------|
| `src/surya_siddhanta.c` (shipped) | Written from the historical constants — Surya Siddhanta ch. 1–2, Burgess 1860 | Public domain / uncopyrightable facts |
| `validation/suryasiddhanta/surya_siddhanta.py` (tooling) | Ported from Reingold & Dershowitz `calendar.l` | **Apache 2.0** |

The Python came first and is openly derived. The C was then written
independently from the primary source, so that no licensed third-party code
enters the shipped library — see [LICENSING.md](LICENSING.md) §8. The two agree
to 1e-8 degrees.

### Constants (exact)

The shipped C derives everything from five integers stated in the text
(chapter 1), rather than transcribing decimals:

```
solar revolutions per mahayuga    4320000
lunar revolutions per mahayuga    57753336
civil days per mahayuga           1577917828
solar apogee revs per kalpa       387          (kalpa = 1000 mahayugas)
lunar apogee revs per mahayuga    488199

sidereal year     = civil_days / solar_revs                       = 365.258756481
anomalistic year  = civil_days*1000 / (solar_revs*1000 - 387)     = 365.258789203
sidereal month    = civil_days / lunar_revs                       =  27.321674163
anomalistic month = civil_days / (lunar_revs - lunar_apogee)      =  27.554597975

epicycle sizes (chapter 2), varying between the odd and even quadrant values:
  Sun   14 deg <-> 13 deg 40'   ->  size 14/360, contraction 1/42
  Moon  32 deg <-> 31 deg 40'   ->  size 32/360, contraction 1/96

epoch  = Kali Yuga, Julian -3102 Feb 18 = RD -1132959
         mean sun and mean moon are both 0 deg here, by definition
anomaly at epoch: solar 3143/4000 turn, lunar 3/4 turn
```

The solar anomaly offset corresponds to a solar apogee of **77.13 degrees**
against the ~77 degrees stated in the text. That was not an input, so it is a
useful check that the revolution counts were transcribed correctly.

### Algorithm

```
sine_table(n)  = the 24 tabulated integers from chapter 2, in units of R = 3438,
                 at 225 arcmin (3.75 deg) intervals.  Stated in the text as
                 differences, which sum to exactly 3438:
                   225 224 222 219 215 210 205 199 191 183 174 164
                   154 143 131 119 106  93  79  65  51  37  22   7
                 The table covers the first quadrant; the anomaly sweeps all
                 four, so the index is reflected and the sign carried.

hindu_sine(t)  = linear interpolation in that table at t/3.75
hindu_arcsin(a)= table search + interpolation (stays in 0..24)

mean_position(tee, period) = 360 * frac(days_since_epoch / period + epoch_offset)

true_position(tee, period, size, anom, change):
    lambda      = mean_position(tee, period)
    offset      = hindu_sine(mean_position(tee, anom))
    contraction = |offset| * change * size
    equation    = hindu_arcsin(offset * (size - contraction))
    return (lambda - equation) mod 360

solar_longitude(tee) = true_position(tee, SIDEREAL_YEAR,  14/360, ANOMALISTIC_YEAR,  1/42)
lunar_longitude(tee) = true_position(tee, SIDEREAL_MONTH, 32/360, ANOMALISTIC_MONTH, 1/96)
lunar_phase(tee)     = (lunar - solar) mod 360
tithi(tee)           = 1 + floor(lunar_phase / 12)
```

### Three things that will bite an implementer

1. **The longitude is already sidereal (nirayana).** Do NOT subtract an
   ayanamsa. The Surya Siddhanta zodiac is anchored to its own epoch. Applying
   Lahiri here produces a ~24 degree error.

2. **The epicycle is not a fixed 14 degrees.** The `contraction` term shrinks it
   by up to 1/42 of its size as a function of anomaly.
   `Docs/PHYSICS.md` describes a flat 14 degrees, which is a simplification.

3. **Precision: the creation offset must be reduced exactly.**
   `CREATION = EPOCH - 1955880000 * SIDEREAL_YEAR` is about -7.1e11. Computing
   `tee - CREATION` in doubles burns most of the 15-16 significant digits,
   leaving ~9 second resolution. It cancels cleanly:

   ```
   (tee - CREATION)/SIDEREAL_YEAR = (tee - EPOCH)/SIDEREAL_YEAR + 1955880000
   ```

   and the integer vanishes under `mod 1`. For the other three periods the
   offset is not an integer, but reduces to exact simple rationals:

   | Argument | frac(CREATION_REVS * SIDEREAL_YEAR / period) |
   |----------|----------------------------------------------|
   | anomalistic year | **3143/4000** |
   | sidereal month | **0** |
   | anomalistic month | **3/4** |

   So in C these are literals, not approximations, and full double precision
   is retained throughout.

**Verification**: this float implementation matches `calendar.l`'s exact
rational arithmetic to **6e-9 degrees (0.00002 arcsec)** for both solar and
lunar longitude, while being roughly 175,000x faster (143 microseconds per
sankranti vs ~25 seconds). The Lisp is unusable for bulk generation because
`invert-angular` bisects on rationals, doubling denominators every iteration on
top of the 1e18 offset.

---

## 3. How the two schools diverge

Comparing `parsed/bengali.csv` against `parsed/bengali_suryasiddhanta.csv`
over all 1,812 months:

```
Identical month start : 1,335 (73.68%)
Differing             :   477 (26.32%)     offsets: -1 day x24, +1 day x453
```

Never more than +/-1 day. Two clear structures:

**Annual signature** — Surya runs late (+1) through Jul-Dec and early (-1)
through Feb-Apr:

| Bengali month | ~Season | Differ | Direction |
|---------------|---------|--------|-----------|
| Ashshin | Sep | 51.7% | +1 |
| Bhadro | Aug | 49.7% | +1 |
| Kartik | Oct | 46.4% | +1 |
| Ogrohaeon | Nov | 39.1% | +1 |
| Srabon | Jul | 37.1% | +1 |
| Falgun / Choitro | Feb / Mar | 8-9% | mixed, mostly -1 |

This is the fingerprint of the **single-epicycle equation of center**: one manda
correction cannot track a real elliptical orbit, so its residual error is
roughly an annual sinusoid. Where the Surya Siddhanta sun runs ahead of reality
the sankranti lands early; where it lags, late.

**Secular drift** — the differing share climbs monotonically, 2.4x over 150 years:

```
1900s 16.7%   1930s 20.0%   1960s 25.8%   1990s 29.2%   2020s 34.2%
1910s 17.5%   1940s 19.2%   1970s 22.5%   2000s 34.2%   2030s 37.5%
1920s 19.2%   1950s 20.0%   1980s 24.2%   2010s 30.8%   2040s 40.8%
```

Surya Siddhanta's fixed mean motions drifting against modern astronomy. A
practical consequence: any tuning fitted on a single era degrades elsewhere,
so validation must span the full range.

---

## 4. The critical-time rule

The arithmetic alone, with the naive rule "month starts the day after the
sankranti", gives **1792/1811 = 98.951%** — 19 exceptions, all within a
~45-minute band around midnight. The residual is entirely the day-assignment
rule, not the astronomy. (If the longitude were wrong, errors would scatter
across all hours and drift with epoch; they do neither.)

The rule turns out to be the **same structure as Bisuddha** (`src/solar.c`
`sankranti_to_civil_day`), with retuned constants:

```
D    = local civil date of the sankranti, after a per-rashi day-edge shift
crit = D's midnight + 40 minutes
if sankranti <= crit:
    month starts on D, unless the tithi rule pushes it to D+1
else:
    month starts on D+1
```

**Tithi rule** (Sewell & Dikshit, 1896): push to D+1 when the tithi current at
sunrise of D-1 has already ended by the moment of the sankranti.

- rashi 4 (Karkata) never pushes
- rashi 10 (Makara) always pushes

Both special cases carry over from Bisuddha **unchanged** and hold across all
151 years — evidence they encode a real Bengali convention rather than fitted
noise.

**Sunrise**: the project's existing drik (astronomical) sunrise, NOT a Surya
Siddhanta mean sunrise. Drikpanchang computes sunrise astronomically regardless
of which panjika arithmetic is selected. This was tested, not assumed, and means
no `hindu-sunrise` port is required.

**Tithi**: Surya Siddhanta tithi (section 2), not drik tithi.

### Fitted parameters

`crit = 40 minutes` after midnight (Bisuddha uses 24), and a per-rashi
**day-edge** offset in minutes *before* midnight:

| Rashi | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|-------|---|---|---|---|---|---|---|---|---|----|----|----|
| edge  | **7** | **11** | 0 | 0 | 0 | **12** | **20** | **22** | **19** | 0 | 0 | 0 |

Only 6 of 12 rashis need a non-zero edge; the rest work at zero (minimal
intervention — 0 was chosen wherever 0 lies inside the admissible window).

### Results

| Model | Match |
|-------|-------|
| Naive "+1 day" | 1792/1811 = 98.951% |
| crit=40 + tithi rule, two global constants | 1806/1811 = 99.724% |
| **+ per-rashi day edge** | **1811/1811 = 100.000%** |

---

## 5. Validation and honest limits

**A time-based rule alone provably cannot work.** With the full range, 4 of the
8 affected rashis have normal cases *interleaved inside* their exception spans:

```
rashi 11:  +7.8 EXC   +11.2 EXC   +19.2 EXC   +30.7 normal   +38.8 EXC   +50.2 normal
rashi  9: -15.5 EXC    -4.0 normal  +4.0 EXC
rashi  7: -14.4 EXC    -6.2 EXC    +5.1 normal  +16.5 normal  +24.7 EXC
rashi  6:  -6.5 EXC    +1.8 normal +13.1 normal +24.4 EXC
```

(minutes from midnight). No cutoff separates those, which is why the tithi rule
is structurally necessary — the same conclusion `Docs/BENGALI_INVESTIGATION.md`
reached for Bisuddha.

Note this was NOT apparent on partial data: with the first third of the scrape
(8 exceptions) every rashi looked separable by a simple threshold. Fitting then
would have shipped six constants that memorised 1900-1948.

**Out-of-sample generalisation: 99.667%.** Fitting the edges on 1900-1975 and
testing on 1976-2050 gives 897/900, missing three (rashi 2 in 2005, rashi 3 in
2015, rashi 5 in 2030). The training half did not contain the cases constraining
those rashis. So the model *structure* generalises, but the per-rashi constants
genuinely require the full range to pin down.

**The constants are not knife-edge.** Admissible windows are 4-11 minutes wide
(rashi 7 works anywhere in 15..25, rashi 8 in 17..27). Midpoints were chosen
rather than boundary values. That is 6 fitted constants against 1,811
observations, each constrained by several independent near-midnight cases.

**Not yet validated**: only the Bengali calendar was scraped in the
Suryasiddhanta flavor. Tamil also exposes the same toolbar toggle (its default
is Thirukanitha); Odia and Malayalam do not appear to. Whether the crit=40 and
the per-rashi edges are Bengali-specific or shared is unknown.

---

## 6. Implementation status

**Implemented and shipped.** Available as the calendar type
`SOLAR_CAL_BENGALI_SURYA`, and from the CLI:

```bash
./hindu-calendar -s bengali-surya -d 18 -m 7 -y 2026
./hindu-calendar -s bengali-surya -y 2026 -m 7        # full month
```

| Piece | Where |
|-------|-------|
| Engine | `src/surya_siddhanta.c` / `.h` (~265 lines) |
| Calendar type | `SOLAR_CAL_BENGALI_SURYA` in `types.h` |
| Rule constants | `critical_time_jd()` and `bengali_day_edge_offset()` in `solar.c` |
| Arithmetic dispatch | `arith_solar_longitude()`, `arith_sankranti()` in `solar.c` |
| Regression test | `tests/test_surya_bengali.c` -- 3,624 assertions, 1,812 months |

The two Bengali variants share every rule in `solar.c`; they differ only in
which astronomy computes the sun, the moon, and the sankranti moment. That
difference is confined to two dispatch helpers rather than scattered through
the calendar logic, and the existing caches key on calendar type so they
separate the variants automatically.

The `hindu-sunrise` port originally budgeted at 40-60 lines is **not needed**
(section 4): both variants use the project's drik sunrise.

### Integration notes

- `sankranti_jd()` and `sankranti_before()` keep their existing signatures and
  stay drik-only, so no public API changed. Internal callers go through
  `arith_sankranti()`.
- `surya_sankranti()` brackets the crossing AROUND its estimate rather than
  searching forward from it, matching `sankranti_jd()`. A forward-only search
  (the natural reading of the classical rule) silently jumps a full year when
  the caller's estimate lands just past the sankranti -- this was caught by
  integration producing a day-of-month of -667.
- `get_config()` previously hardcoded a loop bound of 4 over `SOLAR_CONFIGS`;
  a fifth entry would have been silently ignored. Now computed from the array.
- The CLI display name came from a ternary chain whose fallback was
  "Malayalam", so a new type would have been mislabelled. Replaced with a
  `switch` that has no fallback case.

## 7. Where everything lives

| Path | Contents |
|------|----------|
| `validation/suryasiddhanta/surya_siddhanta.py` | The engine — SS solar/lunar longitude, tithi, sankranti finding |
| `validation/suryasiddhanta/fit_rule.py` | Critical-time rule fitting |
| `validation/suryasiddhanta/compare_flavors.py` | Bisuddha vs Surya divergence analysis |
| `validation/suryasiddhanta/sunrise_tool.c` | Feeds the project's real drik sunrise into the Python experiments |
| `validation/suryasiddhanta/README.md` | Build + usage, expected output |
| `validation/drikpanchang/bengali_suryasiddhanta.csv` | **The scrape** — 1,812 month starts (committed; ~9 VPN cycles to reproduce) |
| `validation/drikpanchang/*.csv` | Same for the four Bisuddha-era calendars |
| `validation/reingold/generate_reingold_solar.lisp` | Independent cross-check oracle (slow) |
| `scraper/solar/{fetch,parse,compare}.py` | `--arithmetic` flag, provenance assertion |

Reproduce the headline numbers without re-scraping:

```bash
python3 validation/suryasiddhanta/compare_flavors.py   # 26.32% divergence
python3 validation/suryasiddhanta/fit_rule.py          # 99.724% two-parameter stage
```
