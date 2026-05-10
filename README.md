# Keil RT1064 Workspace

This workspace contains the AI Vision RT1064 firmware project, shared libraries, hardware documents, and Keil helper scripts.

## Layout

- `Projects/AI_Vision_RT1064/`: AI Vision firmware project.
- `Libraries/official/seekfree/`: SeekFree RT1064 official library, examples, and reference project.
- `Libraries/official/freertos/`: FreeRTOS kernel used by the firmware project.
- `Libraries/user/`: project-owned device and board libraries.
- `docs/`: development notes and hardware reference documents.
- `tools/`: Keil build, flash, and Windows integration scripts.

## Build

```bash
./tools/keil-vscode.sh build
```

See `docs/README-dev.md` for the macOS, Parallels, Windows OpenSSH, Keil, and DAPLink workflow.
