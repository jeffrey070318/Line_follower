/**
 * @file key_task.h
 * @author Jeffrey070318(zhengjiafeng)
 * @brief 应用按键初始化与周期处理接口
 * @version 0.1
 * @date 2026-10-11
 * @copyright Copyright (c) 2026
 */
#ifndef KEY_TASK_H
#define KEY_TASK_H
#include <stdint.h>

uint8_t KeyTask_Init(void);
/* 由底盘任务周期调用，不单独创建线程。 */
void KeyTask_Step(void);
#endif
