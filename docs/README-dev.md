# RT1064DVL6B Mac VS Code + Parallels Keil 工作流

本工作区基于逐飞官方 RT1064 开源库：

- 源仓库：https://gitee.com/seekfree/RT1064_Library
- 检出提交：`915dd7d2da4b37e041b565c8722e8a744245278c`
- 主 MDK 工程：`Projects/mdk/AI_Vision_RT1064.uvprojx`
- Keil 目标：`nor_sdram_zf_dtcm`
- 当前固件：AI 视觉竞赛工程脚手架，带 B9 心跳灯

## 路径

Keil 使用的 Windows 路径：

- 工作区：`C:\Users\jjp\Documents\Keil-work\AI_Vision_RT1064`
- Keil：`C:\Users\jjp\Documents\Keil`
- uVision：`C:\Users\jjp\Documents\Keil\UV4\UV4.exe`
- 构建封装脚本：`C:\Users\jjp\Documents\Keil-work\AI_Vision_RT1064\tools\keil-build.ps1`
- 当前工程：`C:\Users\jjp\Documents\Keil-work\AI_Vision_RT1064\Projects\mdk\AI_Vision_RT1064.uvprojx`

VS Code 使用的 macOS 路径：

- 工作区：`/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064`
- Keil 打开方式：Parallels `Keil uVision5.app`

## VS Code 任务

使用 `Terminal -> Run Task...`：

- `Keil: Open Project`：通过 Parallels 在 Windows Keil 中打开 `AI_Vision_RT1064.uvprojx`。
- `Keil: Build`：通过 Windows OpenSSH 构建当前 MDK 目标。
- `Keil: Rebuild`：通过 Windows OpenSSH 重新构建当前 MDK 目标。
- `Keil: Clean`：通过 Windows OpenSSH 清理当前 MDK 目标。
- `Keil: Flash/Debug`：通过交互式 Windows 计划任务，在已登录的 Windows 桌面会话中触发 Keil 下载。若需要源码级调试，请使用 `Keil: Open Project` 打开工程后，在 uVision 中启动调试会话。

`Keil: Open Project` 不需要 SSH。构建、重新构建和清理会通过 TCP 22 端口使用 Windows OpenSSH Server，因为当前 Parallels 版本不支持 `prlctl exec`。下载同样需要 SSH，但它会以 `Interactive` 登录方式启动 Windows 计划任务，使 Keil 的 CMSIS-DAP DLL 能够从真实的 Windows 桌面会话访问 USB 仿真器。

## 启用 Windows OpenSSH

在以管理员身份运行的 Windows PowerShell 中执行：

```powershell
cd C:\Users\jjp\Documents\Keil-work\AI_Vision_RT1064
powershell.exe -ExecutionPolicy Bypass -File .\tools\enable-openssh-windows.ps1
```

然后在 macOS 中测试：

```bash
./tools/test-windows-ssh.sh
```

如果 SSH 提示 `Permission denied`，请在 Windows PowerShell 中以用户 `jjp` 身份执行一次：

```powershell
cd C:\Users\jjp\Documents\Keil-work\AI_Vision_RT1064
powershell.exe -ExecutionPolicy Bypass -File .\tools\install-mac-ssh-key-windows.ps1
```

VS Code 任务使用专用的无密码密钥 `~/.ssh/keil_windows_ed25519`，因此构建可以非交互运行。

默认 Windows 虚拟机地址为 `10.211.55.3`，此虚拟机的 Windows 账户名看起来是 `Windows`，因此默认 SSH 目标是 `Windows@10.211.55.3`。如果虚拟机 IP 或用户名发生变化，请设置 `KEIL_SSH_HOST`，例如：`export KEIL_SSH_HOST=Windows@10.211.55.8`。

## DAPLink

逐飞 MDK 工程已配置 CMSIS-DAP：

- 调试监视器：`BIN\CMSIS_AGDI.dll`
- `.uvoptx` 中的仿真器名称：`SEEKFREE CMSIS-DAP V2`
- `.uvoptx` 中的仿真器序列号：`SF-3183007613204`
- Flash 算法：`MIMXRT1064_QSPI_4KB_SEC.FLM`

将逐飞 DAPLink 连接到 Windows 虚拟机，然后打开 Keil，并在使用 `Keil: Flash/Debug` 前确认 CMSIS-DAP 仿真器已显示。

如果 macOS 捕获了仿真器，请在 Parallels 中选择 `Devices -> USB & Bluetooth -> SEEKFREE CMSIS-DAP`，并将其分配给 `Windows 11`。Windows 应显示 `SEEKFREE CMSIS-DAP V2`。

已验证的下载日志可能会在 `Erase Done.Programming Done.Verify OK.Application running ...` 后以 `RDDI-DAP Error` 结束。在当前配置下，Keil 仍会返回成功，固件也已经运行；只要没有缺少 `Verify OK`，可将其视为下载后的断开连接警告。

## 首次运行

1. 运行 `Keil: Open Project`。
2. 在 Keil 中确认工程打开时没有缺少 Pack 的警告。
3. 将 DAPLink 连接到 Windows 虚拟机。
4. 配置 Windows OpenSSH 后，运行 `Keil: Build`。
5. 运行 `Keil: Flash/Debug`，如果 SSH 尚未准备好，也可以直接在 Keil 中下载。

当前工程入口为 `Projects/user/src/main.c`。应用任务从 `Projects/app/app_main.c` 开始，视觉模块位于 `Projects/module/vision/`。

## macOS 元数据文件

macOS 在 Windows 挂载盘、U 盘、网络盘上操作文件时，可能会生成 `._*`、`.DS_Store`、`.AppleDouble/` 这类元数据文件。本仓库已经在 `.gitignore` 和 VS Code 设置中隐藏它们；如果再次出现，可以在项目根目录执行：

```bash
find . -name '._*' -type f -delete
```

为了减少生成，尽量避免用 Finder 在 Windows 挂载盘上复制项目；在终端复制时可以临时加上 `COPYFILE_DISABLE=1`。也可以在 macOS 执行：

```bash
defaults write com.apple.desktopservices DSDontWriteUSBStores -bool true
defaults write com.apple.desktopservices DSDontWriteNetworkStores -bool true
```
