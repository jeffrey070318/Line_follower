/**
 * @file key_task.c
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 启停与灰度校准按键处理
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#include "key_task.h"
#include "key.h"
#include "Car_def.h"
#include "chassis_task.h"

static KeyInstance *start_key, *calibrate_key;
static uint8_t initialized;

uint8_t KeyTask_Init(void)
{
    if (initialized) return 1u;
    const Key_Init_Config_s start_config = {
        .gpio = {.GPIOx = KEY_START_PORT, .GPIO_Pin = KEY_START_PIN,
                 .exti_mode = GPIO_EXTI_MODE_NONE},
        .active_level = KEY_ACTIVE_LEVEL, .debounce_ms = KEY_DEBOUNCE_MS,
    };
    const Key_Init_Config_s calibrate_config = {
        .gpio = {.GPIOx = KEY_CALIBRATE_PORT, .GPIO_Pin = KEY_CALIBRATE_PIN,
                 .exti_mode = GPIO_EXTI_MODE_NONE},
        .active_level = KEY_ACTIVE_LEVEL, .debounce_ms = KEY_DEBOUNCE_MS,
    };
    start_key = KeyRegister(&start_config);
    calibrate_key = KeyRegister(&calibrate_config);
    initialized = start_key && calibrate_key;
    return initialized;
}

void KeyTask_Step(void)
{
    if (!initialized) return;
    const uint32_t now = HAL_GetTick();
    const KeyEvent_e start = KeyScan(start_key, now);
    const KeyEvent_e calibrate = KeyScan(calibrate_key, now);
    /* 两键同时点击时优先校准，采样期间不启动。 */
    if (calibrate == KEY_CLICK) Chassis_CalibrateNext();
    else if (start == KEY_CLICK) {
        const ChassisStatus_s status = Chassis_GetStatus();
        Chassis_SetEnabled(status.state != CHASSIS_RUNNING);
    }
}
