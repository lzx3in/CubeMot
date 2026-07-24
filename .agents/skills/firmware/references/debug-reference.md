# GDB + OpenOCD 调试速查

STM32G431RB (Cortex-M4) 调试命令参考。

## GDB 命令速查

### 运行控制

| 命令 | 缩写 | 说明 |
|------|------|------|
| `continue` | `c` | 继续执行 |
| `step` | `s` | 单步（进入函数） |
| `next` | `n` | 单步（跳过函数） |
| `finish` | `fin` | 执行到函数返回 |
| `Ctrl+C` | - | 停止执行 |

### 断点管理

```gdb
break main                    # 函数断点
break main.c:50               # 行号断点
break *0x08000188             # 地址断点
tbreak function_name          # 临时断点
info breakpoints              # 查看所有断点
delete 1                      # 删除断点 1
disable 1                     # 禁用断点
break foo.c:123 if counter == 10  # 条件断点
```

### 观察点（STM32G4 支持 4 个硬件观察点）

```gdb
watch my_variable             # 监控写入
rwatch my_variable            # 监控读取
awatch my_variable            # 监控读写
watch *(int*)0x20000000       # 监控内存地址
```

### 变量与内存

```gdb
print my_variable             # 十进制
print/x my_variable           # 十六进制
print/t my_variable           # 二进制
print array[0]@10             # 数组前10元素
display counter               # 持续显示
set my_variable = 100         # 修改变量

# 内存查看: x/nfu addr (n=数量, f=格式, u=单位)
x/16xw 0x08000000             # Flash 起始
x/64xb 0x20000000             # RAM 起始
x/s 0x20000100                # 字符串
x/10i main                    # 反汇编
```

### 寄存器

```gdb
info registers                # 所有寄存器
info registers r0 pc sp lr    # 指定寄存器
print/x $xpsr                 # 程序状态
print/x $msp                  # 主栈指针
print/x $psp                  # 进程栈指针
set $pc = main                # 修改 PC
```

### 调用栈

```gdb
backtrace                     # 调用栈
backtrace full                # 含局部变量
frame 2                       # 切换栈帧
up / down                     # 上/下移动
info frame                    # 当前帧信息
```

## OpenOCD Telnet 命令

连接：`telnet localhost 4444`

| 命令 | 说明 |
|------|------|
| `halt` | 暂停目标 |
| `resume` | 恢复执行 |
| `reset halt` | 复位并暂停 |
| `mdw <addr> <count>` | 查看内存（字） |
| `mww <addr> <value>` | 修改内存 |
| `reg` | 查看寄存器 |
| `flash list` | Flash 信息 |
| `adapter speed <khz>` | 设置调试速度 |

## 关键寄存器地址

| 地址 | 寄存器 | 用途 |
|------|--------|------|
| `0xE000ED28` | CFSR | 可配置故障状态 |
| `0xE000ED2C` | HFSR | HardFault 状态 |
| `0xE000ED34` | MMFAR | 存储器管理故障地址 |
| `0xE000ED38` | BFAR | 总线故障地址 |
| `0xE0001000` | DWT_CTRL | DWT 控制 |
| `0xE0001004` | DWT_CYCCNT | 周期计数器 |

## RCC 时钟寄存器

| 地址 | 寄存器 |
|------|--------|
| `0x4002104C` | RCC_AHB2ENR |
| `0x40021058` | RCC_APB1ENR1 |
| `0x40021060` | RCC_APB2ENR |

## 典型调试场景

### HardFault 分析

```gdb
break HardFault_Handler
monitor reset halt
continue
# 触发后：
print/x *(uint32_t*)0xE000ED28    # CFSR
print/x *(uint32_t*)0xE000ED2C    # HFSR
backtrace                          # 故障位置
```

### 外设不工作

```gdb
# 检查时钟使能
print/x *(uint32_t*)0x4002104C    # RCC_AHB2ENR
# 检查 GPIO 配置
print/x *(uint32_t*)0x48000000    # GPIOA_MODER
print/x *(uint32_t*)0x48000014    # GPIOA_ODR
```

### 栈溢出检查

```gdb
info registers sp msp
# 确认 SP 在合法 RAM 范围内 (0x20000000 - 0x20008000)
```

### DWT 周期计数（时序测量）

```gdb
# 启用 DWT
set *(uint32_t*)0xE0001000 = 0x40000001
# 读取计数
print *(uint32_t*)0xE0001004
# 时间 = cycles / 170MHz
```

## 硬件限制

- 硬件断点：**6 个**
- 硬件观察点：**4 个**
- Flash：128 KB @ 0x08000000
- RAM：32 KB @ 0x20000000
- CPU：Cortex-M4 @ 170 MHz
