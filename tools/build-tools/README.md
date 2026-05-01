# MQBuild

PowerShell automation for the MacroQuest build/deploy workflow.

Runs on BEAST (Linux). Drives wintest via SSH. Deploys to wks-lt7760 (10.7.30.37).

## Prerequisites

- pwsh (PowerShell 7+) on Linux
- `wintest` SSH alias configured in `~/.ssh/config.d/`
- SSH access to 10.7.30.37

## Install

```bash
# One-time: add to your PowerShell profile or source manually
Import-Module /mnt/DEV/reverse-engineering/MQ-RE/build-tools/MQBuild/MQBuild.psd1
```

Or use the CLI wrapper directly:

```bash
pwsh /mnt/DEV/reverse-engineering/MQ-RE/build-tools/MQBuild/mqbuild.ps1 <command>
```

## Common usage

```bash
# Build and deploy in one shot
pwsh mqbuild.ps1 build && pwsh mqbuild.ps1 deploy

# Build + deploy single command
pwsh mqbuild.ps1 build --deploy

# Clean build (after branch switch with substantial changes)
pwsh mqbuild.ps1 build --clean

# Switch to patch branch
pwsh mqbuild.ps1 switch apr15-2026-live

# Promote patch branch to live
pwsh mqbuild.ps1 promote apr15-2026-live live

# Interactive menu
pwsh mqbuild.ps1 menu

# Status
pwsh mqbuild.ps1 status
```

## Key rules encoded (do not fight these)

- cmake is NEVER used. MSBuild only. See `feedback_crashpad_crt.md`.
- Full solution rebuild always. No partial project builds.
- Deploy via scp to 10.7.30.37. Local cp is wrong and has same path.
- Deploy ALL DLLs + MacroQuest.exe every time. No cherry-picking.
- Deploy to MacroQuest/ root. NOT Release/ subdirectory.
- No force-push to live or test. Promote uses ff-only merge only.
- Verify timestamps on laptop after every deploy.
