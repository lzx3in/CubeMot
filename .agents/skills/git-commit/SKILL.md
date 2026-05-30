---
name: git-commit
description: >
  分析 git 变更，对相关文件分组，生成 Conventional Commits 格式的提交信息并自动执行提交。
  当用户要求提交代码、创建 git commit、暂存文件或提到 "/commit" 时使用。
  确保 pre-commit hook 已安装，并强制执行原子提交规范。
---

# Git Commit

## 概述

按 Conventional Commits 规范创建标准化、语义化的 git 提交。分析实际 diff，将相关变更分组，生成提交信息，并自动执行。

## 工作流程

### 1. 收集状态

```bash
git status --porcelain
git diff --staged          # 已有暂存内容时
git diff                   # 无暂存内容时
```

若无暂存内容且用户未指定文件，则不暂存任何内容——按用户指令操作。

### 2. 分组变更

将变更拆分为**逻辑分组**，每个分组独立构成一个提交。CubeMot 项目按以下层级分组：

| 层级 | 路径 | scope 示例 |
|------|------|-----------|
| 应用层 | `src/app/` | `app` |
| 模块层 | `src/modules/` + `zephyr/src/modules/` | `modules/blink`, `modules/led_controller` |
| 消息总线 | `src/msghub/` | `msghub` |
| 驱动层 | `src/drivers/` + `zephyr/src/` | `drivers/led`, `drivers/button` |
| 板级 | `src/boards/` | `boards/nucleo_g431rb` |
| 芯片层 | `src/chips/` | `chips/stm32g4` |
| 库 | `src/libs/` | `libs/pid`, `libs/crc` |
| 公共 | `src/common/` | `common` |
| Zephyr 构建 | `zephyr/` | `zephyr` |
| 构建系统 | `cmake/`, `CMakeLists.txt` | `build` |
| 测试 | `tests/` | `tests` |
| 工具 | `tools/` | `tools` |

规则：
- **分离** FreeRTOS 与 Zephyr 变更
- **分离**格式化变更与行为变更
- **分离**重构与新功能
- **分离**文档与代码变更
- 每个分组应可独立审查、独立回滚

### 3. 生成提交信息

参考 [commit-types.md](references/commit-types.md) 获取完整的类型目录及各类型格式规则。

为每个分组确定：
- **type** — 从提交类型目录中选择
- **scope** — 受影响的模块/区域
- **description** — 单行摘要，祈使语气，现在时，≤72 字符
- **body** — 解释*为什么*，而非*做了什么*
- **footer** — `BREAKING CHANGE:`、`Closes #N`、`Refs #N` 等（按需）

### 4. 展示计划

向用户简要展示即将提交的内容：

```
提交内容：
  1. refactor(chips/stm32g4): 移除 CMakeLists 中未使用的编译定义
     文件：src/chips/stm32g4/CMakeLists.txt
  2. feat(modules): 添加基于 msghub 的电机控制模块
     文件：src/modules/motor/motor.c, src/topics/topics.c
```

### 5. 执行

无需询问确认，按顺序执行每次提交：

```bash
git add <文件...>
git commit -m "<提交信息>"
```

多行提交信息（含 body/footer）：

```bash
git add <文件...>
git commit -m "<主题>

<body>

<footer>"
```

## Pre-commit Hook 安装

本 skill 将质量检查委托给 git hooks。**提交前不要手动运行 lint/test。**

项目已有 pre-commit hook（clang-format C/C++、ruff Python、shfmt Shell）。git-commit skill 的 hook 合并了这些检查并新增：

- **密钥检测**：扫描暂存文件中的私钥、token、凭证等
- **大文件检查**：警告 >1MB 的文件
- **提交信息格式**：软检查 Conventional Commits 格式

安装合并后的 hook（覆盖现有）：

```bash
cp .agents/skills/git-commit/scripts/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

## 安全协议

- **绝不**修改 git config
- **绝不**在未经明确要求时执行破坏性命令（`--force`、hard reset）
- **绝不**跳过 hooks（`--no-verify`），除非用户明确要求
- **绝不** force push 到 `main` / `master`
- **绝不**提交密钥（`.env`、凭证、私钥）
- 若提交因 hook 失败，修复问题后创建**新**提交（不要 amend）

## 提交规范

### 原子提交

每次提交只做**一件逻辑上的事**。若无法用一句话描述，就拆开。

```
✅ 好：refactor(chips): 更新 GPIO 头文件 include 路径
❌ 差：refactor: 重构 HAL 架构、移除测试目录、重命名 hal 为 chips
```

### 关注点分离

| 变更类型 | 应与其分离 |
|---|---|
| 重构 | 新功能 |
| 格式化/代码风格 | 行为变更 |
| 文档 | 代码变更 |
| Zephyr 迁移 | FreeRTOS 变更 |
| 测试新增 | 实现变更 |

### 变更规模

| 规模 | 建议 |
|---|---|
| ~100 行 | 理想——易于审查和回滚 |
| ~300 行 | 一个逻辑变更可接受 |
| ~1000 行 | 必须拆分为更小的提交 |

### 提交信息规则

- **祈使语气**："添加"而非"添加了"，"修复"而非"修复了"
- **现在时**："更新配置"而非"更新了配置"
- **摘要行不超过 72 字符**
- **引用 issue**：footer 中使用 `Closes #123`、`Refs #456`
- **摘要行末尾不加句号**
- **英文信息**：type、scope、description 使用英文（项目使用英文 commit）

## 验证清单

每次提交前确认：

- [ ] 提交只做了一件逻辑上的事
- [ ] 提交信息遵循 Conventional Commits 格式
- [ ] diff 中无密钥信息
- [ ] 无格式化变更与行为变更混在一起
- [ ] 相关测试/文档已包含或在计划中作为单独提交
- [ ] FreeRTOS 与 Zephyr 变更未混在同一提交中
- [ ] Pre-commit hook 已安装且可用

## 警告信号

- 大量未提交的变更不断累积
- 提交信息如"fix"、"update"、"wip"、"更新代码"
- 格式化与行为变更混在一起
- FreeRTOS 与 Zephyr 迁移混在同一提交
- `.gitignore` 未覆盖构建产物（`build/`、`target/`）
- 向共享分支 force push
