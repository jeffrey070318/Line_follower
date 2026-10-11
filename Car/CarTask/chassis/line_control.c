/**
 * @file line_control.c
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 双灰度开环差速循迹计算
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#include "line_control.h"
LineOutput_s LineControl(uint8_t left_black, uint8_t right_black,
                        int16_t straight_hz, int16_t inner_hz)
{
    if (!left_black && !right_black) return (LineOutput_s){.stop = 1u};
    return (LineOutput_s){
        .left = right_black ? straight_hz : inner_hz,
        .right = left_black ? straight_hz : inner_hz,
    };
}
