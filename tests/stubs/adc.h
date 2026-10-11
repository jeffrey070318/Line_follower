#ifndef TEST_ADC_H
#define TEST_ADC_H
#include "main.h"
typedef struct { uint32_t channel; } ADC_HandleTypeDef;
extern ADC_HandleTypeDef hadc1;
typedef struct {
    uint32_t Channel, Rank, SamplingTime;
} ADC_ChannelConfTypeDef;
#define ADC_CHANNEL_2 2u
#define ADC_CHANNEL_3 3u
#define IS_ADC_CHANNEL(channel) ((channel) <= 17u)
#define ADC_REGULAR_RANK_1 1u
#define ADC_SAMPLETIME_239CYCLES_5 239u
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *, ADC_ChannelConfTypeDef *);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *, uint32_t);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *);
#endif
