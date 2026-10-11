#include "bsp_gpio.h"

static GPIOInstance gpio_pool[GPIO_MX_DEVICE_NUM];
static uint8_t gpio_count;

uint8_t GPIOCanRegister(uint8_t count)
{
    return count <= GPIO_MX_DEVICE_NUM - gpio_count;
}

GPIOInstance *GPIORegister(const GPIO_Init_Config_s *config)
{
    if (!config || !config->GPIOx || !config->GPIO_Pin || !GPIOCanRegister(1u))
        return NULL;
    GPIOInstance *gpio = &gpio_pool[gpio_count++];
    *gpio = (GPIOInstance){
        .GPIOx = config->GPIOx, .GPIO_Pin = config->GPIO_Pin,
        .pin_state = config->pin_state, .exti_mode = config->exti_mode,
        .gpio_model_callback = config->gpio_model_callback, .id = config->id,
    };
    return gpio;
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    for (unsigned i = 0; i < gpio_count; i++) {
        GPIOInstance *gpio = &gpio_pool[i];
        if (gpio->GPIO_Pin == pin && gpio->gpio_model_callback)
            gpio->gpio_model_callback(gpio);
    }
}
void GPIOToggle(GPIOInstance *gpio)
{
    if (gpio) HAL_GPIO_TogglePin(gpio->GPIOx, gpio->GPIO_Pin);
}
void GPIOToggel(GPIOInstance *gpio) { GPIOToggle(gpio); }
void GPIOSet(GPIOInstance *gpio)
{
    if (gpio) HAL_GPIO_WritePin(gpio->GPIOx, gpio->GPIO_Pin, GPIO_PIN_SET);
}
void GPIOReset(GPIOInstance *gpio)
{
    if (gpio) HAL_GPIO_WritePin(gpio->GPIOx, gpio->GPIO_Pin, GPIO_PIN_RESET);
}
GPIO_PinState GPIORead(GPIOInstance *gpio)
{
    return gpio ? HAL_GPIO_ReadPin(gpio->GPIOx, gpio->GPIO_Pin) : GPIO_PIN_RESET;
}
