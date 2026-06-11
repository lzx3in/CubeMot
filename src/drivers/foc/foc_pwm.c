/**
 * @file foc_pwm.c
 * @brief FOC PWM driver — STM32G4 TIM1 direct-register implementation
 *
 * No dependency on LL_TIM_DeInit / LL_TIM_OC_Init (which require
 * stm32g4xx_ll_tim.c to be compiled). All timer config done via
 * direct register writes.
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
    uint32_t tdts_ns = 1000000000UL / FOC_PWM_TIMER_CLK_HZ;
    uint32_t dtg = (deadtime_ns + tdts_ns / 2) / tdts_ns;
    if (dtg > 0x7F) { dtg = 0x7F; }
    return dtg & 0x7F;
}

/* ── Direct-register OC config ─────────────────────── */

/**
 * @brief  Configure output channel for PWM mode (direct register)
 *
 * Replaces LL_TIM_OC_Init() which requires stm32g4xx_ll_tim.c
 */
static void tim_oc_config_pwm(TIM_TypeDef *tim, uint32_t channel,
                               uint32_t compare, uint32_t mode,
                               bool output_enable, bool comp_enable)
{
    /* channel: 0=CH1, 1=CH2, 2=CH3, 3=CH4 */
    uint32_t ch_shift = channel * 8;
    uint32_t ch_shift2 = (channel & 1) * 16;  /* for CCER */

    /* CCxR compare value */
    volatile uint32_t *ccr = &tim->CCR1 + channel;
    *ccr = compare;

    /* CCMRx: OC mode + preload */
    if (channel < 2) {
        /* CH1/CH2 → CCMR1 */
        uint32_t mask = 0xFF << ch_shift;
        uint32_t val = tim->CCMR1 & ~mask;
        /* mode bits [6:4], preload bit [3] */
        val |= (mode << (ch_shift + 4)) | (1 << (ch_shift + 3));
        tim->CCMR1 = val;
    } else {
        /* CH3/CH4 → CCMR2 */
        uint32_t mask = 0xFF << ch_shift2;
        uint32_t val = tim->CCMR2 & ~mask;
        val |= (mode << (ch_shift2 + 4)) | (1 << (ch_shift2 + 3));
        tim->CCMR2 = val;
    }

    /* CCER: output enable + complementary enable + polarity */
    uint32_t ccer_mask = (0xF << (channel * 4));
    uint32_t ccer_val = 0;
    if (output_enable) ccer_val |= (1 << (channel * 4));       /* CCxE */
    if (comp_enable)   ccer_val |= (1 << (channel * 4 + 1));   /* CCxNE */
    /* polarity = 0 → active high */
    tim->CCER = (tim->CCER & ~ccer_mask) | ccer_val;
}

/* ── Init ───────────────────────────────────────────── */

int foc_pwm_init(void)
{
    LOG_INF("Initializing FOC PWM (TIM1)");

    /* Enable TIM1 clock */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

    /* Reset TIM1 via APB2 (replaces LL_TIM_DeInit) */
    LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_TIM1);
    LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_TIM1);

    /* ── Pin configuration ──────────────────────────── */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    /* PA8/PA9/PA10: TIM1_CH1/CH2/CH3 */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_8, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_9, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_10, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    /* PA11: TIM1_BKIN2 (Emergency Stop) */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_11, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_11, LL_GPIO_AF_12);

    /* PB13/PB14/PB15: TIM1_CH1N/CH2N/CH3N */
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_13, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_13, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_13, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_14, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_14, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_14, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_15, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_15, LL_GPIO_AF_6);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_15, LL_GPIO_SPEED_FREQ_VERY_HIGH);

    /* ── TIM1 base config (direct register) ──────────── */

    /* CR1: center-aligned mode 1, up-counting, clock div=2 */
    TIM_MOTOR->CR1 = TIM_CR1_CMS_0 |          /* Center-aligned mode 1 */
                      TIM_CR1_DIR |             /* Up counting (DIR=0 actually, but CMS!=0 means center) */
                      TIM_CR1_CKD_0;            /* Clock division = DIV2 */
    /* Actually: DIR=0 for up in center-aligned. Clear DIR bit. */
    TIM_MOTOR->CR1 = TIM_CR1_CMS_0 | TIM_CR1_CKD_0;

    /* Prescaler = 0 (170MHz timer clock) */
    TIM_MOTOR->PSC = 0;

    /* Auto-reload = half period */
    TIM_MOTOR->ARR = FOC_PWM_HALF_PERIOD;

    /* Repetition counter = 0 */
    TIM_MOTOR->RCR = 0;

    /* ── PWM channels ────────────────────────────────── */
    /* PWM Mode 1 = 0b110 = 6 */
    #define OC_MODE_PWM1 6

    uint32_t half = FOC_PWM_HALF_PERIOD / 2;

    /* CH1/CH2/CH3: PWM1, preload, output+complementary enable */
    tim_oc_config_pwm(TIM_MOTOR, 0, half, OC_MODE_PWM1, true, true);
    tim_oc_config_pwm(TIM_MOTOR, 1, half, OC_MODE_PWM1, true, true);
    tim_oc_config_pwm(TIM_MOTOR, 2, half, OC_MODE_PWM1, true, true);

    /* CH4: PWM2, no output (for TRGO) */
    /* PWM Mode 2 = 0b111 = 7 */
    tim_oc_config_pwm(TIM_MOTOR, 3, 1, 7, false, false);

    /* ── Dead-time ───────────────────────────────────── */
    uint32_t dtg = calc_deadtime_ns_to_dtg(FOC_PWM_DEADTIME_NS);
    LL_TIM_OC_SetDeadTime(TIM_MOTOR, dtg);
    LOG_INF("Dead-time: %u ns → DTG=%u", FOC_PWM_DEADTIME_NS, dtg);

    /* ── Break & Off-State ───────────────────────────── */
    LL_TIM_EnableAllOutputs(TIM_MOTOR);
    LL_TIM_SetOffStates(TIM_MOTOR, LL_TIM_OSSI_ENABLE, LL_TIM_OSSR_ENABLE);

    /* BKIN2: Active Low, Filter=FDIV2_N6 */
    LL_TIM_EnableBRK2(TIM_MOTOR);
    LL_TIM_ConfigBRK2(TIM_MOTOR, LL_TIM_BREAK2_POLARITY_LOW,
                       LL_TIM_BREAK2_FILTER_FDIV2_N6, 0);

    /* ── Master output: TRGO = OC4REF ────────────────── */
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

    g_pwm_enabled = false;

    LOG_INF("FOC PWM initialized: %u Hz, period=%u",
            FOC_PWM_FREQ_HZ, FOC_PWM_HALF_PERIOD);
    return 0;
}

/* ── API implementation ─────────────────────────────── */

void foc_pwm_set_duty(float duty_a, float duty_b, float duty_c)
{
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
