# Krakty MacroQuest Fork — Project Workflow

The methodology codification for this fork. Every rule here exists because we
hit the corresponding problem at least once.

## Host roles

Three hosts. Each has exactly one job.

- **BEAST** (Linux): development host. ALL source and tooling edits originate
  here. Forks live at `/mnt/DEV/reverse-engineering/mq2krakty-staging/`.
- **wintest** (Windows): build server, **pull-only**. Never edits, never
  pushes. Source tree at `C:\macroquest\`. Runs MSBuild against
  `MacroQuest.sln`. SSH'd from BEAST.
- **laptop** (10.7.30.37): deploy target. Runs EQ under Wine in named prefixes
  (`prefix-sinbash`, `prefix-testbench`, `prefix-bolrik`, `prefix-live`, etc.).
  Receives DLLs via scp from BEAST.

**Flow is one-directional**: BEAST → GitHub → wintest (pull) and BEAST → laptop
(scp). Wintest never pushes. Laptop never edits. Violations are recoverable
but cost time (see *Recovery procedure* below).

## Repo layout

Two code repos + one knowledge base:

- `Krakty/MQ2-Krakty` — macroquest fork (parent code). This repo.
- `Krakty/eqlib` — eqlib fork (submodule of this repo at `src/eqlib`).
- `Krakty/MQ-RE` — RE knowledge base: forensics docs, runtime captures,
  methodology notes. Separate repo, lives next to this one on BEAST.

Plus standalone plugin repos (added as submodules):
- `Krakty/MQMCPServer` — at `src/plugins/MQMCPServer` (WIP plugin)
- `Krakty/MQ2EQBC` — at `src/plugins/MQ2EQBC`

## Branch convention

Branches named with `release/` and `patchday/` prefixes for tree-display in
GitHub UI. EQ Live and EQ Test are independent game environments — NOT a
promotion path. Each is tracked independently.

| Branch | Purpose | Lifecycle |
|---|---|---|
| `master` | Mirrors `macroquest/macroquest:master` (upstream) | Permanent |
| `release/live` | Current Live-server canonical state | Permanent, FF forward as patchdays land |
| `release/test` | Current Test-server canonical state | Permanent, FF forward as patchdays land |
| `patchday/live/YYYY-MM-DD` | Per-patchday Live work | Permanent (historical record) |
| `patchday/test/YYYY-MM-DD` | Per-patchday Test work | Permanent |

Tags at deploy time: `live/YYYY-MM-DD-deployed` and `test/YYYY-MM-DD-deployed`
mark the exact commit that shipped to prod.

## Per-patchday workflow

When EQ patches (Live or Test):

1. **TLO inventory survey first.** Before any class RE, enumerate the TLOs in
   the MacroQuest source tree and identify which eqlib classes back them.
   Produce a gap list of which backing classes need patchday-verified offsets.
   This catches the missing-data-struct problem early. (See
   `feedback_tlo_inventory_first.md` in MQ-RE knowledge base.)

2. **Branch from current `release/live` (or `release/test`):**
   ```
   git checkout release/live
   git pull --ff-only
   git checkout -b patchday/live/YYYY-MM-DD
   ```

3. **RE the gap list.** Class-by-class binary forensics + runtime capture
   validation. Findings go to `Krakty/MQ-RE:eq-builds/<live-or-test>/YYYY-MM-DD/forensics/`.

4. **Update headers.** Mostly `src/eqlib/include/eqlib/game/*.h`. Drive
   builds via `mqbuild build` on wintest until clean.

5. **Deploy + smoke test.** `mqbuild deploy --prefix <prefix>` to a test
   prefix on the laptop. Validate basic TLOs (`${Me.Name}`, `${Spell[]}`,
   `${Zone}`, etc.) before declaring stable.

6. **Promote.** `mqbuild promote patchday/live/YYYY-MM-DD release/live`
   (fast-forward only). Tag deploy: `git tag live/YYYY-MM-DD-deployed`.

## Build/deploy rules

These are not negotiable. Violating them costs hours of debugging.

- **MSBuild only. NEVER cmake.** cmake has broken crashpad integration that
  cost a full day to diagnose. Use the MQBuild PowerShell module or invoke
  MSBuild directly against `MacroQuest.sln`.
- **Build the full solution.** No `/t:ProjectName`. Stale cached objects from
  partial builds cause confusing link errors. Use `--clean` after a substantial
  branch switch.
- **scp to 10.7.30.37, NOT local cp.** The path `/home/tlindell/EQ-MASTER/`
  exists on both BEAST and the laptop. cp on BEAST writes to BEAST and the
  game on the laptop never sees it.
- **Deploy ALL DLLs every time.** Not just eqlib + MQ2Main. Every plugin
  every deploy.
- **Deploy to `MacroQuest/` root, NOT `Release/` subdirectory.** MQ loads
  from the root.
- **`MacroQuest.exe` also gets deployed as `mq-bot.exe`** in the same
  directory (the launcher uses the renamed binary).
- **Verify timestamps after deploy.** `ls -la` on the laptop confirms the
  scp landed.

## Promote-to-canonical procedure

To promote a patchday branch to `release/live` or `release/test`:

```
mqbuild promote patchday/live/2026-04-15 release/live
```

The Promote-MQBranch function:
- Verifies the patchday branch is a strict descendant of the target (FF-only)
- Refuses to force-push (no exceptions, no `-Force` override on the FF check)
- Prints the commits being promoted, asks for confirmation
- Pushes via `git push --ff-only`

GitHub branch protection rules also enforce no-force-push on `release/*`
branches as a backstop.

## Upstream sync workflow

Periodically (or when something useful surfaces upstream):

```
cd /mnt/DEV/reverse-engineering/mq2krakty-staging
git fetch upstream
git log --oneline upstream/master..master  # see what we're missing
```

To pull a specific commit:
```
git checkout patchday/<live-or-test>/<active-branch>
git cherry-pick <upstream-commit-sha>
```

Don't `git merge upstream/master` blindly. Upstream is emu-server-focused;
not all their changes are relevant for our Live/Test work. Cherry-pick what
matters.

Wintest does NOT have an `upstream` remote. Wintest only ever pulls from
our fork.

## Plugin development methodology

Per-plugin repos. Each plugin is its own `Krakty/<PluginName>` repo, included
in this fork as a `git submodule` under `src/plugins/<PluginName>`.

To add a new plugin:

1. Create new GitHub repo: `gh repo create Krakty/<PluginName> --public --description "..."`
2. Initial commit on `release/live` branch (set as default).
3. In MQ2-Krakty: `git submodule add git@github.com:Krakty/<PluginName>.git src/plugins/<PluginName>`
4. Add Project entry to `src/MacroQuest.sln` (3 places: Project, GlobalSection
   ProjectConfigurationPlatforms, NestedProjects). See
   `feedback_new_plugin_methodology.md` for the boilerplate.

Plugin's `release/live` branch tracks its own release history independent
of the fork's patchday cycle. Plugin bug fixes don't require a patchday
context.

Per-plugin `vcpkg.json` declares external deps. **No vendoring.** httplib,
nlohmann/json, etc. come from vcpkg, not committed copies.

## Recovery procedure (if pull-only rule violated)

If wintest somehow ends up with uncommitted edits or local-only branches
(usually because someone edited there directly, or a pull conflict was
resolved on wintest):

1. **Audit first.** Don't blindly recover. For each commit/file on the
   stranded branch, determine: PRESENT-ON-TARGET (already there),
   MISSING-FROM-TARGET (needs recovery), CONFLICTS-WITH-TARGET (needs decision).
   Write the audit doc to MQ-RE.
2. `git archive` the MISSING items into a tarball on wintest.
3. `scp` to BEAST.
4. Apply on BEAST clone, on the appropriate patchday branch.
5. Commit + push from BEAST (commit message references the audit doc).
6. Wintest pulls. Verify recovered items present.
7. Delete the recovery branch on wintest.

This procedure handled the apr15-2026 + apr07-2026 stranded recovery
branches. See `Krakty/MQ-RE:eq-builds/live/2026-04-15/iteration-logs/recovery_audit_2026-04-30.md`
for the worked example.

Going forward: don't develop on wintest. Edit on BEAST, commit, push, wintest
pulls. No exceptions, even for "minor" tooling edits.

## Field-rename methodology (avoid breaking scripts)

When RE work renames a field that has a real (non-placeholder) name:

```cpp
/*0x03cc*/ unsigned int LastRangedUsedTime;       // canonical name

// At end of class data, in the alias block:
ALT_MEMBER_ALIAS_DEPRECATED(unsigned int, LastRangedUsedTime, LastSecondaryUseTime,
    "LastSecondaryUseTime has been renamed to LastRangedUsedTime")
```

The macro creates a `__declspec(property)` with `[[deprecated]]` so plugin
code referencing the old name compiles with a warning. Storage is shared.

For placeholder-name renames (`Unknown0x540` → `RealName`), no alias needed
— no script could reference `Unknown0x540`.

Pattern source: `eqlib/Common.h:47` `ALT_MEMBER_ALIAS_DEPRECATED`. Examples:
`Spells.h:1125-1128`, `Items.h:1016`, `EverQuest.h:172`.

## Forensics evidence > eqlibdev offsets

When forensics evidence (binary disassembly + ctor/dtor + accessor analysis)
disagrees with RedGuides/eqlibdev's offset claims, **forensics wins**.
Eqlibdev's field NAMES are canonical; their OFFSETS may not match our binary.

Worked example: apr15 `CSidlScreenWnd::SidlText` was placed at +0x258 per
eqlibdev, broke server-select crash. Forensics-confirmed correct offset is
+0x260 (CSidlScreenWnd dtor `LEA RCX,[RCX+0x260]`). Targeted fix: 8-byte
lead-in pad in CSidlScreenWnd, kept eqlibdev's CXWnd_size = 0x258.

See `feedback_eqlibdev_offsets_not_canonical.md`.

## UNFOUND fields → thunk to binary, don't stub-and-pray

When a field is UNFOUND_AFTER_EXHAUSTIVE_SEARCH and you stub the accessor:
**also check if any non-trivial function in eqlib reads that field**. If
yes, the function will silently produce wrong results.

The fix: thunk to the binary's native function via `FUNCTION_AT_ADDRESS`
using the appropriate `XXX_x` offset constant from `eqlib/offsets/eqgame.h`.
Examples in `src/eqlib/src/game/FunctionDefs.cpp`.

See `feedback_unfound_field_function_thunk.md`.

## Where to learn more

- **`docs/SETUP.md`** — onboarding for a fresh dev environment
- **`docs/SECRETS.md`** — credential mapping (where SSH keys, PATs go)
- **`docs/FUTURE-CI.md`** — v2 architecture goal (GitHub Actions)
- **`Krakty/MQ-RE`** — RE knowledge base + forensics + runtime captures
- **`tools/build-tools/README.md`** — MQBuild PowerShell module reference
