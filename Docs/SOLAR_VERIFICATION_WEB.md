# Plan: Add Solar Calendar Support to Validation Web Page

## Context

The validation web page (`validation/web/index.html`) currently shows only lunisolar (panchang) data. The user wants a dropdown to switch between Lunisolar, Tamil, Bengali, Odia, and Malayalam calendars. Solar calendars are simpler (no adhika/kshaya tithi, no purnima/amavasya, no Reingold overlay) — each cell just shows the Gregorian day, solar month name, day-within-month, and regional era year.

## Approach

### 1. Generate solar JSON data files (new Python script)

Create `tools/csv_to_solar_json.py` that reads the 4 existing solar CSVs and produces per-Gregorian-month JSON files for each calendar.

**Data generation logic:** The solar CSVs have month boundaries (month start date + length), so for each Gregorian month, iterate its days and map each day to its solar month/day/year using the CSV data. This avoids needing to call the C program — pure Python CSV lookup.

**Output directories** (parallel to existing `data/`):
- `validation/web/data/tamil/YYYY-MM.json`
- `validation/web/data/bengali/YYYY-MM.json`
- `validation/web/data/odia/YYYY-MM.json`
- `validation/web/data/malayalam/YYYY-MM.json`

**Solar JSON format** (per day):
```json
{
  "day": 14,           // Gregorian day
  "dow": 1,            // 0=Sun..6=Sat
  "solar_month": 1,    // Regional month number (1-12)
  "solar_month_name": "Chithirai",
  "solar_day": 1,      // Day within solar month
  "solar_year": 1947,  // Regional era year
  "era_name": "Saka",  // "Saka", "Bangabda", "Kollam"
  "is_month_start": true  // true when solar_day == 1 (sankranti day)
}
```

**File count:** 1,812 files per calendar × 4 calendars = 7,248 new files.

### 2. Update `validation/web/index.html`

**Header changes:**
- Add a calendar type dropdown (`<select id="sel-calendar">`) with options: Lunisolar (default), Tamil, Bengali, Odia, Malayalam
- Move R/D overlay checkbox to only show when Lunisolar is selected
- Update title to show which calendar is active

**State changes:**
- New state variable: `curCalendar` (values: `"lunisolar"`, `"tamil"`, `"bengali"`, `"odia"`, `"malayalam"`)
- URL hash format: `#YYYY-MM` (lunisolar, backward-compatible) or `#tamil/YYYY-MM`, `#bengali/YYYY-MM`, etc.

**Data loading:**
- Lunisolar: `data/YYYY-MM.json` (unchanged)
- Solar: `data/{calendar}/YYYY-MM.json`

**Rendering:**
- The same transposed 7-row grid layout is reused for both calendar types
- When calendar is solar, call `renderSolarGrid(days)` instead of `renderGrid(days)`
- Solar cell contents (simpler):
  ```
  14                    ← Gregorian day
  Chithirai 1           ← Solar month name + day
  Saka 1947             ← Era + year
  ```
- Month-start days (`solar_day == 1`) get a highlight (e.g., left border or background tint) to visually show where the solar month transitions happen
- No adhika/kshaya flags, no tithi colors, no Reingold overlay

**Legend:**
- Lunisolar: show current legend (adhika, kshaya, purnima, amavasya, adhika masa, R/D)
- Solar: show simplified legend (just "Month start" indicator)

**drikpanchang link:**
- Lunisolar: current link to month-panchang page
- Solar: link to the appropriate regional page (e.g., `drikpanchang.com/tamil/panchangam/tamil-month-panchangam.html`)

### 3. Files modified/created

| File | Action |
|------|--------|
| `tools/csv_to_solar_json.py` | **Create**: Generate solar JSON from CSVs |
| `validation/web/index.html` | **Modify**: Add calendar dropdown, solar rendering, state management |
| `validation/web/data/tamil/*.json` | **Generated**: 1,812 files |
| `validation/web/data/bengali/*.json` | **Generated**: 1,812 files |
| `validation/web/data/odia/*.json` | **Generated**: 1,812 files |
| `validation/web/data/malayalam/*.json` | **Generated**: 1,812 files |

### 4. Verification

1. `python3 tools/csv_to_solar_json.py` — generates all 7,248 solar JSON files
2. `bash validation/web/serve.sh` — start dev server
3. Open http://localhost:8082 — default shows Lunisolar (same as before)
4. Switch dropdown to Tamil → see Tamil solar dates for current month
5. Navigate to April 2025 → verify Chithirai 1 on Apr 14
6. Switch to Malayalam → verify Chingam 1 on Aug 17
7. Arrow key navigation works across all calendar types
8. URL hash preserves calendar type on reload
9. R/D overlay checkbox only visible in Lunisolar mode
10. drikpanchang link updates per calendar type
