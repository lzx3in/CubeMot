# Issue: scope dump 数据丢失

## 现象

`scope dump` 命令输出 128 行 CSV 数据时，主机端仅收到 ~10 行。
Zephyr shell 报告 `--- N messages dropped ---`，表明 printk/log 队列溢出。

## 根因分析

### 1. Zephyr log 后端队列溢出

`printk()` 在 Zephyr 中经过 log 子系统异步处理：
- `CONFIG_LOG_MODE_DEFERRED=y`（默认）→ printk 入队，后台线程 flush 到 UART
- 队列大小由 `CONFIG_LOG_BUFFER_SIZE` 控制（当前 256B）
- 128 行 × ~40 字符/行 = ~5120B，远超 256B 队列
- 后台 flush 线程被 shell 命令线程抢占，无法及时排空

### 2. Shell 提示符污染

每行 printk 输出后，Zephyr shell 自动重绘提示符 `uart:~$`，
导致原始数据流中穿插 ANSI 转义序列：
```
\x1b[1;32muart:~$ \x1b[m\x1b[8D\x1b[J
```
主机端解析器必须 strip 这些序列才能提取纯 CSV。

### 3. UART 带宽不是瓶颈

- LPUART1 @ 115200 baud → 11520 B/s
- 128 行 × 40B = 5120B → 理论 0.44s 可发完
- 实际瓶颈是 log 队列深度 + 线程调度，非物理带宽

## 影响

- 诊断数据不完整，无法做全窗口频谱分析
- 需要多次 dump 拼接或降低 decimation 才能凑够样本
- Python 解析器复杂度高（需正则 strip ANSI + prompt + dropped 提示）

## 解决方案候选

### 方案 A: 绕过 log 子系统，UART 直写

```c
#include <zephyr/drivers/uart.h>

static const struct device *console_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static void scope_uart_puts(const char *s)
{
    while (*s) {
        uart_poll_out(console_uart, *s++);
    }
}
```

- 优点：零队列开销，128 行连续输出无丢失
- 缺点：阻塞调用线程（每字符 ~87µs @ 115200），128×40×87µs ≈ 445ms 阻塞
- 缓解：每 8 行 `k_msleep(1)` 让出 CPU

### 方案 B: 增大 log 缓冲区

```
CONFIG_LOG_BUFFER_SIZE=8192
```

- 优点：零代码改动
- 缺点：+8KB RAM（当前仅剩 ~800B 空闲，**不可行**）

### 方案 C: 切换 log 为 immediate 模式

```
CONFIG_LOG_MODE_IMMEDIATE=y
```

- 优点：printk 同步输出，不入队
- 缺点：可能影响 ISR 中 LOG_INF 的实时性；全局生效

### 方案 D: 二进制帧 + DMA UART

- 用 `uart_tx()` DMA 模式发送 packed int16 数组
- 128 × 18B = 2304B 一次 DMA 传输
- 优点：零 CPU 开销，零丢失
- 缺点：实现复杂，需 DMA 通道配置 + 主机端二进制解析器

## 推荐

**方案 A**（UART 直写）为最优平衡：
- 实现简单（~10 行代码替换 printk）
- 不消耗额外 RAM
- 445ms 阻塞在 dump 场景可接受（非实时路径）
- 输出纯净无 ANSI 污染

## 复现步骤

```bash
# 1. 烧录固件，启动电机
uart:~$ motor start 500
# 2. 等待进入 RUN 状态 (6s)
# 3. 采集 scope
uart:~$ scope start 1
# 4. 等待 150ms
uart:~$ scope stop
# 5. dump
uart:~$ scope dump
# 观察：仅收到 ~10 行数据 + "messages dropped" 提示
```

## 环境

- 固件: cubemot-esc develop 分支
- 硬件: NUCLEO-G431RB + X-NUCLEO-IHM16M1
- UART: LPUART1 @ 115200 (ST-Link V3 VCP)
- Zephyr: v4.4.1
- CONFIG_LOG_BUFFER_SIZE=256
- CONFIG_LOG_MODE_DEFERRED=y (默认)
