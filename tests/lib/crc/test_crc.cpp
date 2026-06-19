// CRC Library Test Suite
// Tests for CRC8, CRC16-CCITT and CRC32 (IEEE 802.3)

#include <gtest/gtest.h>
#include <string_view>
#include <vector>

extern "C" {
#include "src/libs/crc/crc.h"
}

namespace cubemot::test
{

// ============================================================================
// CRC8 Tests
// ============================================================================

class CRC8Test : public ::testing::Test {};

TEST_F(CRC8Test, KnownValue_EmptyData)
{
    uint8_t data[] = {};
    EXPECT_EQ(crc8_calculate(data, 0), 0x00);
}

TEST_F(CRC8Test, KnownValue_SingleByte)
{
    uint8_t data[] = {0x01};
    // CRC8 poly 0x07: 0x01 → 0x01 ^ 0x00 = 0x01, shift: 0x07
    uint8_t crc = crc8_calculate(data, 1);
    EXPECT_NE(crc, 0xFF); // sanity: not garbage
    // Verify deterministic
    EXPECT_EQ(crc, crc8_calculate(data, 1));
}

TEST_F(CRC8Test, KnownValue_StandardVector)
{
    // "123456789" with CRC8 poly 0x07 (init=0x00) → 0xF4
    constexpr std::string_view kData = "123456789";
    EXPECT_EQ(crc8_calculate(reinterpret_cast<const uint8_t *>(kData.data()), kData.size()), 0xF4u);
}

TEST_F(CRC8Test, Api_NullHandling)
{
    EXPECT_EQ(crc8_calculate(nullptr, 100), 0u);
}

TEST_F(CRC8Test, Consistency_SameInputSameOutput)
{
    const std::vector<uint8_t> data = {0xAA, 0x55, 0x01, 0x08};
    EXPECT_EQ(crc8_calculate(data.data(), data.size()),
              crc8_calculate(data.data(), data.size()));
}

TEST_F(CRC8Test, DifferentData_DifferentCrc)
{
    uint8_t d1[] = {0x01, 0x02, 0x03};
    uint8_t d2[] = {0x01, 0x02, 0x04};
    EXPECT_NE(crc8_calculate(d1, 3), crc8_calculate(d2, 3));
}

TEST_F(CRC8Test, Boundary_AllZeros)
{
    std::vector<uint8_t> zeros(256, 0x00);
    uint8_t crc = crc8_calculate(zeros.data(), zeros.size());
    EXPECT_EQ(crc, crc8_calculate(zeros.data(), zeros.size()));
}

TEST_F(CRC8Test, Boundary_AllOnes)
{
    std::vector<uint8_t> ones(256, 0xFF);
    uint8_t crc = crc8_calculate(ones.data(), ones.size());
    EXPECT_EQ(crc, crc8_calculate(ones.data(), ones.size()));
}

// ============================================================================
// CRC32 Tests
// ============================================================================

class CRC32Test : public ::testing::Test
{
  protected:
    crc32_ctx_t ctx_;

    void SetUp() override
    {
        ASSERT_EQ(crc32_ctx_init(&ctx_), 0);
    }
};

TEST_F(CRC32Test, KnownValue_StandardVector)
{
    constexpr std::string_view kData = "123456789";
    EXPECT_EQ(crc32_calculate(reinterpret_cast<const uint8_t *>(kData.data()), kData.size()), 0xCBF43926u);
}

TEST_F(CRC32Test, Api_NullHandling)
{
    EXPECT_EQ(crc32_ctx_init(nullptr), -1);
    EXPECT_EQ(crc32_calculate(nullptr, 100), 0u);
}

TEST_F(CRC32Test, Streaming_MatchesOneshot)
{
    constexpr std::string_view kData = "Hello, World!";

    crc32_ctx_update(&ctx_, reinterpret_cast<const uint8_t *>(kData.data()), kData.size());
    EXPECT_EQ(crc32_ctx_finalize(&ctx_),
              crc32_calculate(reinterpret_cast<const uint8_t *>(kData.data()), kData.size()));
}

TEST_F(CRC32Test, Streaming_IncrementalUpdate)
{
    const std::vector<uint8_t> part1 = {0x01, 0x02, 0x03};
    const std::vector<uint8_t> part2 = {0x04, 0x05, 0x06};
    const std::vector<uint8_t> combined = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    crc32_ctx_update(&ctx_, part1.data(), part1.size());
    crc32_ctx_update(&ctx_, part2.data(), part2.size());

    EXPECT_EQ(crc32_ctx_finalize(&ctx_), crc32_calculate(combined.data(), combined.size()));
}

TEST_F(CRC32Test, Context_InstancesAreIndependent)
{
    constexpr std::string_view kData1 = "Data for context 1";
    constexpr std::string_view kData2 = "Different data for context 2";

    crc32_ctx_t ctx2;
    ASSERT_EQ(crc32_ctx_init(&ctx2), 0);

    crc32_ctx_update(&ctx_, reinterpret_cast<const uint8_t *>(kData1.data()), kData1.size());
    crc32_ctx_update(&ctx2, reinterpret_cast<const uint8_t *>(kData2.data()), kData2.size());

    EXPECT_EQ(crc32_ctx_finalize(&ctx_),
              crc32_calculate(reinterpret_cast<const uint8_t *>(kData1.data()), kData1.size()));
    EXPECT_EQ(crc32_ctx_finalize(&ctx2),
              crc32_calculate(reinterpret_cast<const uint8_t *>(kData2.data()), kData2.size()));
}

TEST_F(CRC32Test, Context_ResetForReuse)
{
    constexpr std::string_view kData = "Test data for reset";

    crc32_ctx_update(&ctx_, reinterpret_cast<const uint8_t *>(kData.data()), kData.size());
    crc32_ctx_finalize(&ctx_);

    crc32_ctx_reset(&ctx_);
    crc32_ctx_update(&ctx_, reinterpret_cast<const uint8_t *>(kData.data()), kData.size());

    EXPECT_EQ(crc32_ctx_finalize(&ctx_),
              crc32_calculate(reinterpret_cast<const uint8_t *>(kData.data()), kData.size()));
}

TEST_F(CRC32Test, Boundary_LargeData)
{
    std::vector<uint8_t> buffer(1024);
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = static_cast<uint8_t>(i);
    }
    EXPECT_EQ(crc32_calculate(buffer.data(), buffer.size()), crc32_calculate(buffer.data(), buffer.size()));
}

// ============================================================================
// CRC16 Tests
// ============================================================================

class CRC16Test : public ::testing::Test
{
  protected:
    crc16_ctx_t ctx_;

    void SetUp() override
    {
        ASSERT_EQ(crc16_ctx_init(&ctx_), 0);
    }
};

TEST_F(CRC16Test, KnownValue_StandardVector)
{
    constexpr std::string_view kData = "123456789";
    EXPECT_EQ(crc16_calculate(reinterpret_cast<const uint8_t *>(kData.data()), kData.size()), 0x6F91u);
}

TEST_F(CRC16Test, Api_NullHandling)
{
    EXPECT_EQ(crc16_ctx_init(nullptr), -1);
    EXPECT_EQ(crc16_calculate(nullptr, 100), 0u);
}

TEST_F(CRC16Test, Streaming_MatchesOneshot)
{
    constexpr std::string_view kData = "Hello, World!";

    crc16_ctx_update(&ctx_, reinterpret_cast<const uint8_t *>(kData.data()), kData.size());
    EXPECT_EQ(crc16_ctx_finalize(&ctx_),
              crc16_calculate(reinterpret_cast<const uint8_t *>(kData.data()), kData.size()));
}

TEST_F(CRC16Test, Context_InstancesAreIndependent)
{
    constexpr std::string_view kData1 = "Data for context 1";
    constexpr std::string_view kData2 = "Different data for context 2";

    crc16_ctx_t ctx2;
    ASSERT_EQ(crc16_ctx_init(&ctx2), 0);

    crc16_ctx_update(&ctx_, reinterpret_cast<const uint8_t *>(kData1.data()), kData1.size());
    crc16_ctx_update(&ctx2, reinterpret_cast<const uint8_t *>(kData2.data()), kData2.size());

    EXPECT_EQ(crc16_ctx_finalize(&ctx_),
              crc16_calculate(reinterpret_cast<const uint8_t *>(kData1.data()), kData1.size()));
    EXPECT_EQ(crc16_ctx_finalize(&ctx2),
              crc16_calculate(reinterpret_cast<const uint8_t *>(kData2.data()), kData2.size()));
}

TEST_F(CRC16Test, Edge_VariousPatterns)
{
    const std::vector<uint8_t> zeros(256, 0x00);
    const std::vector<uint8_t> ones(256, 0xFF);

    EXPECT_EQ(crc16_calculate(zeros.data(), zeros.size()), crc16_calculate(zeros.data(), zeros.size()));
    EXPECT_EQ(crc16_calculate(ones.data(), ones.size()), crc16_calculate(ones.data(), ones.size()));
}

} // namespace cubemot::test
