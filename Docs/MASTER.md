# Documentation Index

This file describes all documentation in the project.

## Docs/

| File | Purpose |
|------|---------|
| [MASTER.md](MASTER.md) | This file. Index of all documentation. |
| [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) | Original implementation plan. Defines the 5-phase approach, project structure, data structures, algorithms, and key technical decisions. |
| [PROJECT-STATUS.md](PROJECT-STATUS.md) | Current state of the project. Phase completion status, test results, validation coverage, and known limitations. |
| [NEXT-STEPS.md](NEXT-STEPS.md) | Prioritized roadmap for future work. Expanded validation, additional panchang elements (nakshatra, yoga, karana), Purnimanta scheme, and usability improvements. |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Technical deep-dive. Tech stack, module dependency graph, all core algorithms explained (tithi, new moon, masa, sunrise), Swiss Ephemeris usage, data structures, and reference implementations. |
| calendrical-calculations.pdf | Reference book by Reingold & Dershowitz. Chapter 20 covers Hindu calendar mathematics. |

## Root Directory

| File | Purpose |
|------|---------|
| [CHANGELOG.md](../CHANGELOG.md) | Version history. Tracks what changed in each release. |
| [Makefile](../Makefile) | Build system. Targets: `make` (build), `make test` (run tests), `make clean` (remove artifacts). |

## Reading Order

1. **IMPLEMENTATION_PLAN.md** — understand the design and goals
2. **ARCHITECTURE.md** — understand how it works technically
3. **PROJECT-STATUS.md** — see what's done and what's not
4. **NEXT-STEPS.md** — see where the project is headed
5. **CHANGELOG.md** — track version-by-version changes
