#include <assert.h>
#include <stdio.h>
#include "motor.h"
#include "grayscale.h"
#include "line_control.h"

GPIO_TypeDef test_ports[3];
static uint32_t irq_mask;
static HAL_StatusTypeDef adc_result = HAL_OK;
static unsigned adc_fail_stage;
static uint16_t adc_value;

uint32_t __get_PRIMASK(void) { return irq_mask; }
void __disable_irq(void) { irq_mask = 1u; }
void __set_PRIMASK(uint32_t mask) { irq_mask = mask; }
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
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *adc, ADC_ChannelConfTypeDef *config)
{
    adc->channel = config->Channel;
    return adc_fail_stage == 1u ? adc_result : HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *adc)
{
    (void)adc; return adc_fail_stage == 2u ? adc_result : HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *adc, uint32_t timeout)
{
    (void)adc; (void)timeout; return adc_fail_stage == 3u ? adc_result : HAL_OK;
}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *adc) { (void)adc; return adc_value; }
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *adc)
{
    (void)adc; return adc_fail_stage == 4u ? adc_result : HAL_OK;
}

static void Tick(unsigned count)
{
    while (count--) MotorControl();
}

static void TestMotor(void)
{
    Motor_Init_Config_s config = {
        .port = GPIOB, .pin = {1u, 2u, 4u, 8u},
    };
    assert(MotorRegister(NULL) == NULL);
    Motor_Init_Config_s invalid = config;
    invalid.pin[1] = invalid.pin[0];
    assert(MotorRegister(&invalid) == NULL);
    invalid = config;
    invalid.pin[0] = 3u;
    assert(MotorRegister(&invalid) == NULL);
    MotorInstance *motor = MotorRegister(&config);
    assert(motor);
    assert(GPIOB->output == 0u);
    MotorSetSpeed(motor, 500);
    static const uint8_t sequence[] = {1, 3, 2, 6, 4, 12, 8, 9};
    for (unsigned i = 0; i < 8; i++) {
        Tick(2);
        assert(GPIOB->output == sequence[i]);
    }
    MotorSetSpeed(motor, -500);
    Tick(2);
    assert(GPIOB->output == 8u);
    MotorSetSpeed(motor, 0);
    assert(GPIOB->output == 0u);
    Tick(10);
    assert(GPIOB->output == 0u);
    MotorSetSpeed(motor, 30000);
    Tick(1);
    assert(GPIOB->output == 0u);
    Tick(1);
    assert(GPIOB->output == 9u);
    Tick(100);
    assert(GPIOB->output == 0u);
    config.port = GPIOC;
    config.reverse = 1u;
    MotorInstance *reverse = MotorRegister(&config);
    assert(reverse);
    MotorSetSpeed(reverse, 500);
    Tick(2);
    assert(GPIOC->output == 8u); /* Reverse from initial phase 7 to phase 6. */
    assert(MotorRegister(&config) == NULL);
    MotorSetSpeed(reverse, 0);
    irq_mask = 1u;
    MotorSetSpeed(reverse, 0);
    assert(irq_mask == 1u);
    irq_mask = 0u;
    for (unsigned i = 0; i < GPIO_MX_DEVICE_NUM - 8u; i++) {
        GPIO_Init_Config_s gpio = {.GPIOx = GPIOA, .GPIO_Pin = (uint16_t)(1u << i)};
        assert(GPIORegister(&gpio));
    }
    GPIO_Init_Config_s extra = {.GPIOx = GPIOA, .GPIO_Pin = 4u};
    assert(!GPIORegister(&extra));
    assert(!GPIORegister(NULL));
    GPIOSet(NULL);
    GPIOReset(NULL);
    GPIOToggle(NULL);
    assert(GPIORead(NULL) == GPIO_PIN_RESET);
}

static void TestGrayscale(void)
{
    ADC_HandleTypeDef adc = {0};
    Grayscale_Init_Config_s config = {
        .adc = {.hadc = &adc, .channel = ADC_CHANNEL_2},
        .threshold = 2000u, .hysteresis = 100u, .black_high = 1u,
    };
    assert(GrayscaleRegister(NULL) == NULL);
    config.threshold = 4095u;
    assert(GrayscaleRegister(&config) == NULL);
    config.threshold = 2000u;
    GrayscaleInstance *gray = GrayscaleRegister(&config);
    assert(gray);
    adc_value = 2200u;
    assert(GrayscaleRead(gray));
    assert(gray->black && gray->raw == 2200u);
    adc_value = 1950u;
    assert(GrayscaleRead(gray) && gray->black);
    adc_value = 1800u;
    assert(GrayscaleRead(gray) && !gray->black);
    adc_value = 2050u;
    assert(GrayscaleRead(gray) && !gray->black);
    adc_result = HAL_TIMEOUT;
    for (adc_fail_stage = 1u; adc_fail_stage <= 4u; adc_fail_stage++)
        assert(!GrayscaleRead(gray) && !gray->valid);
    adc_fail_stage = 0u;
    adc_result = HAL_OK;
    config.adc.channel = ADC_CHANNEL_3;
    config.black_high = 0u;
    GrayscaleInstance *low = GrayscaleRegister(&config);
    adc_value = 1800u;
    assert(low && GrayscaleRead(low) && low->black);
    adc_value = 2200u;
    assert(GrayscaleRead(low) && !low->black);
    assert(GrayscaleRegister(&config) == NULL);
    assert(!ADCRead(NULL, &adc_value));
    assert(!ADCRead(gray->adc, NULL));
    assert(!GrayscaleRead(NULL));
    assert(GrayscaleCalibrate(gray, 1000u, 3000u));
    assert(gray->threshold == 2000u && gray->black_high && gray->hysteresis == 100u);
    assert(GrayscaleCalibrate(low, 3300u, 900u));
    assert(low->threshold == 2100u && !low->black_high);
    const uint16_t old_threshold = low->threshold;
    assert(!GrayscaleCalibrate(low, 2000u, 2050u));
    assert(low->threshold == old_threshold);
    assert(!GrayscaleCalibrate(gray, 0u, 4096u));
    assert(!GrayscaleCalibrate(NULL, 1000u, 3000u));
}

static void TestLineControl(void)
{
    LineOutput_s out = LineControl(0, 0, 300, 150);
    assert(out.left == 0 && out.right == 0 && out.stop);
    out = LineControl(1, 0, 300, 150);
    assert(out.left == 150 && out.right == 300 && !out.stop);
    out = LineControl(0, 1, 300, 150);
    assert(out.left == 300 && out.right == 150 && !out.stop);
    out = LineControl(1, 1, 300, 150);
    assert(out.left == 300 && out.right == 300 && !out.stop);
}

int main(void)
{
    TestMotor();
    TestGrayscale();
    TestLineControl();
    puts("PASS: motor, grayscale, line control");
    return 0;
}
