# RT1064DVL6B Mac VS Code + Parallels Keil Workflow

This workspace is based on the official SeekFree RT1064 library:

- Source: https://gitee.com/seekfree/RT1064_Library
- Checked out commit: `915dd7d2da4b37e041b565c8722e8a744245278c`
- Main MDK project: `Example/Coreboard_Demo/E01_gpio_demo/mdk/rt1064.uvprojx`
- Keil target: `nor_sdram_zf_dtcm`
- First firmware: GPIO blink demo, toggling B9 in `Example/Coreboard_Demo/E01_gpio_demo/user/src/main.c`

## Paths

Windows paths used by Keil:

- Workspace: `C:\Users\jjp\Documents\Keil-work`
- Keil: `C:\Users\jjp\Documents\Keil`
- uVision: `C:\Users\jjp\Documents\Keil\UV4\UV4.exe`
- Build wrapper: `C:\Users\jjp\Documents\Keil-work\tools\keil-build.ps1`
- Active project: `C:\Users\jjp\Documents\Keil-work\Example\Coreboard_Demo\E01_gpio_demo\mdk\rt1064.uvprojx`

macOS paths used by VS Code:

- Workspace: `/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work`
- Keil opener: Parallels `Keil uVision5.app`

## VS Code Tasks

Use `Terminal -> Run Task...`:

- `Keil: Open Project`: opens `rt1064.uvprojx` in Windows Keil through Parallels.
- `Keil: Build`: builds the current MDK target through Windows OpenSSH.
- `Keil: Rebuild`: rebuilds the current MDK target through Windows OpenSSH.
- `Keil: Clean`: cleans the current MDK target through Windows OpenSSH.
- `Keil: Flash/Debug`: triggers Keil flash download in the logged-in Windows desktop session through an interactive Windows scheduled task. For source-level debugging, use `Keil: Open Project` and start a uVision debug session.

`Keil: Open Project` works without SSH. Build, rebuild, and clean use Windows OpenSSH Server over TCP port 22 because this Parallels edition does not support `prlctl exec`. Flash also needs SSH, but it starts a Windows scheduled task with `Interactive` logon so Keil's CMSIS-DAP DLL can access the USB probe from the real Windows desktop session.

## Enable Windows OpenSSH

In Windows PowerShell running as Administrator:

```powershell
cd C:\Users\jjp\Documents\Keil-work
powershell.exe -ExecutionPolicy Bypass -File .\tools\enable-openssh-windows.ps1
```

Then test from macOS:

```bash
./tools/test-windows-ssh.sh
```

If SSH says `Permission denied`, run this once in Windows PowerShell as user `jjp`:

```powershell
cd C:\Users\jjp\Documents\Keil-work
powershell.exe -ExecutionPolicy Bypass -File .\tools\install-mac-ssh-key-windows.ps1
```

The VS Code tasks use a dedicated no-passphrase key at `~/.ssh/keil_windows_ed25519` so builds can run non-interactively.

The default Windows VM address is `10.211.55.3`, and this VM's Windows account name appears to be `Windows`, so the default SSH target is `Windows@10.211.55.3`. If the VM IP or username changes, set `KEIL_SSH_HOST`, for example `export KEIL_SSH_HOST=Windows@10.211.55.8`.

## DAPLink

The SeekFree MDK project is already configured for CMSIS-DAP:

- Debug monitor: `BIN\CMSIS_AGDI.dll`
- Probe name in `.uvoptx`: `SEEKFREE CMSIS-DAP V2`
- Probe serial in `.uvoptx`: `SF-3183007613204`
- Flash algorithm: `MIMXRT1064_QSPI_4KB_SEC.FLM`

Connect the SeekFree DAPLink to the Windows VM, then open Keil and confirm the CMSIS-DAP probe appears before using `Keil: Flash`.

If macOS captures the probe, use Parallels `Devices -> USB & Bluetooth -> SEEKFREE CMSIS-DAP` and assign it to `Windows 11`. Windows should show `SEEKFREE CMSIS-DAP V2`.

The verified flash log may end with `RDDI-DAP Error` after `Erase Done.Programming Done.Verify OK.Application running ...`. In this setup Keil still returns success and the firmware is already running; treat it as a post-download disconnect warning unless `Verify OK` is missing.

## First Run

1. Run `Keil: Open Project`.
2. In Keil, confirm the project opens without missing Pack warnings.
3. Connect DAPLink to the Windows VM.
4. Run `Keil: Build` after Windows OpenSSH is configured.
5. Run `Keil: Flash/Debug`, or flash directly from Keil if SSH is not ready yet.

The current `main.c` is the GPIO/Blink example at `Example/Coreboard_Demo/E01_gpio_demo/user/src/main.c`.
