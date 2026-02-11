# Reingold/Dershowitz vs Our Calendar — Diff Report

## Context

We want to find all dates where our Hindu calendar calculation (Drik Siddhanta, Swiss Ephemeris) differs from Reingold/Dershowitz's implementation (Surya Siddhanta). The Lisp code at `github.com/EdReingold/calendar-code2/calendar.l` has two modern Hindu lunar calendar functions:

1. **`hindu-lunar-from-fixed`** — Surya Siddhanta with epicycles (traditional astronomy formulas, Hindu sine tables)
2. **`astro-hindu-lunar-from-fixed`** — Astronomical version (uses the book's own VSOP/ELP-based ephemeris)

Both use **Ujjain** (23°9'N, 75°46'E) as the reference location, while we use **New Delhi** (28.6°N, 77.2°E). Both return Vikrama era years (Saka + 135). Differences are expected due to different ephemeris, different location, and different astronomical models.

The user will use the diff report to spot-check disagreement dates against drikpanchang.com — if we match drikpanchang and Reingold/Dershowitz doesn't, that validates our implementation.

## Approach: Run the actual Lisp code via SBCL

SBCL (Steel Bank Common Lisp) is installed at `/usr/local/bin/sbcl`. The Lisp code loads successfully and both functions work. Benchmarked at ~0.055s for 31 dates — full 55,152-date range should take ~2 minutes.

**Why SBCL over Python translation**: Running the original Lisp avoids translation bugs. The Surya Siddhanta version needs ~30 interdependent functions (Hindu sine tables, epicyclic position, binary search). The astronomical version needs 100+ more. SBCL runs both faithfully with zero translation risk.

## Deliverables

### 1. Lisp generator script: `validation/reingold/generate_reingold.lisp`

Loads `calendar.l`, iterates over all dates 1900-01-01 to 2050-12-31, outputs CSV:

```
year,month,day,hl_tithi,hl_masa,hl_adhika,hl_vikrama,al_tithi,al_masa,al_adhika,al_vikrama
```

Where `hl_` = `hindu-lunar-from-fixed` and `al_` = `astro-hindu-lunar-from-fixed`.

Mapping from Lisp output `(year month leap-month day leap-day)`:
- `tithi` = `day` (4th element, 1-30)
- `masa` = `month` (2nd element, 1-12)
- `adhika` = `leap-month` (3rd element, NIL→0, T→1)
- `vikrama` = `year` (1st element)

### 2. Python diff script: `validation/reingold/diff_reingold.py`

Reads both CSVs (our `ref_1900_2050.csv` and the generated Reingold CSV), compares each day, and writes diff reports:

**Output**: `validation/reingold/diffs_hindu_lunar.csv` and `validation/reingold/diffs_astro_hindu.csv`

Each diff file has columns:
```
date,field,ours,theirs
2025-03-15,tithi,5,6
2025-03-15,masa,12,11
```

Plus a summary printed to stdout: total days compared, days with differences, breakdown by field.

### 3. Runner script: `validation/reingold/run.sh`

Orchestrates: clone repo (if needed) → run Lisp generator → run Python diff → print summary.

## Files to create

| File | Purpose |
|------|---------|
| `validation/reingold/generate_reingold.lisp` | SBCL script to generate Reingold CSV |
| `validation/reingold/diff_reingold.py` | Compare our CSV vs Reingold CSV |
| `validation/reingold/run.sh` | One-command orchestrator |

## Generated output files

| File | Purpose |
|------|---------|
| `validation/reingold/reingold_1900_2050.csv` | Reingold's calculations (55,152 rows) |
| `validation/reingold/diffs_hindu_lunar.csv` | Dates where `hindu-lunar-from-fixed` differs from us |
| `validation/reingold/diffs_astro_hindu.csv` | Dates where `astro-hindu-lunar-from-fixed` differs from us |

## Implementation steps

1. Create `validation/reingold/` directory
2. Create `generate_reingold.lisp` — SBCL script that:
   - Defines CC4 package, loads calendar.l from `/tmp/calendar-code2/`
   - Loops from fixed-from-gregorian(1900,1,1) to fixed-from-gregorian(2050,12,31)
   - Calls both `hindu-lunar-from-fixed` and `astro-hindu-lunar-from-fixed`
   - Converts each gregorian-from-fixed back to Y/M/D for the CSV
   - Writes CSV to stdout (redirect to file)
3. Create `diff_reingold.py` — reads both CSVs, compares tithi/masa/adhika for each date, writes diff files
   - Our Saka year → Vikrama = Saka + 135, for year comparison
   - Handle month numbering (both use 1-12, same mapping)
4. Create `run.sh` — clones repo if needed, runs SBCL, runs diff
5. Run everything and verify output

## Verification

1. `bash validation/reingold/run.sh` completes without errors
2. `reingold_1900_2050.csv` has 55,152 data rows
3. Spot-check Jan 1, 2025: both Lisp functions should give tithi=2, masa=10 (verified during exploration)
4. Diff files exist and have reasonable content
5. Summary shows how many dates differ (expected: thousands, since Surya Siddhanta ≠ Drik Siddhanta)
