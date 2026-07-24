# 构建错误模式速查表

CubeMot 嵌入式构建（ARM GCC + Zephyr）常见错误及修复建议。

## 编译错误

### 头文件缺失

```
fatal error: <header>.h: No such file or directory
```

**原因**：include 路径未配置或头文件不存在。

**修复**：
1. 确认头文件路径存在：`find src -name "<header>.h"`
2. 在模块的 `CMakeLists.txt` 添加 include 目录：
   ```cmake
   target_include_directories(app PRIVATE <path-to-header-dir>)
   ```
3. 若头文件属于另一个模块，确保该模块的 include 目录已加入

### 隐式函数声明

```
warning: implicit declaration of function '<func>'
```

**原因**：未包含声明该函数的头文件。

**修复**：在源文件顶部添加对应的 `#include`。若函数来自 Zephyr API，检查是否需要 `#include <zephyr/...>`。

### 类型不匹配

```
error: incompatible types when assigning to type 'X' from type 'Y'
```

**修复**：检查结构体定义和赋值语句，确认类型一致。注意 `float` vs `double`、`uint8_t` vs `int` 等隐式转换。

### 未声明的标识符

```
error: '<identifier>' undeclared
```

**修复**：
1. 检查是否缺少 `#include`
2. 检查拼写错误
3. 检查变量作用域（是否在正确的 block 内）

## 链接错误

### 未定义引用

```
undefined reference to '<function_name>'
```

**原因**：函数已声明但未链接到对应的目标文件。

**修复**：
1. 确认 `.c` 文件已在 `CMakeLists.txt` 的 `target_sources()` 中
2. 若函数属于独立库，检查 `target_link_libraries()` 是否包含该库
3. 检查函数名拼写和签名是否与头文件一致

### 重复定义

```
multiple definition of '<symbol>'
```

**原因**：同一源文件被 `target_sources()` 多次包含，或全局变量在头文件中定义（而非声明）。

**修复**：
1. 检查 `CMakeLists.txt` 是否有重复的源文件条目
2. 将头文件中的变量定义改为 `extern` 声明 + `.c` 文件中定义

### 库未找到

```
cannot find -l<library_name>
```

**修复**：
1. 确认库目标已在 `add_library()` 中创建
2. 检查库名拼写（注意项目中的命名不一致：`pid` vs `lib_crc` vs `lib_serial_protocol`）
3. 确认包含该库的 `CMakeLists.txt` 已被 `add_subdirectory()` 引入

## CMake 配置错误

### 工具链未找到

```
CMake Error: CMAKE_C_COMPILER not set
```

**原因**：ARM 工具链未在容器内正确安装或环境变量缺失。

**修复**：重建容器镜像 `docker buildx bake --load dev-nodes && docker compose up -d`

### Preset 不存在

```
CMake Error: Preset '<name>' not found
```

**修复**：
1. 确认工作目录是 `CubeMot/`（CMakePresets.json 所在目录）
2. 检查 preset 名拼写，有效名称：`embedded-debug`, `embedded-release`, `embedded-minsizerel`, `embedded-relwithdebinfo`, `test-debug`, `test-release`

### BOARD 未配置

```
CMake Error: BOARD is not set
```

**修复**：Zephyr 构建需要在 configure 时指定 BOARD。检查板级配置是否正确传入。

## Kconfig 错误

### 配置选项未定义

```
warning: attempt to assign the value 'y' to the undefined symbol CONFIG_MODULE_*
```

**修复**：
1. 确认模块的 `Kconfig` 文件中定义了该选项
2. 确认模块的 Kconfig 已被父 `Kconfig` 通过 `source` 引入
3. 检查 `prj.conf` 中 `CONFIG_MODULE_*=y` 的拼写

### 依赖不满足

```
warning: CONFIG_X was assigned the value 'y' but got the value 'n'
```

**原因**：`CONFIG_X` 依赖另一个未启用的 `CONFIG_Y`。

**修复**：在 `prj.conf` 中同时启用依赖项，或调整 `depends on` 条件。

## 警告信号

以下警告虽不阻塞构建，但应关注：

| 警告 | 含义 |
|------|------|
| `unused variable` | 可能遗漏了使用该变量的逻辑 |
| `comparison between signed and unsigned` | 潜在整数溢出 |
| `cast increases required alignment` | 内存对齐问题，ARM Cortex-M 上可能导致 HardFault |
| `control reaches end of non-void function` | 缺少 return 语句，未定义行为 |
