# Reingold Diff Overlay on Validation Web Page

## Context
We have `reingold_1900_2050.csv` with both `hindu-lunar-from-fixed` (HL) and `astro-hindu-lunar-from-fixed` (AL) values for every date 1900-2050. We want the validation web page to visually highlight dates where Reingold disagrees with our calculation, showing their values alongside ours.

## Approach: Embed Reingold diffs into per-month JSON, render in HTML

Two files to modify. No new files needed.

## Files to modify

| File | Change |
|------|--------|
| `tools/csv_to_json.py` | Load Reingold CSV, add HL/AL diff fields to each day's JSON when they differ |
| `validation/web/index.html` | CSS for diff cell background + render Reingold values inline |

## Step 1: `tools/csv_to_json.py`

- Load `validation/reingold/reingold_1900_2050.csv` keyed by `(year, month, day)`
- For each day object being built (line ~116), look up Reingold data for that date
- Compare our `tithi`/`masa`/`adhika` vs their `hl_tithi`/`hl_masa`/`hl_adhika` (and `al_*`)
- Add fields **only when different** (keeps JSON small for the 90%+ matching dates):
  - `hl_tithi`, `hl_masa`, `hl_adhika` — only if they differ from ours
  - `hl_diff: true` — convenience flag for quick CSS class check
- Only HL (Surya Siddhanta) diffs shown — it's the closer model (89% match). AL omitted to keep cells clean.
- If Reingold CSV is missing, skip gracefully (the page works without it)

**JSON example for a differing day (Jan 6, 1900):**
```json
{"day":6, "tithi":6, ..., "hl_diff":true, "hl_tithi":5}
```

## Step 2: `validation/web/index.html`

### CSS
- `.reingold-diff` — light orange background `#fff3e0` for cells where HL differs
- `.cell-reingold` — small (9px) red/orange text for Reingold's values, displayed inline after our values

### Rendering (in `renderGrid()`, after line ~293)
For each day cell, if `d.hl_diff`:
1. Add `reingold-diff` CSS class to the `<td>` (orange background)
2. After the tithi div, if `d.hl_tithi` exists, add a small red line: `R/D: S-4 Panchami` (formatted same as our tithi)
3. After the masa div, if `d.hl_masa` exists, add: `R/D: Magha` (or `R/D: Adhika Magha` if hl_adhika)
4. Uses `.cell-reingold` class — small red text, discreet but visible

### Header toggle
- Add a small checkbox in the header bar: `"R/D overlay"` — shows/hides the diff overlay
- Default: on. When off, hides `.reingold-diff` background and `.cell-reingold` text via CSS class on body
- Simple: toggling adds/removes `class="hide-reingold"` on `<body>`, with CSS rule `.hide-reingold .cell-reingold { display:none } .hide-reingold .reingold-diff { background:inherit }`

### Legend
- Add peach swatch: "R/D: Reingold differs"

## Step 3: Regenerate JSON

```bash
python3 tools/csv_to_json.py
```

Regenerates all 1,812 JSON files with Reingold fields embedded.

## Verification
1. `python3 tools/csv_to_json.py` completes, reports 1,812 files
2. Check `data/1900-01.json` — day 6 should have `hl_diff:true, hl_tithi:5`
3. Open web page at `#1900-01` — day 6 has orange background, `[R:5]` after tithi
4. Toggle checkbox hides/shows overlay
5. Navigate to `#2025-01` — no diff cells (HL matches for Jan 2025)
