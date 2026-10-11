#include <assert.h>
#include <stdio.h>
#include "key.h"

GPIO_TypeDef test_ports[3];
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    if (state) port->output |= pin;
    else port->output &= (uint16_t)~pin;
}
void HAL_GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin) { port->output ^= pin; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    return (port->input & pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

int main(void)
{
    GPIOA->input = GPIO_PIN_0 | GPIO_PIN_1;
    Key_Init_Config_s config = {
        .gpio = {.GPIOx = GPIOA, .GPIO_Pin = GPIO_PIN_0, .exti_mode = GPIO_EXTI_MODE_NONE},
        .active_level = GPIO_PIN_RESET, .debounce_ms = 30u,
    };
    assert(!KeyRegister(NULL));
    KeyInstance *key = KeyRegister(&config);
    assert(key);
    assert(!KeyRegister(&config));
    assert(KeyScan(NULL, 0u) == KEY_NONE);
    assert(KeyScan(key, 0u) == KEY_NONE);
    GPIOA->input &= (uint16_t)~GPIO_PIN_0;
    assert(KeyScan(key, 10u) == KEY_NONE);
    GPIOA->input |= GPIO_PIN_0; /* Bounce before stable press. */
    assert(KeyScan(key, 20u) == KEY_NONE);
    GPIOA->input &= (uint16_t)~GPIO_PIN_0;
    assert(KeyScan(key, 30u) == KEY_NONE);
    assert(KeyScan(key, 59u) == KEY_NONE);
    assert(KeyScan(key, 60u) == KEY_NONE);
    assert(KeyScan(key, 5000u) == KEY_NONE); /* Holding never repeats. */
    GPIOA->input |= GPIO_PIN_0;
    assert(KeyScan(key, 5010u) == KEY_NONE);
    assert(KeyScan(key, 5040u) == KEY_CLICK);
    assert(KeyScan(key, 5070u) == KEY_NONE);
    GPIOA->input &= (uint16_t)~GPIO_PIN_0;
    assert(KeyScan(key, UINT32_MAX - 10u) == KEY_NONE);
    assert(KeyScan(key, 20u) == KEY_NONE); /* Stable press across tick wrap. */
    GPIOA->input |= GPIO_PIN_0;
    assert(KeyScan(key, 30u) == KEY_NONE);
    assert(KeyScan(key, 60u) == KEY_CLICK);

    /* Register while held: first release must not synthesize a click. */
    config.gpio.GPIO_Pin = GPIO_PIN_1;
    config.active_level = GPIO_PIN_SET; /* Also support active-high wiring. */
    KeyInstance *held = KeyRegister(&config);
    assert(held);
    GPIOA->input &= (uint16_t)~GPIO_PIN_1;
    assert(KeyScan(held, 0u) == KEY_NONE);
    assert(KeyScan(held, 30u) == KEY_NONE);
    GPIOA->input |= GPIO_PIN_1;
    assert(KeyScan(held, 40u) == KEY_NONE);
    assert(KeyScan(held, 70u) == KEY_NONE);
    GPIOA->input &= (uint16_t)~GPIO_PIN_1;
    assert(KeyScan(held, 80u) == KEY_NONE);
    assert(KeyScan(held, 110u) == KEY_CLICK);
    assert(!KeyRegister(&config));
    puts("PASS: key debounce, hold, release, tick wrap, startup held");
    return 0;
}
