# Plan: NYC Lunisolar Validation Infrastructure

## Context

We have a complete lunisolar validation pipeline for Delhi (55,152 days, 1900-2050) comparing our Moshier output against drikpanchang.com. The user wants the same infrastructure for New York City to validate location-dependent calculations. The main challenge is DST handling — India has no DST, but NYC switches between EST (UTC-5) and EDT (UTC-4).

Drikpanchang.com supports NYC via `geoname-id=5128581` and handles DST correctly (verified: Jan shows EST sunrise 07:18, Jul shows EDT sunrise 05:38).

## Components

### 1. US Eastern DST module

New files: `src/dst.h` + `src/dst.c`

Function: `double us_eastern_offset(int y, int m, int d)` — returns UTC offset (-5.0 or -4.0) for a given Gregorian date.

US Eastern DST rules (for 1900-2050):
- **1900-1917**: No DST → -5.0
- **1918-1919**: Last Sun March – Last Sun October → -4.0 during DST
- **1920-1941**: No federal DST → -5.0 (NYC had local DST but we start with -5.0 and tune if comparison shows issues)
- **1942 – 1945-Sep-30**: Year-round War Time → -4.0
- **1946-1966**: Last Sun April – Last Sun September → -4.0 during DST
- **1967-1973**: Last Sun April – Last Sun October
- **1974**: Jan 6 – Last Sun October (energy crisis)
- **1975**: Last Sun Feb – Last Sun October
- **1976-1986**: Last Sun April – Last Sun October
- **1987-2006**: First Sun April – Last Sun October
- **2007-2050**: Second Sun March – First Sun November

Helper: `static int nth_weekday(int y, int m, int nth, int dow)` — find nth occurrence of a weekday in a month (nth=-1 for last).

### 2. Modify reference generator to accept location

Modify: `tools/generate_ref_data.c`

Add CLI flags:
- `-l LAT,LON` — location (default: Delhi 28.6139,77.2090)
- `-u OFFSET` — fixed UTC offset (default: 5.5)
- `-tz us_eastern` — use DST-aware offset (overrides -u)

When `-tz us_eastern` is specified, call `us_eastern_offset(y, m, d)` for each date instead of using the fixed offset.

### 3. Scraper modifications

**`scraper/common.py`**: Parameterize geoname-id by location:
- Add `get_cookies(location="delhi")` function
- Location map: `{"delhi": "1261481", "nyc": "5128581"}`
- Update `new_session()` to accept location parameter

**`scraper/lunisolar/fetch.py`**: Add `--location` CLI argument (default: "delhi"). Data goes to `scraper/data/lunisolar_{location}/raw/`.

**`scraper/lunisolar/config.py`**: Add `get_paths(location)` function returning location-specific RAW_DIR, PARSED_DIR, PARSED_CSV, REF_CSV paths.

**`scraper/lunisolar/parse.py`**: Add `--location` argument.

**`scraper/lunisolar/compare.py`**: Add `--location` argument, point to `validation/moshier/nyc/ref_1900_2050.csv` for NYC.

### 4. Validation directory structure

```
validation/moshier/nyc/
├── ref_1900_2050.csv
└── adhika_kshaya_tithis.csv
```

### 5. Makefile targets

```makefile
gen-ref-nyc: $(BUILDDIR)/gen_ref
    mkdir -p validation/$(GEN_BACKEND)/nyc
    ./$(BUILDDIR)/gen_ref -l 40.7128,-74.0060 -tz us_eastern -o validation/$(GEN_BACKEND)/nyc
    python3 tools/extract_adhika_kshaya.py \
        validation/$(GEN_BACKEND)/nyc/ref_1900_2050.csv \
        validation/$(GEN_BACKEND)/nyc/adhika_kshaya_tithis.csv
```

## Files to Create

1. `src/dst.h` — `us_eastern_offset()` declaration
2. `src/dst.c` — US Eastern DST rules (~80 lines)

## Files to Modify

1. `tools/generate_ref_data.c` — add `-l`, `-tz` flags, link DST module
2. `scraper/common.py` — parameterize geoname-id
3. `scraper/lunisolar/fetch.py` — add `--location` flag
4. `scraper/lunisolar/config.py` — location-aware paths
5. `scraper/lunisolar/parse.py` — add `--location` flag
6. `scraper/lunisolar/compare.py` — add `--location` flag
7. `Makefile` — add `gen-ref-nyc` target, compile `dst.o`, link into `gen_ref`

## Verification

1. `make clean && make` — builds with dst.o
2. Spot-check DST function: 2025-03-08 → -5, 2025-03-09 → -4, 2025-11-01 → -4, 2025-11-02 → -5
3. `make gen-ref-nyc` — generates 55,152-row NYC CSV
4. Spot-check NYC CSV rows at DST boundaries
5. `./hindu-calendar -y 2025 -m 1 -d 15 -l 40.7128,-74.0060 -u -5` matches DP sunrise 07:18
6. `./hindu-calendar -y 2025 -m 7 -d 15 -l 40.7128,-74.0060 -u -4` matches DP sunrise 05:38
7. `python3 -m scraper.lunisolar.fetch --location nyc --start-year 2025 --end-year 2025` — test scrape
8. `python3 -m scraper.lunisolar.compare --location nyc` — compare against NYC reference

## Implementation Order

1. Create `src/dst.h` + `src/dst.c`
2. Modify `Makefile` to compile dst.o and link into gen_ref
3. Modify `tools/generate_ref_data.c` to accept location/tz flags
4. Run `make gen-ref-nyc` — generate NYC reference CSV
5. Spot-check a few dates against drikpanchang.com manually
6. Modify scraper scripts (common.py, fetch.py, config.py, parse.py, compare.py)
7. Test scrape a small batch (1 year) to verify parsing works for NYC pages
