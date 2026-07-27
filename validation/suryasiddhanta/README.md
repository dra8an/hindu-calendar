# Suryasiddhanta Bengali Panjika — analysis tooling

Reference implementation and fitting scripts for drikpanchang.com's
**Suryasiddhanta** Bengali panjika (`drik-arithmetic=suryasiddhanta`), the
traditional-arithmetic counterpart to the Bisuddhasiddhanta calendar that
`src/solar.c` implements.

Full write-up: [`Docs/SURYASIDDHANTA_PANJIKA.md`](../../Docs/SURYASIDDHANTA_PANJIKA.md)

## Files

| File | Purpose |
|------|---------|
| `surya_siddhanta.py` | The engine. Surya Siddhanta solar + lunar longitude, tithi, and sankranti finding, ported from Reingold/Dershowitz `calendar.l` into floating point. Matches the Lisp to 6e-9 degrees at ~175,000x the speed. |
| `fit_rule.py` | Fits the critical-time rule (crit + per-rashi day edge + tithi rule) against the scraped month starts. |
| `compare_flavors.py` | Characterises how Bisuddhasiddhanta and Suryasiddhanta diverge — offset distribution, per-month and per-decade breakdown. |
| `sunrise_tool.c` | Feeds the project's real drik sunrise into the Python experiments, so sunrise is never reimplemented or approximated. |

## Setup

`fit_rule.py` needs `sunrise_tool`, which links against the built Moshier
objects. From the repo root:

```bash
make
cc -O2 -std=c99 -Ilib/moshier -Isrc \
   -o validation/suryasiddhanta/sunrise_tool \
   validation/suryasiddhanta/sunrise_tool.c \
   build/moshier/*.o build/astro.o build/date_utils.o -lm
```

## Usage

```bash
python3 validation/suryasiddhanta/compare_flavors.py   # Bisuddha vs Surya divergence
python3 validation/suryasiddhanta/fit_rule.py          # scan crit x day_edge
```

Both read the committed scrape in [`../drikpanchang/`](../drikpanchang/), so
they run without re-scraping.

`fit_rule.py` scans two global parameters and reports the best. The final
per-rashi fit (100.000%) is documented in `Docs/SURYASIDDHANTA_PANJIKA.md`
section 4; the scan here reproduces the 99.724% two-parameter stage that
motivates it.

## Expected output

```
compare_flavors.py : 1,335/1,812 identical (73.68%), 477 differing (26.32%)
fit_rule.py        : BEST crit=40 min, day_edge=10 min -> 1806/1811 = 99.724%
```

## Note on the Lisp

`../reingold/generate_reingold_solar.lisp` drives Reingold's `calendar.l` to
emit Surya Siddhanta sankranti moments directly. It is kept as an **independent
cross-check oracle** for spot values only — exact rational arithmetic makes it
~25 seconds per sankranti, so it cannot generate bulk data. `surya_siddhanta.py`
was validated against it before being used.
