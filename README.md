# AI Vision RT1064

This repository contains the AI Vision RT1064 firmware project, shared libraries, hardware documents, and Keil helper scripts.

## Layout

- `Projects/`: AI Vision firmware project sources and MDK project files.
- `Projects/mdk/AI_Vision_RT1064.uvprojx`: Keil MDK project.
- `Projects/app/`, `Projects/bsp/`, `Projects/module/`: application, board support, and feature modules.
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
