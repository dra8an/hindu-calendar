# Fix Malayalam Solar Calendar Critical Time Rule

## Context

~46% of Malayalam solar month starts differ by +1 day from drikpanchang.com. The current implementation uses "apparent noon" (midpoint of sunrise and sunset) as the critical time threshold. When a sankranti falls after noon, the month start is pushed to the next civil day. Since ~half of all sankrantis fall between noon and midnight, this produces a systematic +1 day bias for ~46% of months.

The other three calendars (Tamil=sunset, Bengali=midnight+24min, Odia=end-of-day) all match drikpanchang at 100%. The 46% mismatch rate for Malayalam strongly indicates drikpanchang uses a much later critical time — most likely "end of civil day" (midnight), the same as Odia.

## Step 1: Create diagnostic tool `tools/malayalam_diag.c`

Prints, for each 2025 Malayalam sankranti:
- Exact JD and IST time
- Civil day under noon rule, sunset rule, and midnight rule

This definitively confirms which rule matches drikpanchang before we commit to a fix.

Compile like: `cc -Wall -Wextra -O2 -std=c99 -Ilib/swisseph -Isrc -o build/malayalam_diag tools/malayalam_diag.c build/swe/*.o build/astro.o build/date_utils.o build/tithi.o build/masa.o build/panchang.o build/solar.o -lm`

## Step 2: Run diagnostic and verify against drikpanchang

Run `./build/malayalam_diag`, then fetch drikpanchang.com Malayalam 2025 month data to verify which rule matches all 12 months.

## Step 3: Fix `src/solar.c` critical_time_jd()

**Current** (lines 141-147):
```c
case SOLAR_CAL_MALAYALAM:
    {
        /* Apparent noon = midpoint of sunrise and sunset */
        double sr = sunrise_jd(jd_midnight_ut, loc);
        double ss = sunset_jd(jd_midnight_ut, loc);
        return (sr + ss) / 2.0;
    }
```

**Fix** (end-of-day rule, same formula as Odia):
```c
case SOLAR_CAL_MALAYALAM:
    /* End of civil day — drikpanchang assigns the sankranti to
     * whichever local date it falls on. */
    return jd_midnight_ut + 1.0 - loc->utc_offset / 24.0;
```

Also update the header comment (lines 110-118) to reflect the corrected rule.

## Step 4: Rebuild and quick-check

`make clean && make` then spot-check: `./hindu-calendar -s malayalam -y 2025 -m 8 -d 17`

The 7 currently-matching dates in `test_solar_validation.c` will still pass (those sankrantis fall before noon, so both rules agree).

## Step 5: Regenerate Malayalam CSV

```bash
./build/gen_solar_ref   # regenerates all 4 CSVs; only Malayalam changes
```

Tamil/Bengali/Odia CSVs will be identical (no code change for them).

## Step 6: Verify against drikpanchang.com

Fetch Malayalam month data for 2025 (all 12 months) + sample years, confirm all match the new output.

## Step 7: Update `tests/test_solar_validation.c`

- Expand Malayalam section from 7 to all 12 months of 2025
- Add Chingam 1 dates across decades (same pattern as Tamil Chithirai 1)
- Remove the "~46% differ" disclaimer comment

## Step 8: Check `tests/test_solar.c`

Verify existing Malayalam entries (lines 80-91) still pass. They should — those dates were chosen where both rules agree.

## Step 9: Final `make test`

All suites pass: test_solar, test_solar_validation, test_solar_regression (reads regenerated CSV), plus all lunisolar tests unchanged.

## Files Modified

| File | Change |
|------|--------|
| `src/solar.c` | 2-line fix in `critical_time_jd()` + comment update |
| `validation/solar/malayalam_months_1900_2050.csv` | Regenerated |
| `tests/test_solar_validation.c` | Expand Malayalam from 7 to 12+ entries |

## Files Created

| File | Purpose |
|------|---------|
| `tools/malayalam_diag.c` | Temporary diagnostic (can delete after) |
