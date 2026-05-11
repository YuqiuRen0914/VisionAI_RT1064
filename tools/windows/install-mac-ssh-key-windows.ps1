$ErrorActionPreference = "Stop"

$AuthorizedKeysDir = Join-Path $env:USERPROFILE ".ssh"
$AuthorizedKeys = Join-Path $AuthorizedKeysDir "authorized_keys"
$MacPublicKeys = @(
    'ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAILRwD3ISOPJuaLNtWBsaISup0CrHhTaRoG9FOBny1EWD keil-vscode-windows@10.211.55.3'
)
$Identity = [System.Security.Principal.WindowsIdentity]::GetCurrent()
$Principal = New-Object Security.Principal.WindowsPrincipal($Identity)
$CurrentUser = $Identity.Name
$CurrentUserSid = $Identity.User.Value
$IsAdmin = $Principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

function Add-PublicKey {
    param(
        [string]$Path,
        [string[]]$Keys
    )

    $Dir = Split-Path $Path -Parent
    New-Item -ItemType Directory -Force -Path $Dir | Out-Null

    if (!(Test-Path $Path)) {
        New-Item -ItemType File -Force -Path $Path | Out-Null
    }

    $ValidExisting = @()
    if (Test-Path $Path) {
        $ValidExisting = Get-Content -Path $Path -ErrorAction SilentlyContinue |
            Where-Object { $_ -match '^(ssh-ed25519|ssh-rsa|ecdsa-sha2-nistp[0-9]+) [A-Za-z0-9+/=]+( .*)?$' }
    }

    $Merged = @($ValidExisting + $Keys) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique
    Set-Content -Path $Path -Value $Merged -Encoding ascii
}

Write-Host "Windows account: $CurrentUser"
Write-Host "USERNAME:        $env:USERNAME"
Write-Host "USERPROFILE:     $env:USERPROFILE"
Write-Host "Elevated admin:  $IsAdmin"

Add-PublicKey -Path $AuthorizedKeys -Keys $MacPublicKeys

icacls $AuthorizedKeysDir /inheritance:r | Out-Null
icacls $AuthorizedKeysDir /grant:r "*$($CurrentUserSid):(OI)(CI)F" | Out-Null
icacls $AuthorizedKeys /inheritance:r | Out-Null
icacls $AuthorizedKeys /grant:r "*$($CurrentUserSid):F" | Out-Null

if ($IsAdmin) {
    $AdminAuthorizedKeys = "C:\ProgramData\ssh\administrators_authorized_keys"
    Add-PublicKey -Path $AdminAuthorizedKeys -Keys $MacPublicKeys

    icacls $AdminAuthorizedKeys /inheritance:r | Out-Null
    icacls $AdminAuthorizedKeys /grant:r "*S-1-5-32-544:F" "*S-1-5-18:F" | Out-Null
    Write-Host "Installed admin authorized_keys: $AdminAuthorizedKeys"
} else {
    Write-Host "Not elevated. If this Windows account is in Administrators, rerun this script in Administrator PowerShell."
}

Write-Host "Installed user authorized_keys:  $AuthorizedKeys"
Write-Host "Try from macOS: ssh $env:USERNAME@10.211.55.3"
