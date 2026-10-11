# 循迹小车接线与调试

当前程序使用两个 28BYJ48 + ULN2003 和两个模拟灰度传感器。
保留参考工程的 Register / Instance 封装，设备使用固定实例池。
GPIO 接口参考 FZSD_VOLLEYBALL；其 ADC 文件为空，本工程补充轮询采样封装。
旧 PWM、PID、DWT 文件暂留，不参与编译。

## 接线

| 设备 | 信号 | STM32 引脚 |
| --- | --- | --- |
| 左 ULN2003 | IN1、IN2、IN3、IN4 | PB12、PB13、PB14、PB15 |
| 右 ULN2003 | IN1、IN2、IN3、IN4 | PB0、PB1、PB10、PB11 |
| 左灰度 | S | PA2 / ADC1_IN2 |
| 右灰度 | S | PA3 / ADC1_IN3 |
| 启停按键 | 按下接 GND | PA0，上拉输入 |
| 校准按键 | 按下接 GND | PA1，上拉输入 |
| 板载状态 LED | 默认低电平亮 | PC13 |

灰度传感器使用 3.3V 供电，S 端不得输入 5V 模拟信号。
电机驱动板接独立、容量足够的 5V 电源，不从 MCU GPIO 或 3.3V 供电。
控制板、传感器、电机电源和驱动板必须共地。
电机停止时四相断电，不提供保持力矩。
LED 默认按 PC13 低电平有效配置；若实际最小系统板不同，
同步修改 `CAR_LED_*` 和 CubeMX 引脚配置。

## CubeMX 配置

引脚和 RTOS 已按重新生成的工程适配，并同步 `.ioc` 中漏配的
PB13/PB14、TIM2 节拍、ADC 采样时间和 `vTaskDelayUntil` 开关。

- PB12..PB15、PB0、PB1、PB10、PB11：推挽输出、无上下拉，初始低电平。
- PA2、PA3：模拟输入；移除 PA3 的旧数字循迹配置。
- PA0、PA1：输入上拉，不使用 EXTI；代码及 `.ioc` 已加入默认按键配置。
  更换引脚时同时修改 `Car_def.h` 和 CubeMX 的输入配置。
- PC13：低速推挽输出、无上下拉，初始高电平（LED 熄灭），不再使用 PA12。
- ADC1：12 位右对齐，软件触发，非连续，关闭扫描，规则序列长度 1。
  初始通道为 IN2，采样时间 239.5 cycles；BSP 每次采样前切换 IN2/IN3。
  不需要 ADC DMA。当前 ADC 时钟为 3MHz。
- TIM2：Internal Clock，非编码器模式，不使用通道引脚。
  当前定时器时钟 72MHz，Prescaler=71，Counter Period=999，更新频率 1kHz。
  开启 TIM2 全局中断，抢占优先级 5。
- TIM4 保持 HAL 的 1ms 时基；SysTick 保持 FreeRTOS 时基。
- 关闭旧 TIM1 电机 PWM、TIM3 编码器和 USART3，释放 PB10/PB11。
  删除旧七路数字循迹、超声波及相关 EXTI 配置。
- FreeRTOS：CMSIS-RTOS v1，Tick=1000Hz，Heap=3072 bytes。
  保留 CubeMX 的 defaultTask；底盘任务由 `CarInit()` 创建，
  使用 `osThreadDef` / `osThreadCreate`，栈为 256 个 32 位字（1024 bytes）。
- 调度顺序：外设初始化 -> `CarInit()` -> `MX_FREERTOS_Init()`
  -> `osKernelStart()`。业务初始化在 `main.c` 的 USER CODE BEGIN 2，
  不放入 `freertos.c`；重新生成后检查并保留该调用顺序。
- `HAL_TIM_PeriodElapsedCallback()` 保留 TIM4 的 `HAL_IncTick()`，
  TIM2 分支调用 `MotorControl()`。

已移除 PA3 模拟输入补丁和重复的 `HAL_TIM_Base_MspInit()`。
当前使用 CubeMX 生成的外设初始化，不再依赖旧工程的临时覆盖配置。

## 控制与校准

全部接线选择、方向、阈值和步频参数集中在 `Car/CarTask/Car_def.h`。

1. 上电保持停车，先将两只传感器放在白底上，点击校准键并松手。
   保持位置约 160ms，LED 变为每秒闪 3 次，表示白底采样完成。
2. 将两只传感器同时放在黑胶带上，再点击校准键并松手，
   保持位置约 160ms。LED 常亮表示校准成功。
   每种颜色采集 16 组数据取平均，左右独立计算
   `threshold = (white + black) / 2`，并自动判断黑色是高值还是低值。
   任意一路黑白差值小于 200 ADC counts，或采样失败，均拒绝校准。
3. 将小车放回起点，点击启停键启动，再点击停止。
   校准未完成不能启动；采样中额外点击会被忽略，两键同时点击优先校准。
   运行时点击校准会先停车，重新从白底开始。校准值仅保存在 RAM，
   每次重新上电都需采白、采黑，不自动启动。
4. 首次调试先架空车轮，确认左右轮的正方向均为前进。若不一致，调整
   `MOTOR_*_REVERSE`；默认右轮反转只是安装方向假设。
5. 两传感器布置在黑胶带内部，分别靠近左右边缘，使直行时均看到黑色。
   左黑右白：左轮降速、向左修正；右黑左白：右轮降速、向右修正。
   双黑正常直行，不再触发停车。启动后尚未看到双黑时，双白保持停轮，
   单侧见黑可差速纠偏；看到双黑后才启用持续双白停车判断。
6. 初始直行 300 半步/s、转弯内轮 120 半步/s。
   这些不是实测最佳参数。先低速调试，根据弯道和负载调整；
   驱动限幅 500 半步/s，不采用编码器或速度 PID。

LED 采用一秒为一组的连续短闪，组末停顿，按点亮次数计数：

| 一秒内点亮次数 | 含义 |
| --- | --- |
| 常亮 | 黑白采样及校准完成，无故障；正常停车和运行均常亮 |
| 1 次 | 尚未校准，等待点击采白 |
| 2 次 | 正在采白或采黑 |
| 3 次 | 白底采样正常完成，等待点击采黑 |
| 4 次 | 校准失败，黑白差值不足 |
| 5 次 | ADC 采样故障 |
| 6 次 | 持续双白，脱线或结束停车 |

故障指示优先于校准状态。每次点亮 60ms，随后熄灭 60ms，
剩余时间为组间停顿。采样仅约 160ms，采样中的闪码可能不足一整组；
判定白底采样成功看持续的 3 次/秒，判定整套采样成功看常亮。
ADC 恢复后仍保持故障停车指示，需重新校准或显式启动才能退出。
失败后调整传感器位置并点击校准键重新采白。
环境光、安装高度或地面发生变化后，应重新校准；
两点采样并不能消除运行中所有环境变化。

`Chassis_GetStatus()` 可读取左右黑白均值、当前 ADC、阈值、极性和校准阶段。
`GRAY_*_THRESHOLD`、`GRAY_*_BLACK_HIGH` 仅为上电初值，不能绕过校准启动。
`GRAY_HYSTERESIS` 为迟滞上限，校准时限幅到黑白差值的 1/8。

## 通用按键接口

按键设备在 `Car/Devices/Key/key.c/.h`，与底盘、RTOS 和 LED 无关。
`KeyRegister()` 配置 GPIO、有效电平与消抖时间；
在同一个任务周期调用 `KeyScan(key, now_ms)`，返回 `KEY_NONE` 或 `KEY_CLICK`。
稳定按下再稳定松开只返回一次点击，长按不重复，上电按住时首次松开不触发。
当前默认扫描周期 10ms、消抖 30ms，支持高、低电平有效，不需要 EXTI。

`CarTask/key/key_task.c` 只将两个按键事件映射到启停和校准，
复用底盘任务，不新增线程。其他设备可直接使用 Key 接口。
启动、停止和校准相关任务接口不要从中断调用。

## LED 设备接口

通用驱动在 `Car/Devices/BlindLED/led.c/.h`，支持高、低电平点亮。
`LEDRegister()` 注册 GPIO；`LEDSetBlinkCount(led, count, now_ms)` 设置闪码，
0 表示常亮，1..6 表示每秒点亮次数；`LEDUpdate(led, now_ms)` 周期更新。
重复设置相同次数不会重置闪烁时序，无阻塞延时，也不占用定时器。
`CarTask/led/led_task.c` 根据底盘状态选择闪码，复用现有 10ms 任务。

ADC 读取失败立即锁定停机。正常跟线后出现双白，先以 120 半步/s
低速直行；连续双白达到 `CHASSIS_LINE_LOST_MS`（默认 100ms）后锁定停机。
确认期间任意一路恢复黑色就取消计时。停车后需按启停键重新启动，
重新启动也需先看到双黑才能启用该判断。
任务连续 100ms 未刷新电机指令，驱动自行断电。
这些均不是硬件急停或独立看门狗。

当前按“离开黑线即结束”的规则停车，但持续双白也可能是弯道脱线或胶带断开，
仅靠两只传感器不能区分这些情况与真正终点。
文档未定义 B 点标记，因此此状态表示脱线或结束，并非可靠的 B 点识别。
传感器安装必须保证正常直行时双黑，否则此规则不适用。

## 验证

固件：

```powershell
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

本机测试（使用 MinGW GCC，与 ARM 构建分开）：

```powershell
cmake -S tests -B build/tests -G Ninja -DCMAKE_C_COMPILER=gcc
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

测试使用 HAL 桩覆盖真实设备和底盘逻辑，但不验证实际接线、换相时序、
带载失步、传感器安装位置及弯道通过能力。这些需要上板测试。
