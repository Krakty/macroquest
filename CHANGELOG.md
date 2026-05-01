# Changelog

Notable changes to the Krakty MacroQuest fork. Entries added at each
promotion to `release/live` or `release/test`.

Format loosely follows [Keep a Changelog](https://keepachangelog.com/).
Dates are EQ patchday dates, not commit dates.

## Unreleased

Pending the next patchday or release/* promotion. Things that have already
landed on `patchday/live/2026-04-15` since the last `release/live` promotion:

### Known build issues (deferred to post-refactor cleanup)

The Phase 7 verification build (2026-04-30) failed with two distinct error
classes. Refactor proceeded; these will be fixed in a separate pass:

1. **C2086 redefinition** in `eqlib/include/eqlib/game/PlayerClient.h`:
   - `LastRangedUsedTime` declared at +0x3cc (MY incorrect forward-port from apr7)
     AND +0x3f0 (pre-existing correct apr15 RE)
   - `BearingToTarget` declared at +0x458 (MY incorrect forward-port)
     AND +0x3fc (pre-existing correct apr15 RE)
   - Fix: revert commits b444fc2 (forward-port) and 6249f56 (deprecated aliases).
     Restore `LastSecondaryUseTime` at +0x3cc and `CameraOffset` at +0x458 as
     they are SEPARATE fields from the apr15-RE-correct ones at higher offsets.
     Aliases not needed since the rename was wrong.

2. **C2338 static_assert offset mismatches** for several PlayerClient fields
   (DraggingPlayer @ +0x4b6, SecondaryTintIndex @ +0x4f8, CastingData @ +0x4fc,
   etc.). Pre-existing offset drift — RegistryAsserts.h has been updated with
   apr15 offsets but the field padding in PlayerClient.h hasn't been regenerated.
   Fix: run `tools/playerclient-pad-generator/pad_generator.py` to insert
   the missing padding bytes so each field lands at its commented offset.

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
