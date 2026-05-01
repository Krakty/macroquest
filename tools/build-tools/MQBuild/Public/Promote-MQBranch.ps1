# Promote-MQBranch.ps1 - fast-forward live or test to a patch branch
#
# RULES:
# - No force-push to live or test. EVER. See: feedback_brainiac_rules.md rule 4.
# - Must be a true fast-forward. If target is ahead, abort.
# - -Force only skips the confirmation prompt. The ff-only check is non-bypassable.

function Promote-MQBranch {
    [CmdletBinding(SupportsShouldProcess)]
    param(
        [Parameter(Mandatory)][string]$PatchBranch,   # e.g. 'apr15-2026-live'
        [Parameter(Mandatory)]
        [ValidateSet('live', 'test')]
        [string]$TargetBranch,
        [switch]$Force   # skip interactive prompt only, NOT the ff check
    )

    $cfg  = Get-MQConfig
    $repo = $cfg.WintestRepoRoot

    Write-MQLog "Promote '$PatchBranch' -> '$TargetBranch'" -Level Step

    # Fetch latest
    Write-MQLog 'Fetching origin...' -Level Step
    $fetch = Invoke-MQSSHCommand -Command "git -C `"$repo`" fetch origin"
    if (-not $fetch.Success) {
        Write-MQLog "Fetch failed: $($fetch.Errors -join ' ')" -Level Error
        return [pscustomobject]@{ Success = $false }
    }

    # Show what will be promoted
    $logCmd    = "git -C `"$repo`" log --oneline origin/$TargetBranch..origin/$PatchBranch"
    $logResult = Invoke-MQSSHCommand -Command $logCmd
    $commits   = $logResult.Output | Where-Object { $_.Trim() -ne '' }

    if ($commits.Count -eq 0) {
        Write-MQLog "'$TargetBranch' is already at the same commit as '$PatchBranch'. Nothing to promote." -Level Info
        return [pscustomobject]@{
            Success       = $true
            PatchBranch   = $PatchBranch
            TargetBranch  = $TargetBranch
            CommitsBehind = 0
            Pushed        = $false
        }
    }

    Write-MQLog "Commits to promote ($($commits.Count)):" -Level Info
    $commits | ForEach-Object { Write-MQLog "  $_" -Level Info }

    # Enforce fast-forward -- non-bypassable
    $ffCheck = Invoke-MQSSHCommand -Command "git -C `"$repo`" merge-base --is-ancestor origin/$TargetBranch origin/$PatchBranch"
    if (-not $ffCheck.Success) {
        Write-MQLog "ABORT: '$TargetBranch' is NOT an ancestor of '$PatchBranch'. This is not a fast-forward." -Level Error
        Write-MQLog "Check branch history manually. Do not force-push. See: feedback_brainiac_rules.md rule 4." -Level Error
        return [pscustomobject]@{ Success = $false; CommitsBehind = -1 }
    }

    # Interactive confirmation unless -Force
    if (-not $Force) {
        Write-MQLog "About to fast-forward '$TargetBranch' to '$PatchBranch' and push. This cannot be undone easily." -Level Warn
        $confirm = Read-Host "Type 'yes' to continue"
        if ($confirm -ne 'yes') {
            Write-MQLog 'Aborted by user.' -Level Info
            return [pscustomobject]@{ Success = $false }
        }
    }

    # Checkout target branch and ff-merge
    $checkout = Invoke-MQSSHCommand -Command "git -C `"$repo`" checkout `"$TargetBranch`""
    if (-not $checkout.Success) {
        Write-MQLog "Checkout of '$TargetBranch' failed." -Level Error
        return [pscustomobject]@{ Success = $false }
    }

    $merge = Invoke-MQSSHCommand -Command "git -C `"$repo`" merge --ff-only `"origin/$PatchBranch`""
    if (-not $merge.Success) {
        Write-MQLog "ff-only merge failed: $($merge.Errors -join ' ')" -Level Error
        return [pscustomobject]@{ Success = $false }
    }

    # Push -- no --force
    $push = Invoke-MQSSHCommand -Command "git -C `"$repo`" push origin `"$TargetBranch`""
    if (-not $push.Success) {
        Write-MQLog "Push failed: $($push.Errors -join ' ')" -Level Error
        return [pscustomobject]@{ Success = $false; Pushed = $false }
    }

    Write-MQLog "Promoted '$PatchBranch' -> '$TargetBranch' and pushed." -Level Success

    return [pscustomobject]@{
        Success       = $true
        PatchBranch   = $PatchBranch
        TargetBranch  = $TargetBranch
        CommitsBehind = $commits.Count
        Pushed        = $true
    }
}
