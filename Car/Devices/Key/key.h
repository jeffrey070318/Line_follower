/**
 * @file key.h
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 通用按键注册与扫描接口
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#ifndef KEY_H
#define KEY_H
#include "bsp_gpio.h"

#define KEY_DEVICE_CNT 2u
typedef enum { KEY_NONE, KEY_CLICK } KeyEvent_e;
typedef struct {
    GPIO_Init_Config_s gpio;
    GPIO_PinState active_level;
    uint16_t debounce_ms;
} Key_Init_Config_s;
typedef struct {
    GPIOInstance *gpio;
    uint32_t changed_ms;
    uint16_t debounce_ms;
    GPIO_PinState active_level;
    uint8_t candidate;
    uint8_t pressed;
    uint8_t armed;
} KeyInstance;

/* 输入与上下拉由 CubeMX 配置，扫描由同一个任务负责。 */
KeyInstance *KeyRegister(const Key_Init_Config_s *config);
/* 消抖后松开返回一次点击，长按不重复触发。 */
KeyEvent_e KeyScan(KeyInstance *key, uint32_t now_ms);
#endif
