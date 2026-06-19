// Serial Protocol Codec Test Suite
// Tests for frame encode/decode and CRC8 integration

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

extern "C" {
#include "serial_protocol.h"
}

namespace cubemot::test
{

// ============================================================================
// CRC8 Integration Tests
// ============================================================================

class SpCrc8Test : public ::testing::Test {};

TEST_F(SpCrc8Test, EmptyData_ReturnsZero)
{
    uint8_t data[] = {};
    EXPECT_EQ(sp_crc8(data, 0), 0x00);
}

TEST_F(SpCrc8Test, KnownValue_StandardVector)
{
    // "123456789" with CRC8 poly 0x07 → 0xF4
    const uint8_t data[] = "123456789";
    EXPECT_EQ(sp_crc8(data, 9), 0xF4u);
}

TEST_F(SpCrc8Test, NullData_ReturnsZero)
{
    EXPECT_EQ(sp_crc8(nullptr, 10), 0u);
}

// ============================================================================
// Frame Encoding Tests
// ============================================================================

class SpEncodeTest : public ::testing::Test {
protected:
    uint8_t buf_[SP_MAX_FRAME];

    void SetUp() override {
        memset(buf_, 0xCC, sizeof(buf_));
    }
};

TEST_F(SpEncodeTest, ZeroPayload_MinimumFrame)
{
    // cmd_id=0x06 (PING), len=0 → frame = [AA 55 06 00 CRC]
    size_t len = sp_encode_frame(buf_, 0x06, nullptr, 0);
    EXPECT_EQ(len, 5u);
    EXPECT_EQ(buf_[0], SP_FRAME_HEAD_0);
    EXPECT_EQ(buf_[1], SP_FRAME_HEAD_1);
    EXPECT_EQ(buf_[2], 0x06);
    EXPECT_EQ(buf_[3], 0x00);
    // CRC over [0x06, 0x00]
    uint8_t expected_crc = sp_crc8(&buf_[2], 2);
    EXPECT_EQ(buf_[4], expected_crc);
}

TEST_F(SpEncodeTest, WithPayload_VelCommand)
{
    // CMD_VEL (0x01) with 8 bytes payload
    float linear_x = 1.5f;
    float angular_z = 0.3f;
    uint8_t payload[8];
    memcpy(&payload[0], &linear_x, 4);
    memcpy(&payload[4], &angular_z, 4);

    size_t len = sp_encode_frame(buf_, 0x01, payload, 8);
    EXPECT_EQ(len, 13u);  // 2 + 1 + 1 + 8 + 1
    EXPECT_EQ(buf_[0], 0xAA);
    EXPECT_EQ(buf_[1], 0x55);
    EXPECT_EQ(buf_[2], 0x01);
    EXPECT_EQ(buf_[3], 0x08);

    // Verify payload
    float decoded_lx, decoded_az;
    memcpy(&decoded_lx, &buf_[4], 4);
    memcpy(&decoded_az, &buf_[8], 4);
    EXPECT_FLOAT_EQ(decoded_lx, 1.5f);
    EXPECT_FLOAT_EQ(decoded_az, 0.3f);

    // Verify CRC
    uint8_t expected_crc = sp_crc8(&buf_[2], 2 + 8);
    EXPECT_EQ(buf_[12], expected_crc);
}

TEST_F(SpEncodeTest, MaxPayload)
{
    uint8_t payload[SP_MAX_PAYLOAD];
    memset(payload, 0xAB, sizeof(payload));

    size_t len = sp_encode_frame(buf_, 0x10, payload, SP_MAX_PAYLOAD);
    EXPECT_EQ(len, (size_t)SP_MAX_FRAME);
}

TEST_F(SpEncodeTest, NullBuffer_ReturnsZero)
{
    uint8_t payload[] = {0x01};
    EXPECT_EQ(sp_encode_frame(nullptr, 0x01, payload, 1), 0u);
}

TEST_F(SpEncodeTest, PayloadTooLong_ReturnsZero)
{
    uint8_t payload[SP_MAX_PAYLOAD + 1];
    EXPECT_EQ(sp_encode_frame(buf_, 0x01, payload, SP_MAX_PAYLOAD + 1), 0u);
}

TEST_F(SpEncodeTest, NullPayloadWithNonZeroLen_ReturnsZero)
{
    EXPECT_EQ(sp_encode_frame(buf_, 0x01, nullptr, 5), 0u);
}

TEST_F(SpEncodeTest, AllCommandIds)
{
    // Test encoding with each command ID
    for (uint16_t cmd = 0; cmd < 256; cmd++) {
        size_t len = sp_encode_frame(buf_, (uint8_t)cmd, nullptr, 0);
        EXPECT_EQ(len, 5u);
        EXPECT_EQ(buf_[2], (uint8_t)cmd);
    }
}

// ============================================================================
// Frame Decoding Tests
// ============================================================================

class SpDecodeTest : public ::testing::Test {
protected:
    uint8_t buf_[SP_MAX_FRAME];
    sp_frame_t frame_;

    void SetUp() override {
        memset(buf_, 0, sizeof(buf_));
        memset(&frame_, 0, sizeof(frame_));
    }

    // Helper: encode then decode
    int RoundTrip(uint8_t cmd_id, const uint8_t *payload, uint8_t len) {
        size_t encoded = sp_encode_frame(buf_, cmd_id, payload, len);
        if (encoded == 0) return -1;
        return sp_decode_frame(buf_, encoded, &frame_);
    }
};

TEST_F(SpDecodeTest, RoundTrip_ZeroPayload)
{
    int consumed = RoundTrip(0x06, nullptr, 0);  // PING
    EXPECT_EQ(consumed, 5);
    EXPECT_EQ(frame_.cmd_id, 0x06);
    EXPECT_EQ(frame_.len, 0);
}

TEST_F(SpDecodeTest, RoundTrip_WithPayload)
{
    uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    int consumed = RoundTrip(0x01, payload, 4);
    EXPECT_EQ(consumed, 9);  // 4 + 4 + 1
    EXPECT_EQ(frame_.cmd_id, 0x01);
    EXPECT_EQ(frame_.len, 4);
    EXPECT_EQ(frame_.payload[0], 0xDE);
    EXPECT_EQ(frame_.payload[1], 0xAD);
    EXPECT_EQ(frame_.payload[2], 0xBE);
    EXPECT_EQ(frame_.payload[3], 0xEF);
}

TEST_F(SpDecodeTest, RoundTrip_MaxPayload)
{
    uint8_t payload[SP_MAX_PAYLOAD];
    for (int i = 0; i < SP_MAX_PAYLOAD; i++) payload[i] = (uint8_t)i;

    int consumed = RoundTrip(0x80, payload, SP_MAX_PAYLOAD);
    EXPECT_EQ(consumed, SP_MAX_FRAME);
    EXPECT_EQ(frame_.cmd_id, 0x80);
    EXPECT_EQ(frame_.len, SP_MAX_PAYLOAD);
    for (int i = 0; i < SP_MAX_PAYLOAD; i++) {
        EXPECT_EQ(frame_.payload[i], (uint8_t)i);
    }
}

TEST_F(SpDecodeTest, NullBuffer_ReturnsError)
{
    EXPECT_EQ(sp_decode_frame(nullptr, 10, &frame_), -1);
}

TEST_F(SpDecodeTest, NullFrame_ReturnsError)
{
    sp_encode_frame(buf_, 0x01, nullptr, 0);
    EXPECT_EQ(sp_decode_frame(buf_, 5, nullptr), -1);
}

TEST_F(SpDecodeTest, TooShort_ReturnsError)
{
    buf_[0] = 0xAA;
    buf_[1] = 0x55;
    buf_[2] = 0x01;
    EXPECT_EQ(sp_decode_frame(buf_, 3, &frame_), -1);
}

TEST_F(SpDecodeTest, BadHeader0_ReturnsError)
{
    sp_encode_frame(buf_, 0x01, nullptr, 0);
    buf_[0] = 0x00;  // corrupt header
    EXPECT_EQ(sp_decode_frame(buf_, 5, &frame_), -1);
}

TEST_F(SpDecodeTest, BadHeader1_ReturnsError)
{
    sp_encode_frame(buf_, 0x01, nullptr, 0);
    buf_[1] = 0x00;  // corrupt header
    EXPECT_EQ(sp_decode_frame(buf_, 5, &frame_), -1);
}

TEST_F(SpDecodeTest, BadCrc_ReturnsError)
{
    sp_encode_frame(buf_, 0x01, nullptr, 0);
    buf_[4] ^= 0xFF;  // corrupt CRC
    EXPECT_EQ(sp_decode_frame(buf_, 5, &frame_), -1);
}

TEST_F(SpDecodeTest, PayloadLenExceedsMax_ReturnsError)
{
    buf_[0] = 0xAA;
    buf_[1] = 0x55;
    buf_[2] = 0x01;
    buf_[3] = SP_MAX_PAYLOAD + 1;  // invalid len
    buf_[4] = 0x00;
    EXPECT_EQ(sp_decode_frame(buf_, 5, &frame_), -1);
}

TEST_F(SpDecodeTest, InsufficientDataForPayload_ReturnsError)
{
    sp_encode_frame(buf_, 0x01, (const uint8_t *)"\x01\x02\x03\x04", 4);
    // Only provide partial frame
    EXPECT_EQ(sp_decode_frame(buf_, 6, &frame_), -1);
}

// ============================================================================
// Encode↔Decode Symmetry Tests
// ============================================================================

class SpRoundTripTest : public ::testing::TestWithParam<uint8_t> {};

TEST_P(SpRoundTripTest, EncodeDecodeSymmetry)
{
    uint8_t buf[SP_MAX_FRAME];
    sp_frame_t frame;
    uint8_t cmd_id = GetParam();

    // Various payload sizes
    for (uint8_t plen = 0; plen <= 8; plen++) {
        uint8_t payload[8];
        for (int i = 0; i < plen; i++) payload[i] = (uint8_t)(cmd_id + i);

        size_t encoded = sp_encode_frame(buf, cmd_id, payload, plen);
        ASSERT_GT(encoded, 0u);

        int consumed = sp_decode_frame(buf, encoded, &frame);
        EXPECT_EQ(consumed, (int)encoded);
        EXPECT_EQ(frame.cmd_id, cmd_id);
        EXPECT_EQ(frame.len, plen);
        for (int i = 0; i < plen; i++) {
            EXPECT_EQ(frame.payload[i], (uint8_t)(cmd_id + i));
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    CommandIds,
    SpRoundTripTest,
    ::testing::Values(0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x10, 0x80, 0x81, 0x82, 0x86)
);

// ============================================================================
// Frame with extra data in buffer
// ============================================================================

TEST(SpDecodeExtraTest, IgnoresTrailingBytes)
{
    uint8_t buf[SP_MAX_FRAME + 10];
    sp_frame_t frame;

    // Encode a frame
    size_t len = sp_encode_frame(buf, 0x06, nullptr, 0);
    ASSERT_EQ(len, 5u);

    // Add trailing garbage
    buf[5] = 0xFF;
    buf[6] = 0xFF;

    // Should still decode correctly and report only consumed bytes
    int consumed = sp_decode_frame(buf, len + 2, &frame);
    EXPECT_EQ(consumed, 5);
    EXPECT_EQ(frame.cmd_id, 0x06);
}

// ============================================================================
// Vel command float encoding/decoding
// ============================================================================

TEST(SpVelTest, FloatPayloadRoundTrip)
{
    uint8_t buf[SP_MAX_FRAME];
    sp_frame_t frame;

    float linear_x = 2.5f;
    float angular_z = -1.2f;
    uint8_t payload[8];
    memcpy(&payload[0], &linear_x, 4);
    memcpy(&payload[4], &angular_z, 4);

    size_t len = sp_encode_frame(buf, 0x01, payload, 8);
    ASSERT_GT(len, 0u);

    int consumed = sp_decode_frame(buf, len, &frame);
    ASSERT_EQ(consumed, (int)len);

    float decoded_lx, decoded_az;
    memcpy(&decoded_lx, &frame.payload[0], 4);
    memcpy(&decoded_az, &frame.payload[4], 4);
    EXPECT_FLOAT_EQ(decoded_lx, 2.5f);
    EXPECT_FLOAT_EQ(decoded_az, -1.2f);
}

} // namespace cubemot::test
