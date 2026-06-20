---
name: test
description: >
  自动化 CubeMot 单元测试。运行全量或按模块过滤测试，解析测试结果并展示摘要，
  检测缺失测试的模块，按需生成测试脚手架代码。
  当用户要求运行测试、检查测试、提到 "/test" 或创建新测试时使用。
---

# Test — 单元测试自动化

## 概述

一键运行 CubeMot 的 Google Test 单元测试套件。支持全量运行、按模块过滤、失败分析、缺失测试检测，以及测试脚手架自动生成。

## 工作流程

### 1. 容器就绪检查

测试**必须**在容器 `opsdc-cubemot-habitat` 内执行。

```bash
docker ps --filter name=opsdc-cubemot-habitat --format '{{.Status}}'
```

- **已运行** → 继续
- **未运行** → `docker compose -f /home/lzx/Code/CubeMot/cubemot-workspace/compose.yaml up -d cubemot-habitat`

### 命令前缀约定

后续所有命令均使用以下前缀（下文简写为 `$DEXEC`）：

```bash
DEXEC="docker exec -u lzx -w /home/lzx/Code/CubeMot/cubemot-workspace/CubeMot opsdc-cubemot-habitat"
```

### 2. 确定测试范围

| 用户意图 | 操作 |
|----------|------|
| 默认 / "跑测试" / "run tests" | 全量测试 |
| "测试 pid" / "test crc" / "test math" | 按模块过滤 |
| "哪些模块缺测试" / "测试覆盖" | 缺失检测 |
| "生成 <module> 的测试" | 脚手架生成 |

### 3. 执行测试

> **关键**：CMakePresets.json 在 `CubeMot/` 目录，工作目录必须是 `CubeMot/`。

**workflow preset（推荐，configure + build + test 一步完成）**：

```bash
$DEXEC cmake --workflow --preset test-debug
```

**分步执行**（需要过滤或精细控制时）：

```bash
$DEXEC cmake --preset test-debug
$DEXEC cmake --build --preset test-debug
$DEXEC ctest --preset test-debug               # 全量
$DEXEC ctest --preset test-debug -R pid-test   # 按模块过滤
```

**Preset 选择**：

| Preset | 用途 |
|--------|------|
| `test-debug`（默认） | 开发调试，带调试符号 |
| `test-release` | 验证优化后的行为 |

**按模块过滤**：使用 `ctest -R <regex>` 匹配测试可执行文件名。

| 模块 | 过滤参数 | 说明 |
|------|----------|------|
| pid | `-R pid-test` | PID 控制器 |
| crc | `-R crc-test` | CRC 校验 |
| math | `-R math-test` | FOC 数学库 |
| serial_protocol | `-R serial-protocol-test` | 串口协议 |
| msghub | `-R msghub_test` | 消息总线（注意用下划线） |

### 4. 解析测试结果

GTest 输出标准格式，解析要点：

- `[==========]` 行：总测试数
- `[  PASSED  ]` 行：通过数
- `[  FAILED  ]` 行：失败数 + 失败测试列表
- `[ RUN      ]` → `[       OK ]` 或 `[  FAILED  ]`：单个测试结果
- `Expected:` vs `Actual:`：断言失败的具体差异

**摘要格式**：

```
测试摘要：
  Preset:     test-debug
  总计:       47 tests across 5 test suites
  通过:       45 (95.7%)
  失败:       2
  耗时:       0.8s

  失败测试：
    ❌ PIDTest.Integral_Only
       位置: tests/lib/pid/test_pid.cpp:52
       Expected: 0.1, Actual: 0.099

    ❌ CRCTest.Compute_Overflow
       位置: tests/lib/crc/test_crc.cpp:120
       Expected: 0, Actual: -1

  建议：检查最近对 pid.c 和 crc.c 的变更（git diff）
```

**全部通过时**：

```
测试摘要：
  Preset:     test-debug
  总计:       47 tests across 5 test suites
  通过:       47 (100%)
  失败:       0
  耗时:       0.8s
  状态:       ✅ 全部通过
```

### 5. 缺失测试检测

扫描源码目录，与测试目录对比，报告缺少测试的模块。

**检测算法**：

1. 列出 `src/libs/`、`src/modules/`、`src/comm/` 下包含 `.c` 文件的目录
2. 列出 `tests/lib/` 和 `tests/msghub/` 下所有测试目录
3. 取差集，报告无测试的模块

```bash
# 有源码的模块
find src/libs src/modules src/comm -name "*.c" -printf '%h\n' | sort -u
# 有测试的模块
ls tests/lib/ tests/msghub/ 2>/dev/null
```

报告格式：列出 ✅ 已有测试的模块 和 ❌ 缺失测试的模块，对 `src/libs/` 下的核心库标注优先补充建议。

### 6. 测试脚手架生成

用户要求为新模块生成测试时，参考 [test-scaffolding-template.md](references/test-scaffolding-template.md) 获取完整模板和注册步骤。

**生成清单**：

1. **创建** `tests/lib/<module>/test_<module>.cpp` — 使用模板中的 GTest 结构
2. **创建** `tests/lib/<module>/CMakeLists.txt` — 链接对应库 + `GTest::gtest_main`
3. **注册** 在 `tests/lib/CMakeLists.txt` 添加 `add_subdirectory(<module>)`
4. **确认** 被测模块有 `add_library()` 目标（若无则创建）
5. **Mock** 若模块使用 Zephyr API，链接 `zephyr_mocks`

> **注意**：`src/modules/` 下的模块直接编入 `app` target，无独立 library。测试需提取为独立库或使用 `UNIT_TEST_HOST` 模式。

**验证新测试**：

```bash
$DEXEC cmake --preset test-debug
$DEXEC cmake --build --preset test-debug
$DEXEC ctest --preset test-debug -R <module>-test
```

## 安全协议

- **绝不**在宿主机直接执行 cmake/ctest（必须在容器内）
- 生成测试脚手架时**不要**修改被测源码
- 测试新增应作为**独立提交**（`test(<scope>): ...`），不与实现变更混合
- 容器未运行时**先启动**，不要跳过

## 与其他 Skill 协作

测试全部通过后建议：

> 全部测试通过。可运行 `/commit` 提交变更。

测试失败时建议：

> 测试失败。修复代码后重新运行 `/build` 确认编译通过，再 `/test` 验证。

生成脚手架后提醒：

> 测试脚手架已生成。实现测试用例后运行 `/test` 验证，然后用 `/commit` 提交（type: `test`）。

## 常见问题

### GTest 未找到

```
CMake Error: Could not find GTest
```

→ 初始化 git 子模块：`git submodule update --init --recursive`

### 链接时找不到库

```
/usr/bin/ld: cannot find -l<module>
```

→ 模块缺少 `add_library()` 定义，需在 `src/libs/<module>/CMakeLists.txt` 创建库目标

### Zephyr 头文件缺失

```
fatal error: zephyr/kernel.h: No such file or directory
```

→ 测试构建不含 Zephyr，需使用 `zephyr_mocks` 提供 stub 头文件

### Segfault

```
ctest reports: "Segmentation fault"
```

→ 检查空指针、未初始化结构体、越界数组访问
