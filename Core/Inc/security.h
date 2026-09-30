/*
 * security.h
 *
 *  Created on: Jul 29, 2026
 *      Author: 王尚泽
 */

#ifndef INC_SECURITY_H_
#define INC_SECURITY_H_

#include "main.h"

#define PASSWORD_LENGTH    3
#define PASSWORD_TIMEOUT   10000   // 10秒

// 安全状态
typedef struct {
    uint8_t is_armed;           // 1=布防，0=解防
    uint8_t is_alarming;        // 1=正在报警
    uint8_t is_password_mode;   // 1=密码输入模式
    uint8_t password_index;     // 当前已输入位数
    uint8_t password_buffer[PASSWORD_LENGTH];
    uint32_t password_start_time;   // 密码模式开始时间
    uint32_t arm_start_time;        // 布防开始时间
    uint8_t password_attempts;      // 密码尝试次数（用于输错重试）
} Security_State_t;

// 函数声明
void Security_Init(void);
void Security_ToggleArm(void);       // PB0按下切换
void Security_InputPassword(uint8_t key);  // PB1/PB2/PB3按下
uint8_t Security_GetAlarmState(void);
uint8_t Security_GetArmedState(void);
void Security_CheckIntrusion(uint16_t distance, uint8_t infrared_detected);


#endif /* INC_SECURITY_H_ */
