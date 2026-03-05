#ifndef MSGHUB_H
#define MSGHUB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Type definitions
// ============================================================================

typedef const struct msghub_topic *msghub_topic_t;
typedef uint16_t msghub_publisher_t;
typedef uint16_t msghub_subscriber_t;

typedef enum {
    MSGHUB_OK = 0,
    MSGHUB_ERR_INVALID = -1,
    MSGHUB_ERR_NOT_FOUND = -2,
    MSGHUB_ERR_NO_MEM = -3,
    MSGHUB_ERR_TIMEOUT = -5,
} msghub_err_t;

#define MSGHUB_PUBLISHER_INVALID 0xFFFF
#define MSGHUB_SUBSCRIBER_INVALID 0xFFFF
#define MSGHUB_TOPIC_INVALID NULL

// ============================================================================
// Topic 定义结构（只读，常量）
// ============================================================================

struct msghub_topic {
    const char *name;    // Topic 名称
    uint16_t msg_size;   // 消息大小（字节）
    uint16_t queue_size; // 队列深度（预留，当前未使用）
    uint8_t id;          // Topic ID（0 = 自动分配）
};

// ============================================================================
// Topic API
// ============================================================================

// 在头文件中声明一个 Topic
#define MSGHUB_TOPIC_DECLARE(_name) extern const struct msghub_topic __msghub_##_name

// 在源文件中定义一个 Topic
#define MSGHUB_TOPIC_DEFINE(_name, _msg_type, _queue_size)                                                             \
    const struct msghub_topic __msghub_##_name = {                                                                     \
        .name = #_name, .msg_size = sizeof(_msg_type), .queue_size = (_queue_size), .id = 0}

// 获取 Topic 句柄
#define MSGHUB_TOPIC(_name) (&__msghub_##_name)

// ============================================================================
// 类型安全的 Publisher/Subscriber 宏
// ============================================================================

// 在头文件中：声明类型安全的 API
#define MSGHUB_PUBSUB_DECLARE(_name, _msg_type)                                                                        \
    typedef msghub_publisher_t _name##_pub_t;                                                                          \
    typedef msghub_subscriber_t _name##_sub_t;                                                                         \
    _name##_pub_t _name##_create_publisher(void);                                                                      \
    _name##_pub_t _name##_create_publisher_instance(int instance);                                                     \
    msghub_err_t _name##_destroy_publisher(_name##_pub_t pub);                                                         \
    msghub_err_t _name##_publish(_name##_pub_t pub, const _msg_type *data);                                            \
    _name##_sub_t _name##_create_subscriber(uint8_t instance);                                                         \
    msghub_err_t _name##_destroy_subscriber(_name##_sub_t sub);                                                        \
    msghub_err_t _name##_receive(_name##_sub_t sub, _msg_type *data);                                                  \
    msghub_err_t _name##_try_receive(_name##_sub_t sub, _msg_type *data, bool *updated)

// 在源文件中：实现类型安全的 API
#define MSGHUB_PUBSUB_DEFINE(_name, _msg_type)                                                                         \
    _name##_pub_t _name##_create_publisher(void)                                                                       \
    {                                                                                                                  \
        return (msghub_publisher_t)(uintptr_t)msghub_create_publisher(MSGHUB_TOPIC(_name));                            \
    }                                                                                                                  \
    _name##_pub_t _name##_create_publisher_instance(int instance)                                                      \
    {                                                                                                                  \
        return (msghub_publisher_t)(uintptr_t)msghub_create_publisher_multi(MSGHUB_TOPIC(_name), &instance);           \
    }                                                                                                                  \
    msghub_err_t _name##_destroy_publisher(_name##_pub_t pub)                                                          \
    {                                                                                                                  \
        return msghub_destroy_publisher((msghub_publisher_t)(uintptr_t)pub);                                           \
    }                                                                                                                  \
    msghub_err_t _name##_publish(_name##_pub_t pub, const _msg_type *data)                                             \
    {                                                                                                                  \
        return msghub_publish((msghub_publisher_t)(uintptr_t)pub, data);                                               \
    }                                                                                                                  \
    _name##_sub_t _name##_create_subscriber(uint8_t instance)                                                          \
    {                                                                                                                  \
        return (msghub_subscriber_t)(uintptr_t)msghub_create_subscriber(MSGHUB_TOPIC(_name), instance);                \
    }                                                                                                                  \
    msghub_err_t _name##_destroy_subscriber(_name##_sub_t sub)                                                         \
    {                                                                                                                  \
        return msghub_destroy_subscriber((msghub_subscriber_t)(uintptr_t)sub);                                         \
    }                                                                                                                  \
    msghub_err_t _name##_receive(_name##_sub_t sub, _msg_type *data)                                                   \
    {                                                                                                                  \
        return msghub_receive((msghub_subscriber_t)(uintptr_t)sub, data);                                              \
    }                                                                                                                  \
    msghub_err_t _name##_try_receive(_name##_sub_t sub, _msg_type *data, bool *updated)                                \
    {                                                                                                                  \
        msghub_err_t err = msghub_subscriber_check((msghub_subscriber_t)(uintptr_t)sub, updated);                      \
        if (err == MSGHUB_OK && *updated) {                                                                            \
            return msghub_receive((msghub_subscriber_t)(uintptr_t)sub, data);                                          \
        }                                                                                                              \
        return err;                                                                                                    \
    }

// ============================================================================
// Publisher API
// ============================================================================

msghub_publisher_t msghub_create_publisher(msghub_topic_t topic);
msghub_err_t msghub_destroy_publisher(msghub_publisher_t pub);
msghub_err_t msghub_publish(msghub_publisher_t pub, const void *data);

// 多实例支持
msghub_publisher_t msghub_create_publisher_multi(msghub_topic_t topic, int *instance);

// ============================================================================
// Subscriber API
// ============================================================================

msghub_subscriber_t msghub_create_subscriber(msghub_topic_t topic, uint8_t instance);
msghub_err_t msghub_destroy_subscriber(msghub_subscriber_t sub);
msghub_err_t msghub_receive(msghub_subscriber_t sub, void *data);
msghub_err_t msghub_subscriber_check(msghub_subscriber_t sub, bool *updated);

// ============================================================================
// 高级功能
// ============================================================================

#define MSGHUB_TIMEOUT_INFINITE 0xFFFFFFFF

uint16_t msghub_get_generation(msghub_subscriber_t sub);
msghub_err_t msghub_subscriber_poll(msghub_subscriber_t sub, uint32_t timeout_ms, bool *updated);
msghub_err_t msghub_subscriber_update(msghub_subscriber_t sub, void *data, bool *updated);

// ============================================================================
// Topic 查询功能
// ============================================================================

bool msghub_topic_exists(msghub_topic_t topic, uint8_t instance);
int msghub_topic_publisher_count(msghub_topic_t topic);

// ============================================================================
// 验证功能
// ============================================================================

bool msghub_publisher_valid(msghub_publisher_t pub);
bool msghub_subscriber_valid(msghub_subscriber_t sub);

#ifdef __cplusplus
}
#endif

#endif // MSGHUB_H
