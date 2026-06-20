# 测试脚手架模板

CubeMot 单元测试的标准模板，从现有测试（pid、crc、math、serial_protocol）提取。

## 测试文件模板

### 纯数学/算法库（无 Zephyr 依赖）

适用于 `src/libs/` 下的纯算法模块。

```cpp
// <Module> Test Suite
// Tests for <description>

#include <gtest/gtest.h>
#include <cmath>

extern "C" {
#include "<module>.h"  // 或 "libs/<module>/<module>.h"
}

namespace cubemot::test
{

// ============================================================================
// <Feature> Tests
// ============================================================================

class <Module>Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // 初始化被测对象
    }

    void TearDown() override
    {
        // 清理（如需要）
    }
};

TEST_F(<Module>Test, Init_DefaultState_IsValid)
{
    // 验证默认初始化状态
}

TEST_F(<Module>Test, <Feature>_<Condition>_<ExpectedResult>)
{
    //  Arrange: 准备输入
    //  Act:     调用被测函数
    //  Assert:  验证结果
    EXPECT_NEAR(actual, expected, tolerance);
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_F(<Module>Test, Boundary_ZeroInput_ReturnsZero)
{
    // 零值 / 空输入
}

TEST_F(<Module>Test, Boundary_MaxInput_ClampsOrWraps)
{
    // 最大值 / 溢出
}

// ============================================================================
// 往返测试（如适用）
// ============================================================================

TEST(<Module>RoundTrip, Forward_Inverse_Identity)
{
    // forward(inverse(x)) == x
}

} // namespace cubemot::test
```

### 使用 Zephyr API 的模块

适用于 `src/modules/` 下需要 Zephyr mock 的模块。

```cpp
// <Module> Module Test Suite

#include <gtest/gtest.h>

extern "C" {
#include "<module>.h"
}

namespace cubemot::test
{

class <Module>Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // 初始化模块 + mock 状态
    }
};

TEST_F(<Module>Test, Init_ReturnsSuccess)
{
    int ret = <module>_init();
    EXPECT_EQ(ret, 0);
}

TEST_F(<Module>Test, Process_ValidInput_ProducesExpectedOutput)
{
    // 测试核心处理逻辑
}

TEST_F(<Module>Test, ErrorHandling_InvalidInput_ReturnsError)
{
    // 测试错误路径
    int ret = <module>_process(NULL);
    EXPECT_EQ(ret, -EINVAL);
}

} // namespace cubemot::test
```

## CMakeLists.txt 模板

### 链接独立库（最常见）

```cmake
add_executable(<module>-test
    test_<module>.cpp
)

target_link_libraries(<module>-test PRIVATE
    <cmake_library_target>
    GTest::gtest_main
)

target_include_directories(<module>-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(<module>-test)
```

### Header-only 库（如 math）

```cmake
add_executable(<module>-test
    test_<module>.cpp
)

target_include_directories(<module>-test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/libs/<module>
)

target_link_libraries(<module>-test PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(<module>-test)
```

### 需要 Zephyr Mock 的模块

```cmake
add_executable(<module>-test
    test_<module>.cpp
)

target_compile_definitions(<module>-test PRIVATE
    UNIT_TEST_HOST
)

target_link_libraries(<module>-test PRIVATE
    <cmake_library_target>
    GTest::gtest_main
    zephyr_mocks
)

target_include_directories(<module>-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(<module>-test)
```

## 注册步骤清单

生成脚手架后，按顺序完成以下注册：

| 步骤 | 文件 | 操作 |
|------|------|------|
| 1 | `tests/lib/<module>/CMakeLists.txt` | 创建（上面的模板） |
| 2 | `tests/lib/<module>/test_<module>.cpp` | 创建（上面的模板） |
| 3 | `tests/lib/CMakeLists.txt` | 添加 `add_subdirectory(<module>)` |
| 4 | `src/libs/<module>/CMakeLists.txt` | 确认有 `add_library()`（若无则创建） |
| 5 | `tests/mocks/zephyr/` | 添加所需的 mock 头文件（若模块使用 Zephyr API） |

## 现有库目标名称参考

| 模块路径 | CMake target 名 | 类型 |
|----------|-----------------|------|
| `src/libs/pid/` | `pid` | library |
| `src/libs/crc/` | `lib_crc` | library |
| `src/libs/serial_protocol/` | `lib_serial_protocol` | library（依赖 `lib_crc`） |
| `src/libs/math/` | （无，header-only） | header-only |
| `src/libs/foc/` | （无，待创建） | 需创建 library |
| `src/msghub/` | `msghub` | library（需 `zephyr_mocks`） |

> **注意**：库命名不一致（`pid` vs `lib_crc`）。生成 CMakeLists 时，先读取模块的 `CMakeLists.txt` 确认实际 target 名。
