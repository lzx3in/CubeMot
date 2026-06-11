/**
 * @file foc_pwm.c
 * @brief FOC PWM driver — STM32G4 TIM1 register-level implementation
 */

#include "foc_pwm.h"
#include <stm32g4xx_ll_tim.h>
#include <stm32g4xx_ll_bus.h>
#include <stm32g4xx_ll_rcc.h>
#include <stm32g4xx_ll_gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(foc_pwm, LOG_LEVEL_INF);

/* ── State ──────────────────────────────────────────── */

static TIM_TypeDef *const TIM_MOTOR = TIM1;
static bool g_pwm_enabled = false;

/* ── Dead-time calculation ─────────────────────────── */

static uint32_t calc_deadtime_ns_to_dtg(uint32_t deadtime_ns)
{
    /* TIM1 clock = 170 MHz → Tdtg = 1/170e6 ≈ 5.882 ns
     * DTG = deadtime / Tdtg
     */
    uint32_t tdts_ns = 1000000000UL / FOC_PWM_TIMER_CLK_HZ; // ≈ 5.88 ns
    uint32_t dtg = (deadtime_ns + tdts_ns / 2) / tdts_ns;

    if (dtg > 0x7F) { dtg = 0x7F; } /* 8-bit max */
    return dtg & 0x7F;
}

/* ── Init ───────────────────────────────────────────── */

int foc_pwm_init(void)
{
    LOG_INF("Initializing FOC PWM (TIM1)");

    /* Enable TIM1 clock */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

    /* ── Pin configuration ────────────────────────────
     * TIM1_CH1:  PA8   (AF6)  UH
     * TIM1_CH1N: PB13  (AF6)  UL
     * TIM1_CH2:  PA9   (AF6)  VH
     * TIM1_CH2N: PB14  (AF6)  VL
     * TIM1_CH3:  PA10  (AF6)  WH
     * TIM1_CH3N: PB15  (AF6)  WL
     * TIM1_BKIN2: PA11 (AF12) Emergency stop
     */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    /* PA8: TIM1_CH1 */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_8, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    /* PA9: TIM1_CH2 */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_9, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    /* PA10: TIM1_CH3 */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_10, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    /* PA11: TIM1_BKIN2 (Emergency Stop) */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_11, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_11, LL_GPIO_AF_12);

    /* PB13: TIM1_CH1N */
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_13, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_13, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_13, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    /* PB14: TIM1_CH2N */
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_14, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_14, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_14, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    /* PB15: TIM1_CH3N */
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_15, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_15, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_15, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    /* ── TIM1 basic config ───────────────────────────── */
    LL_TIM_DeInit(TIM_MOTOR);

    /* Prescaler: TIM_CLK = 170MHz / 1 = 170MHz */
    LL_TIM_SetPrescaler(TIM_MOTOR, 0);

    /* Auto-reload: half-period for center-aligned */
    LL_TIM_SetAutoReload(TIM_MOTOR, FOC_PWM_HALF_PERIOD);

    /* Center-aligned mode 1, up-counting */
    LL_TIM_SetCounterMode(TIM_MOTOR, LL_TIM_COUNTERMODE_CENTER_UP);

    /* Clock division = DIV2 (for dead-time fine grain) */
    LL_TIM_SetClockDivision(TIM_MOTOR, LL_TIM_CLOCKDIVISION_DIV2);

    /* Repetition counter = 0 */
    LL_TIM_SetRepetitionCounter(TIM_MOTOR, 0);

    /* ── PWM mode configuration ────────────────────────
     * CH1/CH2/CH3: PWM Mode 1, preload enabled
     *           Output polarity: Active High
     * CH4: PWM Mode 2 (for TRGO timing)
     */
    LL_TIM_OC_InitTypeDef oc_init = {0};
    oc_init.OCMode      = LL_TIM_OCMODE_PWM1;
    oc_init.OCState     = LL_TIM_OCSTATE_ENABLE;
    oc_init.OCPolarity  = LL_TIM_OCPOLARITY_HIGH;
    oc_init.OCNState    = LL_TIM_OCSTATE_ENABLE;
    oc_init.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
    oc_init.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
    oc_init.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;

    /* CH1 */
    oc_init.CompareValue = FOC_PWM_HALF_PERIOD / 2; /* 50% default */
    LL_TIM_OC_Init(TIM_MOTOR, LL_TIM_CHANNEL_CH1, &oc_init);

    /* CH2 */
    LL_TIM_OC_Init(TIM_MOTOR, LL_TIM_CHANNEL_CH2, &oc_init);

    /* CH3 */
    LL_TIM_OC_Init(TIM_MOTOR, LL_TIM_CHANNEL_CH3, &oc_init);

    /* CH4: for TRGO timing */
    oc_init.OCMode = LL_TIM_OCMODE_PWM2;
    oc_init.OCState = LL_TIM_OCSTATE_DISABLE; /* No physical output needed */
    oc_init.CompareValue = 1;
    LL_TIM_OC_Init(TIM_MOTOR, LL_TIM_CHANNEL_CH4, &oc_init);

    /* ── Dead-time ───────────────────────────────────── */
    uint32_t dtg = calc_deadtime_ns_to_dtg(FOC_PWM_DEADTIME_NS);
    LL_TIM_OC_SetDeadTime(TIM_MOTOR, dtg);
    LOG_INF("Dead-time: %u ns → DTG=%u", FOC_PWM_DEADTIME_NS, dtg);

    /* ── Break & Off-State ─────────────────────────────
     * BKIN1: Disabled (no break1 input)
     * BKIN2: Enabled for emergency stop (PA11), Active Low, Filter=3
     * OSSR: Enabled (off-state run mode)
     * OSSI: Enabled (off-state idle mode)
     */
    LL_TIM_EnableAllOutputs(TIM_MOTOR);
    LL_TIM_SetOffStates(TIM_MOTOR, LL_TIM_OSSI_ENABLE, LL_TIM_OSSR_ENABLE);

    /* BKIN2: PA11, Active Low, Filter=FDIV2_N6 (index 3 in workbench = 0x00400000) */
    LL_TIM_EnableBRK2(TIM_MOTOR);
    LL_TIM_ConfigBRK2(TIM_MOTOR, LL_TIM_BREAK2_POLARITY_LOW, LL_TIM_BREAK2_FILTER_FDIV2_N6, 0);

    /* ── Master output: TRGO = OC4REF (ADC trigger) ─── */
    LL_TIM_SetTriggerOutput(TIM_MOTOR, LL_TIM_TRGO_OC4REF);
    LL_TIM_SetTriggerOutput2(TIM_MOTOR, LL_TIM_TRGO2_RESET);

    /* ── Preload ─────────────────────────────────────── */
    LL_TIM_OC_EnablePreload(TIM_MOTOR, LL_TIM_CHANNEL_CH1);
    LL_TIM_OC_EnablePreload(TIM_MOTOR, LL_TIM_CHANNEL_CH2);
    LL_TIM_OC_EnablePreload(TIM_MOTOR, LL_TIM_CHANNEL_CH3);
    LL_TIM_OC_EnablePreload(TIM_MOTOR, LL_TIM_CHANNEL_CH4);
    LL_TIM_DisableARRPreload(TIM_MOTOR);

    /* ── Generate update to load shadow registers ────── */
    LL_TIM_GenerateEvent_UPDATE(TIM_MOTOR);

    /* ── Start counter ───────────────────────────────── */
    LL_TIM_EnableCounter(TIM_MOTOR);

    /* Outputs are initially disabled (safe state) */
    g_pwm_enabled = false;

    LOG_INF("FOC PWM initialized: %u Hz, period=%u",
            FOC_PWM_FREQ_HZ, FOC_PWM_HALF_PERIOD);
    return 0;
}

/* ── API implementation ─────────────────────────────── */

void foc_pwm_set_duty(float duty_a, float duty_b, float duty_c)
{
    /* Clamp and convert to compare values */
    if (duty_a < 0.0f) duty_a = 0.0f;
    if (duty_a > 1.0f) duty_a = 1.0f;
    if (duty_b < 0.0f) duty_b = 0.0f;
    if (duty_b > 1.0f) duty_b = 1.0f;
    if (duty_c < 0.0f) duty_c = 0.0f;
    if (duty_c > 1.0f) duty_c = 1.0f;

    uint32_t ccr1 = (uint32_t)(duty_a * FOC_PWM_HALF_PERIOD);
    uint32_t ccr2 = (uint32_t)(duty_b * FOC_PWM_HALF_PERIOD);
    uint32_t ccr3 = (uint32_t)(duty_c * FOC_PWM_HALF_PERIOD);

    /* Direct register write — ISR-safe */
    TIM_MOTOR->CCR1 = ccr1;
    TIM_MOTOR->CCR2 = ccr2;
    TIM_MOTOR->CCR3 = ccr3;
}

void foc_pwm_enable(void)
{
    LL_TIM_EnableAllOutputs(TIM_MOTOR);
    g_pwm_enabled = true;
    LOG_INF("PWM outputs enabled");
}

void foc_pwm_disable(void)
{
    LL_TIM_DisableAllOutputs(TIM_MOTOR);
    g_pwm_enabled = false;
    LOG_INF("PWM outputs disabled");
}

bool foc_pwm_is_enabled(void)
{
    return g_pwm_enabled;
}

uint32_t foc_pwm_get_period(void)
{
    return FOC_PWM_HALF_PERIOD;
}
