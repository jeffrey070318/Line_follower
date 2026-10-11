/**
 * @file Car.c
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 小车设备与任务初始化
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#include "Car.h"
#include "Car_task.h"
#include "motor.h"
#include "tim.h"

void CarInit(void)
{
    static uint8_t initialized;
    if (initialized) return;
    if (!Chassis_Init() || !KeyTask_Init() || !LEDTask_Init() || !OSTaskInit()) Error_Handler();
    /* 先完成设备注册，再启动电机换相定时器。 */
    __HAL_TIM_SET_COUNTER(&htim2, 0u);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) Error_Handler();
    initialized = 1u;
}
