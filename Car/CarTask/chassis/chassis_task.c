/**
 * @file chassis_task.c
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 底盘循迹、灰度校准与停车控制
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#include "chassis_task.h"
#include "Car_def.h"
#include "motor.h"
#include "grayscale.h"
#include "line_control.h"
#include "FreeRTOS.h"
#include "task.h"

static MotorInstance *left_motor, *right_motor;
static GrayscaleInstance *left_gray, *right_gray;
static ChassisStatus_s status;
static uint8_t initialized, enabled, tracking_seen, both_white_seen;
static uint32_t both_white_since;
static uint32_t sample_left_sum, sample_right_sum, sample_last_ms;
static uint16_t sample_count;
static uint8_t sample_seen;

static void ChassisOutput(int16_t left, int16_t right)
{
    MotorSetSpeed(left_motor, left);
    MotorSetSpeed(right_motor, right);
    status.left_hz = left;
    status.right_hz = right;
}

uint8_t Chassis_Init(void)
{
    if (initialized) return 1u;
    const Motor_Init_Config_s motor_left = {
        .port = MOTOR_LEFT_PORT, .pin = MOTOR_LEFT_PINS,
        .reverse = MOTOR_LEFT_REVERSE,
    };
    const Motor_Init_Config_s motor_right = {
        .port = MOTOR_RIGHT_PORT, .pin = MOTOR_RIGHT_PINS,
        .reverse = MOTOR_RIGHT_REVERSE,
    };
    const Grayscale_Init_Config_s gray_left = {
        .adc = {.hadc = &hadc1, .channel = GRAY_LEFT_CHANNEL},
        .threshold = GRAY_LEFT_THRESHOLD, .hysteresis = GRAY_HYSTERESIS,
        .black_high = GRAY_LEFT_BLACK_HIGH,
    };
    const Grayscale_Init_Config_s gray_right = {
        .adc = {.hadc = &hadc1, .channel = GRAY_RIGHT_CHANNEL},
        .threshold = GRAY_RIGHT_THRESHOLD, .hysteresis = GRAY_HYSTERESIS,
        .black_high = GRAY_RIGHT_BLACK_HIGH,
    };
    left_motor = MotorRegister(&motor_left);
    right_motor = MotorRegister(&motor_right);
    left_gray = GrayscaleRegister(&gray_left);
    right_gray = GrayscaleRegister(&gray_right);
    if (!left_motor || !right_motor || !left_gray || !right_gray ||
        HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
        ChassisOutput(0, 0);
        return 0u;
    }
    initialized = 1u;
    status.state = CHASSIS_DISABLED;
    status.threshold_left = left_gray->threshold;
    status.threshold_right = right_gray->threshold;
    status.black_high_left = left_gray->black_high;
    status.black_high_right = right_gray->black_high;
    return 1u;
}

void Chassis_SetEnabled(uint8_t value)
{
    taskENTER_CRITICAL();
    if (value && (!initialized || !status.calibrated)) {
        taskEXIT_CRITICAL();
        return;
    }
    enabled = initialized && value;
    tracking_seen = both_white_seen = 0u;
    status.state = enabled ? CHASSIS_RUNNING : CHASSIS_DISABLED;
    if (!enabled) ChassisOutput(0, 0);
    taskEXIT_CRITICAL();
}

void Chassis_CalibrateNext(void)
{
    if (!initialized) return;
    taskENTER_CRITICAL();
    if (status.calibration != GRAY_CAL_WHITE && status.calibration != GRAY_CAL_BLACK) {
        enabled = 0u;
        tracking_seen = both_white_seen = 0u;
        ChassisOutput(0, 0);
        status.calibrated = 0u;
        status.state = CHASSIS_CALIBRATING;
        if (status.calibration == GRAY_CAL_WAIT_BLACK)
            status.calibration = GRAY_CAL_BLACK;
        else {
            status.calibration = GRAY_CAL_WHITE;
            status.white_left = status.white_right = 0u;
            status.black_left = status.black_right = 0u;
        }
        sample_count = 0u;
        sample_seen = 0u;
        sample_left_sum = sample_right_sum = 0u;
    }
    taskEXIT_CRITICAL();
}

static void CalibrationSample(void)
{
    const uint32_t now = HAL_GetTick();
    if (sample_seen && (uint32_t)(now - sample_last_ms) < CHASSIS_PERIOD_MS) return;
    sample_seen = 1u;
    sample_last_ms = now;
    sample_left_sum += left_gray->raw;
    sample_right_sum += right_gray->raw;
    if (++sample_count < GRAY_CALIBRATION_SAMPLES) return;
    const uint16_t left = (uint16_t)(sample_left_sum / sample_count);
    const uint16_t right = (uint16_t)(sample_right_sum / sample_count);
    if (status.calibration == GRAY_CAL_WHITE) {
        status.white_left = left;
        status.white_right = right;
        status.calibration = GRAY_CAL_WAIT_BLACK;
        return;
    }
    status.black_left = left;
    status.black_right = right;
    /* 两路校准同时提交，避免单路失败造成配置只更新一半。 */
    GrayscaleInstance next_left = *left_gray, next_right = *right_gray;
    next_left.hysteresis = next_right.hysteresis = GRAY_HYSTERESIS;
    if (!GrayscaleCalibrate(&next_left, status.white_left, left) ||
        !GrayscaleCalibrate(&next_right, status.white_right, right)) {
        status.calibration = GRAY_CAL_FAILED;
    } else {
        *left_gray = next_left;
        *right_gray = next_right;
        status.threshold_left = left_gray->threshold;
        status.threshold_right = right_gray->threshold;
        status.black_high_left = left_gray->black_high;
        status.black_high_right = right_gray->black_high;
        status.calibrated = 1u;
        status.calibration = GRAY_CAL_READY;
    }
    status.state = CHASSIS_DISABLED;
}

void Chassis_Step(void)
{
    if (!initialized) return;
    /* ADC 轮询依赖 HAL 时基，采样必须放在临界区外。 */
    const uint8_t left_valid = GrayscaleRead(left_gray);
    const uint8_t right_valid = GrayscaleRead(right_gray);
    taskENTER_CRITICAL();
    status.gray_left = left_gray->raw;
    status.gray_right = right_gray->raw;
    status.left_black = left_gray->black;
    status.right_black = right_gray->black;
    if (!left_valid || !right_valid) {
        enabled = 0u;
        if (status.calibration == GRAY_CAL_WHITE || status.calibration == GRAY_CAL_BLACK ||
            status.calibration == GRAY_CAL_WAIT_BLACK) {
            status.calibration = GRAY_CAL_FAILED;
            status.calibrated = 0u;
        }
        status.state = CHASSIS_SENSOR_FAULT;
        ChassisOutput(0, 0);
    } else if (status.calibration == GRAY_CAL_WHITE || status.calibration == GRAY_CAL_BLACK) {
        ChassisOutput(0, 0);
        CalibrationSample();
    } else if (!enabled) {
        ChassisOutput(0, 0);
    } else {
        const LineOutput_s output = LineControl(left_gray->black, right_gray->black,
                                                CHASSIS_STRAIGHT_HZ, CHASSIS_INNER_HZ);
        if (output.stop) {
            if (!tracking_seen) {
                ChassisOutput(0, 0);
            } else {
                const uint32_t now = HAL_GetTick();
                if (!both_white_seen) {
                    both_white_seen = 1u;
                    both_white_since = now;
                }
                if ((uint32_t)(now - both_white_since) >= CHASSIS_LINE_LOST_MS) {
                    enabled = 0u;
                    status.state = CHASSIS_LINE_LOST;
                    ChassisOutput(0, 0);
                } else {
                    ChassisOutput(CHASSIS_INNER_HZ, CHASSIS_INNER_HZ);
                }
            }
        } else {
            if (left_gray->black && right_gray->black) tracking_seen = 1u;
            both_white_seen = 0u;
            status.state = CHASSIS_RUNNING;
            ChassisOutput(output.left, output.right);
        }
    }
    taskEXIT_CRITICAL();
}

ChassisStatus_s Chassis_GetStatus(void)
{
    taskENTER_CRITICAL();
    const ChassisStatus_s snapshot = status;
    taskEXIT_CRITICAL();
    return snapshot;
}
