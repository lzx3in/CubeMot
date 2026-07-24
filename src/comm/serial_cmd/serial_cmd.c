/**
 * @file serial_cmd.c
 * @brief Serial command protocol: UART ↔ msghub bridge
 *
 * Frame format: [0xAA 0x55] [CMD_ID] [LEN] [PAYLOAD...] [CRC8]
 *
 * RX: interrupt-driven (uart_irq + fifo_read) into ring buffer
 * TX: polling (uart_poll_out)
 * LPUART1 on NUCLEO-G431RB: PA2(TX), PA3(RX) → ST-Link VCP
 */

#include "comm/serial_cmd.h"
#include "serial_protocol.h"
#include "crc.h"
#include "topics/topics.h"
#include "common_time.h"
#include "common_error.h"
#include "drivers/foc/foc_isr.h"
#include "drivers/foc/foc_pwm.h"
#include <stm32g4xx.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <string.h>
LOG_MODULE_REGISTER(serial_cmd, LOG_LEVEL_INF);

/* ── Configuration ───────────────────────────────────── */

#define RX_THREAD_STACK_SIZE  2048
#define TX_THREAD_STACK_SIZE  2048
#define RX_THREAD_PRIORITY    K_PRIO_COOP(8)
#define TX_THREAD_PRIORITY    K_PRIO_COOP(8)

/* ── CRC8 is now provided by lib_crc (crc8_calculate) ── */

/* ── State ───────────────────────────────────────────── */

static const struct device *uart_dev;
static struct k_mutex tx_mutex;  /* protects tx_buf + uart_poll_out */
static struct k_sem rx_sem;      /* signaled by ISR callback */

/* RX ring buffer */
#define RX_RING_SIZE 128
static uint8_t rx_ring[RX_RING_SIZE];
static size_t rx_head = 0;
static size_t rx_tail = 0;

/* TX buffer */
static uint8_t tx_buf[SP_MAX_FRAME];

/* msghub subscribers/publishers */
static msghub_subscriber_t g_cmd_vel_sub;
static msghub_subscriber_t g_vehicle_state_sub;
static msghub_subscriber_t g_motor_state_sub;
static msghub_subscriber_t g_commander_status_sub;

static msghub_publisher_t g_cmd_vel_pub;
static msghub_publisher_t g_commander_cmd_pub;
static msghub_publisher_t g_motor_cmd_pub;

/* ── Ring buffer helpers ─────────────────────────────── */

static void ring_push(uint8_t byte)
{
    rx_ring[rx_head] = byte;
    rx_head = (rx_head + 1) % RX_RING_SIZE;
    if (rx_head == rx_tail) {
        rx_tail = (rx_tail + 1) % RX_RING_SIZE;
    }
}

static size_t ring_available(void)
{
    if (rx_head >= rx_tail) return rx_head - rx_tail;
    return RX_RING_SIZE - rx_tail + rx_head;
}

static uint8_t ring_peek(size_t offset)
{
    return rx_ring[(rx_tail + offset) % RX_RING_SIZE];
}

static void ring_discard(size_t n)
{
    rx_tail = (rx_tail + n) % RX_RING_SIZE;
}

/* ── UART IRQ callback ──────────────────────────────── */

static void uart_irq_callback(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
        uint8_t buf[16];
        int n = uart_fifo_read(dev, buf, sizeof(buf));
        for (int i = 0; i < n; i++) {
            ring_push(buf[i]);
        }
    }
    k_sem_give(&rx_sem);
}

/* ── Frame parsing ───────────────────────────────────── */

typedef struct {
    uint8_t cmd_id;
    uint8_t len;
    uint8_t payload[SP_MAX_PAYLOAD];
} parsed_frame_t;

static int parse_frame_from_ring(parsed_frame_t *frame)
{
    size_t avail = ring_available();

    if (avail < 4) return -1;

    if (ring_peek(0) != SP_FRAME_HEAD_0 || ring_peek(1) != SP_FRAME_HEAD_1) {
        ring_discard(1);
        return -1;
    }

    frame->cmd_id = ring_peek(2);
    frame->len = ring_peek(3);

    if (frame->len > SP_MAX_PAYLOAD) {
        ring_discard(2);
        return -1;
    }

    size_t frame_len = 4 + frame->len + 1;
    if (avail < frame_len) return -1;

    for (uint8_t i = 0; i < frame->len; i++) {
        frame->payload[i] = ring_peek(4 + i);
    }

    uint8_t crc_buf[2 + SP_MAX_PAYLOAD];
    crc_buf[0] = frame->cmd_id;
    crc_buf[1] = frame->len;
    memcpy(&crc_buf[2], frame->payload, frame->len);
    uint8_t expected_crc = crc8_calculate(crc_buf, 2 + frame->len);
    uint8_t actual_crc = ring_peek(4 + frame->len);

    ring_discard(frame_len);

    if (expected_crc != actual_crc) {
        LOG_WRN("CRC mismatch: expected 0x%02x, got 0x%02x", expected_crc, actual_crc);
        return -1;
    }

    return 0;
}

static size_t encode_frame(uint8_t cmd_id, const uint8_t *payload, uint8_t len)
{
    return sp_encode_frame(tx_buf, cmd_id, payload, len);
}

static void send_response(uint8_t cmd_id, const uint8_t *payload, uint8_t len)
{
    k_mutex_lock(&tx_mutex, K_FOREVER);
    size_t frame_len = encode_frame(cmd_id, payload, len);
    for (size_t i = 0; i < frame_len; i++) {
        uart_poll_out(uart_dev, tx_buf[i]);
    }
    k_mutex_unlock(&tx_mutex);
}

/* ── Command processing ──────────────────────────────── */

static void process_command(const parsed_frame_t *frame)
{
    switch (frame->cmd_id) {
    case CMD_ID_VEL: {
        if (frame->len != 8) {
            LOG_WRN("CMD_VEL: invalid length %u", frame->len);
            break;
        }
        cmd_vel_t vel;
        memcpy(&vel.linear_x, &frame->payload[0], 4);
        memcpy(&vel.angular_z, &frame->payload[4], 4);
        vel.timestamp = common_get_timestamp_ms();
        msghub_publish(g_cmd_vel_pub, &vel);
        break;
    }

    case CMD_ID_ARM: {
        commander_cmd_t cmd = { .op = CMD_OP_ARM, .timestamp = common_get_timestamp_ms() };
        msghub_publish(g_commander_cmd_pub, &cmd);
        break;
    }

    case CMD_ID_DISARM: {
        commander_cmd_t cmd = { .op = CMD_OP_DISARM, .timestamp = common_get_timestamp_ms() };
        msghub_publish(g_commander_cmd_pub, &cmd);
        break;
    }

    case CMD_ID_ESTOP: {
        commander_cmd_t cmd = { .op = CMD_OP_ESTOP, .timestamp = common_get_timestamp_ms() };
        msghub_publish(g_commander_cmd_pub, &cmd);
        break;
    }

    case CMD_ID_RESET: {
        commander_cmd_t cmd = { .op = CMD_OP_RESET_FAULT, .timestamp = common_get_timestamp_ms() };
        msghub_publish(g_commander_cmd_pub, &cmd);
        break;
    }

    case CMD_ID_MOTOR_START: {
        if (frame->len < 5) {
            LOG_WRN("CMD_MOTOR_START: invalid length %u", frame->len);
            break;
        }
        motor_cmd_t cmd = {0};
        cmd.motor_id = frame->payload[0];
        cmd.cmd = MOTOR_CMD_START;
        memcpy(&cmd.target_speed_rpm, &frame->payload[1], 4);
        msghub_publish(g_motor_cmd_pub, &cmd);
        LOG_INF("MOTOR START: id=%u speed=%.0f RPM",
                cmd.motor_id, (double)cmd.target_speed_rpm);
        break;
    }

    case CMD_ID_MOTOR_STOP: {
        if (frame->len < 1) {
            LOG_WRN("CMD_MOTOR_STOP: invalid length %u", frame->len);
            break;
        }
        motor_cmd_t cmd = {0};
        cmd.motor_id = frame->payload[0];
        cmd.cmd = MOTOR_CMD_STOP;
        msghub_publish(g_motor_cmd_pub, &cmd);
        LOG_INF("MOTOR STOP: id=%u", cmd.motor_id);
        break;
    }

    case CMD_ID_PING: {
        uint8_t pong_payload = 0x00;
        send_response(RSP_ID_PONG, &pong_payload, 1);
        break;
    }

    case CMD_ID_TEST: {
        if (frame->len < 5) {
            LOG_WRN("CMD_TEST: invalid length %u", frame->len);
            break;
        }
        uint8_t test_id = frame->payload[0];
        float param;
        memcpy(&param, &frame->payload[1], 4);

        switch (test_id) {
        case 0:
            LOG_INF("TEST: Start FOC Id=%dmA", (int)(param * 1000.0f));
            foc_isr_get_foc()->state.i_d_ref = param;
            foc_isr_get_foc()->state.i_q_ref = 0.0f;
            foc_pwm_enable();
            foc_isr_start();
            break;
        case 1:
            LOG_INF("TEST: Stop FOC");
            foc_isr_stop();
            foc_pwm_disable();
            break;
        case 2:
            LOG_INF("TEST: Set Iq=%dmA", (int)(param * 1000.0f));
            foc_isr_get_foc()->state.i_q_ref = param;
            break;
        case 3: {
            /* ADC register-level diagnostic */
            extern void foc_adc_sw_trigger_test(void);
            extern void foc_adc_get_sw_diag(int16_t *ia, int16_t *ib, int16_t *ic,
                                            int16_t *vbus_raw, float *vbus_v, bool *valid);

            /* Read COMMON_CCR for CKMODE */
            uint32_t common_ccr = ADC12_COMMON->CCR;

            /* Read ADC state BEFORE SW trigger */
            uint32_t adc1_isr_before = ADC1->ISR;
            uint32_t adc1_jsqr = ADC1->JSQR;
            int16_t jdr1_before = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1);
            int16_t jdr2_before = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_2);
            int16_t adc2_jdr1_before = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC2, LL_ADC_INJ_RANK_1);

            /* Run SW trigger test */
            foc_adc_sw_trigger_test();

            int16_t sw_ia, sw_ib, sw_ic, sw_vbus_raw;
            float sw_vbus_v;
            bool sw_valid;
            foc_adc_get_sw_diag(&sw_ia, &sw_ib, &sw_ic,
                                &sw_vbus_raw, &sw_vbus_v, &sw_valid);

            /* Read ADC state AFTER SW trigger */
            uint32_t adc1_isr_after = ADC1->ISR;
            uint16_t adc2_isr_low = (uint16_t)(ADC2->ISR & 0xFFFF);
            int16_t adc2_jdr2_after = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC2, LL_ADC_INJ_RANK_2);

            /* Pack RSP_DIAG (32 bytes) — ADC diagnostic v2 */
            uint8_t dpayload[32];
            memcpy(&dpayload[0],  &adc1_isr_before, 4);
            memcpy(&dpayload[4],  &adc1_jsqr, 2);
            memcpy(&dpayload[6],  &jdr1_before, 2);
            memcpy(&dpayload[8],  &jdr2_before, 2);
            memcpy(&dpayload[10], &adc2_jdr1_before, 2);
            memcpy(&dpayload[12], &sw_ia, 2);
            memcpy(&dpayload[14], &sw_ib, 2);
            memcpy(&dpayload[16], &sw_ic, 2);
            memcpy(&dpayload[18], &sw_vbus_raw, 2);
            memcpy(&dpayload[20], &adc1_isr_after, 4);
            memcpy(&dpayload[24], &adc2_isr_low, 2);
            dpayload[26] = sw_valid ? 1 : 0;
            dpayload[27] = (uint8_t)((common_ccr >> 16) & 0xFF);
            memcpy(&dpayload[28], &adc2_jdr2_after, 2);
            send_response(RSP_ID_DIAG, dpayload, 32);
            break;
        }
        default:
            LOG_WRN("TEST: unknown test_id %u", test_id);
            break;
        }
        break;
    }

    default:
        LOG_WRN("Unknown CMD_ID: 0x%02x", frame->cmd_id);
        break;
    }
}

/* ── RX thread ───────────────────────────────────────── */

void serial_cmd_rx_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    /* Startup delay: wait for UART hardware and msghub topics to be ready */
    k_sleep(K_MSEC(300));
    LOG_INF("Serial RX thread started (IRQ-driven)");

    while (1) {
        /* Wait for ISR callback to signal new data */
        k_sem_take(&rx_sem, K_MSEC(10));

        /* Parse all available frames */
        parsed_frame_t frame;
        while (parse_frame_from_ring(&frame) == 0) {
            process_command(&frame);
        }
    }
}

/* ── TX thread ───────────────────────────────────────── */

void serial_cmd_tx_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    /* Startup delay: wait for UART hardware and msghub topics to be ready */
    k_sleep(K_MSEC(300));
    LOG_INF("Serial TX thread started");

    uint32_t status_counter = 0;
    uint32_t telemetry_counter = 0;
    uint32_t diag_counter = 0;

    while (1) {
        /* STATUS @ 10Hz */
        if (++status_counter >= 10) {
            status_counter = 0;

            commander_status_t status;
            bool updated = false;
            msghub_subscriber_update(g_commander_status_sub, &status, &updated);
            if (updated) {
                uint8_t payload[2] = { (uint8_t)status.state, (uint8_t)status.fault_code };
                send_response(RSP_ID_STATUS, payload, 2);
            }
        }

        /* TELEMETRY @ 20Hz */
        if (++telemetry_counter >= 5) {
            telemetry_counter = 0;

            vehicle_state_t state;
            bool updated = false;
            msghub_subscriber_update(g_vehicle_state_sub, &state, &updated);
            if (updated) {
                uint8_t payload[20];
                memcpy(&payload[0], &state.x, 4);
                memcpy(&payload[4], &state.y, 4);
                memcpy(&payload[8], &state.yaw, 4);
                memcpy(&payload[12], &state.linear_x, 4);
                memcpy(&payload[16], &state.angular_z, 4);

                send_response(RSP_ID_TELEMETRY, payload, 20);
            }
        }

        /* MOTOR state (on change) */
        motor_state_t mstate;
        bool updated = false;
        msghub_subscriber_update(g_motor_state_sub, &mstate, &updated);
        if (updated) {
            uint8_t payload[14];
            payload[0] = mstate.motor_id;
            payload[1] = mstate.state;
            int16_t speed = (int16_t)mstate.speed_rpm;
            memcpy(&payload[2], &speed, 2);
            uint16_t vbus = (uint16_t)(mstate.v_bus * 100.0f);
            memcpy(&payload[4], &vbus, 2);
            memcpy(&payload[6], &mstate.i_d, 4);
            memcpy(&payload[10], &mstate.i_q, 4);

            send_response(RSP_ID_MOTOR, payload, 14);
        }

        /* DIAG: FOC debug (raw ADC + duty) @ 5Hz */
        if (++diag_counter >= 20) {
            diag_counter = 0;
            foc_t *foc = foc_isr_get_foc();
            uint8_t dpayload[20];
            int16_t adc_ia = foc->state.adc_ia;
            int16_t adc_ib = foc->state.adc_ib;
            int16_t adc_ic = foc->state.adc_ic;
            int16_t off_ia = foc->state.adc_ia_offset;
            memcpy(&dpayload[0], &adc_ia, 2);
            memcpy(&dpayload[2], &adc_ib, 2);
            memcpy(&dpayload[4], &adc_ic, 2);
            memcpy(&dpayload[6], &off_ia, 2);
            memcpy(&dpayload[8], &foc->state.duty_a, 4);
            memcpy(&dpayload[12], &foc->state.i_d_ref, 4);
            memcpy(&dpayload[16], &foc->state.i_q_ref, 4);
            send_response(RSP_ID_DIAG, dpayload, 20);
        }

        k_msleep(10);
    }
}

/* ── Init ────────────────────────────────────────────── */

int serial_cmd_init(void)
{
    uart_dev = DEVICE_DT_GET(DT_NODELABEL(lpuart1));
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }

    k_mutex_init(&tx_mutex);
    k_sem_init(&rx_sem, 0, 1);

    /* Set up interrupt-driven RX */
    uart_irq_callback_user_data_set(uart_dev, uart_irq_callback, NULL);
    uart_irq_rx_enable(uart_dev);

    /* Initialize msghub */
    g_cmd_vel_sub = msghub_create_subscriber(MSGHUB_TOPIC(cmd_vel), 0);
    g_vehicle_state_sub = msghub_create_subscriber(MSGHUB_TOPIC(vehicle_state), 0);
    g_motor_state_sub = msghub_create_subscriber(MSGHUB_TOPIC(motor_state), 0);
    g_commander_status_sub = msghub_create_subscriber(MSGHUB_TOPIC(commander_status), 0);

    g_cmd_vel_pub = msghub_create_publisher(MSGHUB_TOPIC(cmd_vel));
    g_commander_cmd_pub = msghub_create_publisher(MSGHUB_TOPIC(commander_cmd));
    g_motor_cmd_pub = msghub_create_publisher(MSGHUB_TOPIC(motor_cmd));

    LOG_INF("Serial command init: LPUART1 @ 115200 baud (IRQ RX)");
    return 0;
}
