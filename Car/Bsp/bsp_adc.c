#include "bsp_adc.h"
static ADCInstance adc_pool[ADC_DEVICE_CNT];
static uint8_t adc_count;

ADCInstance *ADCRegister(const ADC_Init_Config_s *config)
{
    if (!config || !config->hadc || adc_count >= ADC_DEVICE_CNT ||
        !IS_ADC_CHANNEL(config->channel)) return NULL;
    ADCInstance *adc = &adc_pool[adc_count++];
    *adc = *config;
    return adc;
}

uint8_t ADCRead(ADCInstance *adc, uint16_t *value)
{
    if (!adc || !value) return 0u;
    ADC_ChannelConfTypeDef channel = {
        .Channel = adc->channel, .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_239CYCLES_5,
    };
    if (HAL_ADC_ConfigChannel(adc->hadc, &channel) != HAL_OK) return 0u;
    if (HAL_ADC_Start(adc->hadc) != HAL_OK) {
        (void)HAL_ADC_Stop(adc->hadc);
        return 0u;
    }
    const HAL_StatusTypeDef status = HAL_ADC_PollForConversion(adc->hadc, 2u);
    if (status == HAL_OK) *value = (uint16_t)HAL_ADC_GetValue(adc->hadc);
    const HAL_StatusTypeDef stopped = HAL_ADC_Stop(adc->hadc);
    return status == HAL_OK && stopped == HAL_OK;
}
