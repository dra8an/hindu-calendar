# Malayalam Solar Calendar Critical Time Investigation

## Context

The original hypothesis was that ~46% of Malayalam solar month starts differ by +1 day from drikpanchang.com, and the apparent noon critical time should be changed to midnight (end-of-day). This was based on misreading drikpanchang.com's festivals calendar page, which shows **sankramam dates** (the astronomical event), not **month start dates** (the civil day the month begins). These differ when a sankranti falls after apparent noon.

## Investigation

### Step 1: Diagnostic tool (`tools/malayalam_diag.c`)

Created a diagnostic that prints each 2025 Malayalam sankranti with exact JD, IST time, and the civil day under three candidate rules (noon, sunset, midnight).

5 of 12 months in 2025 have the sankranti fall between noon and midnight, producing different results under noon vs midnight rules:

| Month | Sankranti IST | Noon Rule | Midnight Rule |
|-------|--------------|-----------|---------------|
| Kumbham | Feb 12 21:55 | Feb 13 | Feb 12 |
| Meenam | Mar 14 18:50 | Mar 15 | Mar 14 |
| Karkadakam | Jul 16 17:30 | Jul 17 | Jul 16 |
| Thulam | Oct 17 13:45 | Oct 18 | Oct 17 |
| Vrishchikam | Nov 16 13:36 | Nov 17 | Nov 16 |

### Step 2: Verification against prokerala.com + drikpanchang.com

Verified all 5 edge cases against prokerala.com daily calendar pages:

| Month | Noon Rule | Midnight Rule | prokerala.com | Winner |
|-------|-----------|---------------|---------------|--------|
| Kumbham | **Feb 13** | Feb 12 | Feb 13 | **NOON** |
| Meenam | **Mar 15** | Mar 14 | Mar 15 | **NOON** |
| Karkadakam | **Jul 17** | Jul 16 | Jul 17 | **NOON** |
| Thulam | **Oct 18** | Oct 17 | Oct 18 | **NOON** |
| Vrishchikam | **Nov 17** | Nov 16 | Nov 17 | **NOON** |

All 5 edge cases match the **noon rule**. The midnight hypothesis was wrong.

### Step 3: No code fix needed

The apparent noon rule in `src/solar.c` is correct. No changes to `critical_time_jd()`.

### Step 4: Expanded test coverage

Updated `tests/test_solar_validation.c`:
- Expanded Malayalam from 7 to 28 entries
- Added Chingam 1 dates across 16 years (1950-2030, every 5 years)
- Added all 12 months of 2025 (verified against prokerala.com)
- Removed "~46% differ" disclaimer

All 53,143 assertions pass across 10 test suites (updated in v0.3.2 with ayanamsa buffer adjustments — see `Docs/MALAYALAM_ADJUSTMENTS.md`).

## Lesson Learned

drikpanchang.com's festivals/calendar overview page shows **sankramam dates** (when the astronomical event occurs), not the first day of the new month. For Malayalam, these differ whenever the sankranti falls after apparent noon. The **daily panchangam** page is the authoritative source for which civil day belongs to which month.

## Files Modified

| File | Change |
|------|--------|
| `tests/test_solar_validation.c` | Expanded Malayalam from 7 to 28 entries |
| `src/solar.c` | Fixed inaccurate comment (Odia rule description) |

## Files Created

| File | Purpose |
|------|---------|
| `tools/malayalam_diag.c` | Diagnostic tool (retained for future edge-case debugging) |
