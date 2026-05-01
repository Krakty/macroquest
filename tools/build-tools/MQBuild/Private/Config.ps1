# Config.ps1 - all magic strings in one place.
# Edit this file when paths change. Nothing else should contain hardcoded paths.

$script:MQConfig = @{
    # wintest SSH host alias (from ~/.ssh/config.d/)
    WintestHost      = 'wintest'

    # Source and build paths on wintest
    WintestRepoRoot  = 'C:\macroquest'
    WintestSrcRoot   = 'C:\macroquest\src'
    WintestEqlibRoot = 'C:\macroquest\src\eqlib'
    WintestVcpkgRoot = 'C:\macroquest\contrib\vcpkg'
    WintestSln       = 'C:\macroquest\src\MacroQuest.sln'
    WintestBuildRoot = 'C:\macroquest\build\bin\release'
    WintestDebugRoot = 'C:\macroquest\build\bin\debug'

    # MSBuild - DO NOT REPLACE WITH CMAKE
    # cmake has broken crashpad integration (Debug CRT in Release mode).
    # We lost an entire day to this. MSBuild with the .sln is the real build system.
    # See: feedback_crashpad_crt.md, reference_cmake_build.md
    MSBuildExe       = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'

    # comment-update tool (reads Release PDB, writes offset comments to headers)
    CommentUpdateExe = 'C:\macroquest\tools\comment-update\src\comment-update\bin\Release\net9.0-windows8.0\comment-update.exe'
    CommentUpdateDir = 'C:\macroquest\tools\comment-update'

    # Deploy target: wks-lt7760 laptop running EQ under Wine
    # CRITICAL: 10.7.30.37 is the ONLY valid deploy target.
    # The path /home/tlindell/EQ-MASTER/ exists on BOTH beast and wks-lt7760.
    # Using cp on beast writes to beast. The game runs on the laptop. Use scp.
    # See: feedback_deploy_scp.md - this mistake cost 4+ hours.
    DeployTarget     = '10.7.30.37'
    DeployUser       = 'tlindell'
    DeployBasePath   = '/home/tlindell/EQ-MASTER'

    # Local staging dir (two-hop scp: wintest -> beast /tmp/ -> laptop)
    LocalStageDir    = '/tmp/mqbuild-stage'

    # Branches that require confirmation and forbid force-push
    # See: feedback_brainiac_rules.md rule 4
    ProtectedBranches = @('live', 'test')

    # Log file for persistent record of all operations
    LogFile          = '/tmp/mqbuild-stage/mqbuild.log'
}

# Wine prefix map: short name -> directory under $DeployBasePath
# Deploy path = $DeployBasePath/$prefix/drive_c/MacroQuest/
# NEVER append Release/ to this. MQ loads from the root.
# See: feedback_deploy_path_root.md - deploying to Release/ does nothing.
$script:MQPrefixMap = [ordered]@{
    'sinbash'   = 'prefix-sinbash'
    'testbench' = 'prefix-testbench'
    'bolrik'    = 'prefix-bolrik'
}

# Core artifacts that MUST be deployed every build.
# Missing any of these is a hard error.
# See: feedback_deploy_all_plugins.md
$script:MQCoreDLLs = @(
    'eqlib.dll'
    'MQ2Main.dll'
    'MacroQuest.exe'
    'imgui-64.dll'
)

function Get-MQConfig { $script:MQConfig }
function Get-MQPrefixMap { $script:MQPrefixMap }
function Get-MQCoreDLLs { $script:MQCoreDLLs }

function Get-MQDeployPath {
    param([string]$Prefix = 'sinbash')
    if (-not $script:MQPrefixMap.Contains($Prefix)) {
        $valid = $script:MQPrefixMap.Keys -join ', '
        throw "Unknown prefix '$Prefix'. Valid: $valid"
    }
    $prefixDir = $script:MQPrefixMap[$Prefix]
    $base      = $script:MQConfig.DeployBasePath
    # Root path - NEVER append Release/ here
    [pscustomobject]@{
        Root    = "$base/$prefixDir/drive_c/MacroQuest"
        Plugins = "$base/$prefixDir/drive_c/MacroQuest/plugins"
    }
}
