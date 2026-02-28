# Plan: Remove Swiss Ephemeris Naming from Moshier Library

## Context

The Moshier ephemeris library (`lib/moshier/`) implements public-domain algorithms (VSOP87, DE404, IAU 1976, Meeus) without using any Swiss Ephemeris proprietary code. However, several variable names, function names, and comments were carried over from SE's source files during porting. The user wants to sanitize all naming so nothing traces back to SE.

## Scope

### 1. Moshier Library Renames (`lib/moshier/`)

**moshier_moon.c:**
| Old | New | Reason |
|-----|-----|--------|
| `SWELP` | `LP` | "SWiss Ephemeris LP" → standard Delaunay symbol for mean lunar longitude |
| `chewm()` | `accum_series()` | SE-specific function name → descriptive |
| `LR[8*NLR]` | `moon_lr[8*MOON_LR_N]` | SE table naming → lowercase with prefix |
| `LRT[8*NLRT]` | `moon_lr_t1[8*MOON_LR_T1_N]` | Same |
| `LRT2[6*NLRT2]` | `moon_lr_t2[6*MOON_LR_T2_N]` | Same |
| `NLR` | `MOON_LR_N` | SE define → descriptive |
| `NLRT` | `MOON_LR_T1_N` | Same |
| `NLRT2` | `MOON_LR_T2_N` | Same |
| `ss[5][8]` | `sin_tbl[5][8]` | Cryptic → descriptive |
| `cc[5][8]` | `cos_tbl[5][8]` | Same |

**moshier_sun.c:**
| Old | New | Reason |
|-----|-----|--------|
| `eartabl[]` | `earth_coeffs[]` | SE naming → descriptive |
| `earargs[]` | `earth_args[]` | Same |
| `ear_max_harmonic[]` | `earth_max_harm[]` | Same |
| `ss_tbl[9][24]` | `sin_tbl[9][24]` | Cryptic → descriptive |
| `cc_tbl[9][24]` | `cos_tbl[9][24]` | Same |

**moshier_ayanamsa.c:**
| Old | New | Reason |
|-----|-----|--------|
| Comment `prec_offset = SEMOD_PREC_IAU_1976` | Remove SE constant name | Comment-only |

### 2. Comment Cleanup (`lib/moshier/` and `src/`)

Remove or reword all comments that reference SE source files or constants:

- **moshier_ayanamsa.c line 15:** Remove `prec_offset = SEMOD_PREC_IAU_1976`
- **src/astro.h lines 6, 9:** "Initialize Swiss Ephemeris" / "Close Swiss Ephemeris" → "Initialize ephemeris backend" / "Close ephemeris backend"
- **src/solar.c line 127:** `SE_SIDM_LAHIRI` reference in comment → "Lahiri ayanamsa" (no SE prefix)
- **tests/test_astro.c:** Comments about `swe_day_of_week` → describe ISO convention without SE reference

### 3. Java Port (`java/`)

**MoshierMoon.java:** Rename `SWELP` → `LP` (8 references)

**MoshierSun.java:** Check for `eartabl`/`earargs` — if present, rename to match C.

### 4. Rust Port (`rust/`)

**moon.rs:** Rename `swelp` → `lp` (9 references)

**sun.rs:** Rename `EARTABL` → `EARTH_COEFFS`, `EARARGS` → `EARTH_ARGS`, `EAR_MAX_HARMONIC` → `EARTH_MAX_HARM`

## What NOT to Change

- **`#ifdef USE_SWISSEPH` blocks** in `src/astro.c` and `src/date_utils.c` — these genuinely call SE and must use SE's API names
- **`moshier_*` public API names** — already clean, no SE trace
- **`mods3600()`** — descriptive math name (mod 1,296,000 arcsec), not SE-specific
- **`sscc()`** — standard sin/cos Chebyshev recurrence, not SE-specific
- **`freqs[]`, `phases[]`** — standard VSOP87 terminology
- **Data values** — numerical constants are public-domain science, not copyrightable
- **`J2000_JD`, `J1900_JD`, `STR`, `DEG2RAD`** — standard astronomical constants

## Files to Modify

1. `lib/moshier/moshier_moon.c` — SWELP, chewm, LR/LRT/LRT2, NLR/NLRT/NLRT2, ss/cc
2. `lib/moshier/moshier_sun.c` — eartabl, earargs, ear_max_harmonic, ss_tbl/cc_tbl
3. `lib/moshier/moshier_ayanamsa.c` — SEMOD comment
4. `src/astro.h` — SE comments
5. `src/solar.c` — SE_SIDM_LAHIRI comment
6. `tests/test_astro.c` — swe_day_of_week comments
7. `java/src/main/java/com/hindu/calendar/ephemeris/MoshierMoon.java` — SWELP
8. `java/src/main/java/com/hindu/calendar/ephemeris/MoshierSun.java` — eartabl/earargs if present
9. `rust/src/ephemeris/moon.rs` — swelp
10. `rust/src/ephemeris/sun.rs` — EARTABL, EARARGS, EAR_MAX_HARMONIC

## Verification

1. `make clean && make` — build succeeds, no warnings
2. `make test` — all 53,143 assertions pass (100%)
3. `python3 -m scraper.lunisolar.compare` — 55,136/55,152 (16 mismatches with upper limb)
4. `python3 -m scraper.solar.compare --calendar bengali` — 1,811/1,811 (100%)
5. `grep -r 'SWELP\|eartabl\|earargs\|ear_max_harmonic\|chewm' lib/ src/ tests/` — no matches
6. Java: `cd java && ./gradlew test` — 227 tests pass
7. Rust: `cd rust && cargo test` — 275,396 assertions pass
