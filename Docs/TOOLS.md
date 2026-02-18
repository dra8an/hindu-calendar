# Tools

Utility programs in the `tools/` directory. These are standalone C programs and Python scripts used for data generation, edge case investigation, and validation web page support.

## Data Generators

These produce reference data consumed by test suites and the validation web page. Re-run them after any code change that affects calendar output. Data is stored per-backend under `validation/{se,moshier}/` (CSVs) and `validation/web/data/{se,moshier}/` (JSON).

| Tool | Language | Purpose | Output |
|------|----------|---------|--------|
| `generate_ref_data.c` | C | Generates the lunisolar 55,152-day reference CSV (1900–2050). Each row: Gregorian date, tithi, masa, adhika flag, Saka year. Accepts `-o DIR` to write to a directory | `validation/{backend}/ref_1900_2050.csv` |
| `gen_solar_ref.c` | C | Generates 4 solar calendar CSVs with month boundaries (1,811 months each, 1900–2050). Each row: month, year, length, Gregorian start date, month name. Accepts `-o DIR` | `validation/{backend}/solar/{calendar}_months_1900_2050.csv` |
| `extract_adhika_kshaya.py` | Python | Derives adhika/kshaya tithi edge-case CSV from a reference CSV by comparing consecutive days. Same logic as `csv_to_json.py` uses internally | `validation/{backend}/adhika_kshaya_tithis.csv` |
| `csv_to_json.py` | Python | Converts lunisolar CSV + Reingold CSV into 1,812 per-month JSON files for the validation web page. Embeds Reingold diff fields and adhika/kshaya flags. Accepts `--backend {se,moshier}` | `validation/web/data/{backend}/YYYY-MM.json` |
| `csv_to_solar_json.py` | Python | Converts 4 solar CSVs into 7,248 per-month JSON files (1,812 per calendar) for the validation web page. Accepts `--backend {se,moshier}` | `validation/web/data/{backend}/{calendar}/YYYY-MM.json` |
| `generate_all_validation.sh` | Bash | Master script: builds each backend, runs all C generators, extracts adhika/kshaya, produces JSON for both SE and Moshier. Run from project root | All of the above, for both backends |

### Build and run

```bash
# Using Makefile targets (recommended)
make gen-ref                      # Build generators + produce CSVs for current backend (moshier by default)
make gen-json                     # Produce web JSON for current backend
make USE_SWISSEPH=1 gen-ref       # Same, but for Swiss Ephemeris backend
make USE_SWISSEPH=1 gen-json

# Or regenerate everything for both backends at once
bash tools/generate_all_validation.sh

# Manual invocation with -o flag
make && make build/gen_ref build/gen_solar_ref
./build/gen_ref -o validation/moshier
./build/gen_solar_ref -o validation/moshier
python3 tools/extract_adhika_kshaya.py validation/moshier/ref_1900_2050.csv validation/moshier/adhika_kshaya_tithis.csv
python3 tools/csv_to_json.py --backend moshier
python3 tools/csv_to_solar_json.py --backend moshier
```

## Edge Case Scanner

| Tool | Language | Purpose | Output |
|------|----------|---------|--------|
| `solar_boundary_scan.c` | C | Scans all 7,248 sankrantis (1,812 per calendar × 4 calendars, 1900–2050). For each, computes the delta between the sankranti time and the calendar's critical time. Sorts by |delta| and outputs the 100 closest per calendar. Also auto-generates the test file | `validation/solar_edge_cases.csv`, `tests/test_solar_edge.c` |

### Build and run

```bash
cc -O2 -Isrc -Ilib/swisseph tools/solar_boundary_scan.c build/astro.o build/date_utils.o build/tithi.o build/masa.o build/panchang.o build/solar.o build/swe/*.o -lm -o build/solar_boundary_scan
./build/solar_boundary_scan
```

## Diagnostic / Investigation Tools

One-off tools built while reverse-engineering drikpanchang.com's critical time rules. Retained for historical reference and future debugging. Each prints detailed diagnostic output to stdout.

### Malayalam investigation

| Tool | Purpose |
|------|---------|
| `malayalam_diag.c` | Prints each 2025 Malayalam sankranti with exact JD, IST time, and the civil day assignment under three candidate rules (noon, sunset, midnight). Used to confirm end-of-madhyahna (3/5 of daytime) as the correct rule. See `Docs/MALAYALAM_ADJUSTMENTS.md` |

### Odia investigation

| Tool | Purpose |
|------|---------|
| `odia_diag.c` | Prints all 12 sankrantis for a given year with IST times and assignments under multiple candidate rules (midnight, sunrise, sunset, apparent midnight, fixed IST cutoffs) |
| `odia_boundary.c` | Scans all sankrantis 1900–2050 in the 18:00–02:00 IST range. Used to find boundary cases for manual verification against drikpanchang.com |
| `odia_midnight_scan.c` | Scans sankrantis near midnight, compares midnight vs sunset rules. Built to test the midnight hypothesis (which was rejected) |
| `odia_nishita.c` | Tests apparent midnight (nishita midpoint) hypothesis against 11 confirmed boundary cases. Showed that same-distance cases got different assignments, ruling out nishita |
| `odia_cutoff_scan.c` | Scans sankrantis in the 21:30–22:30 IST range, computes distance to apparent midnight. Led to the discovery of the fixed 22:12 IST cutoff. See `Docs/ODIA_ADJUSTMENTS.md` |

### Bengali investigation

| Tool | Purpose |
|------|---------|
| `bengali_tithi_rule.c` | Tests the traditional tithi-based rule (Sewell & Dikshit, 1896) against all 37 verified Bengali edge case entries across 8 configurations ({Delhi, Kolkata} x {prev_day, civil_date} x {our sank, dp sank}). Best result: 36/37 with "Delhi sunrise(prev_day), our sank" |
| `bengali_diag.c` | Prints all 100 Bengali edge cases with multiple candidate rules. Original diagnostic tool |
| `bengali_analysis.c` | Analysis of the 37 verified entries against nishita and midnight variants |
| `bengali_eot.c` | Tests equation of time midnight at IST, Ujjain, Delhi, and Kolkata longitudes. Ruled out |
| `bengali_ayanamsa.c` | Computes critical ayanamsa difference for each entry and tests linear ayanamsa models. Used to measure the ~24 arcsecond SE_SIDM_LAHIRI vs drikpanchang offset |
| `bengali_shifted.c` | Tests shifted sankranti times (with ayanamsa correction) against all fixed IST cutoffs and nishita+buffer combinations. Confirmed max non-trivial score of 22/37 |
| `bengali_weekday.c` | Day-of-week analysis — showed each rashi's edge cases always fall on the same weekday, so weekday alone can't separate W/C entries |

### Edge case correction helper

| Tool | Purpose |
|------|---------|
| `edge_corrections.c` | For each of the 21 wrong edge case entries (6 Tamil + 15 Malayalam), computes the corrected expected values by looking at the previous day's solar date. Used once to generate the corrected test data for `tests/test_solar_edge.c` |

### Build (all diagnostic tools follow the same pattern)

```bash
cc -O2 -Isrc -Ilib/swisseph tools/<tool>.c build/astro.o build/date_utils.o build/tithi.o build/masa.o build/panchang.o build/solar.o build/swe/*.o -lm -o build/<tool>
./build/<tool>
```
