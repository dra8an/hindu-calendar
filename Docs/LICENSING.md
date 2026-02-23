# Intellectual Property and Licensing Analysis

This document analyzes the licensing and intellectual property status of every
scientific theory, data table, and algorithm used in the self-contained Moshier
ephemeris library (`lib/moshier/`). Research conducted February 2026, revised
February 2026 after deep provenance investigation.

**Conclusion: All components are freely usable. The computational code derives
from Steve Moshier's permissively licensed astronomical packages (1991-1995),
not from Swiss Ephemeris creative additions. Data tables are uncopyrightable
scientific facts. See the detailed analysis below.**

---

## Summary

| Component | Ultimate Origin | In Our Code Via | License | Risk |
|-----------|----------------|-----------------|---------|------|
| VSOP87 coefficients | Bretagnon & Francou 1988 | Moshier's plan404 (via SE's swemptab.h) | Uncopyrightable scientific data | None |
| VSOP87 evaluation loop | Moshier's gplan.c (plan404) | SE's swemplan.c | Moshier: "may be used freely" | None |
| DE404 lunar data tables | Moshier's DE404 fit (1995) | SE's swemmoon.c | Moshier: "may be used freely" | None |
| DE404 lunar pipeline | Moshier's cmoon/selenog (1991) | SE's swemmoon.c (restructured) | Moshier: "may be used freely" | Low (see Section 8) |
| EMB→Earth correction | Moshier's plan404 | SE's swemplan.c embofs_mosh() | Moshier: "may be used freely" | None |
| Meeus algorithms | Meeus 1991/1998 | Independently implemented | 17 USC 102(b) | None |
| IAU 1976 precession | Lieske et al. 1977 | Independently implemented | Uncopyrightable constants | None |
| Sinclair refraction | Sinclair 1982 | Independently implemented | Uncopyrightable formula | None |
| IERS delta-T data | IERS Bulletins | SE's delta-T table | Uncopyrightable facts | None |

---

## Code Provenance — Honest Account

**This section was added in the February 2026 revision to accurately describe
how the code was developed. The original version of this document understated
the relationship between our code and Swiss Ephemeris source files.**

### What happened

Our Moshier ephemeris library (`lib/moshier/`) was developed by reading the
Swiss Ephemeris source files (`lib/swisseph/`) and extracting the longitude-only
computation pipelines. The SE source files were the *proximate* source of our
code. However, the *ultimate* origin of the computational content within those
SE files is Steve Moshier's original astronomical packages from 1991-1995,
which SE incorporated and restructured.

### The three layers

```
Layer 1: Moshier's original code (1991-1995)
  - plan404.zip: planetary ephemeris (gplan.c + ear404.c, etc.)
  - cmoon.zip / selenog.zip: lunar ephemeris
  - aa-56.zip: astronomical almanac package
  - License: "may be used freely" + explicit BSD grants

Layer 2: Swiss Ephemeris modifications (1996-2021)
  - Koch/Treindl restructured Moshier's code for SE integration
  - Added: coordinate transforms, speed computation, SE caching,
    thread-local storage, fallback logic, wrappers
  - Renamed: LP_equinox → SWELP (2006), g2plan → moon1-moon4
  - License: AGPL v3 / commercial dual license

Layer 3: Our extraction (2026)
  - Extracted longitude-only pipelines from SE source files
  - Did NOT port: coordinate transforms, speed computation,
    SE wrappers, fallback logic, latitude, radius
  - Simplified: removed unused branches, flattened structure
```

### What we ported, and its true origin

| Our function | From SE file | Moshier original | Koch/Treindl additions ported? |
|-------------|-------------|-----------------|-------------------------------|
| `mods3600()` | swemmoon.c, swemplan.c | gplan.c line 22 (macro) | No — SE only changed it from macro to function |
| `sscc()` | swemmoon.c | gplan.c line 230 | No — identical to Moshier's original |
| `chewm()` | swemmoon.c | chewtab.c | No — we omitted latitude/radius cases that Koch added |
| `mean_elements()` | swemmoon.c | mean.c / gplan.c | Partially — Koch extracted named vars from Args[] array |
| `mean_elements_pl()` | swemmoon.c | gplan.c | No — identical to Moshier's original |
| `moon1()`-`moon4()` | swemmoon.c | **Koch's restructuring** of Moshier's g2plan/g1plan/gmoon | Yes — function boundaries are Koch's design |
| Data: z[], LR[], LRT[], LRT2[] | swemmoon.c | Moshier's DE404 fit coefficients | No — numerical data unchanged |
| `vsop87_earth_longitude()` | swemplan.c | gplan.c `gplan()` function | No — evaluation logic is Moshier's |
| Data: eartabl[], earargs[] | swemptab.h | Moshier's ear404.c | No — numerical data unchanged |
| Data: freqs[], phases[] | swemplan.c | gplan.c lines 35-56 | No — identical to Moshier's original |
| `emb_earth_correction()` | swemplan.c `embofs_mosh()` | Moshier's EMB correction | No — algorithm and coefficients are Moshier's |

### The key nuance: moon1()-moon4()

The `moon1()` through `moon4()` function structure is **Koch's reorganization**
of Moshier's original `g2plan()`, `g1plan()`, and `gmoon()` functions.
Moshier's original used a generic table-driven approach (`struct plantbl` +
`g2plan`); Koch split this into four explicit functions when adapting for SE
in April 1996.

Our code follows Koch's `moon1()`-`moon4()` organization. However:
- The mathematical content (every coefficient, every polynomial) is Moshier's
- The explicit perturbation terms in moon1() (e.g., 6.367278, 12.747036,
  23123.70, -10570.02) are from Moshier's DE404 fit
- The data tables walked by chewm() are Moshier's
- Koch's contribution was the function boundary placement, not the algorithms
- Under merger doctrine, there are limited ways to structure a multi-stage
  perturbation pipeline — the organization follows from the mathematics

### What we did NOT port from SE

The following Koch/Treindl additions were **not** included in our library:

- `swi_moshmoon()` / `swi_moshplan()` — SE wrapper functions
- Ecliptic-of-date to J2000 equatorial coordinate transforms
- Speed computation via numerical differentiation
- `TLS` (thread-local storage) modifications
- The SE fallback mechanism (`goto moshier_planet`)
- Latitude and radius computation (moon and planets)
- `SEFLG_MOSEPH` flag and SE API integration
- Later precession models (IAU 2003 P03, Vondrak 2011)

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

## Component-by-Component Analysis

### 1. VSOP87 Solar Theory (moshier_sun.c)

**What it is:** 135 harmonic terms (amplitudes, frequencies, phases) for
computing the Earth-Moon Barycenter's ecliptic longitude, from Bretagnon &
Francou's VSOP87 planetary theory.

**Proximate source:** The data tables (`eartabl[460]`, `earargs[819]`) and
evaluation loop were adapted from SE's `swemptab.h` and `swemplan.c`. These
SE files contain Moshier's plan404 package (circa 1995), which SE incorporated.
The SE file header states: "Moshier planet routines, modified for SWISSEPH by
Dieter Koch."

**Ultimate source:** Moshier's plan404 package (moshier.net/plan404.zip),
which itself implements the VSOP87 theory published in *Astronomy &
Astrophysics*, 202, 309-315 (1988). Data tables freely distributed by IMCCE
via FTP and the CDS VizieR catalog (VI/81).

**License status:**
- VSOP87 data: No license ever applied. IMCCE FTP and CDS VizieR contain no
  copyright notice or usage restrictions. French open data law (2016) makes
  government institution data open by default.
- Moshier's plan404 code: "May be used freely" + explicit BSD grants.
- Our evaluation loop follows Moshier's `gplan()` algorithm from plan404.
  Koch's additions (latitude/radius evaluation, SE wrappers) were not ported.

**Industry practice:** VSOP87 used without restriction by Stellarium (GPL),
Celestia (GPL), NASA eclipse predictions, and dozens of other projects. The
Stellarium developer explicitly noted: "I can neither allow nor forbid the
usage of VSOP87."

**Our attribution:** Source file header cites "VSOP87 planetary theory
(Bretagnon & Francou 1988)." Should be updated to also credit Moshier's
plan404 implementation.

### 2. DE404 Moshier Lunar Ephemeris (moshier_moon.c)

**What it is:** Steve Moshier's analytical fit of the ELP2000-85 lunar theory
(Chapront-Touze & Chapront 1988) to JPL's DE404 numerical ephemeris. Includes
data tables (z[25], LR[118x8], LRT[38x8], LRT2[25x6]) and a multi-stage
computational pipeline (mean elements, perturbation series, light-time
correction).

**Proximate source:** Adapted from SE's `swemmoon.c`. The SE file header
states: "Steve Moshier's analytical lunar ephemeris" with dates "S. L. Moshier,
August, 1991 / DE404 fit: October, 1995 / Dieter Koch: adaptation to SWISSEPH,
April 1996."

**Ultimate source:** Moshier's selenog.zip / cmoon.zip (moshier.net), published
alongside his paper in *Astronomy & Astrophysics*, 262, 613-616 (1992). The
original code consists of `chewtab.c`, `mean.c`, `mlr404.c`, `mlat404.c`,
`selenog.c`, and the generic evaluator `gplan.c`.

**What SE changed from Moshier's original:**
- Restructured `g2plan()`/`g1plan()`/`gmoon()` into `moon1()`-`moon4()`
- Renamed `LP_equinox` to `SWELP` (2006, due to name collision)
- Extracted `Args[]` array indices to named global variables (MP, D, NF, etc.)
- Reformatted data tables from `long` arrays to `short` arrays
- Added latitude/radius evaluation, coordinate transforms, speed computation
- Added "Bhanu Pinnamaneni fix" for ss/cc initialization (2009)

**What we ported:** The longitude-only pipeline (mean elements, perturbation
accumulation via chewm(), explicit terms in moon1()-moon4(), light-time
correction). We follow Koch's moon1()-moon4() function organization.

**What we did NOT port:** Latitude, radius, coordinate transforms, speed
computation, SE wrappers, TLS modifications.

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

**Provenance:** These modules were independently implemented from the published
formulas, not adapted from SE code. The Meeus algorithms in our library have
no structural similarity to any SE implementation.

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

**Provenance:** Independently implemented from the published paper. Not adapted
from SE code.

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

**Provenance:** Independently implemented. The Sinclair formula in our code
matches SE's `calc_astronomical_refr()` in output but was implemented from the
published formula, not by copying SE code.

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

**Proximate source:** The lookup table format and values were adapted from SE's
delta-T implementation. The values themselves are observational facts.

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

## Swiss Ephemeris AGPL — Full Analysis

The Swiss Ephemeris (Astrodienst AG, Koch & Treindl) is dual-licensed under
AGPL v3 and a commercial license. The question is whether our library, which
was developed by reading SE source files, triggers AGPL obligations.

### What SE's AGPL covers

SE's copyright applies to **their creative additions** to the incorporated
Moshier code:
- The SE wrapper functions (`swi_moshmoon()`, `swi_moshplan()`)
- Coordinate transforms (ecliptic-of-date to J2000 equatorial)
- Speed computation via numerical differentiation
- The SE caching and save infrastructure
- Thread-local storage modifications
- The fallback mechanism and `SEFLG_MOSEPH` API flag
- Later precession models (IAU 2003 P03, Vondrak 2011)

**None of these were ported to our library.**

### What SE's AGPL does NOT cover

SE's license cannot retroactively apply to:
- **Moshier's original code** — written 1991-1995, permissively licensed
  ("may be used freely"), with explicit BSD grants to multiple parties
- **Scientific data tables** — VSOP87 coefficients, DE404 fit parameters,
  precession constants, delta-T values are uncopyrightable facts
- **Mathematical algorithms** — 17 USC 102(b) excludes procedures, processes,
  and methods of operation from copyright

SE's own license header acknowledges this limitation: "The authors of Swiss
Ephemeris have no control or influence over any of the derived works, i.e.
over software or services created by other programmers which use Swiss
Ephemeris functions."

### The moon1()-moon4() question

The one area where Koch made a creative structural contribution that we
*did* follow is the reorganization of Moshier's `g2plan()`/`g1plan()`/`gmoon()`
into four numbered functions `moon1()` through `moon4()`. Our code uses this
same four-function organization.

This carries low risk for several reasons:
- The mathematical content within each function is entirely Moshier's
- The function boundaries follow natural mathematical divisions (planetary
  perturbations, main series, T^1 corrections, final accumulation)
- Under merger doctrine, when an idea can only be expressed in a limited
  number of ways, the expression merges with the idea and is not protectable
- A multi-stage perturbation pipeline has limited organizational options
- The function names are purely descriptive (numbered stages)

### Risk assessment

| Component | Risk | Rationale |
|-----------|------|-----------|
| Data tables (all) | None | Scientific facts, uncopyrightable |
| sscc(), chewm(), mods3600() | None | Moshier's original code, permissive license |
| mean_elements(), mean_elements_pl() | None | Moshier's original code |
| VSOP87 evaluation loop | None | Moshier's gplan() algorithm |
| EMB→Earth correction | None | Moshier's algorithm and coefficients |
| moon1()-moon4() structure | Low | Koch's reorganization of Moshier's algorithms; we follow it, but content is Moshier's, and the structure follows from the mathematics |
| Variable name SWELP | Negligible | Koch's 2006 rename; a single variable name is not substantial creative expression |
| Meeus, IAU 1976, Sinclair modules | None | Independently implemented from published sources |

---

## Our Attribution Practices

Each source file in `lib/moshier/` includes header comments citing the
relevant scientific references. **These should be updated to also credit
Moshier's software packages as the implementation reference.**

| File | Current Citations | Should Add |
|------|-------------------|------------|
| `moshier_sun.c` | Bretagnon & Francou 1988 (VSOP87), Meeus Ch. 22 | Moshier plan404 (evaluation algorithm) |
| `moshier_moon.c` | Moshier 1992 (DE404 fit), Chapront-Touze & Chapront 1988 | Moshier selenog/cmoon (implementation) |
| `moshier_ayanamsa.c` | IAU 1976 precession, Lieske et al. 1977 | (none — independently implemented) |
| `moshier_rise.c` | Meeus Ch. 15, Sinclair 1982 | (none — independently implemented) |
| `moshier_jd.c` | Meeus Ch. 7 | (none — independently implemented) |

---

## Timeline of Code Provenance

| Date | Event |
|------|-------|
| 1988 | Bretagnon & Francou publish VSOP87 in A&A |
| 1988 | Chapront-Touze & Chapront publish ELP2000-85 in A&A |
| 1991 | Moshier writes original lunar ephemeris (DE200 fit) |
| 1992 | Moshier publishes DE200 fit paper in A&A 262 |
| 1995 | Moshier completes DE404 fit, publishes plan404 + selenog |
| 1996 | Koch adapts Moshier's code for Swiss Ephemeris |
| 2006 | Koch renames LP_equinox to SWELP due to name collision |
| 2009 | Bhanu Pinnamaneni adds ss/cc initialization fix |
| 2018 | Moshier grants explicit BSD license to NearForm |
| 2026 | Our library developed by extracting longitude pipelines from SE source files containing Moshier's code |

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

### Moshier's Software Packages
7. plan404.zip — Trigonometric planetary ephemeris fitted to DE404:
   moshier.net/plan404.zip
8. selenog.zip — High-precision lunar ephemeris: moshier.net/selenog.zip
9. cmoon.zip — Compact lunar ephemeris: moshier.net/cmoon.zip
10. aa-56.zip — Astronomical almanac calculator: moshier.net/aa-56.zip

### Legal Authorities
11. 17 USC Section 102(b) — Ideas, procedures, methods not copyrightable
12. Baker v. Selden, 101 U.S. 99 (1879) — Idea-expression dichotomy
13. Feist v. Rural, 499 U.S. 340 (1991) — Facts not copyrightable
14. U.S. Copyright Office Circular 31 — Mathematical formulas excluded

### Licensing Evidence
15. Moshier's Cephes README: "may be used freely" (reproduced in Go stdlib)
16. Moshier's BSD grant to NearForm (2018):
    github.com/nearform/node-cephes/blob/master/LICENSE
17. Debian-legal Cephes discussion (2004):
    lists.debian.org/debian-legal/2004/12/msg00295.html
18. IMCCE VSOP87 FTP: ftp.imcce.fr/pub/ephem/planets/vsop87/ (no license file)
19. CDS VizieR VI/81: cdsarc.cds.unistra.fr/viz-bin/cat/VI/81
20. JPL ephemeris export: ssd.jpl.nasa.gov/planets/eph_export.html
21. IERS data policy: re3data.org/repository/r3d100010312 (Open access)
22. ERFA (BSD-3-Clause): github.com/liberfa/erfa
23. SE swemmoon.c header: "Steve Moshier's analytical lunar ephemeris...
    S. L. Moshier, August, 1991 / DE404 fit: October, 1995 /
    Dieter Koch: adaptation to SWISSEPH, April 1996"
24. SE swemplan.c header: "Moshier planet routines, modified for SWISSEPH
    by Dieter Koch"
