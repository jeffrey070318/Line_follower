/**
 * @file led_task.c
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 校准与底盘状态 LED 指示
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#include "led_task.h"
#include "led.h"
#include "Car_def.h"
#include "chassis_task.h"

static LEDInstance *status_led;

uint8_t LEDTask_Init(void)
{
    if (status_led) return 1u;
    const LED_Init_Config_s config = {
        .gpio = {.GPIOx = CAR_LED_PORT, .GPIO_Pin = CAR_LED_PIN,
                 .exti_mode = GPIO_EXTI_MODE_NONE},
        .active_level = CAR_LED_ACTIVE_LEVEL,
    };
    status_led = LEDRegister(&config);
    return status_led != NULL;
}

void LEDTask_Step(void)
{
    if (!status_led) return;
    const ChassisStatus_s status = Chassis_GetStatus();
    uint8_t count = 0u;
    if (status.state == CHASSIS_SENSOR_FAULT) count = 5u;
    else if (status.state == CHASSIS_LINE_LOST) count = 6u;
    else if (status.calibration == GRAY_CAL_FAILED) count = 4u;
    else if (status.calibration == GRAY_CAL_WHITE || status.calibration == GRAY_CAL_BLACK)
        count = 2u;
    else if (status.calibration == GRAY_CAL_WAIT_BLACK) count = 3u;
    else if (!status.calibrated) count = 1u;
    const uint32_t now = HAL_GetTick();
    (void)LEDSetBlinkCount(status_led, count, now);
    LEDUpdate(status_led, now);
}
