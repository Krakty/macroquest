# Future-State CI/CD Goal (v2)

NOT implemented yet. This is a graft-on goal for later.

## Why

Currently builds happen on wintest (Windows VM with VS2022 + vcpkg) driven
manually via SSH from BEAST. Drawbacks:

- Wintest is a snowflake — its environment is reproducible only via manual setup.
- No automatic verification when commits land on `release/live` or
  `release/test` — someone has to remember to drive a build.
- No history of build artifacts. Once a build is overwritten, that exact
  set of DLLs is gone unless manually archived.
- Single-host bottleneck. If wintest is down, no builds happen.

## Target architecture

GitHub Actions Windows runners with the build environment provisioned in CI:

1. **Trigger**: push to `release/live`, `release/test`, `patchday/live/*`,
   `patchday/test/*`. Plus PR builds for any PR targeting these branches.
2. **Runner setup**: Windows runner with VS2022 + vcpkg (cache vcpkg
   installation between runs to keep it fast).
3. **Build steps**:
   - Checkout with `--recurse-submodules` (pulls eqlib + plugin repos)
   - Restore vcpkg packages
   - MSBuild `MacroQuest.sln` /p:Configuration=Release /p:Platform=x64 /t:Clean,Build
   - Run any in-tree tests
4. **Artifacts**: upload `build/bin/release/*.dll` + `*.exe` as a release
   artifact. Tag-triggered builds also create a GitHub Release.
5. **Status badge**: README shows current build status of `release/live` /
   `release/test`.
6. **Deploy**: optional — manual `mqbuild deploy` triggered by tagging
   `live/YYYY-MM-DD-deployed` could pull artifacts from CI rather than
   building locally.

## Migration path when ready

1. **Audit reproducibility.** Get wintest's build to succeed in a fresh VS2022
   container/VM with our tools/build-tools/MQBuild module. If it builds, CI
   is feasible.
2. **Add `.github/workflows/build.yml`** with the steps above. Start with
   build-only, no deploy.
3. **Run side-by-side** with manual wintest builds for a patchday or two.
   Compare output DLL sizes, timestamps, smoke-test results.
4. **Add release-artifact upload** once builds are stable.
5. **Reduce wintest's role** to deploy-only (or eliminate; `mqbuild deploy`
   can pull artifacts from GitHub Releases instead of wintest's build dir).

## Open questions

- Windows GitHub Actions runners cost more compute minutes than Linux. Is
  the build budget sustainable on personal-tier billing?
- vcpkg cache size — does it fit in GitHub Actions cache limits per repo?
- The custom vcpkg overlays at `contrib/vcpkg-overlays/` — do they "just work"
  in CI, or do they need adjustment?
- Crashpad portfile — known to be fragile (per `feedback_crashpad_crt.md`).
  Does it work in CI environment?

## When NOT to do this

If patchday cadence is rare (months apart) and the manual wintest workflow
is reliable, the CI investment may not pay off. The trigger to actually
implement this:

- Multiple devs need to verify builds independently
- Build verification is needed before Promote-MQBranch (i.e., gate `release/*`
  on green CI)
- Wintest disk fills up too often or hardware becomes unavailable

Until then: this doc records the goal so when the trigger fires, we know
what we're building.
