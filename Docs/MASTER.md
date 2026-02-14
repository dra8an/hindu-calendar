# Documentation Index

This file describes all documentation in the project.

## Claude Code Memory

Claude Code maintains a persistent memory file for this project at:

```
~/.claude/projects/-Users-draganbesevic-Projects-claude-hindu-calendar/memory/MEMORY.md
```

This file is automatically loaded into Claude's system prompt at the start of every conversation. It contains key project facts (overview, technical decisions, phase status, implementation gotchas) so context is preserved across sessions. You can read or edit it directly.

## Docs/

| File | Purpose |
|------|---------|
| [MASTER.md](MASTER.md) | This file. Index of all documentation + quick reference. |
| [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) | Original implementation plan. Defines the 5-phase approach, project structure, data structures, algorithms, and key technical decisions. |
| [PROJECT-STATUS.md](PROJECT-STATUS.md) | Current state of the project. Phase completion status, test results, validation coverage, and known limitations. |
| [NEXT-STEPS.md](NEXT-STEPS.md) | Prioritized roadmap for future work. Expanded validation, additional panchang elements (nakshatra, yoga, karana), Purnimanta scheme, and usability improvements. |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Technical deep-dive. Tech stack, module dependency graph, all core algorithms explained (tithi, new moon, masa, sunrise, solar calendar), Swiss Ephemeris usage, data structures, and reference implementations. |
| [SOLAR_PLAN.md](SOLAR_PLAN.md) | Design document for the Hindu solar calendar module (Tamil, Bengali, Odia, Malayalam). Includes regional differences, critical time rules, era calculations, and implementation findings. |
| [TOOLS.md](TOOLS.md) | Reference for all utility programs in `tools/` — data generators, edge case scanner, and diagnostic/investigation tools. |
| [SOLAR_TESTING_PLAN.md](SOLAR_TESTING_PLAN.md) | Plan for solar calendar reference data generation (1900-2050 CSVs) and regression/validation test suites. |
| [MALAYALAM_NOON_FIX.md](MALAYALAM_NOON_FIX.md) | Investigation into Malayalam critical time rule. Confirmed end-of-madhyahna (3/5 of daytime) is correct; the initial "~46% differ" hypothesis was based on misreading drikpanchang's festivals page. |
| [MALAYALAM_ADJUSTMENTS.md](MALAYALAM_ADJUSTMENTS.md) | Malayalam critical time boundary investigation. Documents the 3/5 daytime discovery and the v0.3.2 ayanamsa buffer (−9.5 min). |
| [ODIA_ADJUSTMENTS.md](ODIA_ADJUSTMENTS.md) | Odia critical time investigation. Documents discovery of fixed 22:12 IST cutoff from 35 boundary cases. |
| [SOLAR_VERIFICATION_WEB.md](SOLAR_VERIFICATION_WEB.md) | Plan for adding solar calendar support to the validation web page. |
| [REINGOLD_VALIDATION.md](REINGOLD_VALIDATION.md) | Approach for comparing our Drik Siddhanta output against Reingold/Dershowitz Surya Siddhanta (HL function via SBCL). |
| [REINGOLD_VALIDATION_WEB_PAGE.md](REINGOLD_VALIDATION_WEB_PAGE.md) | Design for embedding Reingold diff overlay into the validation web page. |
| calendrical-calculations.pdf | Reference book by Reingold & Dershowitz. Chapter 20 covers Hindu calendar mathematics. |

## Root Directory

| File | Purpose |
|------|---------|
| [CHANGELOG.md](../CHANGELOG.md) | Version history. Tracks what changed in each release. |
| [Makefile](../Makefile) | Build system. Targets: `make` (build), `make test` (run tests), `make clean` (remove artifacts). |

## Reading Order

1. **MASTER.md** (this file) — quick reference for key facts
2. **IMPLEMENTATION_PLAN.md** — understand the design and goals
3. **ARCHITECTURE.md** — understand how it works technically
4. **PROJECT-STATUS.md** — see what's done and what's not
5. **NEXT-STEPS.md** — see where the project is headed
6. **CHANGELOG.md** — track version-by-version changes
