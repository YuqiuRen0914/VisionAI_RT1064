param(
    [ValidateSet("build", "rebuild", "clean", "flash", "open")]
    [string]$Action = "build",

    [string]$StatusFile = ""
)

$ErrorActionPreference = "Stop"

$KeilRoot = "C:\Users\jjp\Documents\Keil"
$Uv4 = Join-Path $KeilRoot "UV4\UV4.exe"
$Workspace = "C:\Users\jjp\Documents\Keil-work"
$Project = Join-Path $Workspace "Projects\AI_Vision_RT1064\project\mdk\AI_Vision_RT1064.uvprojx"
$Target = "nor_sdram_zf_dtcm"
$LogDir = Join-Path (Split-Path $Project -Parent) "Objects"
$LogFile = Join-Path $LogDir "vscode-keil-$Action.log"

function Write-TaskStatus {
    param(
        [string]$State,
        [int]$ExitCode = 0,
        [string]$Message = ""
    )

    if ([string]::IsNullOrWhiteSpace($StatusFile)) {
        return
    }

    $StatusDir = Split-Path $StatusFile -Parent
    if (!(Test-Path $StatusDir)) {
        New-Item -ItemType Directory -Force -Path $StatusDir | Out-Null
    }

    @(
        "state=$State"
        "exit_code=$ExitCode"
        "message=$Message"
        "log_file=$LogFile"
        "timestamp=$(Get-Date -Format o)"
    ) | Set-Content -Path $StatusFile -Encoding UTF8
}

Write-TaskStatus -State "running" -Message "Keil $Action started"

if (!(Test-Path $Uv4)) {
    Write-TaskStatus -State "failed" -ExitCode 2 -Message "Keil uVision was not found at $Uv4"
    throw "Keil uVision was not found at $Uv4"
}

if (!(Test-Path $Project)) {
    Write-TaskStatus -State "failed" -ExitCode 3 -Message "Keil project was not found at $Project"
    throw "Keil project was not found at $Project"
}

if ($Action -eq "open") {
    Start-Process -FilePath $Uv4 -ArgumentList "`"$Project`""
    Write-TaskStatus -State "done" -ExitCode 0 -Message "Keil project opened"
    exit 0
}

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

$CommandSwitch = switch ($Action) {
    "build"   { "-b" }
    "rebuild" { "-r" }
    "clean"   { "-c" }
    "flash"   { "-f" }
}

$ArgumentList = @(
    $CommandSwitch,
    "`"$Project`"",
    "-t",
    "`"$Target`"",
    "-j0",
    "-o",
    "`"$LogFile`""
)

Write-Host "Keil: $Action"
Write-Host "Project: $Project"
Write-Host "Target: $Target"

$ExitCode = 1

try {
    $Process = Start-Process -FilePath $Uv4 -ArgumentList $ArgumentList -Wait -PassThru
    $ExitCode = $Process.ExitCode

    if (Test-Path $LogFile) {
        Get-Content $LogFile
    }

    if ($ExitCode -eq 0) {
        Write-TaskStatus -State "done" -ExitCode $ExitCode -Message "Keil $Action finished"
    } else {
        Write-TaskStatus -State "failed" -ExitCode $ExitCode -Message "Keil $Action failed"
    }
} catch {
    Write-TaskStatus -State "failed" -ExitCode $ExitCode -Message $_.Exception.Message
    throw
}

exit $ExitCode
