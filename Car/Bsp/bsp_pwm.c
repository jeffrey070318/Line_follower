/**
  ******************************************************************************
  * @file	 bsp_PWM.c
  * @author  Wang Hongxi
  * @version V1.0.0
  * @date    2020/3/1
  * @brief
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
#include "bsp_pwm.h"
#include "stm32h7xx_hal_rcc_ex.h"

#include <math.h>
#include <stddef.h>

/* 固定实例池：注册时不再依赖堆，避免运行期 malloc 失败导致蜂鸣器失效。 */
static PWMInstance pwm_pool[PWM_DEVICE_CNT];
static uint8_t pwm_count;

static uint32_t PWM_TimerClock(TIM_HandleTypeDef *htim)
{
    if (htim == NULL || htim->Instance == NULL)
        return 0u;

    /* H7 的定时器时钟由 D1PCLK1 驱动，使用专用 API 查当前实际频率，
     * 不像参考工程那样按外设地址区间猜 APB1/APB2，那个判断在本芯片上不可靠。 */
    return HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_TIM);
}

static uint32_t PWM_ChannelCompareIndex(uint32_t channel)
{
    /* 返回 1~4 便于用 0 表示非法通道，因为 TIM_CHANNEL_1 的枚举值是 0。 */
    switch (channel)
    {
    case TIM_CHANNEL_1:
        return 1u;
    case TIM_CHANNEL_2:
        return 2u;
    case TIM_CHANNEL_3:
        return 3u;
    case TIM_CHANNEL_4:
        return 4u;
    default:
        return 0u;
    }
}

PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    if (config == NULL || config->htim == NULL || config->htim->Instance == NULL ||
        PWM_ChannelCompareIndex(config->channel) == 0u || pwm_count >= PWM_DEVICE_CNT)
        return NULL;

    PWMInstance *pwm = &pwm_pool[pwm_count];
    pwm->htim = config->htim;
    pwm->channel = config->channel;
    pwm->tclk = PWM_TimerClock(config->htim);
    pwm->period = 0.001f;
    pwm->dutyratio = 0.0f;
    pwm->callback = config->callback;
    pwm->id = config->id;

    if (pwm->tclk == 0u)
        return NULL;

    if (HAL_TIM_PWM_Start(pwm->htim, pwm->channel) != HAL_OK)
        return NULL;

    /* 先写入用户配置的周期与占空比；period=0 视为保持当前 ARR。 */
    if (config->period > 0.0f)
        PWMSetPeriod(pwm, config->period);
    if (config->dutyratio > 0.0f)
        PWMSetDutyRatio(pwm, config->dutyratio);
    else
        PWMSetDutyRatio(pwm, 0.0f);

    pwm_count++;
    return pwm;
}

void PWMStart(PWMInstance *pwm)
{
    if (pwm != NULL && pwm->htim != NULL)
        (void)HAL_TIM_PWM_Start(pwm->htim, pwm->channel);
}

void PWMStop(PWMInstance *pwm)
{
    if (pwm != NULL && pwm->htim != NULL)
        (void)HAL_TIM_PWM_Stop(pwm->htim, pwm->channel);
}

void PWMSetDutyRatio(PWMInstance *pwm, float dutyratio)
{
    if (pwm == NULL || pwm->htim == NULL || pwm->htim->Instance == NULL)
        return;

    if (!isfinite(dutyratio) || dutyratio < 0.0f)
        dutyratio = 0.0f;
    if (dutyratio > 1.0f)
        dutyratio = 1.0f;

    pwm->dutyratio = dutyratio;
    /* ARR 由 PWMSetPeriod 维护，用当前实际 ARR 换算比较值。 */
    const uint32_t arr = pwm->htim->Instance->ARR;
    __HAL_TIM_SET_COMPARE(pwm->htim, pwm->channel, (uint32_t)(dutyratio * (float)arr));
}

void PWMSetPeriod(PWMInstance *pwm, float period)
{
    if (pwm == NULL || pwm->htim == NULL || pwm->htim->Instance == NULL || pwm->tclk == 0u)
        return;

    if (!isfinite(period) || period <= 0.0f)
        return;

    /* 计数上限：CubeMX 生成的 ARR 初值 65535 表示 16 位定时器（TIM12 就是 16 位）。 */
    const uint32_t max_ticks =
        (pwm->htim->Instance->ARR > 0xFFFFu) ? 0xFFFFFFFFu : 0xFFFFu;

    /* 先用当前预分频试算，不够时自动放大预分频。
     * 16 位定时器在 240 MHz 下直接输出 523 Hz 需要约 45.9 万计数，ARR 装不下，
     * 若不自动预分频，蜂鸣器会一直停在注册时的 1 kHz 上。 */
    uint32_t pscaler = pwm->htim->Init.Prescaler + 1u;
    float ticks = period * ((float)pwm->tclk / (float)pscaler);

    if (ticks > (float)max_ticks)
    {
        const float need = period * (float)pwm->tclk / (float)max_ticks;
        uint32_t want = (uint32_t)ceilf(need);
        if (want < 2u)
            want = 2u;

        /* 预分频寄存器只有低 4 位，向上取到 16 的倍数。 */
        want = (want + 15u) & ~15u;
        if (want > 65536u)
            return; /* 目标频率低到该定时器无法产生，直接放弃并保持原输出。 */

        __HAL_TIM_SET_PRESCALER(pwm->htim, want - 1u);
        pscaler = want;
        ticks = period * ((float)pwm->tclk / (float)pscaler);
        if (ticks < 2.0f)
            return;
    }

    pwm->period = period;
    __HAL_TIM_SET_AUTORELOAD(pwm->htim, (uint32_t)ticks);
}

void TIM_Set_PWM(TIM_HandleTypeDef *tim_pwmHandle, uint8_t Channel, uint16_t value)
{
    if (value > tim_pwmHandle->Instance->ARR)
        value = tim_pwmHandle->Instance->ARR;

    switch (Channel)
    {
    case TIM_CHANNEL_1:
        tim_pwmHandle->Instance->CCR1 = value;
        break;
    case TIM_CHANNEL_2:
        tim_pwmHandle->Instance->CCR2 = value;
        break;
    case TIM_CHANNEL_3:
        tim_pwmHandle->Instance->CCR3 = value;
        break;
    case TIM_CHANNEL_4:
        tim_pwmHandle->Instance->CCR4 = value;
        break;
    }
}