# Tools

Utility programs in the `tools/` directory. These are standalone C programs and Python scripts used for data generation, edge case investigation, and validation web page support.

## Data Generators

These produce reference data consumed by test suites and the validation web page. Re-run them after any code change that affects calendar output.

| Tool | Language | Purpose | Output |
|------|----------|---------|--------|
| `generate_ref_data.c` | C | Generates the lunisolar 55,152-day reference CSV (1900–2050). Each row: Gregorian date, tithi, masa, adhika flag, Saka year | `validation/ref_1900_2050.csv` |
| `gen_solar_ref.c` | C | Generates 4 solar calendar CSVs with month boundaries (1,811 months each, 1900–2050). Each row: month, year, length, Gregorian start date, month name | `validation/solar/{tamil,bengali,odia,malayalam}_months_1900_2050.csv` |
| `csv_to_json.py` | Python | Converts lunisolar CSV + Reingold CSV into 1,812 per-month JSON files for the validation web page. Embeds Reingold diff fields and adhika/kshaya flags | `validation/web/data/YYYY-MM.json` |
| `csv_to_solar_json.py` | Python | Converts 4 solar CSVs into 7,248 per-month JSON files (1,812 per calendar) for the validation web page | `validation/web/data/{tamil,bengali,odia,malayalam}/YYYY-MM.json` |

### Build and run

```bash
# Lunisolar reference CSV
cc -O2 -Isrc -Ilib/swisseph tools/generate_ref_data.c build/*.o build/swe/*.o -lm -o build/generate_ref_data
./build/generate_ref_data > validation/ref_1900_2050.csv

# Solar reference CSVs
cc -O2 -Isrc -Ilib/swisseph tools/gen_solar_ref.c build/astro.o build/date_utils.o build/tithi.o build/masa.o build/panchang.o build/solar.o build/swe/*.o -lm -o build/gen_solar_ref
./build/gen_solar_ref

# Validation web page JSON
python3 tools/csv_to_json.py
python3 tools/csv_to_solar_json.py
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

### Edge case correction helper

| Tool | Purpose |
|------|---------|
| `edge_corrections.c` | For each of the 21 wrong edge case entries (6 Tamil + 15 Malayalam), computes the corrected expected values by looking at the previous day's solar date. Used once to generate the corrected test data for `tests/test_solar_edge.c` |

### Build (all diagnostic tools follow the same pattern)

```bash
cc -O2 -Isrc -Ilib/swisseph tools/<tool>.c build/astro.o build/date_utils.o build/tithi.o build/masa.o build/panchang.o build/solar.o build/swe/*.o -lm -o build/<tool>
./build/<tool>
```
