/*
 * motor.h
 *
 *  Created on: Jul 23, 2026
 *      Author: 王尚泽
 */

#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#include "main.h"

// 引脚定义
#define MOTOR_IN1_Pin       GPIO_PIN_9
#define MOTOR_IN1_Port      GPIOA
#define MOTOR_IN2_Pin       GPIO_PIN_10
#define MOTOR_IN2_Port      GPIOA

// 方向定义
#define MOTOR_OPEN          1
#define MOTOR_CLOSE         0

// 开度档位
#define OPENNESS_0          0
#define OPENNESS_30         30
#define OPENNESS_60         60
#define OPENNESS_90         90
#define OPENNESS_100        100

// 全行程时间（ms）
#define TOTAL_TRAVEL_TIME   5000

// 电机状态
typedef struct {
    uint8_t is_running;          // 0=停止，1=运行中
    uint8_t direction;           // MOTOR_OPEN 或 MOTOR_CLOSE
    uint8_t target_openness;     // 目标开度 (0~100)
    uint8_t current_openness;    // 当前开度 (0~100)
    uint32_t start_time;         // 开始运行的时间戳
    uint32_t run_duration;       // 需要运行的时长（ms）
} Motor_State_t;

// 函数声明
void Motor_Init(void);
void Motor_Update(void);         // 非阻塞状态更新函数（放在主循环中调用）
void Motor_SetOpenness(uint8_t openness);
void Motor_Stop(void);
uint8_t Motor_GetTargetOpenness(uint16_t light_adc);
uint8_t Motor_IsRunning(void);

#endif /* INC_MOTOR_H_ */
