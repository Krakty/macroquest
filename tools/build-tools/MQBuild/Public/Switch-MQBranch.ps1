# Switch-MQBranch.ps1 - checkout a branch on wintest + update submodules
# See: feedback_build_branch.md, feedback_vcpkg_submodule.md

function Switch-MQBranch {
    [CmdletBinding(SupportsShouldProcess)]
    param(
        [Parameter(Mandatory)][string]$Branch,
        [switch]$Stash,   # stash uncommitted changes before switching
        [switch]$Force    # skip interactive confirmation
    )

    $cfg = Get-MQConfig
    $repo = $cfg.WintestRepoRoot

    # Check current branch
    $currentBranch = (Invoke-MQSSHCommand -Command "git -C `"$repo`" branch --show-current").Output |
        Select-Object -First 1 | ForEach-Object { $_.Trim() }

    if ($currentBranch -eq $Branch) {
        Write-MQLog "Already on branch '$Branch'." -Level Info
        return [pscustomobject]@{
            PreviousBranch   = $Branch
            NewBranch        = $Branch
            SubmoduleUpdated = $false
            StashCreated     = $false
            Success          = $true
        }
    }

    Write-MQLog "Switching: '$currentBranch' -> '$Branch'" -Level Step

    # Check for uncommitted changes
    $statusResult = Invoke-MQSSHCommand -Command "git -C `"$repo`" status --short"
    $dirty = $statusResult.Output | Where-Object { $_.Trim() -ne '' }
    $stashCreated = $false

    if ($dirty.Count -gt 0) {
        Write-MQLog "Uncommitted changes detected ($($dirty.Count) files):" -Level Warn
        $dirty | ForEach-Object { Write-MQLog "  $_" -Level Warn }

        if ($Stash) {
            Write-MQLog 'Stashing uncommitted changes...' -Level Step
            $stashMsg = "mqbuild auto-stash before switching to $Branch"
            $stashResult = Invoke-MQSSHCommand -Command "git -C `"$repo`" stash push -m `"$stashMsg`""
            if (-not $stashResult.Success) {
                Write-MQLog 'Stash failed.' -Level Error
                return [pscustomobject]@{ Success = $false }
            }
            $stashCreated = $true
            Write-MQLog 'Stash created.' -Level Success
        } elseif (-not $Force) {
            Write-MQLog 'Pass -Stash to stash them or -Force to proceed (changes may be lost).' -Level Error
            return [pscustomobject]@{ Success = $false }
        } else {
            Write-MQLog 'Proceeding with dirty working tree (-Force).' -Level Warn
        }
    }

    # Fetch
    Write-MQLog 'Fetching origin...' -Level Step
    $fetchResult = Invoke-MQSSHCommand -Command "git -C `"$repo`" fetch origin"
    if (-not $fetchResult.Success) {
        Write-MQLog "Fetch failed: $($fetchResult.Errors -join ' ')" -Level Error
        return [pscustomobject]@{ Success = $false }
    }

    # Checkout
    $checkoutResult = Invoke-MQSSHCommand -Command "git -C `"$repo`" checkout `"$Branch`""
    if (-not $checkoutResult.Success) {
        Write-MQLog "Checkout failed: $($checkoutResult.Errors -join ' ')" -Level Error
        return [pscustomobject]@{ Success = $false }
    }
    Write-MQLog "Checked out '$Branch'." -Level Success

    # Update submodules (NOT --force; that resets working-tree edits in submodules)
    # --force is only safe when explicitly requested and vcpkg is not modified
    # See: feedback_vcpkg_submodule.md
    Write-MQLog 'Updating submodules (--init --recursive)...' -Level Step
    $subResult = Invoke-MQSSHCommand -Command "git -C `"$repo`" submodule update --init --recursive"
    if (-not $subResult.Success) {
        Write-MQLog "Submodule update failed. If vcpkg is broken, run: git submodule update --init --recursive --force" -Level Warn
    }

    # Check vcpkg for local modifications (warn only, do not auto-restore)
    $vcpkgStatus = (Invoke-MQSSHCommand -Command "git -C `"$($cfg.WintestVcpkgRoot)`" status --short").Output |
        Where-Object { $_.Trim() -ne '' }
    if ($vcpkgStatus.Count -gt 0) {
        Write-MQLog "WARNING: contrib/vcpkg has local modifications. Do NOT edit portfiles." -Level Warn
        Write-MQLog "Run: git -C C:\macroquest submodule update --init --recursive --force to restore." -Level Warn
    }

    # Recommend clean build after branch switch
    Write-MQLog "Branch switched. Consider running 'Invoke-MQBuild -Clean' after a substantial branch change." -Level Warn

    return [pscustomobject]@{
        PreviousBranch   = $currentBranch
        NewBranch        = $Branch
        SubmoduleUpdated = $subResult.Success
        StashCreated     = $stashCreated
        VcpkgDirty       = ($vcpkgStatus.Count -gt 0)
        Success          = $true
    }
}
