/**
 * @file motor.h
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 步进电机注册与步频控制接口
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#ifndef MOTOR_H
#define MOTOR_H
#include "bsp_gpio.h"

#define MOTOR_DEVICE_CNT 2u
#define MOTOR_TICK_HZ 1000u
#define MOTOR_MAX_STEP_HZ 500
#define MOTOR_COMMAND_TIMEOUT_MS 100u

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin[4]; /* 按 ULN2003 IN1..IN4 顺序配置。 */
    uint8_t reverse;
} Motor_Init_Config_s;

typedef struct {
    GPIOInstance *coil[4];
    int16_t speed;
    uint16_t accumulator;
    uint16_t command_age;
    uint8_t phase;
    uint8_t reverse;
} MotorInstance;

/* 启动定时器前注册；速度为带方向的半步数每秒。 */
MotorInstance *MotorRegister(const Motor_Init_Config_s *config);
void MotorSetSpeed(MotorInstance *motor, int16_t speed);
/* 仅在 1kHz 定时器中断中调用。 */
void MotorControl(void);
#endif
