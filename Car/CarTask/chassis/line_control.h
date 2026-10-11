/**
 * @file line_control.h
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 循迹差速输出接口
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#ifndef LINE_CONTROL_H
#define LINE_CONTROL_H
#include <stdint.h>
typedef struct {
    int16_t left;
    int16_t right;
    uint8_t stop;
} LineOutput_s;
LineOutput_s LineControl(uint8_t left_black, uint8_t right_black,
                        int16_t straight_hz, int16_t inner_hz);
#endif
