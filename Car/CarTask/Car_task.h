/**
 * @file Car_task.h
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 小车周期任务与线程创建
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#ifndef CAR_TASK_H
#define CAR_TASK_H

/* 仅由 Car.c 包含，集中创建应用任务。 */
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Car_def.h"
#include "chassis_task.h"
#include "key_task.h"
#include "led_task.h"

static void StartChassisTask(void const *argument)
{
    (void)argument;
    const TickType_t period = pdMS_TO_TICKS(CHASSIS_PERIOD_MS);
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        Chassis_Step();
        KeyTask_Step();
        LEDTask_Step();
        const TickType_t now = xTaskGetTickCount();
        if ((TickType_t)(now - wake) >= period) wake = now;
        vTaskDelayUntil(&wake, period);
    }
}

static uint8_t OSTaskInit(void)
{
    /* 当前 CMSIS-RTOS v1 封装以 32 位字为栈深度单位。 */
    osThreadDef(CHASSIS_TASK, StartChassisTask, osPriorityAboveNormal, 0, 256);
    return osThreadCreate(osThread(CHASSIS_TASK), NULL) != NULL;
}
#endif
