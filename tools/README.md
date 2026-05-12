# 工具目录说明

`tools` 目录保存本项目开发、构建、下载和调试过程中使用的辅助工具。

## 目录结构

- `keil/`：Keil MDK 构建、清理、下载和交互式任务脚本。
- `bench/`：硬件台架调试脚本，例如轮速闭环串口自动测试。
- `windows/`：Windows OpenSSH 配置和连通性检查脚本。
- `vendor/`：第三方工具源码或二进制工具。
- `vendor/seekfree_assistant/`：逐飞助手，后续用于串口调试和数据测试。

## 根目录入口

`tools` 根目录只保留通用说明和 VS Code/终端常用入口：

- `keil-vscode.sh`
- `README.md`

常用构建命令保持不变：

```bash
./tools/keil-vscode.sh build
```

如果需要直接执行底层脚本，请使用子目录中的真实路径，例如：

```bash
./tools/windows/test-windows-ssh.sh
```

```bash
./tools/bench/speed-loop-bench.py --plan-only
```

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\tools\windows\enable-openssh-windows.ps1
```
