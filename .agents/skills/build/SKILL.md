---
name: build
description: >
  自动化 CubeMot 嵌入式固件构建。检测 Docker 容器状态，选择 CMake preset，
  执行构建并解析错误，提供修复建议和构建摘要（ELF 尺寸、警告数）。
  当用户要求构建、编译、提到 "/build" 或粘贴构建错误时使用。
---

# Build — 嵌入式固件构建

## 概述

一键完成 CubeMot 固件的交叉编译构建。自动处理容器就绪检查、preset 选择、编译执行、错误解析和二进制摘要，免去手动拼 `docker exec` 命令。

## 工作流程

### 1. 容器就绪检查

构建**必须**在容器 `opsdc-cubemot-habitat` 内执行。

```bash
docker ps --filter name=opsdc-cubemot-habitat --format '{{.Status}}'
```

- **已运行** → 继续
- **未运行** → `docker compose -f /home/lzx/Code/CubeMot/cubemot-workspace/compose.yaml up -d cubemot-habitat`

### 命令前缀约定

后续所有构建命令均使用以下前缀（下文简写为 `$DEXEC`）：

```bash
DEXEC="docker exec -u lzx -w /home/lzx/Code/CubeMot/cubemot-workspace/CubeMot opsdc-cubemot-habitat"
```

### 2. 选择构建 Preset

根据用户意图选择 preset，**默认 `embedded-debug`**：

| 用户意图 | Configure Preset | Build Preset |
|----------|------------------|--------------|
| 默认 / debug / 调试 | `embedded-debug` | `embedded-debug` |
| release / 发布 / 优化 | `embedded-release` | `embedded-release` |
| size / 最小体积 | `embedded-minsizerel` | `embedded-minsizerel` |
| relwithdebinfo / 带调试的发布 | `embedded-relwithdebinfo` | `embedded-relwithdebinfo` |
| clean / 清除重建 | — | `embedded-debug-clean` 或 `embedded-release-clean` |

**完整 preset 列表**见 `CubeMot/CMakePresets.json`。

### 3. 执行构建

> **关键**：CMakePresets.json 在 `CubeMot/` 目录，所有 cmake 命令的工作目录必须是 `CubeMot/`。

**workflow preset（推荐，一步完成 configure + build）**：

```bash
$DEXEC cmake --workflow --preset embedded-debug
```

**分步执行**（需要精细控制时）：

```bash
$DEXEC cmake --preset embedded-debug
$DEXEC cmake --build --preset embedded-debug
```

**清除重建**：

```bash
$DEXEC cmake --build --preset embedded-debug-clean
```

### 4. 解析构建错误

构建失败时，解析编译器输出，按类型分类并给出修复建议。参考 [error-patterns.md](references/error-patterns.md) 获取完整错误目录。

**错误分类与处理**：

| 类型 | 识别特征 | 处理 |
|------|----------|------|
| 编译错误 | `error:` + `file:line:col` | 定位文件行号，建议修复 |
| 头文件缺失 | `fatal error: <h>: No such file` | 检查 `target_include_directories` |
| 链接错误 | `undefined reference to` | 检查 `target_link_libraries` / 源文件 |
| 重复定义 | `multiple definition of` | 检查 CMakeLists.txt 重复源文件 |
| CMake 配置 | `CMake Error` | 工具链 / 依赖 / BOARD 问题 |
| Kconfig | `CONFIG_*` 未定义 | 检查 `prj.conf` 和 Kconfig 文件 |

**修复后**：直接重新执行步骤 3，无需重头开始。

### 5. 构建摘要

构建成功后，输出二进制尺寸信息：

```bash
# 先定位 ELF 文件
$DEXEC find build -name "zephyr.elf" -o -name "CubeMot.elf" 2>/dev/null
# 查看尺寸
$DEXEC arm-none-eabi-size <ELF路径>
```

**摘要格式**：

```
构建摘要：
  Preset:   embedded-debug
  Board:    nucleo_g431rb
  状态:     ✅ 成功
  ELF:      zephyr.elf
  text: 38420 | data: 1204 | bss: 5600 | total: 45224 (44.2 KB)
  警告:     2 个（列出）
```

## 安全协议

- **绝不**在宿主机直接执行 `cmake`/`ninja`/`west build`（必须在容器内）
- **绝不**修改 `CMakePresets.json` 除非用户明确要求
- 构建失败时**不要**自动修改源码——报告错误，由用户决定修复方式
- 容器未运行时**先启动**，不要跳过

## 与其他 Skill 协作

构建成功后建议：

> 构建成功。可运行 `/test` 验证单元测试，然后 `/commit` 提交变更。

构建失败并修复后建议：

> 错误已修复。重新运行 `/build` 验证，然后 `/test` 确认测试通过。

## 常见问题

### 容器未运行

```
Error: No such container: opsdc-cubemot-habitat
```

→ 执行：`docker compose -f /home/lzx/Code/CubeMot/cubemot-workspace/compose.yaml up -d cubemot-habitat`

### 工具链不可用

```
arm-none-eabi-gcc: command not found
```

→ 容器镜像可能未正确构建，执行：`docker buildx bake --load dev-nodes && docker compose up -d`

### 陈旧构建缓存

出现莫名的链接错误或重复定义时：

→ 使用 clean build preset：`cmake --build --preset embedded-debug-clean`

### Kconfig 模块未启用

```
CONFIG_MODULE_MOTOR_CTRL is not defined
```

→ 检查 `prj.conf` 中是否有 `CONFIG_MODULE_MOTOR_CTRL=y`，以及相关 Kconfig 文件
