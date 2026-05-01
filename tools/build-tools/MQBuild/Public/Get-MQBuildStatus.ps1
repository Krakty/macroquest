# Get-MQBuildStatus.ps1 - snapshot of current branch and build state on wintest

function Get-MQBuildStatus {
    [CmdletBinding()]
    param()

    $cfg = Get-MQConfig

    Write-MQLog 'Querying wintest build status...' -Level Step

    # Main repo branch
    $branchResult = Invoke-MQSSHCommand -Command "git -C `"$($cfg.WintestRepoRoot)`" branch --show-current"
    $branch = ($branchResult.Output | Select-Object -First 1).Trim()

    # eqlib submodule branch + commit
    $eqlibBranch = (Invoke-MQSSHCommand -Command "git -C `"$($cfg.WintestEqlibRoot)`" branch --show-current").Output |
        Select-Object -First 1 | ForEach-Object { $_.Trim() }
    $eqlibCommit = (Invoke-MQSSHCommand -Command "git -C `"$($cfg.WintestEqlibRoot)`" rev-parse --short HEAD").Output |
        Select-Object -First 1 | ForEach-Object { $_.Trim() }

    # Uncommitted changes
    $statusResult = Invoke-MQSSHCommand -Command "git -C `"$($cfg.WintestRepoRoot)`" status --short"
    $dirtyFiles   = $statusResult.Output | Where-Object { $_.Trim() -ne '' }

    # Build artifact timestamps (eqlib.dll as proxy for build age)
    $eqlibDll = "$($cfg.WintestBuildRoot)\eqlib.dll"
    $tsResult = Invoke-MQSSHCommand -Command "forfiles /p `"$($cfg.WintestBuildRoot)`" /m eqlib.dll /c `"cmd /c echo @fdate @ftime`" 2>nul"
    $lastBuildRaw = ($tsResult.Output | Select-Object -First 1).Trim()

    $lastBuildTime = $null
    if ($lastBuildRaw -match '\d') {
        try { $lastBuildTime = [datetime]::Parse($lastBuildRaw) } catch { }
    }

    $lastBuildAge = if ($lastBuildTime) {
        $span = (Get-Date) - $lastBuildTime
        if     ($span.TotalMinutes -lt 60)  { "$([int]$span.TotalMinutes) minutes ago" }
        elseif ($span.TotalHours   -lt 24)  { "$([int]$span.TotalHours) hours ago" }
        else                                { "$([int]$span.TotalDays) days ago" }
    } else { 'unknown' }

    # DLL inventory
    $dllList = Get-MQDLLList

    $status = [pscustomobject]@{
        Branch           = $branch
        EqlibBranch      = $eqlibBranch
        EqlibCommit      = $eqlibCommit
        DirtyFiles       = $dirtyFiles
        LastBuildTime    = $lastBuildTime
        LastBuildAge     = $lastBuildAge
        BuildOutputs     = $dllList.CoreFiles + $dllList.PluginFiles
        MissingOutputs   = $dllList.MissingCore
        HasAllCore       = $dllList.HasAllCore
    }

    # Display
    Write-MQLog "Branch:      $($status.Branch)" -Level Info
    Write-MQLog "eqlib:       $($status.EqlibBranch) @ $($status.EqlibCommit)" -Level Info
    Write-MQLog "Last build:  $($status.LastBuildAge)" -Level Info
    if ($status.DirtyFiles.Count -gt 0) {
        Write-MQLog "Dirty files: $($status.DirtyFiles.Count) uncommitted changes" -Level Warn
    }
    if ($status.MissingOutputs.Count -gt 0) {
        Write-MQLog "Missing DLLs: $($status.MissingOutputs -join ', ')" -Level Error
    } else {
        Write-MQLog "Build outputs: all core DLLs present" -Level Success
    }

    return $status
}
