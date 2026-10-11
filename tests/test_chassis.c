#include <assert.h>
#include <stdio.h>
#include "chassis_task.h"
#include "motor.h"
#include "adc.h"
#include "Car_def.h"
#include "key_task.h"
#include "led_task.h"

GPIO_TypeDef test_ports[3];
ADC_HandleTypeDef hadc1;
static uint16_t samples[2] = {1000u, 1000u};
static uint8_t fail_adc;
static uint32_t now, irq_mask;

uint32_t __get_PRIMASK(void) { return irq_mask; }
void __disable_irq(void) { irq_mask = 1u; }
void __set_PRIMASK(uint32_t mask) { irq_mask = mask; }
uint32_t HAL_GetTick(void) { return now; }
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
    return HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *adc) { (void)adc; return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *adc, uint32_t timeout)
{
    (void)adc; (void)timeout;
    return fail_adc ? HAL_TIMEOUT : HAL_OK;
}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *adc) { return samples[adc->channel - 2u]; }
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *adc) { (void)adc; return HAL_OK; }
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *adc)
{
    (void)adc; return HAL_OK;
}

static void Capture(void)
{
    for (unsigned i = 0; i < GRAY_CALIBRATION_SAMPLES; i++) {
        now += CHASSIS_PERIOD_MS;
        Chassis_Step();
    }
}

static void Click(uint16_t pins)
{
    GPIOA->input &= (uint16_t)~pins;
    now += 10u;
    KeyTask_Step();
    now += 30u;
    KeyTask_Step();
    GPIOA->input |= pins;
    now += 10u;
    KeyTask_Step();
    now += 30u;
    KeyTask_Step();
}

static void CheckLED(unsigned expected)
{
    uint8_t previous = 0u;
    unsigned pulses = 0u;
    for (unsigned ms = 0; ms < 1000u; ms += 10u) {
        LEDTask_Step();
        const uint8_t on = (GPIOC->output & GPIO_PIN_13) == 0u;
        if (!expected) assert(on);
        if (on && !previous) pulses++;
        previous = on;
        now += 10u;
    }
    if (expected) assert(pulses == expected);
}

int main(void)
{
    GPIOA->input = GPIO_PIN_0 | GPIO_PIN_1;
    assert(Chassis_Init());
    assert(Chassis_Init());
    assert(KeyTask_Init());
    assert(KeyTask_Init());
    assert(LEDTask_Init());
    assert(LEDTask_Init());
    Chassis_Step();
    ChassisStatus_s status = Chassis_GetStatus();
    assert(status.state == CHASSIS_DISABLED && status.left_hz == 0);
    CheckLED(1u);
    Chassis_SetEnabled(1);
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_DISABLED);
    assert(!Chassis_GetStatus().calibrated);
    Chassis_CalibrateNext();
    assert(Chassis_GetStatus().calibration == GRAY_CAL_WHITE);
    CheckLED(2u);
    Chassis_SetEnabled(1);
    assert(Chassis_GetStatus().state != CHASSIS_RUNNING);
    for (unsigned i = 0; i < GRAY_CALIBRATION_SAMPLES; i++) {
        samples[0] = i & 1u ? 1020u : 1000u;
        samples[1] = i & 1u ? 1240u : 1200u;
        now += CHASSIS_PERIOD_MS;
        Chassis_Step();
    }
    assert(Chassis_GetStatus().calibration == GRAY_CAL_WAIT_BLACK);
    CheckLED(3u);
    Chassis_CalibrateNext();
    for (unsigned i = 0; i < GRAY_CALIBRATION_SAMPLES; i++) {
        samples[0] = i & 1u ? 3000u : 2980u;
        samples[1] = i & 1u ? 2800u : 2760u;
        now += CHASSIS_PERIOD_MS;
        Chassis_Step();
    }
    status = Chassis_GetStatus();
    assert(status.calibrated && status.calibration == GRAY_CAL_READY);
    assert(status.threshold_left == 2000u && status.threshold_right == 2000u);
    assert(status.white_left == 1010u && status.black_left == 2990u);
    assert(status.white_right == 1220u && status.black_right == 2780u);
    CheckLED(0u);
    samples[0] = samples[1] = 1000u;
    Chassis_SetEnabled(1);
    Chassis_Step();
    status = Chassis_GetStatus();
    assert(status.state == CHASSIS_RUNNING && status.left_hz == 0 && status.right_hz == 0);
    now += CHASSIS_LINE_LOST_MS;
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_RUNNING);
    samples[0] = samples[1] = 3000u;
    Chassis_Step();
    status = Chassis_GetStatus();
    assert(status.state == CHASSIS_RUNNING && status.left_hz == 300 && status.right_hz == 300);
    CheckLED(0u);
    now += CHASSIS_LINE_LOST_MS;
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_RUNNING);
    samples[1] = 1000u;
    Chassis_Step();
    status = Chassis_GetStatus();
    assert(status.left_hz == 120 && status.right_hz == 300);
    samples[0] = 1000u;
    samples[1] = 3000u;
    Chassis_Step();
    status = Chassis_GetStatus();
    assert(status.left_hz == 300 && status.right_hz == 120);
    samples[1] = 1000u;
    Chassis_Step();
    status = Chassis_GetStatus();
    assert(status.left_hz == 120 && status.right_hz == 120);
    now += CHASSIS_LINE_LOST_MS - 1u;
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_RUNNING);
    samples[0] = samples[1] = 3000u;
    Chassis_Step(); /* A recovered line cancels the pending stop. */
    samples[0] = samples[1] = 1000u;
    now = UINT32_MAX - 20u;
    Chassis_Step();
    status = Chassis_GetStatus();
    assert(status.left_hz == 120 && status.right_hz == 120);
    now += CHASSIS_LINE_LOST_MS - 1u; /* Cross the HAL tick wrap. */
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_RUNNING);
    now++;
    Chassis_Step();
    status = Chassis_GetStatus();
    assert(status.state == CHASSIS_LINE_LOST && status.left_hz == 0 && status.right_hz == 0);
    assert(GPIOB->output == 0u);
    CheckLED(6u);
    samples[0] = samples[1] = 3000u;
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_LINE_LOST);
    samples[0] = samples[1] = 1000u;
    Chassis_SetEnabled(1);
    Chassis_Step();
    now += CHASSIS_LINE_LOST_MS;
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_RUNNING);
    assert(Chassis_GetStatus().left_hz == 0); /* Restart must rearm tracking. */
    samples[0] = samples[1] = 3000u;
    Chassis_Step();
    for (unsigned i = 0; i < 4u; i++) MotorControl();
    assert(GPIOB->output != 0u);
    Chassis_SetEnabled(0);
    assert(GPIOB->output == 0u);
    assert(Chassis_GetStatus().state == CHASSIS_DISABLED);
    Chassis_SetEnabled(1);
    fail_adc = 1u;
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_SENSOR_FAULT);
    CheckLED(5u);
    fail_adc = 0u;
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_SENSOR_FAULT);
    /* Recalibration stops immediately and invalidates the old calibration. */
    Chassis_SetEnabled(1);
    Chassis_Step();
    samples[0] = samples[1] = 1000u;
    Chassis_CalibrateNext();
    assert(GPIOB->output == 0u && !Chassis_GetStatus().calibrated);
    Capture();
    samples[0] = 1050u;
    samples[1] = 3000u;
    Chassis_CalibrateNext();
    Capture();
    assert(Chassis_GetStatus().calibration == GRAY_CAL_FAILED);
    CheckLED(4u);
    assert(!Chassis_GetStatus().calibrated);
    assert(Chassis_GetStatus().threshold_right == 2000u); /* No partial commit. */
    Chassis_SetEnabled(1);
    assert(Chassis_GetStatus().state != CHASSIS_RUNNING);
    samples[0] = samples[1] = 3200u;
    Chassis_CalibrateNext();
    Capture();
    samples[0] = samples[1] = 800u; /* Black is lower than white. */
    Chassis_CalibrateNext();
    Capture();
    assert(Chassis_GetStatus().calibrated);
    assert(!Chassis_GetStatus().black_high_left);
    Chassis_SetEnabled(1);
    samples[0] = samples[1] = 800u;
    Chassis_Step();
    assert(Chassis_GetStatus().left_hz == 300);
    Chassis_CalibrateNext();
    fail_adc = 1u;
    Chassis_Step();
    assert(Chassis_GetStatus().calibration == GRAY_CAL_FAILED);
    assert(!Chassis_GetStatus().calibrated && GPIOB->output == 0u);
    fail_adc = 0u;
    samples[0] = samples[1] = 1000u;
    Click(KEY_START_PIN);
    assert(Chassis_GetStatus().state != CHASSIS_RUNNING);
    Click(KEY_CALIBRATE_PIN);
    Capture();
    assert(Chassis_GetStatus().calibration == GRAY_CAL_WAIT_BLACK);
    samples[0] = samples[1] = 3000u;
    Click(KEY_CALIBRATE_PIN);
    Capture();
    assert(Chassis_GetStatus().calibrated);
    samples[0] = samples[1] = 3000u;
    Click(KEY_START_PIN);
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_RUNNING);
    Click(KEY_START_PIN);
    assert(Chassis_GetStatus().state == CHASSIS_DISABLED);
    Click(KEY_START_PIN | KEY_CALIBRATE_PIN);
    assert(Chassis_GetStatus().state == CHASSIS_CALIBRATING);
    assert(!Chassis_GetStatus().calibrated);
    assert(GPIOB->output == 0u);
    Chassis_Step();
    assert(Chassis_GetStatus().state == CHASSIS_CALIBRATING);
    puts("PASS: chassis calibration, key commands, turn, stop, tick wrap, sensor fault");
    return 0;
}
