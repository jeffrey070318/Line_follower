/**
 * @file led.c
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 通用 LED 常亮与计数闪烁实现
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#include "led.h"

#define LED_CYCLE_MS 1000u
#define LED_SLOT_MS 120u
#define LED_ON_MS 60u
#define LED_UNSET 255u

static LEDInstance led_instance;
static uint8_t registered;

static void LEDWrite(LEDInstance *led, uint8_t on)
{
    const uint8_t high = on ? led->active_level == GPIO_PIN_SET :
                             led->active_level == GPIO_PIN_RESET;
    if (high) GPIOSet(led->gpio);
    else GPIOReset(led->gpio);
}

LEDInstance *LEDRegister(const LED_Init_Config_s *config)
{
    if (!config || registered || !config->gpio.GPIOx || !config->gpio.GPIO_Pin ||
        (config->gpio.GPIO_Pin & (config->gpio.GPIO_Pin - 1u)) ||
        (unsigned)config->active_level > GPIO_PIN_SET) return NULL;
    GPIOInstance *gpio = GPIORegister(&config->gpio);
    if (!gpio) return NULL;
    led_instance = (LEDInstance){
        .gpio = gpio, .active_level = config->active_level, .blink_count = LED_UNSET,
    };
    LEDWrite(&led_instance, 0u);
    registered = 1u;
    return &led_instance;
}

uint8_t LEDSetBlinkCount(LEDInstance *led, uint8_t count, uint32_t now_ms)
{
    if (!led || count > LED_MAX_BLINK_COUNT) return 0u;
    if (count != led->blink_count) {
        led->blink_count = count;
        led->cycle_ms = now_ms;
    }
    return 1u;
}

void LEDUpdate(LEDInstance *led, uint32_t now_ms)
{
    if (!led) return;
    if (led->blink_count == LED_UNSET) {
        LEDWrite(led, 0u);
        return;
    }
    const uint32_t elapsed = now_ms - led->cycle_ms;
    const uint32_t phase = elapsed % LED_CYCLE_MS;
    led->cycle_ms += elapsed - phase;
    /* 每组短闪指定次数，组末停顿便于计数。 */
    const uint8_t on = !led->blink_count ||
        (phase < led->blink_count * LED_SLOT_MS && phase % LED_SLOT_MS < LED_ON_MS);
    LEDWrite(led, on);
}
