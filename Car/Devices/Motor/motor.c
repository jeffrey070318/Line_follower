/**
 * @file motor.c
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 28BYJ48 步进电机半步驱动实现
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#include "motor.h"

static MotorInstance motor_pool[MOTOR_DEVICE_CNT];
static uint8_t motor_count;
static const uint8_t half_step[8] = {1u, 3u, 2u, 6u, 4u, 12u, 8u, 9u};

static void MotorWrite(MotorInstance *motor, uint8_t mask)
{
    for (unsigned i = 0; i < 4u; i++) {
        if (mask & (1u << i)) GPIOSet(motor->coil[i]);
        else GPIOReset(motor->coil[i]);
    }
}

MotorInstance *MotorRegister(const Motor_Init_Config_s *config)
{
    if (!config || !config->port || motor_count >= MOTOR_DEVICE_CNT)
        return NULL;
    for (unsigned i = 0; i < 4u; i++) {
        if (!config->pin[i] || (config->pin[i] & (config->pin[i] - 1u)))
            return NULL;
        for (unsigned j = 0; j < i; j++)
            if (config->pin[i] == config->pin[j]) return NULL;
        for (unsigned j = 0; j < motor_count; j++)
            for (unsigned k = 0; k < 4u; k++)
                if (motor_pool[j].coil[k]->GPIOx == config->port &&
                    motor_pool[j].coil[k]->GPIO_Pin == config->pin[i]) return NULL;
    }
    if (!GPIOCanRegister(4u)) return NULL;
    MotorInstance *motor = &motor_pool[motor_count];
    for (unsigned i = 0; i < 4u; i++) {
        GPIO_Init_Config_s gpio = {
            .GPIOx = config->port, .GPIO_Pin = config->pin[i],
            .exti_mode = GPIO_EXTI_MODE_NONE,
        };
        motor->coil[i] = GPIORegister(&gpio);
        if (!motor->coil[i]) return NULL;
    }
    motor->phase = 7u;
    motor->reverse = config->reverse;
    MotorWrite(motor, 0u);
    motor_count++;
    return motor;
}

void MotorSetSpeed(MotorInstance *motor, int16_t speed)
{
    if (!motor) return;
    if (speed > MOTOR_MAX_STEP_HZ) speed = MOTOR_MAX_STEP_HZ;
    if (speed < -MOTOR_MAX_STEP_HZ) speed = -MOTOR_MAX_STEP_HZ;
    const uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if ((speed > 0 && motor->speed < 0) || (speed < 0 && motor->speed > 0))
        motor->accumulator = 0u;
    motor->speed = speed;
    motor->command_age = 0u;
    if (!speed) {
        motor->accumulator = 0u;
        MotorWrite(motor, 0u);
    }
    __set_PRIMASK(mask);
}

void MotorControl(void)
{
    for (unsigned i = 0; i < motor_count; i++) {
        MotorInstance *motor = &motor_pool[i];
        if (!motor->speed) continue;
        if (++motor->command_age >= MOTOR_COMMAND_TIMEOUT_MS) {
            motor->speed = 0;
            motor->accumulator = 0u;
            MotorWrite(motor, 0u);
            continue;
        }
        const int16_t speed = motor->reverse ? -motor->speed : motor->speed;
        motor->accumulator += (uint16_t)(speed < 0 ? -speed : speed);
        if (motor->accumulator >= MOTOR_TICK_HZ) {
            motor->accumulator -= MOTOR_TICK_HZ;
            motor->phase = (uint8_t)((motor->phase + (speed > 0 ? 1u : 7u)) & 7u);
            MotorWrite(motor, half_step[motor->phase]);
        }
    }
}
