# Invoke-MQCommentUpdate.ps1 - run the comment-update tool on wintest
#
# comment-update reads the Release PDB and writes compiled struct offsets
# as comments into the header files. This is the ground truth for what
# MSVC actually produces for the current declaration order.
#
# If it fails with "file not found", a vcpkg package path is missing
# from C:\macroquest\tools\comment-update\comment-update.config.
# Add the missing path to the includePaths array in that config.

function Invoke-MQCommentUpdate {
    [CmdletBinding()]
    param()

    $cfg = Get-MQConfig
    $exe = $cfg.CommentUpdateExe
    $dir = $cfg.CommentUpdateDir

    Write-MQLog 'Running comment-update (reads Release PDB, writes offset comments to headers)...' -Level Step
    $startTime = Get-Date

    # Exact command from reference_wintest_ssh.md
    $cmd = "cd `"$dir`" && `"$exe`""
    $result = Invoke-MQSSHCommand -Command $cmd

    $duration = (Get-Date) - $startTime

    if (-not $result.Success) {
        Write-MQLog "comment-update failed after $([int]$duration.TotalSeconds)s." -Level Error
        if ($result.Output -match 'file not found' -or $result.Errors -match 'file not found') {
            Write-MQLog 'Likely cause: a vcpkg package path is missing from comment-update.config.' -Level Warn
            Write-MQLog "Config: $($cfg.CommentUpdateDir)\comment-update.config" -Level Warn
            Write-MQLog 'Add missing package path to includePaths array. Packages: fmt, dxsdk-d3dx, etc.' -Level Warn
        }
    } else {
        Write-MQLog "comment-update finished in $([int]$duration.TotalSeconds)s." -Level Success
    }

    $result.Output | ForEach-Object { Write-MQLog $_ -Level Info }

    return [pscustomobject]@{
        Success  = $result.Success
        Output   = $result.Output
        Duration = $duration
    }
}
