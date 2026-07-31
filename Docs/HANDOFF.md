# Handoff — current working state

**Last updated: 2026-07-31.** Read this first when resuming after a break or a
cleared context. It captures *transient* state that the permanent docs
deliberately do not.

> **This file goes stale.** The permanent documentation (`MASTER.md`,
> `PROJECT-STATUS.md`, `NEXT-STEPS.md`, `CHANGELOG.md`) is the source of truth
> for anything settled. Everything below is "where we are right now", and
> should be updated or trimmed as items are resolved. If the date above is old,
> verify before trusting it.

---

## Read in this order

1. **This file** — what is in flight
2. `Docs/MASTER.md` — index of all documentation
3. `Docs/PROJECT-STATUS.md` — what is done, test counts, known limitations
4. `Docs/NEXT-STEPS.md` — the roadmap and open items
5. `CHANGELOG.md` — most recent release (0.13.0)

Claude Code additionally auto-loads
`~/.claude/projects/-Users-draganbesevic-Projects-claude-hindu-calendar/memory/MEMORY.md`
plus its topic files, which carry the durable project knowledge and the
non-obvious traps.

---

## Immediate state

| | |
|---|---|
| Branch | `main` — commit directly, no feature branches |
| Version | 0.13.0 |
| Tests | 279,313 assertions / 14 suites / 0 failures (Moshier, the default) |
| Working tree | clean as of last commit |

**Two things a fresh session will not otherwise know:**

- **`build/` is in Swiss Ephemeris state.** An interrupted `make gen-ref
  USE_SWISSEPH=1` left `build/swe/` present and `build/moshier/` absent, with
  objects compiled `-DUSE_SWISSEPH`. A plain `make` may reuse them on timestamp
  and produce a mismatched binary. **Run `make clean` before the next build.**
- **Check `git log origin/main..HEAD`** before assuming everything is pushed;
  several documentation commits have been landing in batches.

---

## Open threads

### 1. Two disputed Bengali Suryasiddhanta month boundaries — ACTIVE

An external source covering 2022–2029 agrees with us on 94 of 96 month
boundaries but claims 32-day months where we and drikpanchang give 31:

| Month | Ours (31 days) | External claim (32 days) |
|-------|----------------|--------------------------|
| Srabon 1433 | 2026-07-18 → 2026-08-17 | → 2026-08-18 |
| Asharh 1435 | 2028-06-16 → 2028-07-16 | → 2028-07-17 |

**State of the investigation.** Both disputed sankrantis are near midnight
(−54.0 and +7.4 minutes). A fixed astronomical offset is ruled out, because
five *other* near-midnight boundaries in the same window are agreed, some
closer to midnight. The plain Sewell & Dikshit rule is ruled out too — it gives
the external answer for one case and ours for the other.

**Leading hypothesis: a day-assignment rule difference, not astronomy.** The
1955 Calendar Reform Committee report records Gupta Press's own declared
solar year as 365.258756481 days — identical to ours, no bija correction. The
Odrik-school panjikas pair that classical astronomy with the rules of
Raghunandana's *Ashtavimshati-tattva*, a dharmashastra digest.

**Next concrete action:** ask the external source for (a) which panjika and
edition, and (b) the sankranti *times* it prints. Matching times with differing
dates confirms a rule difference and makes the rule derivable from a handful of
boundary cases. Differing times point at the constants instead.

Full analysis: `Docs/SURYASIDDHANTA_PANJIKA.md` §6, which includes a
step-by-step chronology of the investigation under "How this unfolded" —
including the two hypotheses that were ruled out and one wrong turn, so none
of it gets retried from scratch.

### 2. Stale Swiss Ephemeris reference data — KNOWN, DEFERRED

`validation/se/*.csv` were last regenerated 2026-02-23, before the upper-limb
sunrise change (03-13) and the elevation fix (04-06). This causes **23
`test_adhika_kshaya` failures under `USE_SWISSEPH=1`**. Pre-existing, verified
by reproducing at HEAD in a clean clone. The Moshier default backend is
unaffected.

Fix is `make gen-ref USE_SWISSEPH=1`, but it rewrites six committed CSVs, so
review the diff. **Note:** `gen-ref` writes in place with no temp-and-rename, so
an interrupted run leaves a truncated CSV that still parses as valid. This has
already happened once. Verify line counts afterwards.

### 3. Tamil Suryasiddhanta variant — NOT STARTED

Tamil exposes the same `drik-arithmetic` toggle (its default is Thirukanitha).
Scraping it would be the one genuine out-of-sample test of whether the Bengali
rule constants encode a real convention or are fitted to Bengali data. The
current honest generalisation figure is 99.667%. Cost: ~1,812 pages, roughly 9
VPN cycles.

---

## Small known-wrong things, not yet fixed

- **README example sunrise times predate the upper-limb change.** The
  single-day example was `05:53:08`; the correct value is `05:51:57`, a
  71-second difference that matches the documented upper-limb offset exactly.
  That one is corrected. The four times in the month-panchang example
  (`07:15:05`, `07:15:19`, `07:11:52`, `07:11:22`) are almost certainly stale
  by the same ~71 seconds but were **not** verified, because doing so needs a
  build and `build/` is in the wrong state. Regenerate them with
  `make clean && make && ./hindu-calendar -m 1 -y 2025` before trusting them.

- **Counting test assertions: sum only the `^===` summary lines.** Some suites
  print per-section sub-totals in the same `N/M passed` format, so a naive
  grep over the whole output double-counts. This produced 285,059 instead of
  the correct 279,313 once. Correct incantation:

  ```bash
  make test > /tmp/t.out 2>&1
  grep -E "^=== .*(passed|failed)" /tmp/t.out | grep -oE "[0-9]+/[0-9]+ passed" \
    | cut -d/ -f1 | paste -sd+ - | bc
  ```

- **`git` hits a transient `.git/index.lock` on this machine** every so often,
  with no git process actually running. It clears by itself — just retry the
  command. Do not delete the lock reflexively without first checking
  `pgrep -fl git`.

## Useful recipe: sankranti times with no build

The Python engine in `validation/suryasiddhanta/` needs no compilation and is
numerically identical to the C. This is how the disputed boundaries were
investigated, and it works even when `build/` is in a bad state:

```python
# from validation/suryasiddhanta/
import datetime
from surya_siddhanta import all_sankrantis, solar_longitude, sankranti_at_or_after

LON, ZONE = 77.2090, 5.5                     # New Delhi
to_ist  = lambda m: m - LON/360.0 + ZONE/24.0   # engine moment -> IST
rd_date = lambda rd: datetime.date(1,1,1) + datetime.timedelta(days=int(rd)-1)
rd_of   = lambda y,m,d: (datetime.date(y,m,d) - datetime.date(1,1,1)).days + 1

# every sankranti in a range, as (rashi, moment)
for rashi, m in all_sankrantis(rd_of(2026,1,1), rd_of(2026,12,31)):
    ist = to_ist(m)
    secs = round((ist % 1) * 86400)
    print(rashi, rd_date(ist), f"{secs//3600:02d}:{secs%3600//60:02d}")
```

Bengali month number equals the rashi entered, so rashi 4 (Karka) starts
Srabon. Month starts are one day after the sankranti except in the
midnight-zone cases the rule handles.

## Things that will bite you

- **Drikpanchang rate-limits per IP at exactly 200 requests.** Session rotation
  does not clear it; only a VPN switch does. The scraper stops cleanly at 199
  (`MAX_REQUESTS_PER_RUN`).
- **Never add AI attribution to commit messages.** Enforced by
  `.githooks/commit-msg`; see `CLAUDE.md`. Activate after a fresh clone with
  `git config core.hooksPath .githooks`.
- **Do not port Reingold's `calendar.l` into `src/`.** It is Apache 2.0, and
  `src/`/`lib/` are deliberately free of licensed third-party code. See
  `Docs/LICENSING.md` §8.
- **The Surya Siddhanta longitudes are already sidereal.** Subtracting an
  ayanamsa gives ~24° of error.
- **`surya_sankranti()` brackets around its estimate**, deliberately, rather
  than searching forward. Forward-only jumps a full year when the estimate
  sits just past the sankranti. Do not "fix" it back.

---

## Handing off to the other C/C++ project

`Docs/SURYASIDDHANTA_PORTING_SPEC.md` is the document for that. It lists the
files to copy, the reference CSV to verify against, test vectors, a checklist,
and a failure-mode table. That work is already in progress on the other side.
