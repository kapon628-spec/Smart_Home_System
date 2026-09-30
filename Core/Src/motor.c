/*
 * motor.c
 *
 *  Created on: Jul 23, 2026
 *      Author: 王尚泽
 */


#include "motor.h"

// 电机状态变量
static Motor_State_t motor = {
    .is_running = 0,
    .direction = MOTOR_OPEN,
    .target_openness = 0,
    .current_openness = 0,
    .start_time = 0,
    .run_duration = 0
};

/**
  * @brief  电机初始化
  */
void Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = MOTOR_IN1_Pin | MOTOR_IN2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    Motor_Stop();
}

/**
  * @brief  电机停止
  */
void Motor_Stop(void)
{
    HAL_GPIO_WritePin(MOTOR_IN1_Port, MOTOR_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN2_Port, MOTOR_IN2_Pin, GPIO_PIN_RESET);
    motor.is_running = 0;
}

/**
  * @brief  设置目标开度（非阻塞）
  * @param  openness  0~100
  */
void Motor_SetOpenness(uint8_t openness)
{
    int32_t diff;
    uint32_t run_time;

    // 限制范围
    if (openness > 100) openness = 100;
    motor.target_openness = openness;

    // 计算与当前开度的差值
    diff = (int32_t)openness - (int32_t)motor.current_openness;

    // 如果差值很小（小于5%），忽略，防止频繁抖动
    if (diff > -5 && diff < 5) {
        return;
    }

    // 计算需要的运行时间
    // 开度每变化1%，对应 2000/100 = 20ms
    if (diff >= 0) {
        motor.direction = MOTOR_OPEN;
        run_time = (uint32_t)diff * 20;  // 20ms per 1%
    } else {
        motor.direction = MOTOR_CLOSE;
        run_time = (uint32_t)(-diff) * 20;
    }

    // 最小运行时间保护（防止过短的脉冲）
    if (run_time < 50) run_time = 50;

    // 启动电机
    motor.is_running = 1;
    motor.start_time = HAL_GetTick();
    motor.run_duration = run_time;

    // 设置方向，修改了，正反引脚电平都一样，都朝一个方向转
    if (motor.direction == MOTOR_OPEN) {
        HAL_GPIO_WritePin(MOTOR_IN1_Port, MOTOR_IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_IN2_Port, MOTOR_IN2_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(MOTOR_IN1_Port, MOTOR_IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_IN2_Port, MOTOR_IN2_Pin, GPIO_PIN_SET);
    }
}

/**
  * @brief  电机状态更新（非阻塞轮询，放在主循环中频繁调用）
  *         检查运行时间是否到达，到达则停止电机并更新当前开度
  */
void Motor_Update(void)
{
    if (!motor.is_running) return;

    // 检查是否到达运行时间
    if (HAL_GetTick() - motor.start_time >= motor.run_duration) {
        // 停止电机
        Motor_Stop();

        // 更新当前开度
        motor.current_openness = motor.target_openness;
    }
}

/**
  * @brief  根据光照ADC值计算目标开度（5档）
  * @param  light_adc  0~4095
  * @retval 目标开度档位：0, 30, 60, 90, 100
  * @note   亮度0 → 全开(100%)，亮度4095 → 全关(0%)
  */
uint8_t Motor_GetTargetOpenness(uint16_t light_adc)
{
    // 将ADC值 (0~4095) 反向映射到开度 (100~0)
    // 开度 = 100 - (ADC值 × 100 / 4095)
    uint8_t openness = 100 - (uint8_t)((uint32_t)light_adc * 100 / 4095);

    // 分档：0%, 30%, 60%, 90%, 100%
    if (openness >= 95) {
        return 100;
    } else if (openness >= 75) {
        return 90;
    } else if (openness >= 45) {
        return 60;
    } else if (openness >= 15) {
        return 30;
    } else {
        return 0;
    }
}

/**
  * @brief  查询电机是否正在运行
  */
uint8_t Motor_IsRunning(void)
{
    return motor.is_running;
}
