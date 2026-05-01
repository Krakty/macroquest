# Invoke-MQSSHCommand.ps1 - SSH dispatch to wintest
# Uses the exact incantation from reference_wintest_ssh.md.
# All SSH calls go through here; never hand-roll ssh strings in other functions.

function Invoke-MQSSHCommand {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Command,
        [string]$Host    = (Get-MQConfig).WintestHost,
        [switch]$PassThru   # return raw string array; default returns object
    )

    Write-MQLog "SSH [$Host]: $Command" -Level Step

    $outputLines = & ssh -o RemoteCommand=none -o RequestTTY=no $Host $Command 2>&1
    $exitCode    = $LASTEXITCODE

    if ($PassThru) {
        return $outputLines
    }

    [pscustomobject]@{
        ExitCode = $exitCode
        Output   = $outputLines | Where-Object { $_ -isnot [System.Management.Automation.ErrorRecord] }
        Errors   = $outputLines | Where-Object { $_ -is [System.Management.Automation.ErrorRecord] } |
                   ForEach-Object { $_.ToString() }
        Success  = ($exitCode -eq 0)
    }
}

# Convenience: run a cmd.exe command on wintest (wraps in cmd /c)
function Invoke-MQWintestCmd {
    param([Parameter(Mandatory)][string]$CmdLine)
    Invoke-MQSSHCommand -Command "cmd /c `"$CmdLine`""
}
