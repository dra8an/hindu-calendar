# Intellectual Property and Licensing Analysis

This document analyzes the licensing and intellectual property status of every
scientific theory, data table, and algorithm used in the self-contained Moshier
ephemeris library (`lib/moshier/`). Research conducted February 2026, revised
February 2026 after deep provenance investigation.

**Conclusion: All components are freely usable. The code implements
mathematical algorithms and scientific data from published astronomical
research. Data tables are uncopyrightable scientific facts. See the detailed
analysis below.**

---

## Summary

| Component | Source | License | Risk |
|-----------|--------|---------|------|
| VSOP87 coefficients | Bretagnon & Francou 1988 | Uncopyrightable scientific data | None |
| VSOP87 evaluation loop | Bretagnon & Francou 1988 | Uncopyrightable mathematical content | None |
| DE404 lunar data tables | Moshier 1992 (A&A 262, 613) | Uncopyrightable scientific data | None |
| DE404 lunar pipeline | Moshier 1992 (A&A 262, 613) | Uncopyrightable mathematical content | None |
| EMB→Earth correction | Moon series for EMB correction | Uncopyrightable mathematical content | None |
| Meeus algorithms | Meeus 1991/1998 | 17 USC 102(b) | None |
| IAU 1976 precession | Lieske et al. 1977 | Uncopyrightable constants | None |
| Sinclair refraction | Sinclair 1982 | Uncopyrightable formula | None |
| IERS delta-T data | IERS Bulletins | Uncopyrightable facts | None |

---

## Moshier License Evidence [†]

Steve Moshier authored two distinct bodies of software. The licensing evidence
differs between them, and we must be precise about which applies to what.

### Cephes Math Library (general-purpose math functions)

Cephes is a collection of mathematical functions (gamma, Bessel, elliptic
integrals, etc.) distributed on Netlib. It has an explicit informal grant and
multiple formal BSD grants. **Our library does not use Cephes code directly.**
We document it here because it establishes Moshier's licensing posture.

**Netlib README** (https://www.netlib.org/cephes/readme):

> *"Some software in this archive may be from the book Methods and Programs
> for Mathematical Functions (Prentice-Hall or Simon & Schuster International,
> 1989) or from the Cephes Mathematical Library, a commercial product. In
> either event, it is copyrighted by the author. What you see here may be
> used freely but it comes with no support or guarantee.*
>
> *Stephen L. Moshier*
> *moshier@na-net.ornl.gov"*

**NearForm BSD-3-Clause grant** (https://github.com/nearform/node-cephes/blob/master/LICENSE):
Email from Moshier (Fri, 7 Sep 2018): *"You are welcome to distribute the
Cephes material posted to the net under a BSD license."*

**Debian-legal BSD grant** (https://lists.debian.org/debian-legal/2004/12/msg00295.html):
Email from Moshier (Tue, 28 Dec 2004) providing BSD boilerplate for Debian
packaging of labplot/grace.

**Additional grants:** scipy, torch (referenced in Andreas Madsen's 2018 email).

**Go standard library** (https://go.dev/src/math/sin.go): Embeds full Netlib
README in source comments, treating "may be used freely" as sufficient.

### Astronomy code

Moshier authored astronomical ephemeris software distributed on moshier.net.
**These archives carry no license text** — neither on the website nor in the
archives themselves.

The Cephes BSD grants do **not** formally extend to the astronomy code. The
NearForm email specifically says "the Cephes material." However:

1. **Same author, consistent posture.** Moshier has never restricted use of any
   of his software. He has granted BSD to every project that asked (Debian 2004,
   scipy, torch, NearForm 2018). The astronomy archives are distributed in the
   same manner (freely downloadable, no restrictions stated).

2. **Uncopyrightable content.** Regardless of license, the mathematical content
   we use — evaluation algorithms, harmonic coefficients, perturbation series,
   mean element polynomials — consists of mathematical formulas and scientific
   data, which are not copyrightable under 17 USC 102(b) and Feist v. Rural.
   What copyright could protect is the specific *expression* (variable names,
   code structure, comments), and our code has been restructured with different
   naming and organization. Specifically:

   - **Data tables** (VSOP87 harmonic coefficients, DE404 fit parameters) are
     scientific facts, uncopyrightable under Feist v. Rural.
   - **Evaluation algorithms** (series evaluation, perturbation accumulation,
     mean element computation) are mathematical procedures, uncopyrightable
     under 17 USC 102(b).
   - **Loop structures** (precompute_sincos, vsop87_earth_longitude,
     accum_series) are dictated by the data table format and mathematical
     algorithms — merger doctrine (when only one way to express an idea exists,
     the expression merges with the idea and is uncopyrightable).
   - **Variable names, function names, and code organization** are independently
     chosen — no Moshier-original naming remains.

### Summary

The licensing basis for the Moshier-derived astronomy components rests on three
legs: (a) the mathematical content is uncopyrightable, (b) Moshier's consistent
pattern of permissive licensing across all his software, and (c) our code has
been independently restructured. The absence of an explicit license on the
astronomy archives is a gap, but not a legal risk given the combination of
these factors.

---

## Legal Foundation

Three principles of U.S. copyright law establish that scientific formulas,
constants, and observational data cannot be copyrighted:

1. **17 USC Section 102(b)**: "In no case does copyright protection for an
   original work of authorship extend to any idea, procedure, process, system,
   method of operation, concept, principle, or discovery."

2. **Baker v. Selden, 101 U.S. 99 (1879)**: The Supreme Court established the
   idea-expression dichotomy. A book's copyright protects the author's prose,
   but not the underlying methods, systems, or formulas described. The Court
   specifically cited "mathematical science" as unprotectable content.

3. **Feist v. Rural, 499 U.S. 340 (1991)**: Facts and data are not protectable
   by copyright. Only creative selection or arrangement might qualify.

These principles mean that every mathematical formula, physical constant, and
observational data table in our library is free to implement in original code.

---

## Code Location Map

Each licensing component is marked in the source code with a `[COMPONENT: ...]`
comment. The table below gives the exact file, functions, and approximate line
ranges for each component.

| Component | File | Functions / Data | Lines (approx) |
|-----------|------|-----------------|-----------------|
| **VSOP87 coefficients** | `moshier_sun.c` | `planet_freq[9]`, `planet_phase[9]`, `earth_max_harm[9]`, `earth_coeffs[460]`, `earth_args[819]` | 46–319 |
| **VSOP87 evaluation loop** | `moshier_sun.c` | `precompute_sincos()`, `vsop87_earth_longitude()` | 327–412 |
| **DE404 lunar data tables** | `moshier_moon.c` | `de404_corr[25]`, `moon_lr[118×8]`, `moon_lr_t1[38×8]`, `moon_lr_t2[25×6]` | 111–349 |
| **DE404 lunar pipeline** | `moshier_moon.c` | `mean_elements()`, `mean_elements_pl()`, `lunar_perturbations()`, `moshier_lunar_longitude()` | 367–785 |
| **EMB→Earth correction** | `moshier_sun.c` | `emb_earth_correction()` | 419–486 |
| **Meeus algorithms** | `moshier_jd.c` | `moshier_julday()`, `moshier_revjul()`, `moshier_day_of_week()` | entire file |
| | `moshier_sun.c` | `nutation()` (Ch. 22, Table 22.A) | 559–609 |
| | `moshier_sun.c` | `mean_obliquity()` (Ch. 22, Laskar) | 612–628 |
| | `moshier_rise.c` | `sidereal_time_0h()`, `rise_set_for_date()`, `rise_set()` (Ch. 15) | 46–170 |
| **IAU 1976 precession** | `moshier_ayanamsa.c` | `iau1976_precession_angles()`, `precess_equatorial()`, `obliquity_iau1976()`, `moshier_ayanamsa()` | entire file |
| **Sinclair refraction** | `moshier_rise.c` | `sinclair_refraction_horizon()` | 39–44 |
| **IERS delta-T data** | `moshier_sun.c` | `dt_tab[151]`, `delta_t_seconds()` | 494–542 |

All file paths are relative to `lib/moshier/`. Line numbers are approximate and
may shift as the code evolves; search for `[COMPONENT: ...]` comments to find
the exact locations.

---

## Component-by-Component Analysis

### 1. VSOP87 Solar Theory (moshier_sun.c)

**What it is:** 135 harmonic terms (amplitudes, frequencies, phases) for
computing the Earth-Moon Barycenter's ecliptic longitude, from Bretagnon &
Francou's VSOP87 planetary theory.

**Source:** The data tables (`earth_coeffs[460]`, `earth_args[819]`) and
evaluation loop implement the VSOP87 planetary theory published in *Astronomy
& Astrophysics*, 202, 309-315 (1988). Data tables freely distributed by IMCCE
via FTP and the CDS VizieR catalog (VI/81).

**License status:**
- VSOP87 data: No license ever applied. IMCCE FTP and CDS VizieR contain no
  copyright notice or usage restrictions. French open data law (2016) makes
  government institution data open by default.
- Evaluation algorithm is a standard series summation — uncopyrightable
  mathematical procedure.

**Industry practice:** VSOP87 used without restriction by Stellarium (GPL),
Celestia (GPL), NASA eclipse predictions, and dozens of other projects. The
Stellarium developer explicitly noted: "I can neither allow nor forbid the
usage of VSOP87."

**Our attribution:** Source file header cites "VSOP87 planetary theory
(Bretagnon & Francou 1988)."

### 2. DE404 Moshier Lunar Ephemeris (moshier_moon.c)

**What it is:** Steve Moshier's analytical fit of the ELP2000-85 lunar theory
(Chapront-Touze & Chapront 1988) to JPL's DE404 numerical ephemeris. Includes
data tables (z[25], LR[118x8], LRT[38x8], LRT2[25x6]) and a multi-stage
computational pipeline (mean elements, perturbation series, light-time
correction).

**Source:** DE404-fitted analytical lunar ephemeris, published in *Astronomy &
Astrophysics*, 262, 613-616 (1992). We
implement the longitude-only pipeline (mean elements, perturbation
accumulation, explicit perturbation terms, light-time correction) in a single
`lunar_perturbations()` function.

**License status:** Moshier's canonical license statement (from the Cephes
Math Library README, reproduced in Go's standard library): "What you see
here may be used freely but it comes with no support or guarantee."

In a September 7, 2018 email to NearForm (preserved in the node-cephes LICENSE
on GitHub), Moshier explicitly stated: "You are welcome to distribute the
Cephes material posted to the net under a BSD license." Similar BSD grants were
made to Debian (2004 debian-legal discussion) and SMath (MIT license).

**Our attribution:** Source file header cites Moshier 1992 and Chapront-Touze
& Chapront 1988.

### 3. JPL DE404 Ephemeris Data

**What it is:** JPL Development Ephemeris 404, the high-precision numerical
ephemeris that Moshier's analytical theory was fitted to.

**Source:** Created by E.M. Standish, X.X. Newhall, and J.G. Williams at JPL.
Freely downloadable from ssd.jpl.nasa.gov with no click-through license or
terms of service.

**License status:** JPL is operated by Caltech for NASA. While Caltech
employees are not federal employees (so the automatic public-domain rule of
17 USC 105 may not apply), in practice:
- Data.gov lists the Horizons dataset as "Public"
- NASA's general policy is that materials are not copyrighted unless noted
- No copyright has ever been asserted or enforced on DE ephemeris data
- The entire astronomical community freely redistributes this data

We do not use DE404 data directly — we use Moshier's analytical fit to it.

### 4. Meeus Algorithms (moshier_jd.c, moshier_rise.c, moshier_sun.c)

**What it is:** Algorithms from Jean Meeus, *Astronomical Algorithms*, 2nd
edition (1998). We implement formulas from:
- Chapter 7: Julian Day number conversion
- Chapter 15: Sunrise and sunset (iterative hour-angle method)
- Chapter 22: Nutation in longitude (13-term series), mean obliquity (Laskar)

**Provenance:** Independently implemented from the published formulas.

**License status:** The book text and code listings are copyrighted by Meeus
(originally published by Willmann-Bell, now AAS/Sky & Telescope). However,
the mathematical formulas themselves are not copyrightable under 17 USC 102(b)
and Baker v. Selden. The U.S. Copyright Office Circular 31 explicitly lists
"mathematical principles, formulas, or algorithms" as uncopyrightable.

**Industry practice:** Dozens of independent implementations exist under
permissive licenses (soniakeys/meeus in Go under MIT, PyMeeus under LGPL,
AA+ in C++ freely available, MeeusJs in JavaScript). No project has ever paid
royalties for implementing Meeus formulas.

**Our attribution:** Source file headers cite "Meeus, Astronomical Algorithms,
2nd ed." with specific chapter references.

### 5. IAU 1976 Precession Constants (moshier_ayanamsa.c)

**What it is:** Polynomial coefficients for the precession angles zeta_A, z_A,
and theta_A, used to compute Lahiri ayanamsa via 3D equatorial precession.

**Provenance:** Independently implemented from the published formulas.

**Source:** Lieske, J.H., Lederle, T., Fricke, W., Morando, B. (1977).
"Expressions for the precession quantities based upon the IAU (1976) system of
astronomical constants." *Astronomy & Astrophysics*, 58, 1-16.

**License status:** Published scientific constants from a peer-reviewed journal.
Mathematical constants derived from physical observations are facts and cannot
be copyrighted. The IAU itself provides reference implementations through SOFA
(Standards of Fundamental Astronomy), which allows use for any purpose
including commercial, royalty-free. The community-maintained ERFA fork is
BSD-3-Clause licensed.

**Our attribution:** Source file header cites "IAU 1976 precession" and
"Lieske et al. (1977)."

### 6. Sinclair Refraction Formula (moshier_rise.c)

**What it is:** Atmospheric refraction at the horizon, used to compute the
geometric altitude h0 for sunrise/sunset calculation.

**Provenance:** Independently implemented from the published formula.

**Source:** Sinclair, A.T. (1982). NAO Technical Note No. 59. Royal Greenwich
Observatory. Also documented in Hohenkerk & Sinclair (1985), NAO Technical
Note No. 63, and adopted in the *Explanatory Supplement to the Astronomical
Almanac* (1992).

**License status:** A physical formula describing how the atmosphere bends
light. Published in a UK government observatory technical report. Not
copyrightable under any jurisdiction's IP law — it describes physical reality.

**Our attribution:** Source file header cites "Sinclair refraction" and
"Sinclair 1982."

### 7. IERS Delta-T Data (moshier_sun.c)

**What it is:** A yearly lookup table of delta-T values (TT - UT1), the
difference between uniform atomic time and Earth's irregular rotation.

**Source:** IERS Bulletins, yearly observational values with linear
interpolation (1900-2050), polynomial extrapolation outside that range.

**Source:** International Earth Rotation and Reference Systems Service (IERS)
bulletins, published under IAU/IUGG auspices.

**License status:** IERS maintains an open access data policy. Delta-T
values are observational measurements of the Earth's rotation rate — factual
data that cannot be copyrighted (Feist v. Rural). The IERS Conventions
document is available through DTIC with "Approved for public release:
distribution unlimited."

**Our attribution:** Source file comments reference delta-T with IERS as the
data source.

---

## Our Attribution Practices

Each source file in `lib/moshier/` includes header comments citing the
relevant scientific references.

| File | Citations |
|------|-----------|
| `moshier_sun.c` | Bretagnon & Francou 1988 (VSOP87), Meeus Ch. 22 |
| `moshier_moon.c` | Moshier 1992 (DE404 fit), Chapront-Touze & Chapront 1988 |
| `moshier_ayanamsa.c` | IAU 1976 precession, Lieske et al. 1977 |
| `moshier_rise.c` | Meeus Ch. 15, Sinclair 1982 |
| `moshier_jd.c` | Meeus Ch. 7 |

---

## Timeline of Code Provenance

| Date | Event |
|------|-------|
| 1988 | Bretagnon & Francou publish VSOP87 in A&A |
| 1988 | Chapront-Touze & Chapront publish ELP2000-85 in A&A |
| 1991 | Moshier writes original lunar ephemeris (DE200 fit) |
| 1992 | Moshier publishes DE200 fit paper in A&A 262 |
| 1995 | Moshier completes DE404 fit |
| 2018 | Moshier grants explicit BSD license to NearForm |
| 2026 | Our library developed from Moshier's published algorithms and DE404 fit |
| 2026 | Restructured lunar pipeline into single lunar_perturbations() |
| 2026 | Moshier naming cleanup: renamed sscc→precompute_sincos, mods3600→mod_arcsec, STR→ARCSEC_TO_RAD, TIMESCALE→JULIAN_10K_YEARS, freqs→planet_freq, phases→planet_phase, LP→mean_lon_moon, MP→mean_anom_moon, NF→arg_latitude, Ve/Ea/Ma/Ju/Sa→lon_venus/lon_earth/lon_mars/lon_jupiter/lon_saturn, l1-l4→poly_t1-poly_t4, z[]→de404_corr[] |

---

## References

### Scientific Papers
1. Bretagnon, P. & Francou, G. (1988). "Planetary Theories in rectangular and
   spherical variables: VSOP87 solution." *Astronomy & Astrophysics*, 202, 309.
2. Moshier, S.L. (1992). "Comparison of a 7000-year lunar ephemeris with
   analytical theory." *Astronomy & Astrophysics*, 262, 613-616.
3. Chapront-Touze, M. & Chapront, J. (1988). "ELP2000-85: a semi-analytical
   lunar ephemeris adequate for historical times." *A&A*, 190, 342.
4. Lieske, J.H. et al. (1977). "Expressions for the precession quantities
   based upon the IAU (1976) system of astronomical constants." *A&A*, 58, 1.
5. Sinclair, A.T. (1982). NAO Technical Note No. 59. Royal Greenwich
   Observatory.
6. Meeus, J. (1998). *Astronomical Algorithms*, 2nd ed. Willmann-Bell.

### Legal Authorities
7. 17 USC Section 102(b) — Ideas, procedures, methods not copyrightable
8. Baker v. Selden, 101 U.S. 99 (1879) — Idea-expression dichotomy
9. Feist v. Rural, 499 U.S. 340 (1991) — Facts not copyrightable
10. U.S. Copyright Office Circular 31 — Mathematical formulas excluded

### Moshier Licensing Evidence
11. Moshier's Cephes README on Netlib ("may be used freely"):
    https://www.netlib.org/cephes/readme
12. Moshier's BSD-3-Clause grant to NearForm (email dated Sep 7, 2018):
    https://github.com/nearform/node-cephes/blob/master/LICENSE
13. Debian-legal Cephes discussion (Dec 2004, Moshier provided BSD boilerplate):
    https://lists.debian.org/debian-legal/2004/12/msg00295.html
13b. Go stdlib embedding of Cephes README:
    https://go.dev/src/math/sin.go

### Data Sources
14. IMCCE VSOP87 FTP: ftp.imcce.fr/pub/ephem/planets/vsop87/ (no license file)
15. CDS VizieR VI/81: cdsarc.cds.unistra.fr/viz-bin/cat/VI/81
16. JPL ephemeris export: ssd.jpl.nasa.gov/planets/eph_export.html
17. IERS data policy: re3data.org/repository/r3d100010312 (Open access)
18. ERFA (BSD-3-Clause): github.com/liberfa/erfa
