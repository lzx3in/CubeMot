# Git Commit 配置 — CubeMot

> 本文件由 git-commit skill 自动加载，定义该仓库的提交规范。
> 若本文件不存在，skill 不生效。

## 仓库标识

- **名称**: CubeMot
- **Git 根目录**: `CubeMot/`（workspace 子模块）
- **描述**: STM32G431RB 四轮驱动小车固件，基于 Zephyr RTOS

## Scope 映射

按路径前缀匹配，优先最长前缀：

| 路径前缀 | scope |
|---------|-------|
| `src/app/` | `app` |
| `src/modules/` | `modules/{子目录名}` |
| `src/msghub/` | `msghub` |
| `src/topics/` | `topics` |
| `src/comm/` | `comm` |
| `src/drivers/foc/` | `drivers/foc` |
| `src/drivers/servo/` | `drivers/servo` |
| `src/drivers/` | `drivers/{文件名}` |
| `src/libs/foc/` | `libs/foc` |
| `src/libs/math/` | `libs/math` |
| `src/libs/pid/` | `libs/pid` |
| `src/libs/crc/` | `libs/crc` |
| `src/libs/serial_protocol/` | `libs/serial` |
| `src/common/` | `common` |
| `boards/` | `boards/{文件名}` |
| `zephyr/` | `zephyr` |
| `tests/` | `tests` |
| `tools/` | `tools` |
| `CMakeLists.txt` 或 `cmake/` | `build` |
| `Kconfig` 或 `prj.conf` | `build` |

## 分组规则

- 分离 Zephyr 构建配置变更与固件功能变更
- 分离格式化变更（clang-format）与行为变更
- 分离重构与新功能
- 分离文档与代码变更

## 特殊约束

- 提交信息中 type/scope 用英文，description/body 用中文

## Scope 速查

**硬件抽象**: `drivers/foc`, `drivers/servo`, `drivers/{外设}`, `boards/{板名}`
**功能模块**: `modules/blink`, `modules/button_detector`, `modules/led_controller`, `modules/motor_ctrl`, `modules/commander`, `modules/vehicle`, `modules/servo_ctrl`
**通信**: `msghub`, `topics`, `comm`
**库**: `libs/pid`, `libs/crc`, `libs/math`, `libs/foc`, `libs/serial`
**平台**: `app`, `common`, `zephyr`, `build`
**质量**: `tests`, `tools`
