# Codex Skills 使用指南

本文档介绍当前 Codex 环境中从 `mattpocock/skills` 安装的 skills，以及在本项目中如何调用它们。

这些 skills 安装在本机 Codex 配置目录中，不是固件源码的一部分。第一次安装或更新后，需要重启 Codex 才能被当前会话自动发现。

## 调用方式

在对话中直接写 skill 名称，或用自然语言描述对应任务即可。示例：

```text
使用 diagnose 帮我排查 UART1 没有回包的问题
```

```text
用 tdd 的方式给串口命令解析补测试
```

```text
我不熟悉 Projects/user/app/module/interface/comm.c，zoom-out 给我讲一下它在系统中的位置
```

如果一个 skill 需要项目信息，例如 issue tracker、标签、领域文档位置，先运行 `setup-matt-pocock-skills`。对于当前嵌入式固件项目，很多 issue/PRD 类 skill 可以先配置为本地 Markdown issue 流程，而不一定依赖 GitHub Issues。

## 推荐先配置

### setup-matt-pocock-skills

用途：为本仓库写入 agent skills 的项目级配置，让其他工程类 skills 知道 issue 放在哪里、triage 标签怎么映射、领域文档和 ADR 放在哪里。

适合场景：

- 第一次在本仓库使用 `to-issues`、`to-prd`、`triage`、`diagnose`、`tdd`、`improve-codebase-architecture`、`zoom-out` 前。
- 发现 Codex 不知道项目 issue 工作流、`CONTEXT.md` 或 ADR 位置时。

常用提示：

```text
使用 setup-matt-pocock-skills 为这个项目配置本地 Markdown issue 流程和领域文档位置
```

注意：它会先检查仓库现状，再逐项询问 issue tracker、triage 标签和领域文档布局。

## 日常固件开发常用

### diagnose

用途：按“复现、最小化、假设、插桩、修复、回归测试”的流程排查 bug 或性能退化。

适合场景：

- 串口无响应、命令解析异常、电机输出不符合预期。
- Keil 构建、下载、运行日志出现错误。
- 某个模块偶发失败，需要建立可重复的反馈循环。

常用提示：

```text
使用 diagnose 排查 stream enc100 输出一直为 0 的问题
```

```text
diagnose 这个 Keil build 报错，先帮我找出最小复现和可能根因
```

### tdd

用途：用红绿重构循环实现功能或修复 bug。它强调从公开接口验证行为，而不是测试私有实现细节。

适合场景：

- 给可在主机侧编译的纯逻辑模块补测试，例如命令解析、协议编码、运动解算、状态机。
- 修复 bug 时先写一个失败测试，再做最小实现。

常用提示：

```text
使用 tdd 给串口命令解析增加 motor 命令的边界测试
```

```text
用 tdd 修复 move left 占空比符号错误
```

注意：Keil 固件并不总是适合完整主机测试。优先把可测试逻辑抽成不依赖硬件寄存器的模块，再让 skill 建立快速反馈循环。

### zoom-out

用途：让 Codex 从更高层解释一段代码，画出相关模块、调用方、数据流和职责。

适合场景：

- 接手不熟悉的文件或模块。
- 修改前需要理解 `Projects/user/`、`Libraries/user/`、BSP、驱动层之间的关系。

常用提示：

```text
zoom-out 解释 Projects/user/app/module/interface/comm.c 以及它和 app_main 的关系
```

```text
我不熟悉 drive 模块，使用 zoom-out 给我一张调用关系图
```

### grill-with-docs

用途：围绕一个计划持续追问，把设计决策、项目术语和文档一起整理清楚。它会结合 `CONTEXT.md` 和 ADR。

适合场景：

- 做较大的固件结构调整前，例如重构通信协议、调整底盘驱动接口。
- 需要把设计结论写进项目文档或 ADR。

常用提示：

```text
使用 grill-with-docs 审问我这个串口协议重构方案，并把确定的术语写进文档
```

### grill-me

用途：只做计划拷问，不主动更新项目文档。它会一题一题追问，直到方案关键分支清楚。

适合场景：

- 还没确定方案，只想先暴露遗漏条件。
- 硬件接线、协议格式、控制策略等需要先理清约束。

常用提示：

```text
grill-me 我准备增加一个 IMU 校准流程，帮我找出遗漏的决策
```

### improve-codebase-architecture

用途：寻找架构摩擦点，提出让模块更“深”的重构机会，让接口更稳定、实现更集中、测试更容易。

适合场景：

- 一个功能需要在很多文件来回跳才能理解。
- 驱动、BSP、应用层职责不清。
- 想降低后续修改成本，而不是只修眼前 bug。

常用提示：

```text
使用 improve-codebase-architecture 检查通信模块和底盘驱动之间有没有可以加深的接口
```

### prototype

用途：做一次性原型，用最小成本验证状态机、数据模型、协议交互或 UI 想法。

适合场景：

- 先在主机上验证命令解析、运动控制状态机、串口交互流程。
- 做一个临时脚本或终端小工具，让人可以快速“玩一下”设计。

常用提示：

```text
使用 prototype 做一个终端原型，模拟 open_loop_test 的串口命令状态机
```

注意：原型代码默认应是可删除的临时代码。验证完成后，要么删除，要么把结论吸收进正式实现和文档。

### handoff

用途：把当前对话压缩成交接文档，方便新的 Codex 会话继续。

适合场景：

- 一次调试或重构做了一半，需要换会话。
- 需要记录当前结论、已改文件、剩余风险和建议继续使用的 skill。

常用提示：

```text
使用 handoff，为下一次会话整理串口自检和电机调试的当前进展
```

### caveman

用途：进入极简沟通模式，减少解释性文字，保留技术信息。

适合场景：

- 你只想快速看命令、结论和下一步。
- 长时间调试时想减少输出干扰。

常用提示：

```text
caveman mode，后续回答尽量短
```

退出：

```text
stop caveman，恢复正常说明
```

## 任务拆分和 issue 工作流

### to-prd

用途：把当前对话和代码理解整理成 PRD，并发布到项目 issue tracker。

适合场景：

- 已经聊清楚一个功能目标，需要形成需求文档。
- 想把“我要做什么、为什么、验收标准是什么”固定下来。

常用提示：

```text
使用 to-prd 把当前关于串口调试协议扩展的讨论整理成 PRD
```

注意：它不会重新采访你，而是基于当前上下文综合整理。上下文不足时，先用 `grill-me` 或 `grill-with-docs`。

### to-issues

用途：把计划、PRD 或规格拆成多个可独立完成的 issues，强调端到端的垂直切片。

适合场景：

- 大功能需要分阶段做，且每一阶段都能独立验证。
- 想把“通信协议扩展”“底盘闭环控制”“IMU 校准”等拆成小任务。

常用提示：

```text
使用 to-issues 把这个 open_loop_test 增强计划拆成可独立实现的 issues
```

### triage

用途：按固定状态机管理 issues：`needs-triage`、`needs-info`、`ready-for-agent`、`ready-for-human`、`wontfix`，同时区分 `bug` 和 `enhancement`。

适合场景：

- 收到多个 bug/需求，需要判断是否信息足够、是否能交给 agent 做。
- 想整理本地 Markdown issues 或 GitHub Issues。

常用提示：

```text
使用 triage 检查当前 issue 列表，把能交给 agent 的标为 ready-for-agent
```

## 创建和管理 skills

### write-a-skill

用途：创建新的 agent skill，包含 `SKILL.md`、可选参考文档、示例和脚本。

适合场景：

- 想为本项目固化专用工作流，例如“RT1064 Keil 构建排错”“串口硬件自检”“逐飞库迁移”。
- 某个流程你会反复让 Codex 执行。

常用提示：

```text
使用 write-a-skill，为这个项目写一个 RT1064 Keil build diagnose skill
```

## 当前项目较少使用的 skills

以下 skills 已安装，但主要面向 TypeScript、Web 或课程内容项目。当前 RT1064 固件仓库通常不需要，除非你明确引入对应工具链。

### setup-pre-commit

用途：为 Node 项目配置 Husky、lint-staged、Prettier、typecheck 和 test 的 pre-commit hook。

适合场景：

- 仓库引入了 `package.json`，并希望提交前自动格式化、类型检查和测试。

常用提示：

```text
使用 setup-pre-commit 为这个仓库的前端工具链配置提交前检查
```

注意：当前固件项目没有典型 Node 工具链，不建议直接使用。

### migrate-to-shoehorn

用途：把 TypeScript 测试中的 `as` 类型断言迁移到 `@total-typescript/shoehorn`。

适合场景：

- TypeScript 测试文件里需要构造部分对象，并希望减少危险的 `as`。

常用提示：

```text
使用 migrate-to-shoehorn 迁移 tests 目录里的 TypeScript 测试数据构造
```

注意：当前 C 固件项目通常用不到。

### scaffold-exercises

用途：创建课程练习目录，包括 problem、solution、explainer 等结构。

适合场景：

- 仓库用于课程内容或练习系统，而不是固件产品代码。

常用提示：

```text
使用 scaffold-exercises 创建一个新的课程练习章节
```

注意：当前仓库不是课程仓库，一般不使用。

### git-guardrails-claude-code

用途：为 Claude Code 配置 hook，阻止危险 git 命令，例如 `git push`、`git reset --hard`、`git clean -f`、`git branch -D`。

适合场景：

- 你同时使用 Claude Code，并想在项目级或全局阻止高风险 git 操作。

常用提示：

```text
使用 git-guardrails-claude-code 为这个项目安装危险 git 命令拦截
```

注意：这是 Claude Code hook，不是 Codex 内置安全机制。Codex 本身仍遵守当前会话的权限和协作规则。

## 建议使用顺序

新功能或较大改动：

1. `grill-me` 或 `grill-with-docs` 先把方案问清楚。
2. `to-prd` 把需求固定下来。
3. `to-issues` 拆成可独立验证的小任务。
4. `tdd` 或普通实现流程完成每个任务。
5. `diagnose` 处理失败、硬件异常或测试不稳定。
6. `handoff` 在需要换会话时交接。

排查固件问题：

1. `zoom-out` 先理解相关模块和调用关系。
2. `diagnose` 建立可重复反馈循环。
3. `tdd` 为可主机测试的纯逻辑补回归测试。
4. `grill-with-docs` 把稳定下来的术语和决策写进文档。

