#!/usr/bin/env bash
set -euo pipefail

HOST="${KEIL_SSH_HOST:-Windows@10.211.55.3}"
IP="${HOST#*@}"

echo "Testing TCP port 22 on ${IP}..."
nc -vz -G 4 "${IP}" 22

echo "Testing SSH login for ${HOST}..."
ssh -i "${HOME}/.ssh/keil_windows_ed25519" -o IdentitiesOnly=yes -o PubkeyAuthentication=unbound -o ConnectTimeout=8 "${HOST}" "powershell.exe -NoProfile -Command \"Write-Host Windows SSH OK; Get-Command 'C:\\Users\\jjp\\Documents\\Keil\\UV4\\UV4.exe' | Select-Object -ExpandProperty Source\""
