# Line Follower Implementation Plan

Goal: Replace the inherited DC motor application with two 28BYJ48 motors,
ULN2003 drivers and two analog grayscale sensors.

Architecture: GPIO and ADC wrappers feed registered device instances.
TIM2 drives half-step commutation at 1 kHz. One CMSIS-RTOS v1 chassis task
samples sensors and selects fixed wheel step rates at 100 Hz.
No encoders, PWM motor drive or PID are used.

- [x] Add native tests for registration limits, step sequence, reverse,
  rate limiting, stop, command timeout, ADC failures, grayscale hysteresis,
  and the four sensor combinations.
- [x] Replace GPIO and motor implementations; add ADC and grayscale devices.
- [x] Replace the chassis task and centralize wiring and parameters in Car_def.h.
- [x] Connect CarInit before scheduler startup, configure TIM2 and analog GPIO,
  and explicitly include application sources in CMake.
- [x] Run native tests and Debug/Release ARM builds; document CubeMX settings
  and calibration. Preserve unrelated legacy helpers outside the build.

Wiring: left motor PB12..PB15, right motor PB0/PB1/PB10/PB11;
left/right sensor PA2/PA3. USART3 initialization is removed because PB10/PB11
are reassigned. TIM4 remains the HAL tick source.

Startup is disabled until explicitly enabled. Both-black stops and requires
re-enabling; this is a protective rule, not a claim that B has been detected.
Both-white drives straight, so line loss cannot reliably be detected.
Sensor polarity, thresholds and motor orientation require bench calibration.
