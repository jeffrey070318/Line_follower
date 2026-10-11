#include <assert.h>
#include <stdio.h>
#include "led.h"

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

static uint8_t IsOn(void)
{
    return (GPIOC->output & GPIO_PIN_13) == 0u;
}

int main(void)
{
    const LED_Init_Config_s config = {
        .gpio = {.GPIOx = GPIOC, .GPIO_Pin = GPIO_PIN_13, .exti_mode = GPIO_EXTI_MODE_NONE},
        .active_level = GPIO_PIN_RESET,
    };
    assert(!LEDRegister(NULL));
    LEDInstance *led = LEDRegister(&config);
    assert(led && !IsOn());
    assert(!LEDRegister(&config));
    assert(!LEDSetBlinkCount(NULL, 1u, 0u));
    LEDUpdate(NULL, 0u);
    LEDUpdate(led, 0u);
    assert(!IsOn()); /* Remains off until the application selects a pattern. */
    assert(LEDSetBlinkCount(led, 0u, 0u));
    LEDUpdate(led, 0u);
    assert(IsOn());
    LEDUpdate(led, 900u);
    assert(IsOn());
    assert(!LEDSetBlinkCount(led, LED_MAX_BLINK_COUNT + 1u, 1000u));
    for (unsigned count = 1; count <= LED_MAX_BLINK_COUNT; count++) {
        const uint32_t start = count * 2000u;
        assert(LEDSetBlinkCount(led, (uint8_t)count, start));
        unsigned pulses = 0u;
        uint8_t previous = 0u;
        for (unsigned ms = 0; ms < 1000u; ms += 10u) {
            /* Repeated application updates must not reset the blink cycle. */
            assert(LEDSetBlinkCount(led, (uint8_t)count, start + ms));
            LEDUpdate(led, start + ms);
            if (IsOn() && !previous) pulses++;
            previous = IsOn();
        }
        assert(pulses == count);
        assert(!IsOn()); /* Each burst ends in a visible pause. */
        LEDUpdate(led, start + 1000u);
        assert(IsOn());
    }
    const uint32_t start = UINT32_MAX - 30u;
    assert(LEDSetBlinkCount(led, 1u, start));
    LEDUpdate(led, start);
    assert(IsOn());
    LEDUpdate(led, 40u);
    assert(!IsOn());
    LEDUpdate(led, 969u);
    assert(IsOn()); /* A complete cycle across HAL tick wrap. */
    LEDUpdate(led, 5969u);
    assert(IsOn()); /* Long polling gap doesn't replay missed flashes. */
    puts("PASS: LED polarity, steady, 1..6 pulses, repeated setting, tick wrap");
    return 0;
}
