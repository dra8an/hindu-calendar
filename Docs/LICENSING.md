# Intellectual Property and Licensing Analysis

This document analyzes the licensing and intellectual property status of every
scientific theory, data table, and algorithm used in the self-contained Moshier
ephemeris library (`lib/moshier/`). Research conducted February 2026.

**Conclusion: All components are freely usable with zero licensing risk.**

---

## Summary

| Component | Source | Copyrightable? | Status | Risk |
|-----------|--------|---------------|--------|------|
| VSOP87 coefficients | Bretagnon & Francou 1988 | No (scientific data) | No license; freely distributed | None |
| DE404 Moshier lunar theory | Moshier 1992 | Code: permissive; Data: no | "May be used freely" | None |
| JPL DE404 ephemeris data | JPL/NASA | No (observational data) | Effectively public domain | None |
| Meeus algorithms | Meeus 1991/1998 | No (math formulas) | 17 USC 102(b) | None |
| IAU 1976 precession | Lieske et al. 1977 | No (scientific constants) | Published journal paper | None |
| Sinclair refraction | Sinclair 1982 | No (physical formula) | Published gov't report | None |
| IERS delta-T data | IERS Bulletins | No (observational facts) | Open access mandate | None |

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

**Source:** Published in *Astronomy & Astrophysics*, 202, 309-315 (1988).
Data tables freely distributed by IMCCE (Institut de Mecanique Celeste et de
Calcul des Ephemerides) via FTP and the CDS VizieR catalog (VI/81).

**License status:** No license was ever applied to the VSOP87 data. The
original files on IMCCE's FTP server and CDS VizieR contain no copyright
notice, no license text, and no usage restrictions. The CDS VizieR usage
policy requests citation of the original paper in scientific publications.

**French open data law:** The Bureau des Longitudes (parent of IMCCE) is a
French government institution established in 1795. Under the Loi pour une
Republique numerique (2016), data from French public institutions is open by
default and free for reuse.

**Industry practice:** Used without restriction by Stellarium (GPL), Celestia
(GPL), Swiss Ephemeris (AGPL), NASA eclipse predictions, and dozens of other
projects. The Stellarium developer explicitly noted: "I can neither allow nor
forbid the usage of VSOP87" (i.e., the data belongs to no one). The
gmiller123456/vsop87-multilang project releases VSOP87 implementations under
public domain.

**Our attribution:** Source file header cites "VSOP87 planetary theory
(Bretagnon & Francou 1988)." Data tables labeled "VSOP87 data tables for
Earth (Bretagnon & Francou 1988)."

### 2. DE404 Moshier Lunar Ephemeris (moshier_moon.c)

**What it is:** Steve Moshier's analytical fit of the ELP2000-85 lunar theory
(Chapront-Touze & Chapront 1988) to JPL's DE404 numerical ephemeris. Includes
data tables (z[26], LR[118x8], LRT[38x8], LRT2[25x6]) and a multi-stage
computational pipeline (mean elements, perturbation series, light-time
correction).

**Source:** Published in *Astronomy & Astrophysics*, 262, 613-616 (1992).
Code distributed at moshier.net as cmoon.zip, labeled "Freeware" ($0).

**License status:** Moshier's canonical license statement (from the Cephes
Math Library README, reproduced in Go's standard library): "What you see
here may be used freely but it comes with no support or guarantee."

In a September 7, 2018 email to NearForm (preserved in the node-cephes LICENSE
on GitHub), Moshier explicitly stated: "You are welcome to distribute the
Cephes material posted to the net under a BSD license." Similar BSD grants were
made to Debian (2004 debian-legal discussion) and SMath (MIT license).

**Algorithms vs. code:** Our implementation is independently written C code
implementing the same mathematical algorithms. Under 17 USC 102(b),
mathematical algorithms and numerical coefficients (the DE404 fit parameters)
are not copyrightable. Even if we had copied Moshier's code directly, his
permissive "may be used freely" grant would allow it.

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
- Chapter 47: Position of the Moon (referenced via Moshier's implementation)

**License status:** The book text and code listings are copyrighted by Meeus
(originally published by Willmann-Bell, now AAS/Sky & Telescope). However,
the mathematical formulas themselves are not copyrightable under 17 USC 102(b)
and Baker v. Selden. The U.S. Copyright Office Circular 31 explicitly lists
"mathematical principles, formulas, or algorithms" as uncopyrightable.

**Industry practice:** Dozens of independent implementations exist under
permissive licenses (soniakeys/meeus in Go under MIT, PyMeeus under LGPL,
AA+ in C++ freely available, MeeusJs in JavaScript). No project has ever paid
royalties for implementing Meeus formulas.

**Our implementation:** We wrote original C code implementing the published
formulas. We did not copy any code listings from the book.

**Our attribution:** Source file headers cite "Meeus, Astronomical Algorithms,
2nd ed." with specific chapter references.

### 5. IAU 1976 Precession Constants (moshier_ayanamsa.c)

**What it is:** Polynomial coefficients for the precession angles zeta_A, z_A,
and theta_A, used to compute Lahiri ayanamsa via 3D equatorial precession.

**Source:** Lieske, J.H., Lederle, T., Fricke, W., Morando, B. (1977).
"Expressions for the precession quantities based upon the IAU (1976) system of
astronomical constants." *Astronomy & Astrophysics*, 58, 1-16.

**License status:** Published scientific constants from a peer-reviewed journal.
Mathematical constants derived from physical observations are facts and cannot
be copyrighted. The IAU itself provides reference implementations through SOFA
(Standards of Fundamental Astronomy), which allows use for any purpose
including commercial, royalty-free. The community-maintained ERFA fork is
BSD-3-Clause licensed.

**Our implementation:** We independently implemented the published precession
formulas. We do not use SOFA or ERFA code.

**Our attribution:** Source file header cites "IAU 1976 precession" and
"Lieske et al. (1977)."

### 6. Sinclair Refraction Formula (moshier_rise.c)

**What it is:** Atmospheric refraction at the horizon, used to compute the
geometric altitude h0 for sunrise/sunset calculation.

**Source:** Sinclair, A.T. (1982). NAO Technical Note No. 59. Royal Greenwich
Observatory. Also documented in Hohenkerk & Sinclair (1985), NAO Technical
Note No. 63, and adopted in the *Explanatory Supplement to the Astronomical
Almanac* (1992).

**License status:** A physical formula describing how the atmosphere bends
light. Published in a UK government observatory technical report. Not
copyrightable under any jurisdiction's IP law — it describes physical reality.

**Industry practice:** Implemented in SLALIB (GPL, UK government-funded
Starlink), PAL (GPL), IAU SOFA, and numerous other projects.

**Our attribution:** Source file header cites "Sinclair refraction" and
"Sinclair 1982."

### 7. IERS Delta-T Data (moshier_sun.c)

**What it is:** A yearly lookup table of delta-T values (TT - UT1), the
difference between uniform atomic time and Earth's irregular rotation. Used
to convert between Universal Time and Terrestrial Time.

**Source:** International Earth Rotation and Reference Systems Service (IERS)
bulletins, published under IAU/IUGG auspices.

**License status:** IERS maintains an open access data policy. The Registry
of Research Data Repositories classifies IERS data access as "Open." Delta-T
values are observational measurements of the Earth's rotation rate — factual
data that cannot be copyrighted (Feist v. Rural). The IERS Conventions
document is available through DTIC with "Approved for public release:
distribution unlimited."

**Our attribution:** Source file comments reference delta-T with IERS as the
data source.

---

## Swiss Ephemeris AGPL — Does It Affect Us?

**No.** The Swiss Ephemeris (Astrodienst AG) is dual-licensed under AGPL and a
commercial license. Their license covers their software — their specific C
code, their build system, their documentation. It does not and cannot apply to:

- The underlying scientific theories (VSOP87, Moshier, IAU 1976)
- The published mathematical algorithms
- The numerical coefficient tables (scientific facts)
- Independent implementations of the same algorithms

Under 17 USC 102(b), copyright does not extend to "any idea, procedure,
process, system, method of operation." Our library implements the same
published scientific theories using independently written C code. This is
analogous to how multiple GPS receivers can implement the same orbital
calculation algorithms without infringing each other's copyrights.

Astrodienst cannot retroactively change the license on code they did not
write (Moshier's original algorithms) or on data they did not create (VSOP87,
JPL DE404, IAU constants).

---

## Our Attribution Practices

Each source file in `lib/moshier/` includes header comments citing the
relevant scientific references:

| File | Citations |
|------|-----------|
| `moshier_sun.c` | Bretagnon & Francou 1988 (VSOP87), Meeus Ch. 22 (nutation, obliquity) |
| `moshier_moon.c` | Moshier 1992 (DE404 fit), Chapront-Touze & Chapront 1988 (ELP2000-85) |
| `moshier_ayanamsa.c` | IAU 1976 precession, Lieske et al. 1977 (Lahiri ayanamsa) |
| `moshier_rise.c` | Meeus Ch. 15 (sunrise/sunset), Sinclair 1982 (refraction) |
| `moshier_jd.c` | Meeus Ch. 7 (Julian Day conversion) |

This follows standard practice across the astronomical software community.

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

### Licensing Evidence
11. Moshier's Cephes README: "may be used freely" (reproduced in Go stdlib)
12. Moshier's BSD grant to NearForm (2018): github.com/nearform/node-cephes/blob/master/LICENSE
13. Debian-legal Cephes discussion (2004): lists.debian.org/debian-legal/2004/12/msg00295.html
14. IMCCE VSOP87 FTP: ftp.imcce.fr/pub/ephem/planets/vsop87/ (no license file)
15. CDS VizieR VI/81: cdsarc.cds.unistra.fr/viz-bin/cat/VI/81 (no copyright in ReadMe)
16. JPL ephemeris export: ssd.jpl.nasa.gov/planets/eph_export.html (no restrictions)
17. IERS data policy: re3data.org/repository/r3d100010312 (Open access)
18. ERFA (BSD-3-Clause): github.com/liberfa/erfa
