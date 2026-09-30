#include "ultrasonic.h"
extern TIM_HandleTypeDef htim2;
/**
  * @brief  微秒级延时（粗略）
  * @param  us 微秒数
  * @retval 无
  */
static void delay_us(uint32_t us)
{
    // 72MHz 主频下，约16个循环 = 1µs（粗略估算）
    while (us--) {
        for (uint8_t i = 0; i < 16; i++) {
            __NOP();
        }
    }
}

/**
  * @brief  获取超声波测距值（轮询方式）
  * @retval 距离（cm），超时返回 9999
  */
uint16_t Ultrasonic_GetDistance(void)
{
    uint32_t pulse_start = 0;
    uint32_t pulse_end = 0;
    uint32_t pulse_width_us = 0;
    uint32_t timeout = 0;

    // ---- 1. 发送触发脉冲 ----
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
    delay_us(5);
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);
    delay_us(20);           // 保持高电平至少10µs
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);

    // ---- 2. 等待 ECHO 变高（超时保护） ----
    timeout = 10000;        // 约10ms超时
    while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_RESET) {
        if (--timeout == 0) {
            return 9999;    // 超时：没有收到回波
        }
        delay_us(1);
    }

    // ---- 3. 读取计数器起始值 ----
    pulse_start = __HAL_TIM_GET_COUNTER(&htim2);

    // ---- 4. 等待 ECHO 变低（超时保护） ----
    timeout = 50000;        // 约50ms超时（对应约8.5米）
    while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_SET) {
        if (--timeout == 0) {
            return 9999;    // 超时：回波信号太长
        }
        delay_us(1);
    }

    // ---- 5. 读取计数器结束值 ----
    pulse_end = __HAL_TIM_GET_COUNTER(&htim2);

    // ---- 6. 计算脉冲宽度（微秒） ----
    if (pulse_end >= pulse_start) {
        pulse_width_us = pulse_end - pulse_start;
    } else {
        // 计数器溢出处理（ARR = 65535）
        pulse_width_us = (65535 - pulse_start) + pulse_end;
    }

    // ---- 7. 换算距离：距离(cm) = 脉宽(µs) / 58 ----
    return pulse_width_us / 58;
}

/**
  * @brief  初始化超声波
  * @retval 无
  */
void Ultrasonic_Init(void)
{
    // 启动 TIM2 计数器
    HAL_TIM_Base_Start(&htim2);

    // 初始状态：TRIG 拉低
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
}
