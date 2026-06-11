/**
 * @file servo.c
 * @brief Servo PWM driver implementation
 *
 * TIM3 Configuration for 50Hz PWM:
 * - Clock: 170MHz
 * - PSC: 169 → 1MHz (1μs resolution)
 * - ARR: 19999 → 20ms period (50Hz)
 * - CCR: 500-2500 → 0.5-2.5ms pulse width
 *
 * Pin mapping:
 * - Servo 0: PA6 (TIM3_CH1)
 * - Servo 1: PA7 (TIM3_CH2)
 */

#include "servo.h"
#include <stm32g4xx_ll_tim.h>
#include <stm32g4xx_ll_bus.h>
#include <stm32g4xx_ll_rcc.h>
#include <stm32g4xx_ll_gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(servo, LOG_LEVEL_INF);

/* ── Constants ───────────────────────────────────────── */

#define SERVO_TIMER TIM3
#define SERVO_TIMER_FREQ 1000000  // 1MHz after prescaler
#define SERVO_PWM_FREQ 50         // 50Hz
#define SERVO_PERIOD_US 20000     // 20ms

#define SERVO_PULSE_MIN_US 500
#define SERVO_PULSE_MAX_US 2500
#define SERVO_PULSE_CENTER_US 1500

#define SERVO_ANGLE_MIN -90.0f
#define SERVO_ANGLE_MAX 90.0f

/* ── State ───────────────────────────────────────────── */

static float g_servo_angles[MAX_SERVOS] = {0.0f, 0.0f};
static bool g_initialized = false;

/* ── GPIO Configuration ──────────────────────────────── */

static void servo_gpio_init(void)
{
    // Enable GPIOA clock
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

    // PA6 (TIM3_CH1) - Servo 0
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_6, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_6, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_6, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_6, LL_GPIO_AF_2);  // AF2 = TIM3

    // PA7 (TIM3_CH2) - Servo 1
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_7, LL_GPIO_AF_2);  // AF2 = TIM3

    LOG_INF("Servo GPIO configured: PA6(CH1), PA7(CH2)");
}

/* ── Timer Configuration ─────────────────────────────── */

static void servo_timer_init(void)
{
    // Enable TIM3 clock
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

    // Time base configuration
    LL_TIM_SetPrescaler(SERVO_TIMER, 169);      // 170MHz / 170 = 1MHz
    LL_TIM_SetAutoReload(SERVO_TIMER, 19999);   // 1MHz / 20000 = 50Hz
    LL_TIM_SetCounterMode(SERVO_TIMER, LL_TIM_COUNTERMODE_UP);

    // Channel 1 (PA6) - PWM Mode 1
    LL_TIM_OC_SetMode(SERVO_TIMER, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(SERVO_TIMER, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH);
    LL_TIM_OC_EnableFast(SERVO_TIMER, LL_TIM_CHANNEL_CH1);
    LL_TIM_OC_EnablePreload(SERVO_TIMER, LL_TIM_CHANNEL_CH1);
    LL_TIM_OC_SetCompareCH1(SERVO_TIMER, SERVO_PULSE_CENTER_US);  // Center position

    // Channel 2 (PA7) - PWM Mode 1
    LL_TIM_OC_SetMode(SERVO_TIMER, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(SERVO_TIMER, LL_TIM_CHANNEL_CH2, LL_TIM_OCPOLARITY_HIGH);
    LL_TIM_OC_EnableFast(SERVO_TIMER, LL_TIM_CHANNEL_CH2);
    LL_TIM_OC_EnablePreload(SERVO_TIMER, LL_TIM_CHANNEL_CH2);
    LL_TIM_OC_SetCompareCH2(SERVO_TIMER, SERVO_PULSE_CENTER_US);  // Center position

    // Enable outputs
    LL_TIM_CC_EnableChannel(SERVO_TIMER, LL_TIM_CHANNEL_CH1);
    LL_TIM_CC_EnableChannel(SERVO_TIMER, LL_TIM_CHANNEL_CH2);

    // Generate update event to load registers
    LL_TIM_GenerateEvent_UPDATE(SERVO_TIMER);

    // Enable auto-reload preload
    LL_TIM_EnableARRPreload(SERVO_TIMER);

    // Start timer
    LL_TIM_EnableCounter(SERVO_TIMER);

    LOG_INF("Servo timer configured: 50Hz, 1μs resolution");
}

/* ── Public API ──────────────────────────────────────── */

int servo_init(void)
{
    if (g_initialized) {
        LOG_WRN("Servo already initialized");
        return 0;
    }

    servo_gpio_init();
    servo_timer_init();

    g_initialized = true;
    LOG_INF("Servo driver initialized (2 channels)");
    return 0;
}

int servo_set_angle(uint8_t servo_id, float angle_deg)
{
    if (!g_initialized) {
        LOG_ERR("Servo not initialized");
        return -1;
    }

    if (servo_id >= MAX_SERVOS) {
        LOG_ERR("Invalid servo_id: %u", servo_id);
        return -1;
    }

    // Clamp angle
    if (angle_deg < SERVO_ANGLE_MIN) angle_deg = SERVO_ANGLE_MIN;
    if (angle_deg > SERVO_ANGLE_MAX) angle_deg = SERVO_ANGLE_MAX;

    // Convert angle to pulse width
    // -90° → 500μs, 0° → 1500μs, +90° → 2500μs
    float pulse_us = SERVO_PULSE_CENTER_US + 
                     (angle_deg / SERVO_ANGLE_MAX) * 
                     (SERVO_PULSE_MAX_US - SERVO_PULSE_CENTER_US);

    g_servo_angles[servo_id] = angle_deg;

    return servo_set_pulse_us(servo_id, (uint16_t)pulse_us);
}

int servo_set_pulse_us(uint8_t servo_id, uint16_t pulse_us)
{
    if (!g_initialized) {
        LOG_ERR("Servo not initialized");
        return -1;
    }

    if (servo_id >= MAX_SERVOS) {
        LOG_ERR("Invalid servo_id: %u", servo_id);
        return -1;
    }

    // Clamp pulse width
    if (pulse_us < SERVO_PULSE_MIN_US) pulse_us = SERVO_PULSE_MIN_US;
    if (pulse_us > SERVO_PULSE_MAX_US) pulse_us = SERVO_PULSE_MAX_US;

    // Set compare value
    if (servo_id == 0) {
        LL_TIM_OC_SetCompareCH1(SERVO_TIMER, pulse_us);
    } else {
        LL_TIM_OC_SetCompareCH2(SERVO_TIMER, pulse_us);
    }

    LOG_DBG("Servo %u: %u μs", servo_id, pulse_us);
    return 0;
}

float servo_get_angle(uint8_t servo_id)
{
    if (servo_id >= MAX_SERVOS) {
        return 0.0f;
    }
    return g_servo_angles[servo_id];
}
