---
name: git-commit
description: >
  分析 git 变更，自动识别变更所属仓库，加载该仓库的 commit-config.md 配置，
  按配置分组并生成 Conventional Commits 格式的提交信息，自动执行提交。
  当用户要求提交代码、创建 git commit、暂存文件或提到 "/commit" 时使用。
  无配置的仓库不干预提交。
---

# Git Commit

## 概述

为工作区内多个 Git 仓库创建标准化、语义化的提交。自动检测变更所属仓库，加载该仓库的 `.agents/skills/git-commit/commit-config.md` 配置，按 Conventional Commits 规范分组并生成提交信息。无配置的仓库不干预。

## 工作流程

### 0. 识别仓库与加载配置

对当前有未提交变更的每个 git 仓库：

1. **确定仓库**：在变更目录中执行 `git rev-parse --show-toplevel` 获取仓库根目录
2. **查找配置**：检查 `<repo-root>/.agents/skills/git-commit/commit-config.md` 是否存在
3. **无配置则跳过**：若仓库没有 commit-config.md，该仓库的变更不使用本 skill——以默认 git commit 行为处理
4. **有配置则加载**：读取配置文件中的 scope 映射、分组规则和特殊约束，后续步骤按此配置执行

跨仓库场景：若多个仓库同时有变更，按仓库分别处理，每个仓库独立走完整流程。

### 1. 收集状态

```bash
git status --porcelain
git diff --staged          # 已有暂存内容时
git diff                   # 无暂存内容时
```

若无暂存内容且用户未指定文件，则不暂存任何内容——按用户指令操作。

### 2. 分组变更

按 commit-config.md 中的**分组规则**和 **scope 映射**将变更拆分为逻辑分组。

确定每个变更文件的 scope：
1. 按文件路径匹配 commit-config.md 中的 scope 映射表（最长前缀优先）
2. 未匹配时使用兜底规则

分组原则（通用 + 仓库配置中列出的额外规则）：
- 每个分组独立构成一个提交
- 每个分组应可独立审查、独立回滚
- 分离格式化变更与行为变更
- 分离重构与新功能
- 分离文档与代码变更

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
  [cubemot]
  1. refactor(drivers/foc): 移除未使用的编译定义
     文件：src/drivers/foc/CMakeLists.txt
  2. feat(modules/motor_ctrl): 添加基于 msghub 的电机控制模块
     文件：src/modules/motor_ctrl/motor_ctrl.c, src/topics/topics.c
  [workspace]
  3. chore(docker/habitat): 更新开发容器基础镜像
     文件：docker/habitat/Dockerfile
```

### 5. 执行

无需询问确认，在每个仓库的根目录下按顺序执行每次提交：

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
- **绝不**在无 commit-config.md 的仓库中使用本 skill 的规范（回退到默认 git 行为）
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
- **中文内容**：type、scope 使用英文，description 和 body 使用中文（如 `refactor(chips): 更新 GPIO 头文件 include 路径`）

## 验证清单

每次提交前确认：

- [ ] 提交只做了一件逻辑上的事
- [ ] 提交信息遵循 Conventional Commits 格式
- [ ] diff 中无密钥信息
- [ ] 无格式化变更与行为变更混在一起
- [ ] 相关测试/文档已包含或在计划中作为单独提交
- [ ] 符合 commit-config.md 中的分组规则
- [ ] 符合 commit-config.md 中的特殊约束
- [ ] Pre-commit hook 已安装且可用

## 警告信号

- 大量未提交的变更不断累积
- 提交信息如"fix"、"update"、"wip"、"更新代码"
- 格式化与行为变更混在一起
- 违反 commit-config.md 分组规则的混合提交
- `.gitignore` 未覆盖构建产物（`build/`、`target/`）
- 向共享分支 force push

## 与其他 Skill 协作

提交前建议先验证变更：

> 运行 `/build` 确认编译通过，运行 `/test` 确认测试通过，然后 `/commit`。

提交类型与开发 Skill 的对应：

| 开发动作 | 验证 | 提交类型 |
|----------|------|----------|
| 新增功能 | `/build` + `/test` | `feat(<scope>): ...` |
| Bug 修复 | `/build` + `/test` | `fix(<scope>): ...` |
| 新增测试 | `/test` | `test(<scope>): ...` |
| 构建系统 | `/build` | `build(<scope>): ...` |
| 重构 | `/build` + `/test` | `refactor(<scope>): ...` |
