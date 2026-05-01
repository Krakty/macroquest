#!/usr/bin/env pwsh
# mqbuild.ps1 - CLI entry point for MQBuild module
# Usage: pwsh mqbuild.ps1 <command> [options]

param(
    [Parameter(Position=0)][string]$Command,
    [Parameter(Position=1)][string]$Arg1,
    [Parameter(Position=2)][string]$Arg2,
    [switch]$Clean,
    [switch]$Deploy,
    [switch]$Stash,
    [switch]$Force,
    [switch]$DryRun,
    [switch]$CommentUpdate,
    [ValidateSet('Release','Debug')][string]$Configuration = 'Release',
    [string]$Target,
    [string]$Prefix = 'sinbash'
)

$ErrorActionPreference = 'Stop'

# Load the module from the same directory as this script
$moduleDir = Join-Path $PSScriptRoot 'MQBuild.psd1'
if (-not (Test-Path $moduleDir)) {
    $moduleDir = Join-Path (Split-Path $PSScriptRoot -Parent) 'MQBuild/MQBuild.psd1'
}
Import-Module $moduleDir -Force

function Show-Usage {
    Write-Host @'
mqbuild - MacroQuest build/deploy automation

Usage:
  mqbuild build [--clean] [--debug] [--deploy] [--comment-update]
  mqbuild deploy [--target <ip>] [--prefix sinbash|testbench|bolrik] [--dry-run]
  mqbuild switch <branch> [--stash] [--force]
  mqbuild promote <patch-branch> <live|test> [--force]
  mqbuild sync [<branch>]
  mqbuild status
  mqbuild comment-update
  mqbuild menu

Commands:
  build          Build MacroQuest.sln with MSBuild (Release by default)
  deploy         Deploy all DLLs via scp to the laptop (10.7.30.37)
  switch         Checkout a branch on wintest + update submodules
  promote        Fast-forward live/test to a patch branch
  sync           Fetch + pull + submodule update
  status         Show current branch and build state
  comment-update Run comment-update tool (reads Release PDB)
  menu           Interactive TUI

Options:
  --clean         /t:Clean,Build -- full rebuild (recommended after branch switch)
  --debug         Configuration=Debug (SIZE_CHECK active -- struct sizes must match)
  --deploy        Also deploy after build
  --comment-update  Run comment-update after build
  --target <ip>   Deploy target IP (default: 10.7.30.37)
  --prefix <name> Wine prefix: sinbash | testbench | bolrik (default: sinbash)
  --dry-run       Print deploy commands without executing
  --stash         Stash uncommitted changes before branch switch
  --force         Skip confirmation prompts (ff-only check on promote is non-bypassable)

Notes:
  cmake is NEVER used. MSBuild only. See feedback_crashpad_crt.md.
  Full solution only. No partial project builds.
  Deploy via scp to 10.7.30.37. Local cp is wrong.
  Deploy ALL DLLs every time. Deploy to MacroQuest/ root, not Release/.
'@
}

$exitCode = 0

try {
    switch ($Command.ToLower()) {
        'build' {
            $cfg = if ($Configuration -eq 'Release') { 'Release' } else { 'Debug' }
            $result = Invoke-MQBuild -Configuration $cfg -Clean:$Clean `
                -CommentUpdate:$CommentUpdate -Deploy:$Deploy -DeployPrefix $Prefix
            if (-not $result.Success) { $exitCode = 1 }
        }
        'deploy' {
            $deployArgs = @{ Prefix = $Prefix; DryRun = $DryRun.IsPresent }
            if ($Target) { $deployArgs.Target = $Target }
            $result = Invoke-MQDeploy @deployArgs
            if (-not $result.Success) { $exitCode = 1 }
        }
        'switch' {
            if (-not $Arg1) { Write-Error 'switch requires a branch name'; $exitCode = 2; break }
            $result = Switch-MQBranch -Branch $Arg1 -Stash:$Stash -Force:$Force
            if (-not $result.Success) { $exitCode = 1 }
        }
        'promote' {
            if (-not $Arg1 -or -not $Arg2) {
                Write-Error 'promote requires <patch-branch> and <live|test>'
                $exitCode = 2; break
            }
            $result = Promote-MQBranch -PatchBranch $Arg1 -TargetBranch $Arg2 -Force:$Force
            if (-not $result.Success) { $exitCode = 1 }
        }
        'sync' {
            $syncArgs = @{}
            if ($Arg1) { $syncArgs.Branch = $Arg1 }
            if ($Force) { $syncArgs.Force = $true }
            $result = Sync-MQWintest @syncArgs
            if (-not $result.Success) { $exitCode = 1 }
        }
        'status' {
            Get-MQBuildStatus | Format-List
        }
        'comment-update' {
            $result = Invoke-MQCommentUpdate
            if (-not $result.Success) { $exitCode = 1 }
        }
        'menu' {
            Show-MQMenu
        }
        { $_ -in @('help', '--help', '-h', '') } {
            Show-Usage
        }
        default {
            Write-Host "Unknown command: '$Command'" -ForegroundColor Red
            Show-Usage
            $exitCode = 2
        }
    }
} catch {
    Write-Host "Error: $_" -ForegroundColor Red
    $exitCode = 1
}

exit $exitCode
