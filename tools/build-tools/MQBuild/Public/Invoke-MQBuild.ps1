# Invoke-MQBuild.ps1 - run MSBuild on MacroQuest.sln on wintest
#
# DO NOT ADD CMAKE. cmake has broken crashpad integration (Debug CRT in Release).
# We lost an entire day to this. MSBuild with the .sln is the correct build system.
# See: feedback_crashpad_crt.md, reference_cmake_build.md
#
# ALWAYS build the full solution. Never target individual projects.
# See: feedback_clean_first.md

function Invoke-MQBuild {
    [CmdletBinding()]
    param(
        [ValidateSet('Release', 'Debug')]
        [string]$Configuration = 'Release',

        # Adds /t:Clean,Build -- MSBuild equivalent of cmake --clean-first.
        # Recommended after branch switches with substantial changes.
        # See: feedback_build_gotchas.md (stale cached objects)
        [switch]$Clean,

        # Run comment-update after a successful build.
        # comment-update uses Release PDB to write offset comments to headers.
        [switch]$CommentUpdate,

        # Also deploy after a successful build.
        [switch]$Deploy,
        [string]$DeployPrefix = 'sinbash'
    )

    $cfg     = Get-MQConfig
    $msbuild = $cfg.MSBuildExe
    $sln     = $cfg.WintestSln

    # Note: SIZE_CHECK macro is ACTIVE in Debug builds.
    # If struct sizes are wrong, Debug build fails. Release is the primary deploy artifact.
    if ($Configuration -eq 'Debug') {
        Write-MQLog 'Debug build requested. SIZE_CHECK is active -- build will fail if struct sizes are wrong.' -Level Warn
    }

    $verbosity = if ($VerbosePreference -eq 'Continue') { 'normal' } else { 'minimal' }

    # Full solution only -- never /t:ProjectName
    $buildCmd = "`"$msbuild`" `"$sln`" /NoLogo /Verbosity:$verbosity /p:Configuration=$Configuration /p:Platform=x64"
    if ($Clean) {
        $buildCmd += ' /t:Clean,Build'
        Write-MQLog 'Clean build requested -- /t:Clean,Build added.' -Level Warn
    }

    Write-MQLog "Building MacroQuest.sln [$Configuration]..." -Level Step
    $startTime = Get-Date

    # Capture output to a timestamped log on the Linux side via SSH
    $logFile = "$($cfg.LocalStageDir)/mqbuild-$(Get-Date -Format 'yyyyMMdd-HHmmss').log"
    if (-not (Test-Path (Split-Path $logFile -Parent))) {
        New-Item -ItemType Directory -Path (Split-Path $logFile -Parent) -Force | Out-Null
    }

    $result = Invoke-MQSSHCommand -Command $buildCmd
    $result.Output | Set-Content -Path $logFile -Encoding UTF8

    $duration = (Get-Date) - $startTime

    # Parse MSBuild output for errors and warnings
    $errorLines   = $result.Output | Where-Object { $_ -match '\s+error\s+' }
    $warningLines = $result.Output | Where-Object { $_ -match '\s+warning\s+' }
    $succeeded    = $result.Output | Where-Object { $_ -match 'Build succeeded' }
    $failed       = $result.Output | Where-Object { $_ -match 'Build FAILED' }

    $success = ($result.Success -and $succeeded.Count -gt 0 -and $failed.Count -eq 0)

    if ($success) {
        Write-MQLog "Build succeeded in $([int]$duration.TotalSeconds)s. Warnings: $($warningLines.Count)" -Level Success
    } else {
        Write-MQLog "Build FAILED in $([int]$duration.TotalSeconds)s." -Level Error
        $errorLines | Select-Object -Last 20 | ForEach-Object { Write-MQLog $_ -Level Error }
        Write-MQLog "Full log: $logFile" -Level Info
    }

    # Verify build output actually changed (stale build detection)
    # See: feedback_build_gotchas.md
    if ($success) {
        $eqlibDll = "$($cfg.WintestBuildRoot)\eqlib.dll"
        $mtimeResult = Invoke-MQSSHCommand -Command "forfiles /p `"$($cfg.WintestBuildRoot)`" /m eqlib.dll /c `"cmd /c echo @ftime`" 2>nul"
        Write-MQLog "eqlib.dll mtime: $($mtimeResult.Output | Select-Object -First 1)" -Level Info
    }

    if ($success -and $CommentUpdate) {
        Invoke-MQCommentUpdate
    }

    if ($success -and $Deploy) {
        Invoke-MQDeploy -Prefix $DeployPrefix
    }

    return [pscustomobject]@{
        Success       = $success
        Configuration = $Configuration
        Duration      = $duration
        Errors        = $errorLines
        Warnings      = $warningLines.Count
        BuildLog      = $logFile
        Clean         = $Clean.IsPresent
    }
}
