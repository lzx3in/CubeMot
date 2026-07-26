/**
 * @file foc_adc.c
 * @brief FOC ADC driver — STM32G4 ADC1+ADC2 register-level implementation
 *
 * Dual injected mode:
 *   ADC1 injected: Rank1=CH14(Ib), Rank2=CH2(Ia)
 *   ADC2 injected: Rank1=CH4(Ic), Rank2=CH14(IbS)
 *
 * Triggered by TIM1_TRGO on every PWM cycle center.
 */
#include "foc_adc.h"
#include "foc_types.h"
#include <stm32g4xx_ll_adc.h>
#include <stm32g4xx_ll_bus.h>
#include <stm32g4xx_ll_rcc.h>
#include <stm32g4xx_ll_gpio.h>
#include <stm32g4xx_ll_tim.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(foc_adc, LOG_LEVEL_INF);

/* ── Hardware ───────────────────────────────────────── */

#define ADC_INSTANCE1   ADC1
#define ADC_INSTANCE2   ADC2

/* ADC channels */
#define ADC_CH_IA    LL_ADC_CHANNEL_2   /* PA1  — Phase U current */
#define ADC_CH_IB    LL_ADC_CHANNEL_14  /* PB11 — Phase V current (shared) */
#define ADC_CH_IC    LL_ADC_CHANNEL_4   /* PA7  — Phase W current */
#define ADC_CH_VBUS  LL_ADC_CHANNEL_11  /* PC5  — Bus voltage */

/* Rank number for sequencer (1-based) */
#define ADC_INJ_RANK1  LL_ADC_INJ_RANK_1
#define ADC_INJ_RANK2  LL_ADC_INJ_RANK_2
#define ADC_REG_RANK1  LL_ADC_REG_RANK_1

/* ── Init ───────────────────────────────────────────── */

/* Stored at init for diagnostic */
static uint32_t g_common_ccr_at_init = 0xDEAD;

int foc_adc_init(void)
{
    LOG_INF("Initializing FOC ADC (ADC1+ADC2)");

    /* ── Ensure ADC kernel clock is enabled ────────────── */
    /* Select SYSCLK as ADC clock source (default, but be explicit) */
    LL_RCC_SetADCClockSource(LL_RCC_ADC12_CLKSOURCE_SYSCLK);

    /* Enable ADC bus clocks */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC12);

    /* Enable GPIO clocks for analog pins */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    /* Configure analog pins to analog mode */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1, LL_GPIO_MODE_ANALOG);   /* PA1: Ia */
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_11, LL_GPIO_MODE_ANALOG);  /* PB11: Ib */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_ANALOG);   /* PA7: Ic */
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_5, LL_GPIO_MODE_ANALOG);   /* PC5: Vbus */

    /* ── ADC Common ────────────────────────────────────── */
    /* Use synchronous clock (from AHB) - no async clock config needed */
    LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_CLOCK_SYNC_PCLK_DIV4);

    /* Verify COMMON_CCR write — diagnose clock configuration */
    uint32_t ccr_readback = __LL_ADC_COMMON_INSTANCE(ADC1)->CCR;
    LOG_INF("COMMON_CCR after SetCommonClock: 0x%08X (CKMODE=%lu)",
            (unsigned)ccr_readback, (unsigned long)((ccr_readback >> 16) & 0x3));
    if (((ccr_readback >> 16) & 0x3) == 0) {
        LOG_WRN("CKMODE=0! Retrying COMMON_CCR write...");
        LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_CLOCK_SYNC_PCLK_DIV4);
        ccr_readback = __LL_ADC_COMMON_INSTANCE(ADC1)->CCR;
        LOG_INF("COMMON_CCR after retry: 0x%08X (CKMODE=%lu)",
                (unsigned)ccr_readback, (unsigned long)((ccr_readback >> 16) & 0x3));
    }

    /* ── ADC1: Calibrate ───────────────────────────────── */
    LL_ADC_DisableDeepPowerDown(ADC_INSTANCE1);
    LL_ADC_EnableInternalRegulator(ADC_INSTANCE1);
    k_busy_wait(20);
    LL_ADC_StartCalibration(ADC_INSTANCE1, LL_ADC_SINGLE_ENDED);
    while (LL_ADC_IsCalibrationOnGoing(ADC_INSTANCE1)) { /* wait */ }

    LL_ADC_SetResolution(ADC_INSTANCE1, LL_ADC_RESOLUTION_12B);
    LL_ADC_SetDataAlignment(ADC_INSTANCE1, LL_ADC_DATA_ALIGN_LEFT);
    /* No AUTOWAIT: injected conversions must free-run at TIM1 TRGO rate (30kHz) */

    /* Injected sequence: 2 conversions */
    LL_ADC_INJ_SetSequencerLength(ADC_INSTANCE1, LL_ADC_INJ_SEQ_SCAN_ENABLE_2RANKS);

    /* Rank 1: CH14 (Ib shared) */
    LL_ADC_INJ_SetSequencerRanks(ADC_INSTANCE1, ADC_INJ_RANK1, ADC_CH_IB);
    LL_ADC_SetChannelSamplingTime(ADC_INSTANCE1, ADC_CH_IB, LL_ADC_SAMPLINGTIME_6CYCLES_5);

    /* Rank 2: CH2 (Ia) */
    LL_ADC_INJ_SetSequencerRanks(ADC_INSTANCE1, ADC_INJ_RANK2, ADC_CH_IA);
    LL_ADC_SetChannelSamplingTime(ADC_INSTANCE1, ADC_CH_IA, LL_ADC_SAMPLINGTIME_6CYCLES_5);

    /* Trigger: software (ISR will trigger each cycle) */
    LL_ADC_INJ_SetTriggerSource(ADC_INSTANCE1, LL_ADC_INJ_TRIG_SOFTWARE);
    LL_ADC_INJ_SetTriggerEdge(ADC_INSTANCE1, LL_ADC_INJ_TRIG_SOFTWARE);

    /* ── ADC2: Calibrate ───────────────────────────────── */
    LL_ADC_DisableDeepPowerDown(ADC_INSTANCE2);
    LL_ADC_EnableInternalRegulator(ADC_INSTANCE2);
    k_busy_wait(20);
    LL_ADC_StartCalibration(ADC_INSTANCE2, LL_ADC_SINGLE_ENDED);
    while (LL_ADC_IsCalibrationOnGoing(ADC_INSTANCE2)) { /* wait */ }

    LL_ADC_SetResolution(ADC_INSTANCE2, LL_ADC_RESOLUTION_12B);
    LL_ADC_SetDataAlignment(ADC_INSTANCE2, LL_ADC_DATA_ALIGN_LEFT);
    /* No AUTOWAIT: injected conversions must free-run at TIM1 TRGO rate (30kHz) */

    /* Injected sequence: 2 conversions */
    LL_ADC_INJ_SetSequencerLength(ADC_INSTANCE2, LL_ADC_INJ_SEQ_SCAN_ENABLE_2RANKS);

    /* Rank 1: CH4 (Ic) */
    LL_ADC_INJ_SetSequencerRanks(ADC_INSTANCE2, ADC_INJ_RANK1, ADC_CH_IC);
    LL_ADC_SetChannelSamplingTime(ADC_INSTANCE2, ADC_CH_IC, LL_ADC_SAMPLINGTIME_6CYCLES_5);

    /* Rank 2: CH14 (Ib shared) */
    LL_ADC_INJ_SetSequencerRanks(ADC_INSTANCE2, ADC_INJ_RANK2, ADC_CH_IB);
    LL_ADC_SetChannelSamplingTime(ADC_INSTANCE2, ADC_CH_IB, LL_ADC_SAMPLINGTIME_6CYCLES_5);

    /* Trigger: software (ISR will trigger each cycle) */
    LL_ADC_INJ_SetTriggerSource(ADC_INSTANCE2, LL_ADC_INJ_TRIG_SOFTWARE);
    LL_ADC_INJ_SetTriggerEdge(ADC_INSTANCE2, LL_ADC_INJ_TRIG_SOFTWARE);

    /* ── Regular channel for Vbus (ADC2) ────────────────── */
    LL_ADC_REG_SetSequencerLength(ADC_INSTANCE2, LL_ADC_REG_SEQ_SCAN_DISABLE);
    LL_ADC_REG_SetSequencerRanks(ADC_INSTANCE2, ADC_REG_RANK1, ADC_CH_VBUS);
    LL_ADC_SetChannelSamplingTime(ADC_INSTANCE2, ADC_CH_VBUS, LL_ADC_SAMPLINGTIME_47CYCLES_5);
    LL_ADC_REG_SetTriggerSource(ADC_INSTANCE2, LL_ADC_REG_TRIG_SOFTWARE);

    /* ── Enable ADCs ────────────────────────────────────── */
    LL_ADC_Enable(ADC_INSTANCE1);
    LL_ADC_Enable(ADC_INSTANCE2);

    /* Wait for ready */
    while (!LL_ADC_IsActiveFlag_ADRDY(ADC_INSTANCE1)) { /* wait */ }
    while (!LL_ADC_IsActiveFlag_ADRDY(ADC_INSTANCE2)) { /* wait */ }

    /* Start injected conversions (triggered by TIM1 TRGO) */
    LL_ADC_INJ_StartConversion(ADC_INSTANCE1);
    LL_ADC_INJ_StartConversion(ADC_INSTANCE2);

    LOG_INF("FOC ADC initialized. COMMON_CCR=0x%08X ADC1_CR=0x%08X ADC2_CR=0x%08X",
            (unsigned)__LL_ADC_COMMON_INSTANCE(ADC1)->CCR,
            (unsigned)ADC1->CR, (unsigned)ADC2->CR);
    g_common_ccr_at_init = __LL_ADC_COMMON_INSTANCE(ADC1)->CCR;
    return 0;
}

/* ── API implementation ─────────────────────────────── */

void foc_adc_read_raw(foc_adc_raw_t *out)
{
    /* Pipeline mode: read PREVIOUS conversion result, then trigger NEXT.
     * At 30kHz ISR rate (33µs period), the previous conversion (≈1µs)
     * has long completed. This avoids JEOC polling in ISR context. */
    out->ia = (int16_t)(LL_ADC_INJ_ReadConversionData12(ADC_INSTANCE1, ADC_INJ_RANK2) >> 4);
    out->ib = (int16_t)(LL_ADC_INJ_ReadConversionData12(ADC_INSTANCE1, ADC_INJ_RANK1) >> 4);
    out->ic = (int16_t)(LL_ADC_INJ_ReadConversionData12(ADC_INSTANCE2, ADC_INJ_RANK1) >> 4);
    out->vbus = 0;

    /* Clear JEOC and trigger next conversion for next ISR call */
    LL_ADC_ClearFlag_JEOC(ADC_INSTANCE1);
    LL_ADC_ClearFlag_JEOC(ADC_INSTANCE2);
    LL_ADC_INJ_StartConversion(ADC_INSTANCE1);
    LL_ADC_INJ_StartConversion(ADC_INSTANCE2);
}

void foc_adc_start_vbus(void)
{
    LL_ADC_REG_StartConversion(ADC_INSTANCE2);
}

bool foc_adc_vbus_ready(void)
{
    return LL_ADC_IsActiveFlag_EOC(ADC_INSTANCE2);
}

float foc_adc_read_vbus_blocking(void)
{
    LL_ADC_ClearFlag_EOC(ADC_INSTANCE2);
    LL_ADC_REG_StartConversion(ADC_INSTANCE2);

    int timeout = 10000;
    while (!LL_ADC_IsActiveFlag_EOC(ADC_INSTANCE2) && timeout > 0) {
        timeout--;
        k_busy_wait(1);
    }

    int16_t raw = (int16_t)(LL_ADC_REG_ReadConversionData12(ADC_INSTANCE2) >> 4);
    return foc_adc_to_vbus(raw);
}

void foc_adc_get_offsets(int16_t *ia_offset, int16_t *ib_offset, int16_t *ic_offset)
{
    foc_adc_raw_t raw;
    int32_t sum_ia = 0, sum_ib = 0, sum_ic = 0;
    const int num_samples = 256;

    /*
     * Software-triggered injected conversions (no TIM1 needed).
     * PWM outputs stay disabled (MOE=0) — motor does not spin.
     */
    k_busy_wait(100);  /* let ADC settle */

    for (int i = 0; i < num_samples; i++) {
        /* Wait for a fresh conversion (one PWM cycle ~ 33 us) */
        k_busy_wait(50);
        foc_adc_read_raw(&raw);
        sum_ia += raw.ia;
        sum_ib += raw.ib;
        sum_ic += raw.ic;
    }

    *ia_offset = (int16_t)(sum_ia / num_samples);
    *ib_offset = (int16_t)(sum_ib / num_samples);
    *ic_offset = (int16_t)(sum_ic / num_samples);

    LOG_INF("ADC offsets: ia=%d ib=%d ic=%d", *ia_offset, *ib_offset, *ic_offset);
}

/* ── Software trigger diagnostic ─────────────────────── */

/* Stored results for telemetry readout */
typedef struct {
    int16_t sw_ia, sw_ib, sw_ic;
    int16_t sw_vbus_raw;
    float sw_vbus_v;
    bool valid;
} adc_sw_diag_t;

static adc_sw_diag_t g_adc_diag;

void foc_adc_sw_trigger_test(void)
{
    /*
     * Force a software-triggered injected conversion on ADC1 and ADC2
     * to verify the ADC itself works (bypasses TIM1 trigger).
     */
    g_adc_diag.valid = false;

    /* Diagnostic: dump ADC state */
    LOG_INF("ADC DIAG: ADC1_CR=0x%08x ADC2_CR=0x%08x",
            (unsigned)ADC1->CR, (unsigned)ADC2->CR);
    LOG_INF("ADC DIAG: ADC1_ISR=0x%08x ADC2_ISR=0x%08x",
            (unsigned)ADC1->ISR, (unsigned)ADC2->ISR);
    LOG_INF("ADC DIAG: ADC_COMMON_CCR=0x%08x RCC_CCIP=0x%08x",
            (unsigned)ADC12_COMMON->CCR, (unsigned)RCC->CCIPR);

    /* Force ADC enable if not already enabled */
    if (!(ADC1->CR & ADC_CR_ADEN)) {
        LOG_WRN("ADC1 not enabled! Forcing enable...");
        ADC1->CR &= ~ADC_CR_DEEPPWD;  /* disable deep power-down */
        ADC1->CR |= ADC_CR_ADVREGEN;  /* enable voltage regulator */
        k_busy_wait(20);
        ADC1->CR |= ADC_CR_ADEN;      /* enable ADC */
        k_busy_wait(100);
        LOG_INF("After force: ADC1_CR=0x%08x ISR=0x%08x",
                (unsigned)ADC1->CR, (unsigned)ADC1->ISR);
    }
    if (!(ADC2->CR & ADC_CR_ADEN)) {
        LOG_WRN("ADC2 not enabled! Forcing enable...");
        ADC2->CR &= ~ADC_CR_DEEPPWD;
        ADC2->CR |= ADC_CR_ADVREGEN;
        k_busy_wait(20);
        ADC2->CR |= ADC_CR_ADEN;
        k_busy_wait(100);
        LOG_INF("After force: ADC2_CR=0x%08x ISR=0x%08x",
                (unsigned)ADC2->CR, (unsigned)ADC2->ISR);
    }

    /* Temporarily switch to software trigger */
    LL_ADC_INJ_SetTriggerEdge(ADC_INSTANCE1, LL_ADC_INJ_TRIG_SOFTWARE);
    LL_ADC_INJ_SetTriggerSource(ADC_INSTANCE1, LL_ADC_INJ_TRIG_SOFTWARE);
    LL_ADC_INJ_SetTriggerEdge(ADC_INSTANCE2, LL_ADC_INJ_TRIG_SOFTWARE);
    LL_ADC_INJ_SetTriggerSource(ADC_INSTANCE2, LL_ADC_INJ_TRIG_SOFTWARE);

    /* CRITICAL: Clear stale flags from previous TIM1-triggered conversions */
    LL_ADC_ClearFlag_JEOC(ADC_INSTANCE1);
    LL_ADC_ClearFlag_JEOC(ADC_INSTANCE2);
    LL_ADC_ClearFlag_EOC(ADC_INSTANCE2);
    LL_ADC_ClearFlag_JEOS(ADC_INSTANCE1);
    LL_ADC_ClearFlag_JEOS(ADC_INSTANCE2);

    k_busy_wait(100);

    LOG_INF("PRE-CONV: ADC1_CR=0x%08X JSQR=0x%08X ISR=0x%08X",
            (unsigned)ADC1->CR, (unsigned)ADC1->JSQR, (unsigned)ADC1->ISR);
    LOG_INF("PRE-CONV: ADC2_CR=0x%08X JSQR=0x%08X ISR=0x%08X",
            (unsigned)ADC2->CR, (unsigned)ADC2->JSQR, (unsigned)ADC2->ISR);

    /* Force software conversion */
    LL_ADC_INJ_StartConversion(ADC_INSTANCE1);
    LL_ADC_INJ_StartConversion(ADC_INSTANCE2);

    k_busy_wait(10);  /* small delay for conversion to start */
    LOG_INF("POST-START: ADC1_CR=0x%08X ISR=0x%08X",
            (unsigned)ADC1->CR, (unsigned)ADC1->ISR);

    /* Wait for completion (JEOC flag) */
    int timeout = 10000;
    while (!LL_ADC_IsActiveFlag_JEOC(ADC_INSTANCE1) && timeout > 0) {
        timeout--;
        k_busy_wait(10);
    }
    timeout = 10000;
    while (!LL_ADC_IsActiveFlag_JEOC(ADC_INSTANCE2) && timeout > 0) {
        timeout--;
        k_busy_wait(10);
    }

    /* Read results */
    g_adc_diag.sw_ia = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC_INSTANCE1, ADC_INJ_RANK2);
    g_adc_diag.sw_ib = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC_INSTANCE1, ADC_INJ_RANK1);
    g_adc_diag.sw_ic = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC_INSTANCE2, ADC_INJ_RANK1);

    /* Also read Vbus (regular channel, software trigger) */
    LL_ADC_ClearFlag_EOC(ADC_INSTANCE2);  /* clear stale EOC before starting */
    LL_ADC_REG_StartConversion(ADC_INSTANCE2);
    timeout = 10000;
    while (!LL_ADC_IsActiveFlag_EOC(ADC_INSTANCE2) && timeout > 0) {
        timeout--;
        k_busy_wait(10);
    }
    g_adc_diag.sw_vbus_raw = (int16_t)(LL_ADC_REG_ReadConversionData12(ADC_INSTANCE2) >> 4);
    g_adc_diag.sw_vbus_v = foc_adc_to_vbus(g_adc_diag.sw_vbus_raw);

    /* Clear flags */
    LL_ADC_ClearFlag_JEOC(ADC_INSTANCE1);
    LL_ADC_ClearFlag_JEOC(ADC_INSTANCE2);
    LL_ADC_ClearFlag_EOC(ADC_INSTANCE2);

    /* Restore TIM1_TRGO trigger (source first, then edge to avoid JEXTEN=0 intermediate) */
    LL_ADC_INJ_SetTriggerSource(ADC_INSTANCE1, LL_ADC_INJ_TRIG_EXT_TIM1_TRGO);
    LL_ADC_INJ_SetTriggerSource(ADC_INSTANCE2, LL_ADC_INJ_TRIG_EXT_TIM1_TRGO);
    LL_ADC_INJ_SetTriggerEdge(ADC_INSTANCE1, LL_ADC_INJ_TRIG_EXT_RISING);
    LL_ADC_INJ_SetTriggerEdge(ADC_INSTANCE2, LL_ADC_INJ_TRIG_EXT_RISING);

    /* Re-start injected conversions (waiting for trigger) */
    LL_ADC_INJ_StartConversion(ADC_INSTANCE1);
    LL_ADC_INJ_StartConversion(ADC_INSTANCE2);

    g_adc_diag.valid = true;
}

void foc_adc_get_sw_diag(int16_t *ia, int16_t *ib, int16_t *ic,
                         int16_t *vbus_raw, float *vbus_v, bool *valid)
{
    *ia = g_adc_diag.sw_ia;
    *ib = g_adc_diag.sw_ib;
    *ic = g_adc_diag.sw_ic;
    *vbus_raw = g_adc_diag.sw_vbus_raw;
    *vbus_v = g_adc_diag.sw_vbus_v;
    *valid = g_adc_diag.valid;
}

uint32_t foc_adc_get_common_ccr_init(void)
{
    return g_common_ccr_at_init;
}
