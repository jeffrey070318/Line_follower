# Line_follower

STM32F103C8T6 循迹小车：双灰度、28BYJ48 + ULN2003、FreeRTOS，开环差速。

- PA1：依次采白、采黑；PA0：启停。
- 双黑直行，单侧白纠偏，持续双白 100ms 停车。
- PC13：正常常亮；每秒闪 1..6 次分别表示未校准、采样、待采黑、校准失败、ADC 故障、脱线停车。
- 参数：`Car/CarTask/Car_def.h`；接线：[调试说明](Car/Doc/README.md)。

构建：`cmake --preset Debug`，`cmake --build --preset Debug`（需 ARM GCC、CMake、Ninja）。
