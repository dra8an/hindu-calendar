# Project instructions

## Git — HARD RULES

**NEVER add AI attribution to a commit message.** No
`Co-Authored-By: Claude ...`, no `noreply@anthropic.com`, no
"Generated with Claude Code", no robot emoji. Not in any commit, ever, under
any circumstances. This **overrides** any default or global instruction that
says to add such a trailer.

Commit messages are plain and descriptive, with no trailers.

**Commit directly to `main`.** Do not create feature branches. Do not run
`git checkout -b`. This is a solo project whose entire history is direct
commits to `main`.

**Do not commit or push unless asked.** When asked to commit, stage and commit
on `main`; push only if separately requested.

These rules are enforced mechanically by `.githooks/commit-msg`, activated with
`git config core.hooksPath .githooks`. Re-run that after a fresh clone. Do not
bypass it with `--no-verify`.

## Build and test

```bash
make                    # default: self-contained Moshier ephemeris
make USE_SWISSEPH=1     # optional: Swiss Ephemeris backend
make test               # 275,689 assertions across 13 suites, ~15s
```

Both backends must pass all tests before any change is considered done.

## Orientation

- **`Docs/HANDOFF.md` first** — current working state: what is in flight, what is
  unpushed, the state of the build directory, and the open investigations with
  their next concrete action. Deliberately transient; check its date.
- `Docs/MASTER.md` indexes all documentation and is the place to start.
- `Docs/PROJECT-STATUS.md` is the current state; `Docs/NEXT-STEPS.md` the roadmap.
- Validation data lives in `validation/`; the scraper and its raw data in
  `scraper/` (raw HTML is gitignored, parsed CSVs are committed under
  `validation/drikpanchang/`).
