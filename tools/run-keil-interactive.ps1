param(
    [ValidateSet("build", "rebuild", "clean", "flash")]
    [string]$Action = "flash"
)

$ErrorActionPreference = "Stop"

$Workspace = "C:\Users\jjp\Documents\Keil-work"
$KeilBuildScript = Join-Path $Workspace "tools\keil-build.ps1"
$TaskDir = Join-Path $Workspace ".keil-task"
$StatusFile = Join-Path $TaskDir "$Action.status"
$TaskName = "KeilVsCode-$Action"
$User = "$env:COMPUTERNAME\$env:USERNAME"

if (!(Test-Path $KeilBuildScript)) {
    throw "Keil build script was not found at $KeilBuildScript"
}

New-Item -ItemType Directory -Force -Path $TaskDir | Out-Null
Remove-Item -Path $StatusFile -ErrorAction SilentlyContinue

$TaskArguments = @(
    "-NoProfile"
    "-ExecutionPolicy"
    "Bypass"
    "-File"
    "`"$KeilBuildScript`""
    $Action
    "-StatusFile"
    "`"$StatusFile`""
) -join " "

$TaskAction = New-ScheduledTaskAction `
    -Execute "powershell.exe" `
    -Argument $TaskArguments `
    -WorkingDirectory $Workspace

$TaskTrigger = New-ScheduledTaskTrigger -Once -At (Get-Date).AddMinutes(1)
$TaskPrincipal = New-ScheduledTaskPrincipal `
    -UserId $User `
    -LogonType Interactive `
    -RunLevel Highest

Register-ScheduledTask `
    -TaskName $TaskName `
    -Action $TaskAction `
    -Trigger $TaskTrigger `
    -Principal $TaskPrincipal `
    -Force | Out-Null

Start-ScheduledTask -TaskName $TaskName

Write-Host "Started interactive Windows task '$TaskName' as $User"
Write-Host "Status file: $StatusFile"
