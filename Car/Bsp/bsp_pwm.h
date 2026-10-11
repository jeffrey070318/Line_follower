/**
  ******************************************************************************
  * @file	 bsp_PWM.h
  * @author  Wang Hongxi
  * @version V1.0.0
  * @date    2020/3/1
  * @brief   
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
#ifndef BSP_PWM_H
#define BSP_PWM_H

#include <stdint.h>
#include "tim.h"

/* 旧接口：IMU 加热等模块直接写 CCR，保留兼容。 */
void TIM_Set_PWM(TIM_HandleTypeDef *tim_pwmHandle, uint8_t Channel, uint16_t value);

#define PWM_DEVICE_CNT 8u

/* PWM 实例：封装一个定时器通道，供蜂鸣器、舵机、LED 等模块注册使用。 */
typedef struct PWMInstance
{
    TIM_HandleTypeDef *htim;                  /* 定时器句柄 */
    uint32_t channel;                         /* 通道，如 TIM_CHANNEL_2 */
    uint32_t tclk;                            /* 定时器时钟频率（Hz） */
    float period;                             /* 周期，单位 s */
    float dutyratio;                          /* 占空比 0~1 */
    void (*callback)(struct PWMInstance *);   /* 可选回调 */
    void *id;                                 /* 用户自定义 ID */
} PWMInstance;

/* PWM 注册配置。 */
typedef struct
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    float period;
    float dutyratio;
    void (*callback)(PWMInstance *);
    void *id;
} PWM_Init_Config_s;

/* 注册一个 PWM 实例并启动输出；配置无效或池已满返回 NULL。 */
PWMInstance *PWMRegister(PWM_Init_Config_s *config);

/* 启动/停止 PWM 输出。 */
void PWMStart(PWMInstance *pwm);
void PWMStop(PWMInstance *pwm);

/* 设置占空比（0~1），内部限幅。 */
void PWMSetDutyRatio(PWMInstance *pwm, float dutyratio);

/* 设置周期（单位 s），内部限幅到定时器计数范围。 */
void PWMSetPeriod(PWMInstance *pwm, float period);

#endif /* BSP_PWM_H */
