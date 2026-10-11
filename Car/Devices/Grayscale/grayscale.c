/**
 * @file grayscale.c
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 模拟灰度采样与黑白校准实现
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#include "grayscale.h"
static GrayscaleInstance gray_pool[2];
static uint8_t gray_count;

GrayscaleInstance *GrayscaleRegister(const Grayscale_Init_Config_s *config)
{
    if (!config || gray_count >= 2u || config->threshold > 4095u ||
        config->hysteresis > config->threshold ||
        config->hysteresis > 4095u - config->threshold) return NULL;
    ADCInstance *adc = ADCRegister(&config->adc);
    if (!adc) return NULL;
    GrayscaleInstance *gray = &gray_pool[gray_count++];
    *gray = (GrayscaleInstance){
        .adc = adc, .threshold = config->threshold,
        .hysteresis = config->hysteresis, .black_high = config->black_high,
    };
    return gray;
}
uint8_t GrayscaleRead(GrayscaleInstance *gray)
{
    if (!gray) return 0u;
    gray->valid = ADCRead(gray->adc, &gray->raw);
    if (!gray->valid) return 0u;
    const uint16_t low = gray->threshold - gray->hysteresis;
    const uint16_t high = gray->threshold + gray->hysteresis;
    if (gray->raw >= high) gray->black = gray->black_high != 0u;
    else if (gray->raw <= low) gray->black = gray->black_high == 0u;
    return 1u;
}

uint8_t GrayscaleCalibrate(GrayscaleInstance *gray, uint16_t white, uint16_t black)
{
    if (!gray || white > 4095u || black > 4095u) return 0u;
    const uint16_t span = white > black ? white - black : black - white;
    if (span < GRAYSCALE_MIN_SPAN) return 0u;
    gray->threshold = (uint16_t)(((uint32_t)white + black) / 2u);
    gray->black_high = black > white;
    /* 限制迟滞大小，避免掩盖实测黑白差值。 */
    if (gray->hysteresis > span / 8u) gray->hysteresis = span / 8u;
    gray->black = 0u;
    return 1u;
}
