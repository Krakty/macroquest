# MQ2-Krakty Dev Environment Setup

Onboarding doc for setting up the MQ2-Krakty fork dev environment from scratch. Read this top-to-bottom; every step matters. If a step is unclear, prefer the linked memory file over guessing.

## 1. Hosts overview

Three hosts cooperate. **BEAST** (this Linux workstation) is the only place edits ever happen: source code, PowerShell tooling, configs, docs. **wintest** is a Windows VM that holds the MSBuild toolchain and acts strictly as a build server -- it pulls from GitHub, compiles, and exposes its build output over SSH; it never originates a commit and has no push credentials. The **laptop at 10.7.30.37** (wks-lt7760) runs EQ under Wine; built DLLs are scp'd directly into its Wine prefixes. The flow is one-direction: BEAST -> GitHub -> wintest -> (scp) -> 10.7.30.37.

## 2. Prerequisites

Tools required on BEAST:

- `git` (with submodule support)
- `ssh` and `scp` (OpenSSH)
- `pwsh` (PowerShell 7+) -- `pacman -S powershell-bin` on Arch, or via Microsoft repo
- `gh` (GitHub CLI) -- for fork-related PRs and remote ops

Credentials required:

- GitHub PAT for `gh` -- run `gh auth login` once
- SSH key authorized on wintest (see SSH config below)
- SSH key authorized on 10.7.30.37 as user `tlindell`

Real key paths, hostnames, and PAT location are listed in `docs/SECRETS.md` (sibling to this file). That file is gitignored. Do not put real credentials in this doc.

Tools required on wintest (already installed, listed for completeness):

- Visual Studio 2022 BuildTools -- MSBuild at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe`
- Git for Windows
- .NET 9.0 SDK (for the comment-update tool)
- OpenSSH server

## 3. Initial clone on BEAST

```bash
cd /mnt/DEV/reverse-engineering
git clone --recurse-submodules git@github.com:Krakty/MQ2-Krakty.git mq2krakty-staging
cd mq2krakty-staging
git remote add upstream https://github.com/macroquest/macroquest.git
git fetch upstream
```

The `mq2krakty-staging` directory name matches what the build tooling expects under `/mnt/DEV/reverse-engineering/`. Don't rename it without updating `Get-MQConfig` in the PowerShell module.

The clone has nested submodules (`eqlib`, `contrib/vcpkg`, plugin sub-submodules). `--recurse-submodules` handles them. If you forget, run:

```bash
git submodule update --init --recursive
```

CRITICAL: never edit anything inside `contrib/vcpkg/` -- it's a custom MQ submodule and edits get clobbered. See `feedback_vcpkg_submodule.md` in the MQ-RE memory.

## 4. SSH config

Create per-host config files in `~/.ssh/config.d/` so they include cleanly into your main `~/.ssh/config`. If you don't already have an `Include ~/.ssh/config.d/*` line in `~/.ssh/config`, add one.

`~/.ssh/config.d/wintest`:

```
Host wintest
    HostName <wintest-ip-or-hostname>
    User <wintest-user>
    IdentityFile ~/.ssh/<key-for-wintest>
    RequestTTY no
```

`~/.ssh/config.d/laptop`:

```
Host 10.7.30.37
    User tlindell
    IdentityFile ~/.ssh/<key-for-laptop>
```

Real values live in `docs/SECRETS.md`. Test both:

```bash
ssh -o RemoteCommand=none -o RequestTTY=no wintest "echo wintest ok"
ssh 10.7.30.37 "echo laptop ok"
```

Both must succeed without password prompts before continuing. The `-o RemoteCommand=none -o RequestTTY=no` flags are required for non-interactive command execution from the PowerShell module.

## 5. PowerShell module install

The build automation lives at `tools/build-tools/MQBuild/`. From BEAST, with `pwsh` in your shell:

```bash
pwsh
Import-Module /mnt/DEV/reverse-engineering/mq2krakty-staging/tools/build-tools/MQBuild/MQBuild.psd1
```

To make the `mqbuild` CLI wrapper invokable, either alias it or call directly:

```bash
alias mqbuild='pwsh /mnt/DEV/reverse-engineering/mq2krakty-staging/tools/build-tools/MQBuild/mqbuild.ps1'
```

Add the alias to your shell rc file (`~/.zshrc` / `~/.bashrc`) so it persists.

The module exports:

- `Invoke-MQBuild` -- run MSBuild on `MacroQuest.sln`
- `Invoke-MQDeploy` -- scp all DLLs/EXE to a Wine prefix on the laptop
- `Invoke-MQCommentUpdate` -- run the comment-update tool (reads Release PDB, writes offset comments to headers)
- `Get-MQBuildStatus` -- snapshot of branch + build state on wintest
- `Switch-MQBranch` -- checkout a branch on wintest + update submodules
- `Sync-MQWintest` -- fetch + ff-only pull + submodule update
- `Promote-MQBranch` -- fast-forward `live` or `test` to a patch branch (no force-push, ever)

The CLI surface is `mqbuild build|deploy|switch|promote|sync|status|comment-update|menu`. Run `mqbuild help` for full flag list.

PowerShell 7+ on Linux is the only supported runtime. PowerShell 5 (Windows-only) is not supported and the module does not target it.

## 6. Wintest first-time pull

If wintest already has `C:\macroquest\` populated, skip this section. To check:

```bash
ssh -o RemoteCommand=none -o RequestTTY=no wintest "if exist C:\macroquest\src\MacroQuest.sln (echo present) else (echo missing)"
```

If missing, set up wintest:

1. SSH to wintest and create `~/.ssh/config.d/` entries on **wintest** for both `origin` (Krakty fork) and any other relevant remotes. The wintest user needs an SSH key authorized on GitHub for read-only clone of the Krakty fork. (Use a deploy key, not a personal PAT.)

2. Initial clone on wintest:

   ```cmd
   cd C:\
   git clone --recurse-submodules git@github.com:Krakty/MQ2-Krakty.git macroquest
   cd macroquest
   git remote add krakty git@github.com:Krakty/MQ2-Krakty.git
   ```

   The `krakty` remote is referenced by `Sync-MQWintest` and `Switch-MQBranch` for the patch branches.

3. Verify it builds (do this from BEAST, not wintest):

   ```bash
   mqbuild status
   mqbuild build --clean
   ```

CRITICAL: wintest is pull-only. It must never have local-only branches, uncommitted edits, or anything that didn't originate on BEAST. This applies to source code AND build tooling AND helper scripts. If `mqbuild status` ever reports dirty files on wintest, you have a methodology violation -- audit and recover per `feedback_wintest_pull_only.md` before proceeding.

## 7. Verify build

Confirm wintest sees the right branch and the build tree is clean:

```bash
mqbuild status
```

Expected output: `Branch: <expected>`, `eqlib: <branch> @ <short-sha>`, no dirty files, build outputs present (or absent if never built).

Run a clean Release build:

```bash
mqbuild build --clean
```

This issues `MSBuild MacroQuest.sln /t:Clean,Build /p:Configuration=Release /p:Platform=x64` over SSH. Expect 5-15 minutes depending on the change set. Success looks like:

```
Build succeeded in <N>s. Warnings: <count>
eqlib.dll mtime: <today>
```

Failure prints the last 20 error lines and the path to a full log under `LocalStageDir` (typically `/tmp/mqbuild-logs/`).

If build fails on first run, common causes:

- vcpkg submodule out of date -- run `mqbuild sync` then retry
- comment-update.config missing a vcpkg include path -- see `Invoke-MQCommentUpdate.ps1` header comment
- Crashpad CRT mismatch -- you accidentally tried cmake; revert and use MSBuild

## 8. First deploy

Dry-run first to see exactly what would happen:

```bash
mqbuild deploy --prefix sinbash --dry-run
```

Expected output: a list of `[DRY] scp wintest:'C:/macroquest/build/bin/release/<file>' -> tlindell@10.7.30.37:/home/tlindell/EQ-MASTER/prefix-sinbash/drive_c/MacroQuest/<file>` lines for every core file (eqlib.dll, MQ2Main.dll, MacroQuest.exe, imgui*.dll) and every plugin DLL.

Real deploy:

```bash
mqbuild deploy --prefix sinbash
```

The module:

1. scp's each artifact from wintest -> BEAST `/tmp/` staging
2. scp's from staging -> `tlindell@10.7.30.37:/home/tlindell/EQ-MASTER/prefix-<name>/drive_c/MacroQuest/`
3. Renames `MacroQuest.exe` to `mq-bot.exe` on the laptop (the launcher injects via the renamed binary)
4. Verifies post-deploy timestamps are today's date

Available prefixes: `sinbash`, `testbench`, `bolrik`. Each is a separate Wine prefix on the laptop.

Build and deploy together:

```bash
mqbuild build --clean --deploy --prefix sinbash
```

## 9. Common gotchas

Short list with pointers to the deeper writeups in the MQ-RE memory directory.

- **cmake is NEVER used.** MSBuild only. cmake's crashpad integration injects Debug CRT into Release mode. Lost an entire day to this. See `feedback_crashpad_crt.md` and the comment block at the top of `Invoke-MQBuild.ps1`.
- **scp not cp for deploy.** The path `/home/tlindell/EQ-MASTER/prefix-sinbash/drive_c/MacroQuest/` exists on BOTH BEAST and the laptop as separate local filesystems. `cp` writes to BEAST's copy; the game never sees it. Always scp to `10.7.30.37`. Cost 4+ hours once. See `feedback_deploy_scp.md`.
- **Deploy to MacroQuest/ root, NOT Release/ subdirectory.** MQ loads from the root. The `Release/` subdir is leftover build artifacts; deploying there does nothing. See `feedback_deploy_path_root.md`.
- **Deploy ALL DLLs every time.** Plugins link against eqlib too. Cherry-picking eqlib + MQ2Main leaves plugins built against stale headers. The module deploys everything by default; don't disable that. See `feedback_deploy_all_plugins.md`.
- **Always `--clean` after a non-trivial branch switch.** MSBuild caches stale objects across branches. `Switch-MQBranch` prints a reminder; heed it. See `feedback_clean_first.md`.
- **Wintest is pull-only.** No edits originate on wintest -- not source, not `.ps1`, not `.bat`, not configs. ALL edits happen on BEAST, get pushed to GitHub, and wintest pulls them. See `feedback_wintest_pull_only.md` for the audit-first recovery procedure if this gets violated.
- **Full solution rebuilds only.** Never `/t:ProjectName`. Partial builds risk stale cached objects, mismatched DLLs, and PDB inconsistencies. See `feedback_clean_first.md`.
- **No force-push to live or test.** `Promote-MQBranch` enforces fast-forward only and refuses to bypass. See `feedback_brainiac_rules.md` rule 4.
- **Never edit `contrib/vcpkg/` portfiles.** It's a custom MQ submodule. To restore: `git submodule update --init --recursive --force`. See `feedback_vcpkg_submodule.md`.
- **Plugin source-tree presence requires sln registration.** If `src/plugins/<Name>/` exists but the plugin isn't in `MacroQuest.sln`, the build silently skips it. See `feedback_new_plugin_methodology.md` for the 3-place sln registration pattern (Project + ProjectConfigurationPlatforms + NestedProjects).
- **CRLF line endings in eqlib files.** Upstream uses LF; brainiac's diff tooling expects CRLF in some places. Strip CR after pulling files via `ssh ... type` -- `sed -i 's/\r$//' file`. See `feedback_crlf_line_endings.md`.

## 10. Where to learn more

- `docs/WORKFLOW.md` -- patch-day methodology, RE pipeline, struct shuffling background
- `docs/SECRETS.md` -- credential mapping (gitignored, populate locally)
- `tools/build-tools/README.md` -- build tooling quick-reference
- `/mnt/DEV/reverse-engineering/MQ-RE/` -- separate repo with the full RE knowledge base, raw captures, forensics docs
- `~/.claude/projects/-mnt-DEV-reverse-engineering-MQ-RE/memory/` -- per-feedback memory files referenced throughout this doc; each one documents a specific lesson learned the hard way
- `https://github.com/macroquest/macroquest` -- upstream macroquest repo (registered as `upstream` remote)
- `https://github.com/Krakty/MQ2-Krakty` -- the fork itself, branches: `live`, `test`, `apr15-2026-live`, `apr15-2026-test`, etc.

When in doubt about a specific feedback file, read it before acting. Each one represents hours-to-days of wasted time encoded as a one-page warning.
