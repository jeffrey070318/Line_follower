/**
 * @file grayscale.h
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 灰度传感器注册、采样与校准接口
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#ifndef GRAYSCALE_H
#define GRAYSCALE_H
#include "bsp_adc.h"
#define GRAYSCALE_MIN_SPAN 200u
typedef struct {
    ADC_Init_Config_s adc;
    uint16_t threshold;
    uint16_t hysteresis;
    uint8_t black_high;
} Grayscale_Init_Config_s;
typedef struct {
    ADCInstance *adc;
    uint16_t raw;
    uint16_t threshold;
    uint16_t hysteresis;
    uint8_t black_high;
    uint8_t black;
    uint8_t valid;
} GrayscaleInstance;
GrayscaleInstance *GrayscaleRegister(const Grayscale_Init_Config_s *config);
uint8_t GrayscaleRead(GrayscaleInstance *gray);
/* 黑白差值不足时拒绝校准；成功后才更新阈值和极性。 */
uint8_t GrayscaleCalibrate(GrayscaleInstance *gray, uint16_t white, uint16_t black);
#endif
