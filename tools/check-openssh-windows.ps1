$ErrorActionPreference = "Continue"

Write-Host "== OpenSSH Capability =="
Get-WindowsCapability -Online | Where-Object Name -like "OpenSSH.Server*" | Format-List Name, State

Write-Host "== sshd Service =="
Get-Service sshd -ErrorAction SilentlyContinue | Format-List Name, Status, StartType

Write-Host "== Port 22 Listener =="
Get-NetTCPConnection -LocalPort 22 -State Listen -ErrorAction SilentlyContinue

Write-Host "== Firewall Rule =="
Get-NetFirewallRule -Name "OpenSSH-Server-In-TCP" -ErrorAction SilentlyContinue | Format-List Name, Enabled, Direction, Action, Profile

Write-Host "== Network Profile =="
Get-NetConnectionProfile | Format-Table Name, InterfaceAlias, NetworkCategory

Write-Host "== Windows IP =="
Get-NetIPAddress -AddressFamily IPv4 | Where-Object IPAddress -like "10.211.*" | Format-Table IPAddress, InterfaceAlias
