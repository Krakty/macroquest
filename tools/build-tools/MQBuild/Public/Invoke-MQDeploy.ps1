# Invoke-MQDeploy.ps1 - scp all build artifacts from wintest to the laptop
#
# CRITICAL RULES (all learned the hard way):
# 1. Use scp to 10.7.30.37. NEVER local cp. Same path exists on beast and laptop.
#    See: feedback_deploy_scp.md -- cost 4+ hours of testing stale DLLs.
# 2. Deploy ALL DLLs every time: eqlib, MQ2Main, MacroQuest.exe, imgui-64, ALL plugins.
#    See: feedback_deploy_all_plugins.md
# 3. Deploy to MacroQuest/ ROOT. NEVER to Release/ subdirectory.
#    See: feedback_deploy_path_root.md -- cost an entire session debugging stale DLLs.
# 4. Verify timestamps on laptop after every deploy.
#    See: reference_build_deploy_workflow.md

function Invoke-MQDeploy {
    [CmdletBinding(SupportsShouldProcess)]
    param(
        [string]$Target   = (Get-MQConfig).DeployTarget,   # must be 10.7.30.37
        [string]$Prefix   = 'sinbash',   # sinbash | testbench | bolrik
        [switch]$DryRun,
        [switch]$SkipVerify
    )

    $cfg        = Get-MQConfig
    $stageDir   = $cfg.LocalStageDir
    $buildRoot  = $cfg.WintestBuildRoot
    $wintestHost = $cfg.WintestHost

    # Guard: never cp, always scp
    if ($Target -eq 'localhost' -or $Target -eq '127.0.0.1') {
        throw "Deploy target '$Target' looks like localhost. Use scp to 10.7.30.37, not cp. See feedback_deploy_scp.md."
    }

    $paths = Get-MQDeployPath -Prefix $Prefix
    Write-MQLog "Deploy target: $Target -> $($paths.Root)" -Level Step

    if ($DryRun) {
        Write-MQLog '[DRY RUN] No files will be transferred.' -Level Warn
    }

    # Ensure local stage directory exists
    if (-not (Test-Path $stageDir)) {
        New-Item -ItemType Directory -Path $stageDir -Force | Out-Null
    }

    # Get canonical DLL list from wintest
    $dllList = Get-MQDLLList -BuildRoot $buildRoot

    if (-not $dllList.HasAllCore) {
        $missing = $dllList.MissingCore -join ', '
        throw "Missing core DLLs: $missing. Run Invoke-MQBuild first."
    }

    $deployed = [System.Collections.Generic.List[string]]::new()
    $failed   = [System.Collections.Generic.List[string]]::new()

    # Deploy core files (DLLs + EXE) to MacroQuest/ root
    Write-MQLog "Deploying $($dllList.CoreFiles.Count) core files..." -Level Step
    foreach ($file in $dllList.CoreFiles) {
        $wintestPath = "$buildRoot\$file"
        $stagePath   = Join-Path $stageDir $file
        $remotePath  = "$($cfg.DeployUser)@${Target}:$($paths.Root)/$file"

        if ($DryRun) {
            Write-MQLog "  [DRY] scp $wintestHost`:'$wintestPath' -> $remotePath" -Level Info
            continue
        }

        # Step 1: wintest -> beast /tmp/
        $r1 = & scp "$wintestHost`:'$($wintestPath -replace '\\','/')'" $stagePath 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-MQLog "scp from wintest failed for $file`: $r1" -Level Error
            $failed.Add($file)
            continue
        }

        # Step 2: beast /tmp/ -> laptop
        $r2 = & scp $stagePath $remotePath 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-MQLog "scp to laptop failed for $file`: $r2" -Level Error
            $failed.Add($file)
            continue
        }

        $deployed.Add($file)
        Write-MQLog "  $file" -Level Success
    }

    # Handle MacroQuest.exe -> mq-bot.exe rename
    # See: feedback_deploy_all_plugins.md -- launcher uses mq-bot.exe
    if (-not $DryRun -and $dllList.CoreFiles -contains 'MacroQuest.exe') {
        $exeStagePath = Join-Path $stageDir 'MacroQuest.exe'
        $botRemote    = "$($cfg.DeployUser)@${Target}:$($paths.Root)/mq-bot.exe"
        $r3 = & scp $exeStagePath $botRemote 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-MQLog "Warning: mq-bot.exe deploy failed: $r3" -Level Warn
        } else {
            Write-MQLog '  MacroQuest.exe -> mq-bot.exe (launcher rename)' -Level Success
        }
    }

    # Deploy plugins to MacroQuest/plugins/
    if ($dllList.PluginFiles.Count -gt 0) {
        Write-MQLog "Deploying $($dllList.PluginFiles.Count) plugins..." -Level Step
        foreach ($plugin in $dllList.PluginFiles) {
            # MQ2ChatWnd warning (pre-existing crash at mq2chatwnd.dll+0x1B60)
            # See: reference_build_deploy_workflow.md
            if ($plugin -eq 'MQ2ChatWnd.dll') {
                Write-MQLog '  Note: MQ2ChatWnd has a pre-existing crash (+0x1B60). Deploying anyway. User manages .disabled manually.' -Level Warn
            }

            $wintestPluginPath = "$buildRoot\plugins\$plugin"
            $stagePluginPath   = Join-Path $stageDir $plugin
            $remotePluginPath  = "$($cfg.DeployUser)@${Target}:$($paths.Plugins)/$plugin"

            if ($DryRun) {
                Write-MQLog "  [DRY] scp $wintestHost`:'$wintestPluginPath' -> $remotePluginPath" -Level Info
                continue
            }

            $r1 = & scp "$wintestHost`:'$($wintestPluginPath -replace '\\','/')'" $stagePluginPath 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-MQLog "  scp from wintest failed for plugin $plugin" -Level Warn
                $failed.Add($plugin)
                continue
            }

            $r2 = & scp $stagePluginPath $remotePluginPath 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-MQLog "  scp to laptop failed for plugin $plugin" -Level Warn
                $failed.Add($plugin)
                continue
            }

            $deployed.Add($plugin)
            Write-MQLog "  $plugin" -Level Success
        }
    }

    # Post-deploy timestamp verification
    $verifiedFiles  = [System.Collections.Generic.List[string]]::new()
    $verifyFailed   = [System.Collections.Generic.List[string]]::new()

    if (-not $SkipVerify -and -not $DryRun) {
        Write-MQLog 'Verifying deploy timestamps on laptop...' -Level Step
        $verifyCmd = "ls -la $($paths.Root)/eqlib.dll $($paths.Root)/MQ2Main.dll $($paths.Root)/MacroQuest.exe 2>/dev/null"
        $verifyOut = & ssh $Target $verifyCmd 2>&1
        $verifyOut | ForEach-Object { Write-MQLog "  $_" -Level Info }

        $today = Get-Date -Format 'MMM d'
        $verifyOut | ForEach-Object {
            if ($_ -match $today) { $verifiedFiles.Add($_) }
            else                  { $verifyFailed.Add($_) }
        }

        if ($verifyFailed.Count -gt 0) {
            Write-MQLog "Timestamp check failed for $($verifyFailed.Count) files -- they may not have deployed." -Level Warn
        } else {
            Write-MQLog 'Timestamp verification passed.' -Level Success
        }
    }

    $totalSuccess = ($failed.Count -eq 0)
    if ($totalSuccess) {
        Write-MQLog "Deploy complete: $($deployed.Count) files, 0 failures." -Level Success
    } else {
        Write-MQLog "Deploy finished with $($failed.Count) failures." -Level Error
    }

    return [pscustomobject]@{
        Success       = $totalSuccess
        FilesDeployed = $deployed.ToArray()
        FailedFiles   = $failed.ToArray()
        DeployPath    = $paths.Root
        VerifiedFiles = $verifiedFiles.ToArray()
        VerifyFailed  = $verifyFailed.ToArray()
        DryRun        = $DryRun.IsPresent
    }
}
