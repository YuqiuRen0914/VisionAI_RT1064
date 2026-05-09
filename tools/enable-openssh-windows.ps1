$ErrorActionPreference = "Stop"

Write-Host "== Windows OpenSSH setup ==" -ForegroundColor Cyan

$Principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (!$Principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "Please run this script in Windows PowerShell as Administrator."
}

$Capability = Get-WindowsCapability -Online | Where-Object Name -like "OpenSSH.Server*"
Write-Host "Capability before: $($Capability.Name) $($Capability.State)"

if ($Capability.State -ne "Installed") {
    Add-WindowsCapability -Online -Name $Capability.Name
}

$Capability = Get-WindowsCapability -Online | Where-Object Name -like "OpenSSH.Server*"
Write-Host "Capability after:  $($Capability.Name) $($Capability.State)"

if ($Capability.State -ne "Installed") {
    throw "OpenSSH.Server is not installed yet. Wait for Windows capability installation to finish, then rerun this script."
}

Start-Service sshd
Set-Service -Name sshd -StartupType Automatic

$RuleNames = @("OpenSSH-Server-In-TCP", "sshd")
foreach ($RuleName in $RuleNames) {
    $Rule = Get-NetFirewallRule -Name $RuleName -ErrorAction SilentlyContinue
    if ($null -ne $Rule) {
        Enable-NetFirewallRule -Name $RuleName
    }
}

if ($null -eq (Get-NetFirewallRule -Name "OpenSSH-Server-In-TCP" -ErrorAction SilentlyContinue)) {
    New-NetFirewallRule -Name "OpenSSH-Server-In-TCP" -DisplayName "OpenSSH Server (sshd)" -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22 -Profile Any | Out-Null
} else {
    Set-NetFirewallRule -Name "OpenSSH-Server-In-TCP" -Enabled True -Direction Inbound -Action Allow -Profile Any
}

Get-Service sshd
Get-NetFirewallRule -Name "OpenSSH-Server-In-TCP" | Format-List Name, Enabled, Direction, Action, Profile

$Listener = Get-NetTCPConnection -LocalPort 22 -State Listen -ErrorAction SilentlyContinue
if ($null -eq $Listener) {
    throw "sshd service is not listening on TCP port 22."
}

$Listener
Write-Host "OpenSSH is ready. Now test from macOS: ./tools/test-windows-ssh.sh" -ForegroundColor Green
