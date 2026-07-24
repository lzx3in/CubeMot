---
name: firmware
description: >
  CubeMot 固件全生命周期操作：构建(build)、烧录(flash)、调试(debug)。
  自动处理 WSL2 USB 探针接入、Docker 容器模式切换、CMake preset 构建、
  OpenOCD 烧录、GDB 调试会话。当用户提到 "构建"、"编译"、"build"、
  "烧录"、"flash"、"下载固件"、"调试"、"debug"、"gdb"、"/firmware" 时使用。
---

# Firmware — 固件构建·烧录·调试

## 意图路由

| 用户意图 | 子命令 | 跳转 |
|----------|--------|------|
| 构建 / 编译 / build | `build` | §构建 |
| 烧录 / flash / 下载 | `flash` | §烧录 |
| 调试 / debug / gdb | `debug` | §调试 |
| 连接设备 / attach / probe | `probe` | §探针接入 |
| 状态 / status | `status` | §状态检查 |

复合意图（如 "构建并烧录"）按顺序串联执行。

---

## 执行层级

```
┌─ WSL2 宿主机 ─────────────────────────────┐
│  probe-usb attach/detach/status            │
│  docker compose --profile debug up -d      │
└────────────────────────────────────────────┘
         │ /dev/bus/usb 透传
         ▼
┌─ 容器 opsdc-cubemot-habitat ──────────────┐
│  cmake build / openocd flash / gdb debug   │
└────────────────────────────────────────────┘
```

### 命令前缀约定

```bash
# 宿主机直接执行（探针管理）
PROBE="/home/lzx/Code/CubeMot/cubemot-workspace/tools/probe-usb"

# 容器内执行（构建/烧录/调试）
DEXEC="docker exec -u lzx -w /home/lzx/Code/CubeMot/cubemot-workspace/CubeMot opsdc-cubemot-habitat"
```

---

## §状态检查

快速诊断当前链路就绪状态：

```bash
# 1. 探针状态（宿主机）
$PROBE status

# 2. 容器运行状态
docker ps --filter name=opsdc-cubemot-habitat --format '{{.Names}} {{.Status}}'

# 3. 容器内 USB 设备可见性
docker exec opsdc-cubemot-habitat lsusb 2>/dev/null | grep -i "0483"

# 4. 构建产物存在性
ls CubeMot/build/zephyr/zephyr.elf 2>/dev/null && echo "ELF ready"
```

**状态摘要格式**：

```
固件链路状态：
  探针:     ST-Link Attached ✓ (busid 5-9)
  容器:     opsdc-cubemot-habitat Up (debug profile)
  USB透传:  /dev/bus/usb 可见 ✓
  构建产物: zephyr.elf ✓ (2024-01-15 14:30)
  就绪:     ✅ 可执行 flash/debug
```

---

## §探针接入

> 执行位置：**WSL2 宿主机**（非容器内）

### 自动接入

```bash
$PROBE attach
```

脚本自动识别 ST-Link（VID 0483）并 attach 到 WSL2。

### 手动指定

```bash
$PROBE detect          # 列出所有调试探针
$PROBE attach 5-9      # 指定 busid
$PROBE detach          # 释放设备（归还 Windows）
```

### 接入后验证

```bash
lsusb | grep 0483:374b   # 应看到 ST-Link V2-1
```

**失败排查**：
- `usbipd.exe not found` → 安装: `winget install dorssel.usbipd-win`
- attach 后 lsusb 无设备 → `probe-usb detach` 再重新 attach
- 权限问题 → 确认 WSL 内核支持 usbip: `lsmod | grep usbip`

---

## §容器模式

### 标准模式（仅构建，无 USB）

```bash
docker compose -f /home/lzx/Code/CubeMot/cubemot-workspace/compose.yaml up -d cubemot-habitat
```

### 调试模式（USB 透传，flash/debug 必需）

```bash
# 前提：probe-usb attach 已完成
docker compose -f /home/lzx/Code/CubeMot/cubemot-workspace/compose.yaml --profile debug up -d cubemot-habitat-debug
```

> 两个 profile 共享 `container_name: opsdc-cubemot-habitat`，切换时自动替换。

### 验证 USB 透传

```bash
docker exec opsdc-cubemot-habitat lsusb | grep 0483
# 应输出: Bus 00x Device 00x: ID 0483:374b STMicroelectronics ST-LINK/V2.1
```

---

## §构建

> 执行位置：**容器内**

### Preset 选择

| 用户意图 | Preset |
|----------|--------|
| 默认 / debug | `embedded-debug` |
| release / 优化 | `embedded-release` |
| 最小体积 | `embedded-minsizerel` |
| 清除重建 | `embedded-debug-clean` |

### 执行

```bash
$DEXEC cmake --workflow --preset embedded-debug
```

### 错误处理

构建失败时解析输出，参考 [error-patterns.md](references/error-patterns.md)。

| 类型 | 识别特征 | 处理 |
|------|----------|------|
| 编译错误 | `error:` + `file:line:col` | 定位文件行号 |
| 头文件缺失 | `fatal error: <h>: No such file` | 检查 include 路径 |
| 链接错误 | `undefined reference to` | 检查源文件/库链接 |
| CMake 配置 | `CMake Error` | 工具链/preset 问题 |
| Kconfig | `CONFIG_*` 未定义 | 检查 prj.conf |

### 构建摘要

```bash
$DEXEC arm-none-eabi-size build/zephyr/zephyr.elf
```

输出格式：

```
构建摘要：
  Preset:   embedded-debug
  Board:    nucleo_g431rb
  状态:     ✅ 成功
  ELF:      build/zephyr/zephyr.elf
  text: 38420 | data: 1204 | bss: 5600 | total: 45224 (44.2 KB)
  警告:     2 个
```

---

## §烧录

> 执行位置：**容器内**（需 debug profile + USB 透传）

### 前置检查

1. 探针已 attach（`$PROBE status` 显示 Attached）
2. 容器以 debug profile 运行
3. 构建产物存在（`build/zephyr/zephyr.elf`）

任一不满足则先执行对应步骤。

### 执行烧录

```bash
$DEXEC openocd \
  -f /home/lzx/Code/CubeMot/cubemot-workspace/zephyr/boards/st/nucleo_g431rb/support/openocd.cfg \
  -c "program build/zephyr/zephyr.elf verify reset exit"
```

### 成功标志

```
** Programming Finished **
** Verify OK **
** Resetting Target **
```

### 烧录摘要

```
烧录摘要：
  工具:     OpenOCD 0.12.0
  接口:     ST-LINK/V2-1 (SWD)
  目标:     STM32G431RB (Cortex-M4)
  文件:     build/zephyr/zephyr.elf
  状态:     ✅ 烧录成功，目标已复位运行
```

### 失败排查

| 错误 | 原因 | 修复 |
|------|------|------|
| `libusb_open() failed` | USB 未透传 | 检查 probe attach + debug profile |
| `Error: open failed` | ST-Link 被占用 | 关闭其他 OpenOCD/GDB 进程 |
| `Target not examined yet` | SWD 连接失败 | 检查线缆，降低 adapter speed |
| `flash write failed` | Flash 保护 | `stm32g4x unlock 0` 后重试 |

---

## §调试

> 执行位置：**容器内**（需 debug profile + USB 透传）

### 快速调试（推荐）

一步启动 OpenOCD + GDB：

```bash
# 终端 1: OpenOCD GDB server（后台）
$DEXEC openocd \
  -f /home/lzx/Code/CubeMot/cubemot-workspace/zephyr/boards/st/nucleo_g431rb/support/openocd.cfg &

# 终端 2: GDB 连接
$DEXEC gdb-multiarch build/zephyr/zephyr.elf \
  -ex "target remote localhost:3333" \
  -ex "load" \
  -ex "monitor reset halt" \
  -ex "break main" \
  -ex "continue"
```

### 交互式调试

若用户需要交互式 GDB 会话，使用 `docker exec -it`：

```bash
docker exec -it -u lzx -w /home/lzx/Code/CubeMot/cubemot-workspace/CubeMot opsdc-cubemot-habitat \
  gdb-multiarch build/zephyr/zephyr.elf
```

然后在 GDB 中：

```gdb
(gdb) target remote localhost:3333
(gdb) load
(gdb) monitor reset halt
(gdb) break main
(gdb) continue
```

### 常用调试命令

| 命令 | 缩写 | 说明 |
|------|------|------|
| `continue` | `c` | 继续执行 |
| `step` | `s` | 单步（进入函数） |
| `next` | `n` | 单步（跳过函数） |
| `break <loc>` | `b` | 设置断点 |
| `print <expr>` | `p` | 打印变量 |
| `info registers` | `i r` | 查看寄存器 |
| `backtrace` | `bt` | 调用栈 |
| `x/16xw 0x08000000` | | 查看内存 |

更多 GDB 命令参考 [debug-reference.md](references/debug-reference.md)。

### HardFault 诊断

```gdb
(gdb) print/x *(uint32_t*)0xE000ED28    # CFSR
(gdb) print/x *(uint32_t*)0xE000ED2C    # HFSR
(gdb) print/x *(uint32_t*)0xE000ED34    # MMFAR
(gdb) print/x *(uint32_t*)0xE000ED38    # BFAR
```

---

## 安全协议

- **绝不**在宿主机直接执行 `cmake`/`openocd`/`gdb`（必须在容器内）
- **绝不**修改 `CMakePresets.json` 除非用户明确要求
- 构建失败时**不要**自动修改源码——报告错误，由用户决定
- 容器未运行时**先启动**，不要跳过
- flash/debug 前**必须**确认 USB 透传就绪
- 调试结束**提醒**用户 detach 探针（归还 Windows）

## 与其他 Skill 协作

构建成功后：

> 构建成功。可运行 `/firmware flash` 烧录，或 `/test` 验证单元测试。

烧录成功后：

> 烧录完成，目标已复位运行。可运行 `/firmware debug` 进入调试，或 `/commit` 提交变更。

调试结束后：

> 调试会话结束。建议运行 `probe-usb detach` 释放设备。

---

## 常见问题

### 容器内看不到 USB 设备

```bash
docker exec opsdc-cubemot-habitat lsusb  # 无 0483 设备
```

→ 确认：1) `probe-usb attach` 成功 2) 容器以 debug profile 运行

### OpenOCD 连接不稳定

→ 在 openocd.cfg 中降低速度：添加 `-c "adapter speed 1000"`

### 构建缓存异常

→ 使用 clean preset：`cmake --workflow --preset embedded-debug-clean`

### GDB 连接被拒

```
localhost:3333: Connection refused
```

→ 确认 OpenOCD 正在运行：`docker exec opsdc-cubemot-habitat pgrep openocd`
