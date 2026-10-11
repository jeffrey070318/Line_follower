#ifndef BSP_GPIO_H
#define BSP_GPIO_H
#include "gpio.h"
#include <stddef.h>

#define GPIO_MX_DEVICE_NUM 12u
typedef enum {
    GPIO_EXTI_MODE_RISING,
    GPIO_EXTI_MODE_FALLING,
    GPIO_EXTI_MODE_RISING_FALLING,
    GPIO_EXTI_MODE_NONE,
} GPIO_EXTI_MODE_e;
typedef struct GPIOInstance {
    GPIO_TypeDef *GPIOx;
    GPIO_PinState pin_state;
    GPIO_EXTI_MODE_e exti_mode;
    uint16_t GPIO_Pin;
    void (*gpio_model_callback)(struct GPIOInstance *);
    void *id;
} GPIOInstance;
typedef struct {
    GPIO_TypeDef *GPIOx;
    GPIO_PinState pin_state;
    GPIO_EXTI_MODE_e exti_mode;
    uint16_t GPIO_Pin;
    void (*gpio_model_callback)(GPIOInstance *);
    void *id;
} GPIO_Init_Config_s;

uint8_t GPIOCanRegister(uint8_t count);
GPIOInstance *GPIORegister(const GPIO_Init_Config_s *config);
void GPIOToggle(GPIOInstance *gpio);
void GPIOToggel(GPIOInstance *gpio); /* 兼容参考 BSP 的旧接口。 */
void GPIOSet(GPIOInstance *gpio);
void GPIOReset(GPIOInstance *gpio);
GPIO_PinState GPIORead(GPIOInstance *gpio);
#endif
