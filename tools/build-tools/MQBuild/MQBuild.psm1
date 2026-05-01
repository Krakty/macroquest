# MQBuild.psm1 - root module loader
# Dot-sources Private helpers first, then Public exported functions, then TUI.

$ErrorActionPreference = 'Stop'

$privatePath = Join-Path $PSScriptRoot 'Private'
$publicPath  = Join-Path $PSScriptRoot 'Public'
$tuiPath     = Join-Path $PSScriptRoot 'tui'

foreach ($dir in @($privatePath, $publicPath, $tuiPath)) {
    if (Test-Path $dir) {
        Get-ChildItem -Path "$dir/*.ps1" | ForEach-Object { . $_.FullName }
    }
}
