# Drikpanchang.com Scraper

Scrapes drikpanchang.com month panchang pages to obtain independent tithi
data for validation against our computed reference CSV.

## Setup

```bash
pip install -r requirements.txt
```

## Usage

### Phase 1: Fetch raw HTML

```bash
# Fetch a few months to test
python3 fetch.py --start-year 2024 --end-year 2025 --delay 5

# Full range (1,812 months, ~10 hours at 20s delay)
python3 fetch.py

# Resume: already-downloaded months are skipped automatically
# Ctrl+C: graceful shutdown, restart to continue
```

HTML files are saved to `data/raw/month/YYYY-MM.html`.

### Phase 2: Parse HTML to CSV

```bash
python3 parse.py
```

Reads all `data/raw/month/*.html` and produces
`data/parsed/drikpanchang_lunisolar.csv` with columns:
`year,month,day,tithi`

### Phase 3: Compare against reference

```bash
python3 compare.py
```

Diffs parsed tithis against `validation/moshier/ref_1900_2050.csv` and
writes `data/comparison_report.txt`.

To compare against Swiss Ephemeris reference:
```bash
python3 compare.py --ref ../validation/se/ref_1900_2050.csv
```

## Settings

Fetches use these cookies to get Amanta scheme + New Delhi data:
- `drik-school-name=amanta`
- `drik-geoname-id=1261481` (New Delhi)
- `drik-ayanamsha-type=chitra-paksha` (Lahiri)

## Data

Each month page is ~150-200 KB. Full range = ~300 MB for 1,812 files.

The parser extracts tithi (1-30) per day from the `dpCellTithi` element
in each grid cell. Tithi is scheme-independent (same value regardless of
Amanta/Purnimanta setting).
