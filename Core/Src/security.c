/*
 * security.c
 *
 *  Created on: Jul 29, 2026
 *      Author: 王尚泽
 */


#include "security.h"
#include "oled.h"
#include <stdio.h>
#include <string.h>

Security_State_t security = {
		.is_armed = 0,
		.is_alarming = 0,
		.is_password_mode = 0,
		.password_index = 0,
		.password_attempts = 0,
		.arm_start_time = 0,
		.password_start_time = 0
};
void Security_Update(void)
{
    // 如果还有其他需要周期性处理的状态，放在这里
    // 目前蜂鸣器/LED控制已在主函数中实现，此函数可留空
    // 但必须存在，否则编译报错
}
static uint8_t alarm_triggered = 0;  // 本次布防周期是否已触发报警

// 验证密码
static uint8_t Security_VerifyPassword(void)
{
    return (security.password_buffer[0] == 1 &&
            security.password_buffer[1] == 2 &&
            security.password_buffer[2] == 3);
}

// 密码正确响应
static void Security_PasswordCorrect(void)
{
    security.is_armed = 0;
    security.is_alarming = 0;
    security.is_password_mode = 0;
    security.password_index = 0;
    alarm_triggered = 0;
}

// 触发报警
static void Security_TriggerAlarm(void)
{
    if (alarm_triggered) return;  // 已经报警则不再重复触发

    security.is_alarming = 1;
    security.is_password_mode = 0;
    alarm_triggered = 1;

}

/**
  * @brief  初始化安全模块
  */
void Security_Init(void)
{
	security.is_armed = 0;
	security.is_alarming = 0;
	security.is_password_mode = 0;
	security.password_index = 0;
	security.password_attempts = 0;
	alarm_triggered = 0;
}

/**
  * @brief  切换布防/解防（PB0按下）
  */
void Security_ToggleArm(void)
{
    // 如果正在报警，按PB0直接解防
    if (security.is_alarming) {
        security.is_alarming = 0;
        security.is_armed = 0;
        security.is_password_mode = 0;
        security.password_index = 0;
        alarm_triggered = 0;
        return;
    }

    // 切换布防/解防
    if (security.is_armed == 0) {
        // 布防
        security.is_armed = 1;
        security.is_alarming = 0;
        security.is_password_mode = 0;
        security.password_index = 0;
        security.password_attempts = 0;
        security.arm_start_time = HAL_GetTick();
        alarm_triggered = 0;
    } else {
        // 解防（正常解防，不是报警状态）
        security.is_armed = 0;
        security.is_alarming = 0;
        security.is_password_mode = 0;
        security.password_index = 0;
        alarm_triggered = 0;
    }
}

/**
  * @brief  输入密码按键
  */
void Security_InputPassword(uint8_t key)
{
    // 只有布防且未报警且密码模式未超时才能输入
    if (!security.is_armed || security.is_alarming) {
        return;
    }

    // 如果还未进入密码模式，启动
    if (security.password_index == 0) {
        security.is_password_mode = 1;
        security.password_start_time = HAL_GetTick();
    }

    // 检查是否超时（10秒）
    if (HAL_GetTick() - security.password_start_time >= PASSWORD_TIMEOUT) {
        // 超时：进入警戒状态，不再接受密码输入
        security.is_password_mode = 0;
        security.password_index = 0;
        return;
    }

    // 记录密码
    if (security.password_index < PASSWORD_LENGTH) {
        security.password_buffer[security.password_index] = key;
        security.password_index++;
    }

    // 密码输入完毕，验证
    if (security.password_index == PASSWORD_LENGTH) {
        if (Security_VerifyPassword()) {
            Security_PasswordCorrect();
        } else {
            // 密码错误：清空，允许重新输入
            security.password_index = 0;
            security.password_attempts++;
            // 重新设置起始时间（延长输入机会）
            security.password_start_time = HAL_GetTick();
        }
    }
}

/**
  * @brief  检查入侵条件（在主循环中调用）
  */
void Security_CheckIntrusion(uint16_t distance, uint8_t infrared_detected)
{
    // 只有布防且未报警才检查
    if (!security.is_armed || security.is_alarming) {
        return;
    }

    // 如果还在密码输入窗口期（10秒内），不检查入侵
    if (HAL_GetTick() - security.arm_start_time < PASSWORD_TIMEOUT) {
        return;
    }

    // 密码窗口期已过，进入警戒状态
    security.is_password_mode = 0;

    // 检查入侵条件：距离<2米 AND 红外检测到人
    if (distance < 200 && infrared_detected) {
        Security_TriggerAlarm();
    }
}

/**
  * @brief  安全模块状态更新（主循环中频繁调用）
  */


uint8_t Security_GetAlarmState(void)
{
    return security.is_alarming;
}

uint8_t Security_GetArmedState(void)
{
    return security.is_armed;
}
