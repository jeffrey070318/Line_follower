/**
 * @file key.c
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 通用按键消抖与点击检测实现
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#include "key.h"

static KeyInstance key_pool[KEY_DEVICE_CNT];
static uint8_t key_count;

KeyInstance *KeyRegister(const Key_Init_Config_s *config)
{
    if (!config || !config->gpio.GPIOx || !config->gpio.GPIO_Pin ||
        (config->gpio.GPIO_Pin & (config->gpio.GPIO_Pin - 1u)) ||
        (unsigned)config->active_level > GPIO_PIN_SET || !config->debounce_ms ||
        key_count >= KEY_DEVICE_CNT) return NULL;
    for (unsigned i = 0; i < key_count; i++)
        if (key_pool[i].gpio->GPIOx == config->gpio.GPIOx &&
            key_pool[i].gpio->GPIO_Pin == config->gpio.GPIO_Pin) return NULL;
    GPIOInstance *gpio = GPIORegister(&config->gpio);
    if (!gpio) return NULL;
    KeyInstance *key = &key_pool[key_count++];
    *key = (KeyInstance){
        .gpio = gpio, .active_level = config->active_level,
        .debounce_ms = config->debounce_ms,
        .pressed = GPIORead(gpio) == config->active_level,
    };
    key->candidate = key->pressed;
    return key;
}

KeyEvent_e KeyScan(KeyInstance *key, uint32_t now_ms)
{
    if (!key) return KEY_NONE;
    const uint8_t pressed = GPIORead(key->gpio) == key->active_level;
    if (pressed != key->candidate) {
        key->candidate = pressed;
        key->changed_ms = now_ms;
    }
    if (key->candidate == key->pressed ||
        (uint32_t)(now_ms - key->changed_ms) < key->debounce_ms) return KEY_NONE;
    key->pressed = key->candidate;
    if (key->pressed) key->armed = 1u;
    else if (key->armed) {
        key->armed = 0u;
        return KEY_CLICK;
    }
    return KEY_NONE;
}
