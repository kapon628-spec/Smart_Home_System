#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "main.h"

// 引脚定义（根据你的实际连接修改）
#define TRIG_Pin       GPIO_PIN_12
#define TRIG_GPIO_Port GPIOA
#define ECHO_Pin       GPIO_PIN_15
#define ECHO_GPIO_Port GPIOA

// 函数声明
void Ultrasonic_Init(void);
uint16_t Ultrasonic_GetDistance(void);

#endif
