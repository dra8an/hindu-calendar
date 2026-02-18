# Plan: Dual-Backend Validation Data (SE + Moshier)

## Context

All validation reference CSVs and web JSON files were generated using the Swiss Ephemeris backend. We need identical reference data for the Moshier backend too, so we can compare backends side-by-side on the web validation page. This also enables proper per-backend regression testing.

## New Directory Structure

Reorganize `validation/` to separate backend-specific data from backend-independent data:

```
validation/
├── se/                                    ← MOVED from current locations
│   ├── ref_1900_2050.csv                  ← was drikpanchang_data/ref_1900_2050.csv
│   ├── adhika_kshaya_tithis.csv           ← was drikpanchang_data/adhika_kshaya_tithis.csv
│   └── solar/
│       ├── tamil_months_1900_2050.csv     ← was solar/tamil_months_1900_2050.csv
│       ├── bengali_months_1900_2050.csv
│       ├── odia_months_1900_2050.csv
│       └── malayalam_months_1900_2050.csv
├── moshier/                               ← NEW (same structure, freshly generated)
│   ├── ref_1900_2050.csv
│   ├── adhika_kshaya_tithis.csv
│   └── solar/
│       ├── tamil_months_1900_2050.csv
│       ├── bengali_months_1900_2050.csv
│       ├── odia_months_1900_2050.csv
│       └── malayalam_months_1900_2050.csv
├── web/
│   ├── index.html                         ← MODIFIED (add backend selector)
│   ├── serve.sh
│   └── data/
│       ├── se/                            ← MOVED from current data/
│       │   ├── YYYY-MM.json              (1,812 lunisolar)
│       │   ├── tamil/YYYY-MM.json        (1,812 per solar calendar)
│       │   ├── bengali/...
│       │   ├── odia/...
│       │   └── malayalam/...
│       └── moshier/                       ← NEW (same structure)
│           ├── YYYY-MM.json
│           ├── tamil/YYYY-MM.json
│           ├── bengali/...
│           ├── odia/...
│           └── malayalam/...
├── reingold/                              ← UNCHANGED (backend-independent)
├── solar_edge_cases.csv                   ← UNCHANGED (drikpanchang-verified)
└── malayalam_boundary_cases.csv           ← UNCHANGED (drikpanchang-verified)
```

Old directories `drikpanchang_data/` and `solar/` removed after move.

## Step-by-Step Implementation

### Step 1: Move existing SE files to new structure

```bash
mkdir -p validation/se/solar
mv validation/drikpanchang_data/ref_1900_2050.csv validation/se/
mv validation/drikpanchang_data/adhika_kshaya_tithis.csv validation/se/
mv validation/solar/*.csv validation/se/solar/
rmdir validation/drikpanchang_data validation/solar

mkdir -p validation/web/data/se
mv validation/web/data/*.json validation/web/data/se/
for cal in tamil bengali odia malayalam; do
    mv validation/web/data/$cal validation/web/data/se/$cal
done
```

### Step 2: Update test CSV paths

So tests pass immediately after the move:

- **`tests/test_csv_regression.c`** (line 34-35): `validation/se/ref_1900_2050.csv`
- **`tests/test_adhika_kshaya.c`** (line 31-32): `validation/se/adhika_kshaya_tithis.csv`
- **`tests/test_solar_regression.c`** (line 35): prefix → `validation/se/solar/`

Verify: `make clean && make && make test` — 53,141/53,143 pass, `make USE_SWISSEPH=1 && make test` — 53,143/53,143 pass.

### Step 3: Update C generators to accept `-o DIR` flag

**`tools/generate_ref_data.c`**:
- Add `-o DIR` argument parsing
- When given: write to `DIR/ref_1900_2050.csv` (fopen)
- When not given: write to stdout (current behavior preserved)

**`tools/gen_solar_ref.c`**:
- Add `-o DIR` argument parsing
- Write to `DIR/solar/{calendar}_months_1900_2050.csv`
- Create `DIR/solar/` directory if needed

### Step 4: Create `tools/extract_adhika_kshaya.py`

New Python script to derive adhika/kshaya tithis from `ref_1900_2050.csv`:
- Input: path to `ref_1900_2050.csv`
- Output: path to `adhika_kshaya_tithis.csv`
- Logic: compare consecutive days — same tithi = adhika, skipped tithi = kshaya (same logic as `csv_to_json.py` lines 108-119)
- Usage: `python3 tools/extract_adhika_kshaya.py INPUT_CSV OUTPUT_CSV`

### Step 5: Update Python JSON generators

**`tools/csv_to_json.py`** — add `--backend {se,moshier}` argument:
- Input CSV: `validation/{backend}/ref_1900_2050.csv`
- Reingold CSV: unchanged (`validation/reingold/reingold_1900_2050.csv`)
- Output dir: `validation/web/data/{backend}/`
- Default: `--backend se` (backward compatible)

**`tools/csv_to_solar_json.py`** — add `--backend {se,moshier}` argument:
- Input CSVs: `validation/{backend}/solar/`
- Output dir: `validation/web/data/{backend}/`
- Default: `--backend se`

### Step 6: Generate Moshier validation data

Build with moshier (default), run generators:
```bash
make clean && make
./build/gen_ref -o validation/moshier
./build/gen_solar_ref -o validation/moshier
python3 tools/extract_adhika_kshaya.py validation/moshier/ref_1900_2050.csv validation/moshier/adhika_kshaya_tithis.csv
python3 tools/csv_to_json.py --backend moshier
python3 tools/csv_to_solar_json.py --backend moshier
```

Also regenerate SE JSON (paths changed):
```bash
python3 tools/csv_to_json.py --backend se
python3 tools/csv_to_solar_json.py --backend se
```

### Step 7: Create `tools/generate_all_validation.sh`

Master script that regenerates everything for both backends:
1. Build moshier → generate moshier CSVs → extract adhika/kshaya → generate moshier JSON
2. Build SE → generate SE CSVs → extract adhika/kshaya → generate SE JSON
3. Rebuild default (moshier) at the end

### Step 8: Update web page with backend selector

**`validation/web/index.html`**:
- Add "Backend" dropdown in header bar: **SE** | **Moshier**
- Data fetch path: `data/${backend}/${ym}.json` (lunisolar) or `data/${backend}/${calendar}/${ym}.json` (solar)
- URL hash: `#backend/calendar/YYYY-MM` (e.g., `#se/lunisolar/2025-03`, `#moshier/tamil/1950-06`)
- Backward compat: bare `#YYYY-MM` → se/lunisolar, `#tamil/YYYY-MM` → se/tamil
- Reingold overlay: works with both backends (Reingold data is backend-independent)
- Backend state persists across navigation (month/year changes keep same backend)

### Step 9: Add Makefile convenience targets

```makefile
gen-ref:      # Build generators and produce CSVs for current backend
gen-json:     # Run Python scripts to produce JSON for current backend
```

## Files Modified/Created

| File | Action |
|------|--------|
| `tests/test_csv_regression.c` | Update CSV path |
| `tests/test_adhika_kshaya.c` | Update CSV path |
| `tests/test_solar_regression.c` | Update CSV path prefix |
| `tools/generate_ref_data.c` | Add `-o DIR` flag |
| `tools/gen_solar_ref.c` | Add `-o DIR` flag |
| `tools/extract_adhika_kshaya.py` | **NEW** — derive adhika/kshaya from ref CSV |
| `tools/csv_to_json.py` | Add `--backend` arg, update paths |
| `tools/csv_to_solar_json.py` | Add `--backend` arg, update paths |
| `tools/generate_all_validation.sh` | **NEW** — master generation script |
| `validation/web/index.html` | Add backend selector dropdown |
| `Makefile` | Add `gen-ref` and `gen-json` targets |

## Verification

1. `make clean && make && make test` — 53,141/53,143 (paths updated, moshier still works)
2. `make clean && make USE_SWISSEPH=1 && make test USE_SWISSEPH=1` — 53,143/53,143
3. Web page: SE backend selected → identical to current display
4. Web page: Moshier backend selected → data loads, mostly identical to SE
5. `diff validation/se/ref_1900_2050.csv validation/moshier/ref_1900_2050.csv` — shows exactly the expected differences
6. Reingold overlay works with both backends
7. URL hash navigation works: `#se/lunisolar/2025-03`, `#moshier/tamil/1950-06`, bare `#2025-03`
