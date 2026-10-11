/**
 * @file chassis_task.h
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 底盘状态、启停与灰度校准接口
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#ifndef CHASSIS_TASK_H
#define CHASSIS_TASK_H
#include <stdint.h>

typedef enum {
    CHASSIS_DISABLED,
    CHASSIS_RUNNING,
    CHASSIS_SENSOR_FAULT,
    CHASSIS_LINE_LOST,
    CHASSIS_CALIBRATING,
} ChassisState_e;

typedef enum {
    GRAY_CAL_IDLE,
    GRAY_CAL_WHITE,
    GRAY_CAL_WAIT_BLACK,
    GRAY_CAL_BLACK,
    GRAY_CAL_READY,
    GRAY_CAL_FAILED,
} GrayCalibration_e;

typedef struct {
    uint16_t gray_left;
    uint16_t gray_right;
    int16_t left_hz;
    int16_t right_hz;
    uint8_t left_black;
    uint8_t right_black;
    uint8_t calibrated;
    uint8_t black_high_left;
    uint8_t black_high_right;
    uint16_t threshold_left;
    uint16_t threshold_right;
    uint16_t white_left;
    uint16_t white_right;
    uint16_t black_left;
    uint16_t black_right;
    GrayCalibration_e calibration;
    ChassisState_e state;
} ChassisStatus_s;

uint8_t Chassis_Init(void);
void Chassis_Step(void);
/* 仅在任务中调用；停车时线圈断电，不提供保持力矩。 */
void Chassis_SetEnabled(uint8_t enabled);
/* 第一次点击采白，第二次点击采黑；校准时始终停轮。 */
void Chassis_CalibrateNext(void);
ChassisStatus_s Chassis_GetStatus(void);
#endif
