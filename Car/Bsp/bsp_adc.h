#ifndef BSP_ADC_H
#define BSP_ADC_H
#include "adc.h"
#include <stddef.h>
#define ADC_DEVICE_CNT 2u
typedef struct {
    ADC_HandleTypeDef *hadc;
    uint32_t channel;
} ADC_Init_Config_s;
typedef ADC_Init_Config_s ADCInstance;
/* ADC 由同一个任务独占，不混用并发轮询或 DMA。 */
ADCInstance *ADCRegister(const ADC_Init_Config_s *config);
uint8_t ADCRead(ADCInstance *adc, uint16_t *value);
#endif
