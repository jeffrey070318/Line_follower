#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
#include <stddef.h>
typedef struct { uint16_t output, input; } GPIO_TypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
extern GPIO_TypeDef test_ports[3];
#define GPIOA (&test_ports[0])
#define GPIOB (&test_ports[1])
#define GPIOC (&test_ports[2])
#define GPIO_PIN_0 1u
#define GPIO_PIN_1 2u
#define GPIO_PIN_10 1024u
#define GPIO_PIN_11 2048u
#define GPIO_PIN_12 4096u
#define GPIO_PIN_13 8192u
#define GPIO_PIN_14 16384u
#define GPIO_PIN_15 32768u
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t mask);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
void HAL_GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
uint32_t HAL_GetTick(void);
#endif
