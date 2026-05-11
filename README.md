# AI Vision RT1064

本仓库包含 AI Vision RT1064 固件工程、用户驱动库、硬件文档和 Keil 辅助脚本。

## 目录结构

- `Projects/`：AI Vision 固件用户源码和 MDK 工程文件。
- `Projects/mdk/AI_Vision_RT1064.uvprojx`：Keil MDK 主工程。
- `Projects/user/`：项目自有固件代码，包括 `app/`、`bsp/`、`common/` 和 `rtos/`。
- `Libraries/official/seekfree/`：逐飞 RT1064 官方库、例程和参考工程。
- `Libraries/official/freertos/`：固件工程使用的 FreeRTOS 内核。
- `Libraries/user/`：项目自有设备和底盘驱动库。
- `docs/`：开发说明和硬件参考文档。
- `tools/`：Keil 构建、下载、Windows 集成脚本和第三方调试工具。

## 构建

```bash
./tools/keil-vscode.sh build
```

macOS、Parallels、Windows OpenSSH、Keil 和 DAPLink 的工作流说明见 `docs/README-dev.md`。
