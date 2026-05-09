# AI Vision RT1064

RT1064DVL6B project for the Artificial Intelligence Vision Group competition.

## Layout

- `project/mdk/AI_Vision_RT1064.uvprojx`: Keil MDK project
- `project/user/src/main.c`: board startup and application loop
- `project/code/vision_app.c`: vision application entry points
- `libraries/`: local copy of the SeekFree RT1064 support libraries

## Build And Flash

From the repository root:

```bash
./tools/keil-vscode.sh build
./tools/keil-vscode.sh flash
```
