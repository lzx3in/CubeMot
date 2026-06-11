# CubeMot 实施计划

> 四轮驱动 + 转向舵机小车 | Zephyr RTOS | STM32G431RB
> 最后更新: 2026-06-12 00:58 GMT+8

---

## 总体阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| **V1** | 1 电机 FOC + 无感启动 + msghub 集成 | ✅ 代码完成，⏸ 待硬件验证 |
| **V2** | 2 电机 + 1 舵机 + Ackermann + MQTT 遥控 | ⬜ 待实施 |
| **V3** | 4 电机 + 2 舵机 + 完整四轮 | ⬜ 远期 |
| **V4** | IMU + 里程计 + 自主导航 | ⬜ 远期 |

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

### 待硬件验证 ⏸

> 需要 NUCLEO-G431RB + X-NUCLEO-IHM16M1 + 电机 + 电源

| # | 验证项 | 方法 | 预期结果 |
|---|--------|------|---------|
| V.1 | TIM1 PWM 输出 | 示波器探头 PA8/PA9/PA10 | 30kHz 三相互补，死区 550ns |
| V.2 | ADC 零电流偏移 | 电机未通电，读 `foc_adc_get_offsets` | 稳定偏移值 |
| V.3 | 电流环开环 | Id=0.8A, Iq=0, θ 强制旋转 | 相电流正弦波形 |
| V.4 | 启动序列 | 发送 MOTOR_CMD_START | Phase1 对齐 → Phase2 斜坡 → 切换闭环 |
| V.5 | 速度闭环 | 给定 500 RPM 目标 | 稳态速度误差 <5% |

---

## V2 实施计划（下次会话）

### 2.1 Commander 状态机 ⬜

```
INIT → STANDBY → ARMED → ACTIVE
              ↑         ↓
              └─ FAULT ←┘
```

| 任务 | 文件 | 说明 |
|------|------|------|
| 消息流：MQTT/serial → commander → cmd_vel → vehicle | `modules/commander/` | PX4 风格状态机 |
| 模式管理：STANDBY/ARMED/ACTIVE/FAULT | 同上 | 管理启停逻辑 |
| 紧急停止流：cmd_emergency → 全电机断电 | 同上 | 高优先级中断处理 |

### 2.2 MQTT 上位机 ⬜

| 任务 | 文件 | 说明 |
|------|------|------|
| MQTT 客户端 (Zephyr net/mqtt) | `comm/mqtt/mqtt_link.c` | 连接 broker |
| MQTT ↔ msghub 话题桥接 | `comm/mqtt/mqtt_topics.c` | 双向翻译 |
| 话题设计 | 见 `docs/hardware-reference.md` §5.5 | cubemot/cmd_vel, cubemot/telemetry |
| 外部 WiFi 模块连接 | 确认 ESP32 波特率/协议 | USART2 1.8Mbps |

### 2.3 Vehicle 运动学 ⬜

| 任务 | 文件 | 说明 |
|------|------|------|
| Ackermann 混控器 | `modules/vehicle/mixer.c` | cmd_vel → motor/servo 指令 |
| 里程计 | `modules/vehicle/odometry.c` | 速度积分 + 编码器 |
| Ackermann 几何公式 | 同上 | 内外轮差速计算 |

### 2.4 第二电机 + 舵机 ⬜

| 任务 | 文件 | 说明 |
|------|------|------|
| 舵机 PWM 驱动 | `drivers/servo/servo.c` | 50Hz PWM, 500-2500μs 脉宽 |
| motor_ctrl 多实例化 | `modules/motor_ctrl/motor_ctrl.c` | 支持 motor_id=0,1 |
| servo_ctrl 模块 | `modules/servo_ctrl/servo_ctrl.c` | 角度控制 + msghub |

---

## V3 远期计划

| 任务 | 说明 |
|------|------|
| 4 电机独立 FOC | 需要 TIM8+TIM20 或外部栅极驱动 |
| 2 舵机 (前轮独立转向) | 真正的 Ackermann 四轮 |
| 电流/电压保护 | 过流关断、欠压刹车 |
| 参数调优工具 | 上位机电调参数实时调整 |

## V4 远期计划

| 任务 | 说明 |
|------|------|
| IMU (MPU6050/ICM-42688) | 姿态估计 |
| 轮式里程计 | 编码器 + IMU 融合 |
| 自主导航 | 路径规划 + 跟踪控制 |

---

## 开发备忘

### 构建环境

```bash
cd /home/lzx/Code/CubeMot/cubemot-workspace
source .venv/bin/activate
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/usr
cd CubeMot
west build -b nucleo_g431rb
```

### 运行单元测试

```bash
cd CubeMot
gcc -o test_math test_math.c -Isrc/libs/math -lm -Wall -O2 && ./test_math
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
