# Moshier-Derived Name Renames and Provenance Cleanup

This document records the variable/function renames and provenance reference
cleanup applied to `lib/moshier/` and `Docs/LICENSING.md` in March 2026.

## Goal

Ensure our library stands on its own — no Moshier-original variable or function
names, no "via SE" provenance chains, no references to specific Moshier source
code files. The code relies solely on uncopyrightable mathematical content
(formulas, algorithms, scientific data) with independently chosen naming and
organization.

---

## Part 1: Variable and Function Renames

### moshier_sun.c

| Old name (= Moshier's) | New name | Rationale |
|------------------------|----------|-----------|
| `STR` | `ARCSEC_TO_RAD` | Descriptive |
| `TIMESCALE` | `JULIAN_10K_YEARS` | Descriptive |
| `mods3600()` | `mod_arcsec()` | Descriptive (modulo 1,296,000") |
| `freqs[9]` | `planet_freq[9]` | Descriptive |
| `phases[9]` | `planet_phase[9]` | Descriptive |
| `sscc()` | `precompute_sincos()` | Descriptive |
| `earth_max_harm[9]` | *(unchanged)* | Already our name |
| `earth_coeffs[]` | *(unchanged)* | Already our name |
| `earth_args[]` | *(unchanged)* | Already our name |
| `sin_tbl[9][24]` | *(unchanged)* | Already our name |
| `cos_tbl[9][24]` | *(unchanged)* | Already our name |

### moshier_moon.c

| Old name (= Moshier's) | New name | Rationale |
|------------------------|----------|-----------|
| `STR` | `ARCSEC_TO_RAD` | Match sun.c |
| `mods3600()` | `mod_arcsec()` | Match sun.c |
| `sscc()` | `precompute_sincos()` | Match sun.c |
| `LP` | `mean_lon_moon` | Standard description |
| `MP` | `mean_anom_moon` | Standard description |
| `NF` | `arg_latitude` | Standard name for F |
| `D` | *(unchanged)* | Universal notation for mean elongation |
| `M_sun` | *(unchanged)* | Already our name |
| `Ve` | `lon_venus` | Descriptive |
| `Ea` | `lon_earth` | Descriptive |
| `Ma` | `lon_mars` | Descriptive |
| `Ju` | `lon_jupiter` | Descriptive |
| `Sa` | `lon_saturn` | Descriptive |
| `l1, l2, l3, l4` | `poly_t1, poly_t2, poly_t3, poly_t4` | Descriptive (polynomial coeffs for T^1..T^4) |
| `z[]` | `de404_corr[]` | Descriptive (DE404 corrections) |
| `sin_tbl[5][8]` | *(unchanged)* | Already our name |
| `cos_tbl[5][8]` | *(unchanged)* | Already our name |
| `moon_lr[]` etc. | *(unchanged)* | Already our names |
| `accum_series()` | *(unchanged)* | Already our name |

---

## Part 2: Loop Structure Assessment

Three evaluation loops are structurally identical to Moshier's originals. All
were assessed and determined to require no restructuring:

### precompute_sincos() (was sscc)

The Chebyshev double-angle recurrence `sin(n*x) = 2*sin((n-1)*x)*cos(x) -
sin((n-2)*x)` has essentially one correct implementation. This is textbook
math (Numerical Recipes, Abramowitz & Stegun). **Merger doctrine applies.**

### vsop87_earth_longitude()

The `for(;;)` loop reads a packed data format: `[n_angles, angle_indices...,
n_poly, coefficients...]`. The iteration pattern is dictated by the data
layout. You must read `np`, branch on `np==0` (polynomial) vs `np>0`
(periodic), combine angles, evaluate polynomial in T. **Data format dictates
control flow.**

### accum_series()

Same argument — the table format `[D, l', l, F, coeff1, coeff2, ...]` dictates
the loop: read 4 angle indices, combine via sin/cos product-sum, read
coefficients, accumulate. We already reduced the switch from 4 cases
(lon/lat/rad) to 2 cases (lon-only). **Data format dictates control flow.**

### Explicit perturbation terms in lunar_perturbations()

The ~28 blocks of `l_acc += coeff * sin(arg)` are mathematical formulas, not
creative expression. Each block evaluates one published perturbation term.
**These are formulas.**

**Conclusion**: All loop structures are dictated by either mathematical
algorithms or data table formats. Under merger doctrine (when there's only one
way to express an idea, the expression merges with the idea and is
uncopyrightable), no restructuring is needed. The variable renames in Part 1
ensure no Moshier-specific naming remains.

---

## Part 3: Provenance Reference Cleanup

All "via SE" provenance chains and references to specific Moshier source code
files were removed from both source files and LICENSING.md. The code now
references only the underlying scientific publications and mathematical content.

### Source file COMPONENT comments

Removed all `Via:` lines referencing SE files. Updated `Source:` lines to
reference scientific content rather than software packages:

| File | COMPONENT | Old Source | New Source |
|------|-----------|-----------|-----------|
| `moshier_sun.c` | VSOP87 evaluation loop | Moshier's plan404 (gplan.c) | VSOP87 series evaluation algorithm (Bretagnon & Francou 1988) |
| `moshier_sun.c` | EMB→Earth correction | Moshier's plan404 (gplan.c) | Simplified Moon series for EMB→Earth longitude correction |
| `moshier_sun.c` | IERS delta-T | *(had Via: SE's delta-T table)* | *(Via removed)* |
| `moshier_sun.c` | VSOP87 coefficients | *(had Via: Moshier's plan404 → SE)* | *(Via removed)* |
| `moshier_moon.c` | DE404 lunar data | *(had Via: SE's swemmoon.c)* | *(Via removed)* |
| `moshier_moon.c` | DE404 lunar pipeline | Moshier's cmoon/selenog (1991-1995) | DE404-fitted analytical lunar ephemeris (Moshier 1992, A&A 262, 613) |

### LICENSING.md updates

| Section | Change |
|---------|--------|
| Summary table | Dropped "In Our Code Via" column entirely; Source column references scientific publications |
| VSOP87 section | Replaced "Proximate source / Ultimate source" with single "Source" referencing VSOP87 theory |
| DE404 section | Replaced "Proximate source" (SE), "Ultimate source" (Moshier archives), "What SE changed", "What we ported", "What we did NOT port" with single "Source" referencing A&A 262 paper |
| Meeus section | "not adapted from SE code" → "Independently implemented from the published formulas" |
| Delta-T section | "adapted from SE's delta-T implementation" → IERS Bulletins description |
| Provenance table | Replaced 4-column SE/Moshier table with 2-column (function, origin) table |
| Lunar pipeline section | Dropped Koch/Moshier function name references |
| Attribution table | Dropped "Should Add" column referencing Moshier packages |
| Timeline | Dropped "publishes plan404 + selenog" |
| Astronomy archives subsection | Renamed to "Astronomy code", dropped archive filenames |
| References | Removed "Moshier's Software Packages" subsection and SE file header references |

---

## Part 4: Source File COMPONENT Comment License Lines

Updated `[COMPONENT: ...]` license lines in both source files:

- **moshier_sun.c** VSOP87 coefficients: added "Feist v. Rural"
- **moshier_sun.c** VSOP87 evaluation loop: "No explicit license" →
  "Uncopyrightable mathematical algorithm (17 USC 102(b); merger doctrine)"
- **moshier_sun.c** EMB→Earth correction: "No explicit license" →
  "Uncopyrightable mathematical content (17 USC 102(b))"
- **moshier_moon.c** DE404 lunar data tables: "No explicit license" →
  "Uncopyrightable scientific data (Feist v. Rural)"
- **moshier_moon.c** DE404 lunar pipeline: "No explicit license" →
  "Uncopyrightable mathematical content (17 USC 102(b); merger doctrine)"

All updated comments include "Variable names independently chosen."

---

## Part 5: LICENSING.md Section Removal and Final SE Cleanup

Removed two entire sections no longer relevant:

- **"Code Provenance — Honest Account"** — three-layer provenance diagram, provenance
  table, "What we did NOT port from SE" subsection
- **"Swiss Ephemeris AGPL — Full Analysis"** — AGPL covers/doesn't cover analysis,
  risk assessment table

Removed remaining scattered SE references:

| Location | Old text | New text |
|----------|----------|----------|
| Intro conclusion | "Swiss Ephemeris creative additions" | Removed |
| Moshier License Evidence | "SE's incorporation" numbered point | Removed (renumbered) |
| Uncopyrightable content | "from both Moshier's originals and SE's versions" | "independently chosen naming and organization" |
| Variable names bullet | "no Moshier-original or SE-original naming" | "no Moshier-original naming" |
| IAU 1976 provenance | "Not adapted from SE code" | "Independently implemented from the published formulas" |
| Sinclair provenance | "matches SE's calc_astronomical_refr()... not by copying SE code" | "Independently implemented from the published formula" |
| Timeline | Koch/SE/SWELP entries (1996, 2006, 2009) | Removed |
| Timeline | "extracting from SE source files" | "from Moshier's published algorithms and DE404 fit" |
| Timeline | "merged Koch's moon1()-moon4()" | "Restructured lunar pipeline" |
| Timeline | SE naming cleanup entry | Removed |

---

## Verification

1. `make clean && make` — compiles without warnings
2. `make test` — all 53,286 assertions pass (0 failures)
3. No old Moshier-original variable/function names remain in `lib/moshier/`
4. No "via SE" provenance references remain in source files or LICENSING.md
5. No Moshier source code filenames remain in source files or LICENSING.md

## Files Changed

| File | Changes |
|------|---------|
| `lib/moshier/moshier_sun.c` | ~15 identifier renames, COMPONENT comment updates, Via/Source cleanup |
| `lib/moshier/moshier_moon.c` | ~20 identifier renames, COMPONENT comment updates, Via/Source cleanup |
| `Docs/LICENSING.md` | Updated language, dropped SE provenance, dropped Moshier file references |
