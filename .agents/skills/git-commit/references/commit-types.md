# CubeMot 提交类型

基于 Conventional Commits 规范，针对嵌入式固件项目（STM32 + FreeRTOS / Zephyr RTOS）定制。

## 类型目录

| 类型 | 描述 | 示例 scope |
|------|------|-----------|
| `feat` | 新功能或能力 | `modules`, `drivers`, `msghub` |
| `fix` | Bug 修复 | `chips`, `modules`, `msghub` |
| `docs` | 仅文档（无代码变更） | `readme`, `api`, `zephyr` |
| `style` | 格式化、空格、分号等——无逻辑变更 | `all`, `src` |
| `refactor` | 代码重构，不改变行为 | `chips`, `boards`, `drivers`, `msghub` |
| `perf` | 性能优化 | `libs`, `msghub`, `modules` |
| `test` | 新增或更新测试 | `msghub`, `libs/pid`, `libs/crc` |
| `build` | 构建系统或外部依赖变更 | `cmake`, `zephyr`, `freertos`, `deps` |
| `ci` | CI/CD 配置或脚本 | `github`, `docker` |
| `chore` | 维护、工具、配置（无生产代码） | `tools`, `scripts`, `hooks` |
| `revert` | 回滚之前的提交 | 被回滚提交的 scope |

## 破坏性变更

任意类型均可携带破坏性变更标记：

```
# 方式一：类型（或类型+scope）后加感叹号
feat!: 移除已弃用的旧 HAL 抽象层
refactor(msghub)!: 将 publisher handle 从指针改为值类型

# 方式二：BREAKING CHANGE footer
feat: 允许模块动态注册

BREAKING CHANGE: 模块 init 函数现在必须返回 int 而非 void
```

## 各类型格式规则

### `feat`

- **Scope**：必填——使用模块或功能区域名
- **Body**：功能非平凡时建议填写——解释动机
- **Footer**：实现追踪中的 issue 时使用 `Closes #N`

示例：
```
feat(modules): 添加基于 FOC 的电机电流环控制模块

通过 msghub 订阅电流采样数据，运行 PID 控制器，
发布 PWM 占空比命令。支持 3 个独立电机通道。

Closes #12
```

### `fix`

- **Scope**：必填——bug 所在的模块
- **Body**：建议填写——描述根因及修复方式
- **Footer**：使用 `Fixes #N` 关联 bug 报告

示例：
```
fix(chips/stm32g4): 修正 EXTI IRQ 对引脚 5-9 的错误路由

之前 pin 5-9 未正确路由到 EXTI9_5_IRQn，
导致这些引脚的中断无法触发。

Fixes #23
```

### `docs`

- **Scope**：可选
- **Body**：不需要
- **注意**：应独立于代码变更单独提交

示例：
```
docs(zephyr): 添加 Zephyr 迁移指南和快速入门文档
```

### `style`

- **Scope**：可选
- **Body**：不需要
- **注意**：必须零逻辑变更——仅格式化（clang-format）

示例：
```
style(src): 统一 include 排序和空白行规范
```

### `refactor`

- **Scope**：必填——重构的模块名
- **Body**：建议填写——说明简化或改进了什么
- **注意**：不能添加功能或修复 bug

示例：
```
refactor(msghub): 将临界区宏提取为独立的环境适配层

BREAKING CHANGE: msghub_state.h 不再直接包含 FreeRTOS.h，
需在项目编译定义中添加 FREERTOS_ENV 或 __ZEPHYR__。
```

### `perf`

- **Scope**：必填——性能优化的模块名
- **Body**：建议填写——描述优化方式及其影响

示例：
```
perf(msghub): 将发布者回调遍历从 O(n) 优化为 O(1) 订阅查找
```

### `test`

- **Scope**：必填——被测试的模块名
- **Body**：不需要
- **注意**：应独立于实现变更单独提交

示例：
```
test(msghub): 添加 ISR 上下文中 publish 的并发安全测试
```

### `build`

- **Scope**：可选——依赖更新用 `deps`，或用构建系统名
- **Body**：不需要

示例：
```
build(zephyr): 添加 Zephyr v3.7 构建支持及 nucleo_g431rb 板级配置
build(deps): 升级 FreeRTOS 内核到 v11.2.0
```

### `ci`

- **Scope**：可选——使用 CI 工具或流水线名
- **Body**：不需要

示例：
```
ci(github): 添加 clang-format 格式检查到 CI 流水线
```

### `chore`

- **Scope**：可选——使用工具或配置区域名
- **Body**：不需要
- **注意**：不能涉及生产代码

示例：
```
chore(hooks): 安装 git-commit skill 的 pre-commit hook
chore(tools): 添加 gen_config.py Kconfig 配置生成工具
```

### `revert`

- **Scope**：可选——使用被回滚提交的 scope
- **Body**：必填——说明回滚了哪个提交及原因
- **Footer**：必须包含 `Reverts <commit-hash>`

示例：
```
revert(zephyr): 回滚 Zephyr 构建系统中错误的 include 路径修改

Zephyr 入口的模块 header 路径导致重复定义。
在找到正确方案前先回滚。

Reverts a1b2c3d
```

## CubeMot 项目 scope 参考

### 硬件抽象层
- `chips` — 芯片级抽象（`chips/stm32g4`）
- `boards` — 板级支持（`boards/nucleo_g431rb`）
- `boards/nucleo_g431rb` — 具体板级配置

### 驱动与通信
- `drivers/led`、`drivers/button` — 外设驱动
- `msghub` — 发布-订阅消息总线
- `topics` — 消息主题定义

### 模块
- `modules/blink`、`modules/led_controller`、`modules/button_detector`
- `modules/motor`（计划中）

### 库
- `libs/pid`、`libs/crc`

### 平台
- `app` — 应用入口与状态机
- `common` — 公共类型、错误码、时间戳
- `freertos` — FreeRTOS 相关
- `zephyr` — Zephyr RTOS 相关

### 基础设施
- `build` — CMake/构建系统
- `ci` — GitHub Actions / CI
- `tests` — 单元测试
- `tools` — 脚本工具
TYPEEEOF
echo "commit-types.md adapted"
__code=$?; pgrep -g 0 >/tmp/shell_pgrep_475bf8ebdd19.tmp 2>&1; (exit $__code)