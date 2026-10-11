/**
 * @file led.h
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 通用 LED 注册与闪烁控制接口
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#ifndef LED_H
#define LED_H
#include "bsp_gpio.h"

#define LED_MAX_BLINK_COUNT 6u

typedef struct {
    GPIO_Init_Config_s gpio;
    GPIO_PinState active_level;
} LED_Init_Config_s;
typedef struct {
    GPIOInstance *gpio;
    GPIO_PinState active_level;
    uint32_t cycle_ms;
    uint8_t blink_count;
} LEDInstance;

/* 输出模式由 CubeMX 配置，更新由同一个任务负责。 */
LEDInstance *LEDRegister(const LED_Init_Config_s *config);
/* 0 为常亮，1..6 为每秒闪烁次数；重复设置相同次数不重置时序。 */
uint8_t LEDSetBlinkCount(LEDInstance *led, uint8_t count, uint32_t now_ms);
void LEDUpdate(LEDInstance *led, uint32_t now_ms);
#endif
