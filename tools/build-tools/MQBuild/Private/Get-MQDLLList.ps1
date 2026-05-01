# Get-MQDLLList.ps1 - enumerate build artifacts that must be deployed
# Queries wintest live so the list is always current.
# See: feedback_deploy_all_plugins.md

function Get-MQDLLList {
    [CmdletBinding()]
    param(
        [string]$BuildRoot = (Get-MQConfig).WintestBuildRoot
    )

    $cfg = Get-MQConfig

    # List core DLLs
    $coreResult = Invoke-MQSSHCommand -Command "dir /b `"$BuildRoot\*.dll`" `"$BuildRoot\*.exe`" 2>nul"

    $coreFiles = $coreResult.Output |
        Where-Object { $_ -match '\.(dll|exe)$' } |
        ForEach-Object { $_.Trim() }

    # Validate all expected core files are present
    $coreDLLs   = Get-MQCoreDLLs
    $missing    = $coreDLLs | Where-Object { $_ -notin $coreFiles }

    # List plugins (separate subdir)
    $pluginResult = Invoke-MQSSHCommand -Command "dir /b `"$BuildRoot\plugins\*.dll`" 2>nul"
    $pluginFiles  = $pluginResult.Output |
        Where-Object { $_ -match '\.dll$' } |
        ForEach-Object { $_.Trim() }

    [pscustomobject]@{
        CoreFiles    = $coreFiles
        PluginFiles  = $pluginFiles
        MissingCore  = $missing
        HasAllCore   = ($missing.Count -eq 0)
        BuildRoot    = $BuildRoot
    }
}
