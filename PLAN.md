# CubeMot 实施计划

> 四轮驱动 + 转向舵机小车 | Zephyr RTOS | STM32G431RB
> 最后更新: 2026-07-25 23:30 GMT+8

---

## 总体阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| **V1** | 1 电机 FOC + 无感启动 + msghub 集成 | ✅ V.1~V.4 硬件验证通过，V.5 待观测器调参 |
| **V2** | 2 电机 + 1 舵机 + Commander + Serial 遥控 | ✅ 代码完成，⏸ 待硬件（当前仅 1 电机，无舵机） |
| **V3** | 4 电机 + 2 舵机 + 完整四轮 | 🚫 搁置 |
| **V4** | IMU + 里程计 + 自主导航 | 🚫 搁置 |

---

## V1 详细子任务

### 已完成 ✅

| # | 任务 | 文件 | 测试 |
|---|------|------|------|
| 1.1 | FOC 数学库：Clarke/Park/iPark/iClarke 变换 | `libs/math/transform.h` | ✅ 20 tests |
| 1.2 | SVPWM min-max 注入调制 | `libs/math/svpwm.h` | ✅ 5 tests |
| 1.3 | 数字滤波器：LPF + 滑动平均 | `libs/math/filter.h` | ✅ 10 tests |
| 2.1 | PWM 驱动：TIM1 30kHz 三相互补 + 死区 + BKIN2 | `drivers/foc/foc_pwm.c` | ✅ 编译通过 |
| 2.2 | ADC 驱动：ADC1+2 注入采样 + Vbus | `drivers/foc/foc_adc.c` | ✅ 编译通过 |
| 3.1 | FOC 电流环：Clarke→Park→PI→iPark→SVPWM | `libs/foc/foc_core.c` | ✅ 编译通过 |
| 3.2 | 滑模观测器 + PLL (无感) | `libs/foc/observer.c` | ✅ 编译通过 |
| 3.3 | FOC 数据类型 + ADC/速度单位转换 | `libs/foc/foc_types.h` | ✅ 编译通过 |
| 4.1 | motor_ctrl 模块：速度环 PID (1kHz) | `modules/motor_ctrl/motor_ctrl.c` | ✅ 编译通过 |
| 4.2 | 无感启动序列：Align→Forced Ramp→Switchover | 同上 | ✅ 编译通过 |
| 5.1 | motor_cmd / motor_state msghub topics | `topics/topics.h` | ✅ 编译通过 |

### 硬件验证结果（2026-07-25）

> 硬件：NUCLEO-G431RB + X-NUCLEO-IHM16M1 + GBM2804H-100T + 12V
> 验证手段：Zephyr Shell（USB LPUART1 → ST-Link V3 VCP）
> 电源：12V（非 24V）

| # | 验证项 | 结果 | 关键数据 |
|---|--------|------|----------|
| V.1 | TIM1 PWM 输出 | ✅ | 30kHz, MOE=1, ES_GPIO 下管使能 |
| V.2 | ADC 零电流偏移 | ✅ | Ia=1939, Ib=1944, Ic=1950 (≈2048±100) |
| V.3 | 电流环开环 | ✅ | Id=679.8mA (ref=800mA), 电机锁定有力矩 |
| V.4 | 启动序列 | ✅ | IDLE→ALIGN→START→RUN 完整通过，电机转动 |
| V.5 | 速度闭环 | ⚠️ | 状态机进入 RUN，观测器未收敛→速度=0 |

#### V.5 阻塞原因

滑模观测器（Luenberger + PLL）在 Phase2 开环阶段未正确收敛，
导致切换到闭环后 theta 估计错误。属于**参数调优**问题。

待解决：
- 观测器增益需匹配 GBM2804H-100T 实际 Rs/Ls
- PLL Kp/Ki 需降低以容忍噪声
- 可参考 MCSDK Workbench 生成的定点参数转换

#### 本次硬件验证修复的 bug

1. Shell USB 接入：VCP 实际桥接 LPUART1（非 USART1）
2. ADC 左对齐 >>4 修正
3. ADC 注入转换改为流水线模式（解决 ISR 中读零）
4. PB13/14/15 改为 ES_GPIO 模式（STSPIN830 不接受互补 PWM）
5. 观测器双重调用修复（ISR + motor_ctrl 重复 step）
6. 三重过流保护（ISR 级 + Iq 限幅 + Phase2 超时停止）

#### 验证操作速查（Shell 命令）

```bash
# 连接 USB 后打开终端
minicom -D /dev/ttyACM0 -b 115200   # 或 screen /dev/ttyACM0 115200

# V.2: ADC 偏移（电机未通电）
uart:~$ adc offset

# V.3: 开环电流环
uart:~$ foc start 800       # Id=800mA
uart:~$ foc status          # 查看 Id/Iq/Vbus/duty
uart:~$ foc stop

# V.4/V.5: 启动序列 + 速度闭环
uart:~$ motor start 500     # 500 RPM
uart:~$ motor watch 500 10  # 每 500ms 打印，共 10 次
uart:~$ motor stop

# 诊断
uart:~$ adc diag            # ADC 软件触发全诊断
```

#### 信号分析仪测量点（可选）

| 测量点 | 引脚 | 预期 |
|--------|------|------|
| PWM 载波 | PA8 (UH) | 30kHz 中心对齐 |
| 相电流 | PA1 (Ia sense) | 正弦波，58.3Hz @ 500RPM |
| 母线电压 | PC5 (分压后) | ≈1.5V DC（对应 24V） |

---

## V2 实施计划（已完成 ✅）

### 2.1 Commander 状态机 ✅

```
INIT → STANDBY → ARMED → ACTIVE
              ↑         ↓
              └─ FAULT ←┘
```

| 任务 | 文件 | 状态 |
|------|------|------|
| 消息流：serial → commander → cmd_vel → vehicle | `modules/commander/commander.c` | ✅ |
| 模式管理：STANDBY/ARMED/ACTIVE/FAULT | 同上 | ✅ |
| 紧急停止流：cmd_emergency → 全电机断电 | 同上 | ✅ |

### 2.2 Serial 命令协议 ✅

| 任务 | 文件 | 状态 |
|------|------|------|
| 二进制帧协议 [0xAA 0x55] [CMD] [LEN] [DATA] [CRC8] | `comm/serial_cmd/serial_cmd.c` | ✅ |
| USART1 @ 115200 baud (PB6/PB7) | 同上 | ✅ |
| 命令：CMD_VEL/ARM/DISARM/ESTOP/PING | 同上 | ✅ |
| 响应：RSP_STATUS(10Hz)/RSP_TELEMETRY(20Hz)/RSP_MOTOR | 同上 | ✅ |

### 2.3 Vehicle 运动学 ✅

| 任务 | 文件 | 状态 |
|------|------|------|
| 差速驱动混控器 | `modules/vehicle/vehicle.c` | ✅ |
| 里程计（Euler 积分） | 同上 | ✅ |
| 使用 fast_sincos LUT（无 libm 依赖） | 同上 | ✅ |

### 2.4 第二电机 + 舵机 ✅

| 任务 | 文件 | 状态 |
|------|------|------|
| motor_ctrl 多实例化（MAX_MOTORS=2） | `modules/motor_ctrl/motor_ctrl.c` | ✅ |
| 舵机 PWM 驱动（TIM3 CH1/CH2, 50Hz） | `drivers/servo/servo.c` | ✅ |
| servo_ctrl 模块（角度控制 + 平滑插值） | `modules/servo_ctrl/servo_ctrl.c` | ✅ |

---

## V3 远期计划（🚫 搁置）

> 当前硬件仅 1 电机，无舵机。V3 等待硬件齐备后再启动。

| 任务 | 说明 |
|------|------|
| 4 电机独立 FOC | 需要 TIM8+TIM20 或外部栅极驱动 |
| 2 舵机 (前轮独立转向) | 真正的 Ackermann 四轮 |
| 电流/电压保护 | 过流关断、欠压刹车 |
| 参数调优工具 | 上位机电调参数实时调整 |

## V4 远期计划（🚫 搁置）

| 任务 | 说明 |
|------|------|
| IMU (MPU6050/ICM-42688) | 姿态估计 |
| 轮式里程计 | 编码器 + IMU 融合 |
| 自主导航 | 路径规划 + 跟踪控制 |

---

## 开发备忘

### 构建环境

```bash
# 在 habitat 容器内执行
docker exec -u lzx -w /home/lzx/Code/CubeMot/cubemot-workspace opsdc-cubemot-habitat \
  bash -c "export PATH=\$HOME/.local/bin:\$PATH && west build -b nucleo_g431rb cubemot-esc --pristine"
```

### 运行单元测试

```bash
# 宿主机直接执行（tests/ 目录，CMake + GTest）
cd cubemot-esc && cmake -B build_test -S tests && cmake --build build_test && ctest --test-dir build_test
```

### 硬件连接

| 信号 | 引脚 | NUCLEO 位置 |
|------|------|------------|
| Phase UH / UL | PA8 / PB13 | CN5-1 / CN5-4 |
| Phase VH / VL | PA9 / PB14 | CN5-2 / CN5-5 |
| Phase WH / WL | PA10 / PB15 | CN5-3 / CN5-6 |
| Ia (U相电流) | PA1 | CN8-1 |
| Ib (V相电流) | PB11 | CN8-3 |
| Ic (W相电流) | PA7 | CN8-2 |
| Vbus | PC5 | CN8-4 |
| Emergency Stop | PA11 | CN5-7 |
| User Button | PC13 | 板载蓝键 |

### 关键参考

| 文档 | 路径 |
|------|------|
| 硬件参考 | `docs/hardware-reference.md` |
| 架构设计 | `~/.openclaw/workspace/cubemot-architecture.md` |
| 电机参数 | GimBal GBM2804H-100T, 7对极, Rs=5.29Ω, Ls=1.058mH |
| 功率板 | X-NUCLEO-IHM16M1, STSPIN830, 三电阻采样 |
| PWM 频率 | 30kHz 中心对齐 |
| 速度环频率 | 1kHz |
