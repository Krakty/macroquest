# Write-MQLog.ps1 - color-coded output + persistent log file

function Write-MQLog {
    param(
        [Parameter(Mandatory)][string]$Message,
        [ValidateSet('Info','Success','Warn','Error','Step')][string]$Level = 'Info'
    )

    $colors = @{
        Info    = 'Cyan'
        Success = 'Green'
        Warn    = 'Yellow'
        Error   = 'Red'
        Step    = 'White'
    }

    $prefixes = @{
        Info    = '[  ]'
        Success = '[OK]'
        Warn    = '[!!]'
        Error   = '[XX]'
        Step    = '[>>]'
    }

    $color  = $colors[$Level]
    $prefix = $prefixes[$Level]
    $ts     = Get-Date -Format 'HH:mm:ss'
    $line   = "$ts $prefix $Message"

    Write-Host $line -ForegroundColor $color

    # Persist to log file
    $cfg = Get-MQConfig
    $logDir = Split-Path $cfg.LogFile -Parent
    if (-not (Test-Path $logDir)) {
        New-Item -ItemType Directory -Path $logDir -Force | Out-Null
    }
    Add-Content -Path $cfg.LogFile -Value $line -Encoding UTF8
}
