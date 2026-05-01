@{
    ModuleVersion     = '0.1.0'
    GUID              = 'a3f1c2d4-8e5b-4a7f-9c1d-2b3e4f5a6c7d'
    Author            = 'tlindell'
    Description       = 'MacroQuest build/deploy automation for wintest -> wks-lt7760'
    PowerShellVersion = '7.0'
    RootModule        = 'MQBuild.psm1'
    FunctionsToExport = @(
        'Get-MQBuildStatus',
        'Switch-MQBranch',
        'Invoke-MQBuild',
        'Invoke-MQDeploy',
        'Promote-MQBranch',
        'Sync-MQWintest',
        'Invoke-MQCommentUpdate',
        'Show-MQMenu'
    )
    AliasesToExport   = @()
    VariablesToExport = @()
    CmdletsToExport   = @()
}
