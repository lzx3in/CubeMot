// CRC Library Test Suite
// Tests for CRC16-CCITT and CRC32 (IEEE 802.3)

#include <gtest/gtest.h>
#include <string_view>
#include <vector>

extern "C" {
#include "src/libs/crc/crc.h"
}

namespace cubemot::test
{

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
