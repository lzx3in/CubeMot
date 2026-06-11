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
#include <stm32g4xx_ll_adc.h>
#include <stm32g4xx_ll_bus.h>
#include <stm32g4xx_ll_rcc.h>
#include <stm32g4xx_ll_gpio.h>
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

int foc_adc_init(void)
{
    LOG_INF("Initializing FOC ADC (ADC1+ADC2)");

    /* Enable ADC clocks */
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
    LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_CLOCK_ASYNC_DIV1);

    /* ── ADC1: Calibrate ───────────────────────────────── */
    LL_ADC_DisableDeepPowerDown(ADC_INSTANCE1);
    LL_ADC_EnableInternalRegulator(ADC_INSTANCE1);
    k_busy_wait(20);
    LL_ADC_StartCalibration(ADC_INSTANCE1, LL_ADC_SINGLE_ENDED);
    while (LL_ADC_IsCalibrationOnGoing(ADC_INSTANCE1)) { /* wait */ }

    LL_ADC_SetResolution(ADC_INSTANCE1, LL_ADC_RESOLUTION_12B);
    LL_ADC_SetDataAlignment(ADC_INSTANCE1, LL_ADC_DATA_ALIGN_LEFT);
    LL_ADC_SetLowPowerMode(ADC_INSTANCE1, LL_ADC_LP_AUTOWAIT);

    /* Injected sequence: 2 conversions */
    LL_ADC_INJ_SetSequencerLength(ADC_INSTANCE1, LL_ADC_INJ_SEQ_SCAN_ENABLE_2RANKS);

    /* Rank 1: CH14 (Ib shared) */
    LL_ADC_INJ_SetSequencerRanks(ADC_INSTANCE1, ADC_INJ_RANK1, ADC_CH_IB);
    LL_ADC_SetChannelSamplingTime(ADC_INSTANCE1, ADC_CH_IB, LL_ADC_SAMPLINGTIME_6CYCLES_5);

    /* Rank 2: CH2 (Ia) */
    LL_ADC_INJ_SetSequencerRanks(ADC_INSTANCE1, ADC_INJ_RANK2, ADC_CH_IA);
    LL_ADC_SetChannelSamplingTime(ADC_INSTANCE1, ADC_CH_IA, LL_ADC_SAMPLINGTIME_6CYCLES_5);

    /* Trigger: TIM1_TRGO, rising edge */
    LL_ADC_INJ_SetTriggerSource(ADC_INSTANCE1, LL_ADC_INJ_TRIG_EXT_TIM1_TRGO);
    LL_ADC_INJ_SetTriggerEdge(ADC_INSTANCE1, LL_ADC_INJ_TRIG_EXT_RISING);

    /* ── ADC2: Calibrate ───────────────────────────────── */
    LL_ADC_DisableDeepPowerDown(ADC_INSTANCE2);
    LL_ADC_EnableInternalRegulator(ADC_INSTANCE2);
    k_busy_wait(20);
    LL_ADC_StartCalibration(ADC_INSTANCE2, LL_ADC_SINGLE_ENDED);
    while (LL_ADC_IsCalibrationOnGoing(ADC_INSTANCE2)) { /* wait */ }

    LL_ADC_SetResolution(ADC_INSTANCE2, LL_ADC_RESOLUTION_12B);
    LL_ADC_SetDataAlignment(ADC_INSTANCE2, LL_ADC_DATA_ALIGN_LEFT);
    LL_ADC_SetLowPowerMode(ADC_INSTANCE2, LL_ADC_LP_AUTOWAIT);

    /* Injected sequence: 2 conversions */
    LL_ADC_INJ_SetSequencerLength(ADC_INSTANCE2, LL_ADC_INJ_SEQ_SCAN_ENABLE_2RANKS);

    /* Rank 1: CH4 (Ic) */
    LL_ADC_INJ_SetSequencerRanks(ADC_INSTANCE2, ADC_INJ_RANK1, ADC_CH_IC);
    LL_ADC_SetChannelSamplingTime(ADC_INSTANCE2, ADC_CH_IC, LL_ADC_SAMPLINGTIME_6CYCLES_5);

    /* Rank 2: CH14 (Ib shared) */
    LL_ADC_INJ_SetSequencerRanks(ADC_INSTANCE2, ADC_INJ_RANK2, ADC_CH_IB);
    LL_ADC_SetChannelSamplingTime(ADC_INSTANCE2, ADC_CH_IB, LL_ADC_SAMPLINGTIME_6CYCLES_5);

    /* Trigger: TIM1_TRGO, rising edge */
    LL_ADC_INJ_SetTriggerSource(ADC_INSTANCE2, LL_ADC_INJ_TRIG_EXT_TIM1_TRGO);
    LL_ADC_INJ_SetTriggerEdge(ADC_INSTANCE2, LL_ADC_INJ_TRIG_EXT_RISING);

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

    LOG_INF("FOC ADC initialized");
    return 0;
}

/* ── API implementation ─────────────────────────────── */

void foc_adc_read_raw(foc_adc_raw_t *out)
{
    /* ADC1 injected data registers:
     * Rank 1 → JDR1 (CH14 = Ib)
     * Rank 2 → JDR2 (CH2  = Ia)
     */
    out->ia = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC_INSTANCE1, ADC_INJ_RANK2);
    out->ib = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC_INSTANCE1, ADC_INJ_RANK1);

    /* ADC2 injected data registers:
     * Rank 1 → JDR1 (CH4  = Ic)
     */
    out->ic = (int16_t)LL_ADC_INJ_ReadConversionData12(ADC_INSTANCE2, ADC_INJ_RANK1);
    out->vbus = 0;
}

void foc_adc_start_vbus(void)
{
    LL_ADC_REG_StartConversion(ADC_INSTANCE2);
}

bool foc_adc_vbus_ready(void)
{
    return LL_ADC_IsActiveFlag_EOC(ADC_INSTANCE2);
}

void foc_adc_get_offsets(int16_t *ia_offset, int16_t *ib_offset, int16_t *ic_offset)
{
    foc_adc_raw_t raw;
    int32_t sum_ia = 0, sum_ib = 0, sum_ic = 0;
    const int num_samples = 256;

    for (int i = 0; i < num_samples; i++) {
        foc_adc_read_raw(&raw);
        sum_ia += raw.ia;
        sum_ib += raw.ib;
        sum_ic += raw.ic;
        k_busy_wait(50);
    }

    *ia_offset = (int16_t)(sum_ia / num_samples);
    *ib_offset = (int16_t)(sum_ib / num_samples);
    *ic_offset = (int16_t)(sum_ic / num_samples);

    LOG_INF("ADC offsets: ia=%d ib=%d ic=%d", *ia_offset, *ib_offset, *ic_offset);
}
