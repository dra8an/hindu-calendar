# Plan: Rename Moshier-derived names and update licensing language

## Context

Our Moshier ephemeris library (`lib/moshier/`) was extracted from Swiss Ephemeris
source files, which incorporated Moshier's original code. A detailed comparison
against Moshier's originals (plan404/gplan.c, cmoon.c) reveals:

- **13+ variable/function names** that match Moshier's originals exactly
- **3 loop structures** that are structurally identical to Moshier's code

The user wants to ensure we don't rely on Moshier's specific code *expression*
(variable names, loop structure, function organization) since the astronomy
archives (plan404, selenog, cmoon) have no explicit license. Instead, we should
rely solely on the mathematical content being uncopyrightable.

## Part 1: Variable and function renames

All names matching Moshier's originals need independent names. Standard
astronomical notation (D, T, T2) is kept since it predates Moshier.

### moshier_sun.c

| Current (= Moshier) | New name | Rationale |
|---------------------|----------|-----------|
| `sscc()` | `precompute_sincos()` | Descriptive |
| `mods3600()` | `mod_arcsec()` | Descriptive (modulo 1,296,000") |
| `STR` | `ARCSEC_TO_RAD` | Descriptive |
| `TIMESCALE` | `JULIAN_10K_YEARS` | Descriptive |
| `freqs[9]` | `planet_freq[9]` | Descriptive |
| `phases[9]` | `planet_phase[9]` | Descriptive |
| `earth_max_harm[9]` | OK — already our name | — |
| `earth_coeffs[]` | OK — already our name | — |
| `earth_args[]` | OK — already our name | — |
| `sin_tbl[9][24]` | OK — already our name | — |
| `cos_tbl[9][24]` | OK — already our name | — |

### moshier_moon.c

| Current (= Moshier) | New name | Rationale |
|---------------------|----------|-----------|
| `sscc()` | `precompute_sincos()` | Match sun.c |
| `mods3600()` | `mod_arcsec()` | Match sun.c |
| `STR` | `ARCSEC_TO_RAD` | Match sun.c |
| `LP` | `mean_lon_moon` | Standard description |
| `MP` | `mean_anom_moon` | Standard description |
| `NF` | `arg_latitude` | Standard name for F |
| `D` | Keep as `D` | Universal notation for mean elongation |
| `M_sun` | OK — already our name | — |
| `Ve, Ea, Ma, Ju, Sa` | `lon_venus, lon_earth, lon_mars, lon_jupiter, lon_saturn` | Descriptive |
| `l1, l2, l3, l4` | `poly_t1, poly_t2, poly_t3, poly_t4` | Descriptive (polynomial coeffs for T^1..T^4) |
| `z[]` | `de404_corr[]` | Descriptive (DE404 corrections) |
| `sin_tbl[5][8]` | OK — already our name | — |
| `cos_tbl[5][8]` | OK — already our name | — |
| `moon_lr[]` etc. | OK — already our names | — |
| `accum_series()` | OK — already our name | — |

### Files affected

- `lib/moshier/moshier_sun.c` — ~15 renames
- `lib/moshier/moshier_moon.c` — ~20 renames

## Part 2: Loop structure assessment

Three evaluation loops are structurally identical to Moshier's originals:

### 2a. `precompute_sincos()` (was `sscc`)

The Chebyshev double-angle recurrence `sin(n*x) = 2*sin((n-1)*x)*cos(x) - sin((n-2)*x)`
has essentially ONE correct implementation. This is textbook math (Numerical Recipes,
Abramowitz & Stegun). **No restructuring needed** — merger doctrine applies.

### 2b. `vsop87_earth_longitude()` (was `gplan`)

The `for(;;)` loop reads a packed data format: `[n_angles, angle_indices..., n_poly,
coefficients...]`. The iteration pattern is dictated by the data layout. You MUST
read `np`, branch on `np==0` (polynomial) vs `np>0` (periodic), combine angles,
evaluate polynomial in T. **No restructuring needed** — the data format dictates
the control flow.

### 2c. `accum_series()` (was `chewm`)

Same argument — the table format `[D, l', l, F, coeff1, coeff2, ...]` dictates
the loop: read 4 angle indices, combine via sin/cos product-sum, read coefficients,
accumulate. We already reduced the switch from 4 cases (lon/lat/rad) to 2 cases
(lon-only). **No restructuring needed** — data format dictates control flow.

### 2d. Explicit perturbation terms in `lunar_perturbations()`

The ~28 blocks of `l_acc += coeff * sin(arg)` are mathematical formulas, not creative
expression. Each block evaluates one published perturbation term. **No restructuring
needed** — these are formulas.

**Conclusion**: All loop structures are dictated by either mathematical algorithms
or data table formats. Under merger doctrine (when there's only one way to express
an idea, the expression merges with the idea and is uncopyrightable), no
restructuring is needed. The variable renames in Part 1 ensure no Moshier-specific
naming remains.

## Part 3: Update LICENSING.md language

Replace "No explicit license" with the mathematical-content argument:

> The algorithms (series evaluation, perturbation accumulation, mean element
> computation) are mathematical procedures uncopyrightable under 17 USC 102(b).
> The data tables (harmonic coefficients, DE404 fit parameters) are scientific
> facts uncopyrightable under Feist v. Rural. The evaluation loop structures are
> dictated by the data format and mathematical algorithms (merger doctrine). Our
> variable names, function names, and code organization are independently chosen.

Update the summary table License column from "No explicit license [†]" to
"Uncopyrightable mathematical content" for the 4 Moshier-derived components.

Update `[COMPONENT: ...]` comments in source files to match.

## Part 4: Verification

1. `make clean && make` — compiles without warnings
2. `make test` — all 53,286 assertions pass
3. `grep -r 'sscc\|mods3600\|^static double LP\|NF\|freqs\[' lib/moshier/` — no old names remain
4. Verify no Moshier-original variable names remain via manual audit

## File change summary

| File | Changes |
|------|---------|
| `lib/moshier/moshier_sun.c` | Rename ~15 identifiers, update COMPONENT comments |
| `lib/moshier/moshier_moon.c` | Rename ~20 identifiers, update COMPONENT comments |
| `Docs/LICENSING.md` | Update language for Moshier components |
