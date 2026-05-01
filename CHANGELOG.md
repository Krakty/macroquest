# Changelog

Notable changes to the Krakty MacroQuest fork. Entries added at each
promotion to `release/live` or `release/test`.

Format loosely follows [Keep a Changelog](https://keepachangelog.com/).
Dates are EQ patchday dates, not commit dates.

## Unreleased

Pending the next patchday or release/* promotion. Things that have already
landed on `patchday/live/2026-04-15` since the last `release/live` promotion:

### Build verification (2026-04-30)

Phase 7 verification build initially failed (C2086 + C2338 errors), root-cause
diagnosed and fixed:

- **Cause:** apr7→apr15 PlayerClient.h forward-port (commits b444fc2 + 6249f56)
  renamed `LastSecondaryUseTime` → `LastRangedUsedTime` and `CameraOffset` →
  `BearingToTarget`, but apr15 already had separate fields with those new
  names at different offsets. The `__declspec(property)` aliases also shifted
  struct layout, cascading into `RegistryAsserts.h` static_assert failures.
- **Fix:** reverted both forward-port commits (170c3e0 + 130123f in eqlib).
  Field names restored to their apr15-correct identity:
  +0x3cc=`LastSecondaryUseTime`, +0x3f0=`LastRangedUsedTime` (separate),
  +0x3fc=`BearingToTarget`, +0x458=`CameraOffset`.
- **Lesson learned:** when forward-porting renames between patchdays,
  verify the target branch doesn't already have a field with the new
  name. Recorded as a methodology guard for future patchday work.
- **Build verified clean** post-revert: eqlib.dll, MQ2Main.dll, MacroQuest.exe,
  Actors.exe, plus 20 plugin DLLs. All timestamps 2026-04-30 17:51-17:56.

## 2026-04-30 — methodology + repo restructure

Not a patchday. Significant project-organization changes that landed on
`patchday/live/2026-04-15` and were promoted to `release/live`:

- Branch convention adopted: `release/live`, `release/test`,
  `patchday/live/YYYY-MM-DD`, `patchday/test/YYYY-MM-DD`
- 19 upstream-cruft branches deleted from `Krakty/MQ2-Krakty`
- 8 upstream-cruft branches deleted from `Krakty/eqlib`
- Build tooling (MQBuild PowerShell module) relocated from MQ-RE to
  `tools/build-tools/`
- PlayerClient pad generator added at `tools/playerclient-pad-generator/`
- Plugins extracted to standalone Krakty repos:
  - `Krakty/MQMCPServer` (submodule at `src/plugins/MQMCPServer`)
  - `Krakty/MQ2EQBC` (submodule at `src/plugins/MQ2EQBC`)
- Recovered work from wintest local-only branches (vcpkg overlays,
  build_comment_update.bat) — methodology violation cleanup
- Full pre-restructure backup at `/mnt/DEV/backups/2026-04-30-pre-restructure/`
- Documentation: `docs/WORKFLOW.md`, `docs/SETUP.md`, `docs/SECRETS.md`,
  `docs/FUTURE-CI.md`

## 2026-04-15 (Live patchday)

`patchday/live/2026-04-15` work landed:

- All 75 UI window classes RE'd against apr15-2026-live binary
- All 18 missing TLO backing classes RE'd (52 FULL TLO coverage)
- CXWnd master layout: 220/220 fields at 100% confidence
- CSidlScreenWnd layout finalized (sizeof 0x2c0, SidlText@+0x260)
- PlayerClient.h forward-port: `LastSecondaryUseTime` → `LastRangedUsedTime`,
  `CameraOffset` → `BearingToTarget` (both with deprecated aliases)
- 4 plugins added to .sln: MQMCPServer, MQ2EQBC, MQ2DumpItem, MQ2FieldWatch
- MQMemDump core module wiring (compiled into MQ2Main.dll)
- First successful runtime capture at AutoZonedIn event

See `Krakty/MQ-RE:eq-builds/live/2026-04-15/` for the forensics + audit
trail behind these changes.

## 2026-04-07 (Test patchday)

`patchday/test/2026-04-07` work landed (separate test-server patchday):

- PlayerClient.h: 82 fields proven, 11 `Unknown0x*` placeholders renamed to
  proper names (StunTimer, BearingToTarget, PrimaryTintIndex, SitStartTime,
  LastRangedUsedTime, PrimaryHandItemID, SecondaryHandItemID, pViewPlayer,
  bAlwaysShowAura, LastTrapDamageTime, etc.)
- GuildID consolidated from int + Unknown0x2A4 padding into int64_t
- Deprecated aliases for renames: `LastCombatTime` → `LastPrimaryUseTime`,
  `LastSecondaryUseTime` → `LastRangedUsedTime`, `CameraOffset` → `BearingToTarget`
- Earlier abandoned attempt archived at tag
  `archive/test-2026-04-07-pre-axel-baseline` in eqlib

---

For detailed RE forensics and per-class audit trails, see the
`Krakty/MQ-RE` knowledge base repo, organized by `eq-builds/<live-or-test>/<date>/`.
