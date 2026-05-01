# Show-MQMenu.ps1 - interactive console TUI
# Pure PowerShell menu loop, no external module dependencies.

function Show-MQMenu {
    [CmdletBinding()]
    param()

    $banner = @'
  __  __  ___  ____        _ _     _
 |  \/  |/ _ \| __ ) _   _(_) | __| |
 | |\/| | | | |  _ \| | | | | |/ _` |
 | |  | | |_| | |_) | |_| | | | (_| |
 |_|  |_|\__\_\____/ \__,_|_|_|\__,_|
'@

    while ($true) {
        Clear-Host
        Write-Host $banner -ForegroundColor Cyan
        Write-Host ''

        # Show status header
        try {
            $status = Get-MQBuildStatus
            Write-Host "  Branch: " -NoNewline -ForegroundColor Gray
            Write-Host $status.Branch -ForegroundColor Yellow -NoNewline
            Write-Host "  |  eqlib: " -NoNewline -ForegroundColor Gray
            Write-Host "$($status.EqlibBranch) @ $($status.EqlibCommit)" -ForegroundColor Yellow -NoNewline
            Write-Host "  |  Build: " -NoNewline -ForegroundColor Gray
            $ageColor = if ($status.LastBuildAge -match 'minutes|unknown') { 'Green' } else { 'DarkYellow' }
            Write-Host $status.LastBuildAge -ForegroundColor $ageColor
        } catch {
            Write-Host '  [status unavailable - check SSH to wintest]' -ForegroundColor Red
        }

        Write-Host ''
        Write-Host ('  ' + '-' * 50) -ForegroundColor DarkGray
        Write-Host '  1  Build (Release)'                             -ForegroundColor White
        Write-Host '  2  Build + Deploy'                             -ForegroundColor White
        Write-Host '  3  Build (Clean + Release)'                    -ForegroundColor White
        Write-Host '  4  Deploy only'                                -ForegroundColor White
        Write-Host '  5  Switch branch'                              -ForegroundColor White
        Write-Host '  6  Promote patch branch to live/test'          -ForegroundColor White
        Write-Host '  7  Sync wintest (fetch + pull + submodules)'   -ForegroundColor White
        Write-Host '  8  Run comment-update'                         -ForegroundColor White
        Write-Host '  9  Show build status'                          -ForegroundColor White
        Write-Host ('  ' + '-' * 50) -ForegroundColor DarkGray
        Write-Host '  q  Quit'                                       -ForegroundColor DarkGray
        Write-Host ''

        $choice = (Read-Host '  Select').Trim().ToLower()

        switch ($choice) {
            '1' {
                Invoke-MQBuild -Configuration Release
                Read-Host 'Press Enter to continue'
            }
            '2' {
                $result = Invoke-MQBuild -Configuration Release
                if ($result.Success) {
                    $prefix = Read-Host 'Deploy to prefix [sinbash/testbench/bolrik] (default: sinbash)'
                    if ([string]::IsNullOrWhiteSpace($prefix)) { $prefix = 'sinbash' }
                    Invoke-MQDeploy -Prefix $prefix
                } else {
                    Write-MQLog 'Build failed -- deploy skipped.' -Level Error
                }
                Read-Host 'Press Enter to continue'
            }
            '3' {
                Invoke-MQBuild -Configuration Release -Clean
                Read-Host 'Press Enter to continue'
            }
            '4' {
                $prefix = Read-Host 'Deploy to prefix [sinbash/testbench/bolrik] (default: sinbash)'
                if ([string]::IsNullOrWhiteSpace($prefix)) { $prefix = 'sinbash' }
                Invoke-MQDeploy -Prefix $prefix
                Read-Host 'Press Enter to continue'
            }
            '5' {
                $branch = Read-Host 'Branch name (e.g. apr15-2026-live)'
                if ($branch) {
                    $stash = Read-Host 'Stash uncommitted changes? [y/N]'
                    $stashSwitch = ($stash -eq 'y')
                    Switch-MQBranch -Branch $branch -Stash:$stashSwitch
                }
                Read-Host 'Press Enter to continue'
            }
            '6' {
                $patch  = Read-Host 'Patch branch (e.g. apr15-2026-live)'
                $target = Read-Host 'Target [live/test]'
                if ($patch -and $target -in @('live','test')) {
                    Promote-MQBranch -PatchBranch $patch -TargetBranch $target
                } else {
                    Write-MQLog 'Invalid input.' -Level Error
                }
                Read-Host 'Press Enter to continue'
            }
            '7' {
                Sync-MQWintest
                Read-Host 'Press Enter to continue'
            }
            '8' {
                Invoke-MQCommentUpdate
                Read-Host 'Press Enter to continue'
            }
            '9' {
                Get-MQBuildStatus | Format-List
                Read-Host 'Press Enter to continue'
            }
            { $_ -in @('q', 'quit', 'exit') } {
                Write-Host 'Goodbye.' -ForegroundColor Gray
                return
            }
            default {
                Write-MQLog "Unknown option: '$choice'" -Level Warn
                Start-Sleep -Seconds 1
            }
        }
    }
}
