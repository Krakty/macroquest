# Sync-MQWintest.ps1 - fetch + pull + submodule update on wintest
# See: feedback_vcpkg_submodule.md -- NEVER run submodule update --force unless explicitly requested

function Sync-MQWintest {
    [CmdletBinding()]
    param(
        [string]$Branch,   # if omitted, syncs current branch in-place
        [switch]$Force     # passes --force to submodule update (resets working-tree edits)
    )

    $cfg  = Get-MQConfig
    $repo = $cfg.WintestRepoRoot

    # Get current branch if not specified
    if (-not $Branch) {
        $Branch = (Invoke-MQSSHCommand -Command "git -C `"$repo`" branch --show-current").Output |
            Select-Object -First 1 | ForEach-Object { $_.Trim() }
    }

    Write-MQLog "Syncing '$Branch' on wintest..." -Level Step

    # Fetch
    $fetch = Invoke-MQSSHCommand -Command "git -C `"$repo`" fetch origin"
    if (-not $fetch.Success) {
        Write-MQLog "Fetch failed: $($fetch.Errors -join ' ')" -Level Error
        return [pscustomobject]@{ Success = $false }
    }

    # Count incoming commits
    $logResult = Invoke-MQSSHCommand -Command "git -C `"$repo`" log --oneline HEAD..origin/$Branch"
    $incoming  = ($logResult.Output | Where-Object { $_.Trim() -ne '' }).Count
    Write-MQLog "Incoming commits: $incoming" -Level Info

    # Pull
    $pull = Invoke-MQSSHCommand -Command "git -C `"$repo`" pull --ff-only origin `"$Branch`""
    if (-not $pull.Success) {
        Write-MQLog "Pull failed (possibly not ff-able): $($pull.Errors -join ' ')" -Level Error
        Write-MQLog "You may need to stash and retry, or use Switch-MQBranch -Force." -Level Warn
        return [pscustomobject]@{ Success = $false; FetchedCommits = $incoming }
    }

    # Submodule update
    # --force resets working-tree edits in submodules -- dangerous for in-progress RE work
    # Only add it if explicitly requested
    $subFlags = '--init --recursive'
    if ($Force) {
        $subFlags += ' --force'
        Write-MQLog 'Submodule update with --force requested. This resets all submodule working-tree changes.' -Level Warn
    }
    $subUpdate = Invoke-MQSSHCommand -Command "git -C `"$repo`" submodule update $subFlags"
    $subOk     = $subUpdate.Success

    if (-not $subOk) {
        Write-MQLog "Submodule update had issues: $($subUpdate.Errors -join ' ')" -Level Warn
    }

    # vcpkg dirty check
    $vcpkgStatus = (Invoke-MQSSHCommand -Command "git -C `"$($cfg.WintestVcpkgRoot)`" status --short").Output |
        Where-Object { $_.Trim() -ne '' }
    $vcpkgDirty = ($vcpkgStatus.Count -gt 0)

    if ($vcpkgDirty) {
        Write-MQLog 'WARNING: contrib/vcpkg has local modifications.' -Level Warn
        Write-MQLog 'NEVER edit vcpkg portfiles directly. Restore with:' -Level Warn
        Write-MQLog '  git -C C:\macroquest submodule update --init --recursive --force' -Level Warn
        Write-MQLog 'See: feedback_vcpkg_submodule.md' -Level Warn
    }

    Write-MQLog "Sync complete. $incoming commits pulled." -Level Success

    return [pscustomobject]@{
        Success          = $true
        Branch           = $Branch
        FetchedCommits   = $incoming
        SubmoduleUpdated = $subOk
        VcpkgModified    = $vcpkgDirty
    }
}
