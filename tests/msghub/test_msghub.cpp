// MSGHUB Test Suite
//
// Test Naming Convention: Feature_Condition_Result
//   Example: Publisher_Create_ValidTopic_ReturnsValidHandle
//
// Test Fixtures:
//   - MsgHubTest: Base fixture, fresh msghub instance per test
//   - MsgHubPublisherTest: Auto-manages publisher lifecycle
//   - MsgHubSubscriberTest: Auto-manages publisher + subscriber lifecycle

#include <gtest/gtest.h>

extern "C" {
#include "msghub/msghub.h"
#include "msghub_state.h"

typedef struct {
    uint32_t timestamp;
    int32_t val;
} msghub_test_t;

MSGHUB_TOPIC_DEFINE(msghub_test, msghub_test_t, 10);
}

// =============================================================================
// Test Fixtures
// =============================================================================

// Base fixture: Fresh msghub instance for each test
class MsgHubTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        msghub_reset();
    }

    void TearDown() override
    {
        msghub_reset();
    }
};

// Publisher fixture: Auto-manages single-instance publisher lifecycle
class MsgHubPublisherTest : public MsgHubTest
{
  protected:
    msghub_publisher_t pub_ = MSGHUB_PUBLISHER_INVALID;

    void SetUp() override
    {
        MsgHubTest::SetUp();
        pub_ = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    }

    void TearDown() override
    {
        if (msghub_publisher_valid(pub_)) {
            msghub_destroy_publisher(pub_);
        }
        MsgHubTest::TearDown();
    }
};

// Subscriber fixture: Auto-manages publisher + subscriber lifecycle
class MsgHubSubscriberTest : public MsgHubTest
{
  protected:
    msghub_publisher_t pub_ = MSGHUB_PUBLISHER_INVALID;
    msghub_subscriber_t sub_ = MSGHUB_SUBSCRIBER_INVALID;

    void SetUp() override
    {
        MsgHubTest::SetUp();
        pub_ = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
        sub_ = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    }

    void TearDown() override
    {
        if (msghub_subscriber_valid(sub_)) {
            msghub_destroy_subscriber(sub_);
        }
        if (msghub_publisher_valid(pub_)) {
            msghub_destroy_publisher(pub_);
        }
        MsgHubTest::TearDown();
    }
};

// =============================================================================
// Publisher Tests
// =============================================================================

// -----------------------------------------------------------------------------
// Creation
// -----------------------------------------------------------------------------

TEST_F(MsgHubTest, Publisher_Create_ValidTopic_ReturnsValidHandle)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    EXPECT_NE(pub, MSGHUB_PUBLISHER_INVALID);
}

TEST_F(MsgHubTest, Publisher_Create_ValidTopic_IsValid)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    EXPECT_TRUE(msghub_publisher_valid(pub));
}

// -----------------------------------------------------------------------------
// Destruction
// -----------------------------------------------------------------------------

TEST_F(MsgHubTest, Publisher_Destroy_ValidHandle_ReturnsOk)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_err_t err = msghub_destroy_publisher(pub);
    EXPECT_EQ(err, MSGHUB_OK);
}

TEST_F(MsgHubTest, Publisher_Destroy_ValidHandle_MakesInvalid)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_destroy_publisher(pub);
    EXPECT_FALSE(msghub_publisher_valid(pub));
}

TEST_F(MsgHubTest, Publisher_Destroy_InvalidHandle_ReturnsError)
{
    msghub_err_t err = msghub_destroy_publisher(MSGHUB_PUBLISHER_INVALID);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);
}

// -----------------------------------------------------------------------------
// Publish
// -----------------------------------------------------------------------------

TEST_F(MsgHubPublisherTest, Publish_ValidData_ReturnsOk)
{
    msghub_test_t data = {.timestamp = 1000, .val = 42};
    msghub_err_t err = msghub_publish(pub_, &data);
    EXPECT_EQ(err, MSGHUB_OK);
}

TEST_F(MsgHubPublisherTest, Publish_NullData_ReturnsError)
{
    msghub_err_t err = msghub_publish(pub_, NULL);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);
}

TEST_F(MsgHubTest, Publisher_Destroyed_CannotPublish)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_destroy_publisher(pub);

    msghub_test_t data = {.val = 42};
    msghub_err_t err = msghub_publish(pub, &data);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);
}

// =============================================================================
// Subscriber Tests
// =============================================================================

// -----------------------------------------------------------------------------
// Creation
// -----------------------------------------------------------------------------

TEST_F(MsgHubTest, Subscriber_Create_ValidTopic_ReturnsValidHandle)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_subscriber_t sub = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    EXPECT_NE(sub, MSGHUB_SUBSCRIBER_INVALID);

    msghub_destroy_subscriber(sub);
    msghub_destroy_publisher(pub);
}

TEST_F(MsgHubTest, Subscriber_Create_ValidTopic_IsValid)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_subscriber_t sub = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    EXPECT_TRUE(msghub_subscriber_valid(sub));

    msghub_destroy_subscriber(sub);
    msghub_destroy_publisher(pub);
}

// -----------------------------------------------------------------------------
// Destruction
// -----------------------------------------------------------------------------

TEST_F(MsgHubTest, Subscriber_Destroy_ValidHandle_ReturnsOk)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_subscriber_t sub = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    ASSERT_NE(sub, MSGHUB_SUBSCRIBER_INVALID);

    msghub_err_t err = msghub_destroy_subscriber(sub);
    EXPECT_EQ(err, MSGHUB_OK);

    msghub_destroy_publisher(pub);
}

TEST_F(MsgHubTest, Subscriber_Destroy_ValidHandle_MakesInvalid)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_subscriber_t sub = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    ASSERT_NE(sub, MSGHUB_SUBSCRIBER_INVALID);

    msghub_destroy_subscriber(sub);
    EXPECT_FALSE(msghub_subscriber_valid(sub));

    msghub_destroy_publisher(pub);
}

TEST_F(MsgHubTest, Subscriber_Destroy_InvalidHandle_ReturnsError)
{
    msghub_err_t err = msghub_destroy_subscriber(MSGHUB_SUBSCRIBER_INVALID);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);
}

// -----------------------------------------------------------------------------
// Receive
// -----------------------------------------------------------------------------

TEST_F(MsgHubSubscriberTest, Receive_PublishedData_ReturnsOk)
{
    msghub_test_t published = {.timestamp = 1000, .val = 42};
    msghub_test_t received = {0};

    msghub_publish(pub_, &published);
    msghub_err_t err = msghub_receive(sub_, &received);

    EXPECT_EQ(err, MSGHUB_OK);
    EXPECT_EQ(received.timestamp, 1000);
    EXPECT_EQ(received.val, 42);
}

TEST_F(MsgHubSubscriberTest, Receive_NullBuffer_ReturnsError)
{
    msghub_err_t err = msghub_receive(sub_, NULL);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);
}

TEST_F(MsgHubTest, Subscriber_Destroyed_CannotReceive)
{
    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_subscriber_t sub = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    ASSERT_NE(sub, MSGHUB_SUBSCRIBER_INVALID);

    msghub_destroy_subscriber(sub);

    msghub_test_t received = {0};
    msghub_err_t err = msghub_receive(sub, &received);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);

    msghub_destroy_publisher(pub);
}

// =============================================================================
// Subscriber Check API Tests
// =============================================================================

TEST_F(MsgHubSubscriberTest, Check_NoUpdate_ReturnsFalse)
{
    bool updated = true;
    msghub_test_t initial_data = {0};

    msghub_publish(pub_, &initial_data);
    msghub_test_t dummy;
    msghub_receive(sub_, &dummy);

    msghub_err_t err = msghub_subscriber_check(sub_, &updated);

    EXPECT_EQ(err, MSGHUB_OK);
    EXPECT_FALSE(updated);
}

TEST_F(MsgHubSubscriberTest, Check_AfterPublish_ReturnsTrue)
{
    bool updated = false;
    msghub_test_t data = {.val = 1};
    msghub_test_t initial_data = {0};

    msghub_publish(pub_, &initial_data);
    msghub_test_t dummy;
    msghub_receive(sub_, &dummy);

    msghub_publish(pub_, &data);
    msghub_err_t err = msghub_subscriber_check(sub_, &updated);

    EXPECT_EQ(err, MSGHUB_OK);
    EXPECT_TRUE(updated);
}

TEST_F(MsgHubSubscriberTest, Check_AfterConsume_ReturnsFalse)
{
    bool updated = false;
    msghub_test_t data = {.val = 1};
    msghub_test_t received = {0};
    msghub_test_t initial_data = {0};

    msghub_publish(pub_, &initial_data);
    msghub_receive(sub_, &received);

    msghub_publish(pub_, &data);
    msghub_subscriber_check(sub_, &updated);
    EXPECT_TRUE(updated);

    msghub_receive(sub_, &received);
    msghub_subscriber_check(sub_, &updated);
    EXPECT_FALSE(updated);
}

TEST_F(MsgHubSubscriberTest, Check_NullUpdated_ReturnsError)
{
    msghub_err_t err = msghub_subscriber_check(sub_, NULL);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);
}

// =============================================================================
// Subscriber Update/Poll API Tests
// =============================================================================

TEST_F(MsgHubSubscriberTest, Update_NoUpdate_ReturnsFalse)
{
    msghub_test_t received = {0};
    bool updated = false;

    msghub_err_t err = msghub_subscriber_update(sub_, &received, &updated);
    EXPECT_EQ(err, MSGHUB_OK);
    EXPECT_FALSE(updated);
}

TEST_F(MsgHubSubscriberTest, Update_AfterPublish_ReturnsTrueWithData)
{
    msghub_test_t received = {0};
    bool updated = false;
    msghub_test_t data = {.val = 42};

    msghub_publish(pub_, &data);

    msghub_err_t err = msghub_subscriber_update(sub_, &received, &updated);
    EXPECT_EQ(err, MSGHUB_OK);
    EXPECT_TRUE(updated);
    EXPECT_EQ(received.val, 42);
}

TEST_F(MsgHubSubscriberTest, Update_AfterConsume_ReturnsFalse)
{
    msghub_test_t received = {0};
    bool updated = false;
    msghub_test_t data = {.val = 42};

    msghub_publish(pub_, &data);
    msghub_subscriber_update(sub_, &received, &updated);

    msghub_err_t err = msghub_subscriber_update(sub_, &received, &updated);
    EXPECT_EQ(err, MSGHUB_OK);
    EXPECT_FALSE(updated);
}

TEST_F(MsgHubSubscriberTest, Poll_NoData_ReturnsFalse)
{
    bool updated = false;
    msghub_err_t err = msghub_subscriber_poll(sub_, 0, &updated);
    EXPECT_EQ(err, MSGHUB_OK);
    EXPECT_FALSE(updated);
}

TEST_F(MsgHubSubscriberTest, Poll_WithData_ReturnsTrue)
{
    bool updated = false;
    msghub_test_t data = {.val = 1};

    msghub_publish(pub_, &data);

    msghub_err_t err = msghub_subscriber_poll(sub_, 0, &updated);
    EXPECT_EQ(err, MSGHUB_OK);
    EXPECT_TRUE(updated);
}

// =============================================================================
// Multi-Instance Tests
// =============================================================================

// -----------------------------------------------------------------------------
// Publisher Multi-Instance Creation
// -----------------------------------------------------------------------------

TEST_F(MsgHubTest, Publisher_CreateMulti_SequentialInstances_CorrectAllocation)
{
    int instance = -1;
    msghub_publisher_t pubs[4];

    // Create 4 publishers sequentially, verify instance allocation
    for (int expected = 0; expected < 4; expected++) {
        instance = -1;
        pubs[expected] = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
        EXPECT_EQ(instance, expected) << "Instance " << expected;
        EXPECT_NE(pubs[expected], MSGHUB_PUBLISHER_INVALID);
    }

    // Cleanup
    for (int i = 0; i < 4; i++) {
        msghub_destroy_publisher(pubs[i]);
    }
}

// -----------------------------------------------------------------------------
// Instance Count Tracking
// -----------------------------------------------------------------------------

TEST_F(MsgHubTest, Publisher_CreateMulti_Count_TracksCorrectly)
{
    int instance = -1;
    msghub_publisher_t pub0 = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    ASSERT_NE(pub0, MSGHUB_PUBLISHER_INVALID);
    ASSERT_EQ(instance, 0);

    instance = -1;
    msghub_publisher_t pub1 = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    ASSERT_NE(pub1, MSGHUB_PUBLISHER_INVALID);
    ASSERT_EQ(instance, 1);

    instance = -1;
    msghub_publisher_t pub2 = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    ASSERT_NE(pub2, MSGHUB_PUBLISHER_INVALID);
    ASSERT_EQ(instance, 2);

    EXPECT_EQ(msghub_topic_publisher_count(MSGHUB_TOPIC(msghub_test)), 3);

    msghub_destroy_publisher(pub0);
    msghub_destroy_publisher(pub1);
    msghub_destroy_publisher(pub2);
}

// -----------------------------------------------------------------------------
// Instance Allocation Strategy
// -----------------------------------------------------------------------------

TEST_F(MsgHubTest, Instance_Allocation_SmallestFirst)
{
    int instance = -1;
    msghub_publisher_t pubs[4];

    // Allocate 0, 1, 2, 3
    for (int i = 0; i < 4; i++) {
        instance = -1;
        pubs[i] = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
        ASSERT_EQ(instance, i);
    }

    // Free 0 and 1
    msghub_destroy_publisher(pubs[0]);
    msghub_destroy_publisher(pubs[1]);

    // Should reuse 0
    instance = -1;
    msghub_publisher_t pub_new = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    EXPECT_EQ(instance, 0);

    // Then reuse 1
    instance = -1;
    msghub_publisher_t pub_new2 = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    EXPECT_EQ(instance, 1);

    msghub_destroy_publisher(pubs[2]);
    msghub_destroy_publisher(pubs[3]);
    msghub_destroy_publisher(pub_new);
    msghub_destroy_publisher(pub_new2);
}

// -----------------------------------------------------------------------------
// Instance Data Isolation
// -----------------------------------------------------------------------------

TEST_F(MsgHubTest, Instances_DataIsolation_Independent)
{
    msghub_test_t received = {0};
    msghub_test_t data = {0};
    int instance = -1;

    msghub_publisher_t pub0 = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    ASSERT_EQ(instance, 0);

    instance = -1;
    msghub_publisher_t pub1 = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    ASSERT_EQ(instance, 1);

    msghub_subscriber_t sub0 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    msghub_subscriber_t sub1 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 1);

    data.val = 100;
    msghub_publish(pub0, &data);

    data.val = 200;
    msghub_publish(pub1, &data);

    msghub_receive(sub0, &received);
    EXPECT_EQ(received.val, 100);

    msghub_receive(sub1, &received);
    EXPECT_EQ(received.val, 200);

    msghub_destroy_subscriber(sub0);
    msghub_destroy_subscriber(sub1);
    msghub_destroy_publisher(pub0);
    msghub_destroy_publisher(pub1);
}

TEST_F(MsgHubTest, Subscriber_SubscribeToSpecificInstance_ReceiveCorrectData)
{
    msghub_test_t data = {0};
    int instance = -1;

    msghub_publisher_t pub0 = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    ASSERT_EQ(instance, 0);

    instance = -1;
    msghub_publisher_t pub1 = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    ASSERT_EQ(instance, 1);

    msghub_subscriber_t sub0 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    ASSERT_NE(sub0, MSGHUB_SUBSCRIBER_INVALID);

    msghub_subscriber_t sub1 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 1);
    ASSERT_NE(sub1, MSGHUB_SUBSCRIBER_INVALID);

    data.val = 111;
    msghub_publish(pub0, &data);

    data.val = 222;
    msghub_publish(pub1, &data);

    msghub_test_t received;
    msghub_receive(sub0, &received);
    EXPECT_EQ(received.val, 111);

    msghub_receive(sub1, &received);
    EXPECT_EQ(received.val, 222);

    msghub_destroy_subscriber(sub0);
    msghub_destroy_subscriber(sub1);
    msghub_destroy_publisher(pub0);
    msghub_destroy_publisher(pub1);
}

// =============================================================================
// Capacity Limits Tests
// =============================================================================

TEST_F(MsgHubTest, Publisher_CreateMulti_MaxExceeded_ReturnsInvalid)
{
    int instance = -1;
    const int max_inst = 4;
    msghub_publisher_t pubs[4];
    msghub_test_t data = {0};

    for (int i = 0; i < max_inst; i++) {
        instance = -1;
        pubs[i] = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
        ASSERT_NE(pubs[i], MSGHUB_PUBLISHER_INVALID);
        msghub_publish(pubs[i], &data);
    }

    EXPECT_EQ(msghub_topic_publisher_count(MSGHUB_TOPIC(msghub_test)), max_inst);

    instance = -1;
    msghub_publisher_t pub_fail = msghub_create_publisher_multi(MSGHUB_TOPIC(msghub_test), &instance);
    EXPECT_EQ(pub_fail, MSGHUB_PUBLISHER_INVALID);

    for (int i = 0; i < max_inst; i++) {
        msghub_destroy_publisher(pubs[i]);
    }
}

TEST_F(MsgHubTest, Subscriber_Create_MaxExceeded_ReturnsInvalid)
{
    const int max_subs = 16;
    msghub_subscriber_t subs[16];

    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    for (int i = 0; i < max_subs; i++) {
        subs[i] = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
        ASSERT_NE(subs[i], MSGHUB_SUBSCRIBER_INVALID);
    }

    msghub_subscriber_t sub_fail = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    EXPECT_EQ(sub_fail, MSGHUB_SUBSCRIBER_INVALID);

    for (int i = 0; i < max_subs; i++) {
        msghub_destroy_subscriber(subs[i]);
    }
    msghub_destroy_publisher(pub);
}

// =============================================================================
// Invalid Handle Tests
// =============================================================================

TEST_F(MsgHubTest, Publisher_InvalidHandle_IsInvalid)
{
    msghub_publisher_t invalid_pub = MSGHUB_PUBLISHER_INVALID;
    EXPECT_FALSE(msghub_publisher_valid(invalid_pub));
}

TEST_F(MsgHubTest, Publisher_InvalidHandle_Publish_ReturnsError)
{
    msghub_test_t data = {.val = 42};
    msghub_err_t err = msghub_publish(MSGHUB_PUBLISHER_INVALID, &data);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);
}

TEST_F(MsgHubTest, Subscriber_InvalidHandle_IsInvalid)
{
    msghub_subscriber_t invalid_sub = MSGHUB_SUBSCRIBER_INVALID;
    EXPECT_FALSE(msghub_subscriber_valid(invalid_sub));
}

TEST_F(MsgHubTest, Subscriber_InvalidHandle_Receive_ReturnsError)
{
    msghub_test_t received = {0};
    msghub_err_t err = msghub_receive(MSGHUB_SUBSCRIBER_INVALID, &received);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);
}

TEST_F(MsgHubTest, Subscriber_InvalidHandle_Check_ReturnsError)
{
    bool updated = false;
    msghub_err_t err = msghub_subscriber_check(MSGHUB_SUBSCRIBER_INVALID, &updated);
    EXPECT_EQ(err, MSGHUB_ERR_INVALID);
}

// =============================================================================
// Multi-Subscriber Scenarios
// =============================================================================

TEST_F(MsgHubTest, MultipleSubscribers_ReceiveSameData)
{
    msghub_test_t data = {.val = 42};
    msghub_test_t received1 = {0};
    msghub_test_t received2 = {0};

    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_subscriber_t sub1 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    msghub_subscriber_t sub2 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);

    data.val = 123;
    msghub_publish(pub, &data);

    msghub_receive(sub1, &received1);
    msghub_receive(sub2, &received2);

    EXPECT_EQ(received1.val, 123);
    EXPECT_EQ(received2.val, 123);

    msghub_destroy_subscriber(sub1);
    msghub_destroy_subscriber(sub2);
    msghub_destroy_publisher(pub);
}

TEST_F(MsgHubTest, MultipleSubscribers_UpdateFlags_Independent)
{
    msghub_test_t data = {.val = 0};
    bool updated1 = false;
    bool updated2 = false;

    msghub_publisher_t pub = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub, MSGHUB_PUBLISHER_INVALID);

    msghub_subscriber_t sub1 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    msghub_subscriber_t sub2 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);

    data.val = 1;
    msghub_publish(pub, &data);

    msghub_subscriber_check(sub1, &updated1);
    msghub_subscriber_check(sub2, &updated2);
    EXPECT_TRUE(updated1);
    EXPECT_TRUE(updated2);

    msghub_test_t received;
    msghub_receive(sub1, &received);

    msghub_subscriber_check(sub1, &updated1);
    msghub_subscriber_check(sub2, &updated2);
    EXPECT_FALSE(updated1);
    EXPECT_TRUE(updated2);

    msghub_receive(sub2, &received);
    msghub_subscriber_check(sub2, &updated2);
    EXPECT_FALSE(updated2);

    msghub_destroy_subscriber(sub1);
    msghub_destroy_subscriber(sub2);
    msghub_destroy_publisher(pub);
}

// =============================================================================
// Lifecycle Integration Tests
// =============================================================================

TEST_F(MsgHubTest, FullLifecycle_MultipleCycles_DataIntegrity)
{
    msghub_test_t data = {0};
    msghub_test_t received = {0};

    // Cycle 1
    msghub_publisher_t pub1 = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub1, MSGHUB_PUBLISHER_INVALID);
    msghub_subscriber_t sub1 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    ASSERT_NE(sub1, MSGHUB_SUBSCRIBER_INVALID);

    data.val = 100;
    msghub_publish(pub1, &data);
    msghub_receive(sub1, &received);
    EXPECT_EQ(received.val, 100);

    msghub_destroy_subscriber(sub1);
    msghub_destroy_publisher(pub1);

    // Cycle 2
    msghub_publisher_t pub2 = msghub_create_publisher(MSGHUB_TOPIC(msghub_test));
    ASSERT_NE(pub2, MSGHUB_PUBLISHER_INVALID);
    msghub_subscriber_t sub2 = msghub_create_subscriber(MSGHUB_TOPIC(msghub_test), 0);
    ASSERT_NE(sub2, MSGHUB_SUBSCRIBER_INVALID);

    data.val = 200;
    msghub_publish(pub2, &data);
    msghub_receive(sub2, &received);
    EXPECT_EQ(received.val, 200);

    msghub_destroy_subscriber(sub2);
    msghub_destroy_publisher(pub2);
}
