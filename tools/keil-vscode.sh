#!/usr/bin/env bash
set -euo pipefail

ACTION="${1:-build}"
WORKSPACE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAC_PROJECT="${WORKSPACE_DIR}/Projects/AI_Vision_RT1064/project/mdk/AI_Vision_RT1064.uvprojx"
WINDOWS_PROJECT='C:\Users\jjp\Documents\Keil-work\Projects\AI_Vision_RT1064\project\mdk\AI_Vision_RT1064.uvprojx'
KEIL_SSH_HOST="${KEIL_SSH_HOST:-Windows@10.211.55.3}"
KEIL_TRANSPORT="${KEIL_TRANSPORT:-ssh}"
REMOTE_SCRIPT='C:\Users\jjp\Documents\Keil-work\tools\keil-build.ps1'
REMOTE_INTERACTIVE_SCRIPT='C:\Users\jjp\Documents\Keil-work\tools\run-keil-interactive.ps1'
WINDOWS_WORKSPACE='C:\Users\jjp\Documents\Keil-work'

if [[ -z "${KEIL_MAC_APP:-}" ]]; then
  KEIL_MAC_APP='/Users/roderick/Applications (Parallels)/{d013f16b-1e31-403d-90a7-037c7ed44306} Applications.localized/Keil uVision5.app'
fi

if [[ -z "${POWERSHELL_MAC_APP:-}" ]]; then
  POWERSHELL_MAC_APP='/Users/roderick/Applications (Parallels)/{d013f16b-1e31-403d-90a7-037c7ed44306} Applications.localized/Windows PowerShell.app'
fi

open_project() {
  if [[ -d "${KEIL_MAC_APP}" ]]; then
    open -n "${KEIL_MAC_APP}" --args "${WINDOWS_PROJECT}"
  else
    open "${MAC_PROJECT}"
  fi
}

run_remote() {
  if ! command -v ssh >/dev/null 2>&1; then
    echo "ssh is not available on this Mac. Use 'Keil: Open Project' and build in uVision." >&2
    exit 127
  fi

  echo "Running Keil ${ACTION} on Windows host '${KEIL_SSH_HOST}'..."
  echo "If this fails, configure ~/.ssh/config with a Host named '${KEIL_SSH_HOST}', or set KEIL_SSH_HOST."
  ssh -i "${HOME}/.ssh/keil_windows_ed25519" -o IdentitiesOnly=yes -o PubkeyAuthentication=unbound -o ConnectTimeout=8 "${KEIL_SSH_HOST}" "powershell.exe -NoProfile -ExecutionPolicy Bypass -File ${REMOTE_SCRIPT} ${ACTION}"
}

status_value() {
  local file="$1"
  local key="$2"
  LC_ALL=C awk -F= -v key="${key}" '
    NR == 1 { sub(/^\xef\xbb\xbf/, "", $1) }
    $1 == key {
      if (NF > 1) {
        value = substr($0, index($0, $2))
        sub(/\r$/, "", value)
        print value
      }
      exit
    }
  ' "${file}"
}

run_remote_interactive() {
  if ! command -v ssh >/dev/null 2>&1; then
    echo "ssh is not available on this Mac. Use 'Keil: Open Project' and flash in uVision." >&2
    exit 127
  fi

  local task_dir="${WORKSPACE_DIR}/.keil-task"
  local status_file="${task_dir}/${ACTION}.status"
  mkdir -p "${task_dir}"
  rm -f "${status_file}"

  echo "Starting Keil ${ACTION} in the logged-in Windows desktop session..."
  ssh -i "${HOME}/.ssh/keil_windows_ed25519" \
    -o IdentitiesOnly=yes \
    -o PubkeyAuthentication=unbound \
    -o ConnectTimeout=8 \
    "${KEIL_SSH_HOST}" \
    "powershell.exe -NoProfile -ExecutionPolicy Bypass -File ${REMOTE_INTERACTIVE_SCRIPT} ${ACTION}"

  local waited=0
  while [[ ${waited} -lt 1800 ]]; do
    if [[ -f "${status_file}" ]]; then
      local state exit_code log_file message
      state="$(status_value "${status_file}" state)"
      exit_code="$(status_value "${status_file}" exit_code)"
      message="$(status_value "${status_file}" message)"
      log_file="$(status_value "${status_file}" log_file)"

      if [[ "${state}" == "done" || "${state}" == "failed" ]]; then
        echo "${message}"
        if [[ -n "${log_file}" ]]; then
          local mac_log
          mac_log="$(win_path_to_mac "${log_file}")"
          if [[ -f "${mac_log}" ]]; then
            cat "${mac_log}"
          else
            echo "Keil log: ${log_file}"
          fi
        fi
        exit "${exit_code:-1}"
      fi
    fi

    sleep 1
    waited=$((waited + 1))
  done

  echo "Timed out waiting for the interactive Windows Keil task. Check the Windows desktop for Keil or PowerShell prompts." >&2
  exit 124
}

win_path_to_mac() {
  local win_path="$1"
  local suffix="${win_path#C:\\Users\\jjp\\Documents\\}"
  suffix="${suffix//\\//}"
  printf '/Volumes/[C] Windows 11/Users/jjp/Documents/%s\n' "${suffix}"
}

run_parallels_powershell() {
  if [[ ! -d "${POWERSHELL_MAC_APP}" ]]; then
    echo "Windows PowerShell.app was not found. Use KEIL_TRANSPORT=ssh after configuring Windows OpenSSH." >&2
    exit 127
  fi

  local task_dir="${WORKSPACE_DIR}/.keil-task"
  local status_file="${task_dir}/${ACTION}.status"
  local win_status_file="${WINDOWS_WORKSPACE}\\.keil-task\\${ACTION}.status"
  mkdir -p "${task_dir}"
  rm -f "${status_file}"

  echo "Starting Keil ${ACTION} through Parallels Windows PowerShell..."
  open -n "${POWERSHELL_MAC_APP}" --args -NoProfile -ExecutionPolicy Bypass -File "${REMOTE_SCRIPT}" "${ACTION}" -StatusFile "${win_status_file}"

  local waited=0
  while [[ ${waited} -lt 1800 ]]; do
    if [[ -f "${status_file}" ]]; then
      local state exit_code log_file message
      state="$(status_value "${status_file}" state)"
      exit_code="$(status_value "${status_file}" exit_code)"
      message="$(status_value "${status_file}" message)"
      log_file="$(status_value "${status_file}" log_file)"

      if [[ "${state}" == "done" || "${state}" == "failed" ]]; then
        echo "${message}"
        if [[ -n "${log_file}" ]]; then
          local mac_log
          mac_log="$(win_path_to_mac "${log_file}")"
          if [[ -f "${mac_log}" ]]; then
            cat "${mac_log}"
          else
            echo "Keil log: ${log_file}"
          fi
        fi
        exit "${exit_code:-1}"
      fi
    fi

    sleep 1
    waited=$((waited + 1))
  done

  echo "Timed out waiting for Windows PowerShell/Keil to finish. Check Keil or PowerShell in the Windows VM." >&2
  exit 124
}

case "${ACTION}" in
  open)
    open_project
    ;;
  build|rebuild|clean|flash)
    if [[ "${ACTION}" == "flash" && "${KEIL_TRANSPORT}" == "ssh" ]]; then
      run_remote_interactive
    elif [[ "${KEIL_TRANSPORT}" == "ssh" ]]; then
      run_remote
    else
      run_parallels_powershell
    fi
    ;;
  *)
    echo "Usage: $0 {build|rebuild|clean|flash|open}" >&2
    exit 2
    ;;
esac
