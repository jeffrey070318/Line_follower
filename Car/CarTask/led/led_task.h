/**
 * @file led_task.h
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 状态 LED 初始化与周期更新接口
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#ifndef LED_TASK_H
#define LED_TASK_H
#include <stdint.h>

uint8_t LEDTask_Init(void);
void LEDTask_Step(void);
#endif
