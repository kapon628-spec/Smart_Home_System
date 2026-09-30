/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "ultrasonic.h"
#include "motor.h"
#include "security.h"
#include <stdio.h>     // 添加 sprintf 需要
#include <string.h>    // 添加 strlen 需要
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint16_t light_adc = 0;                 // 存放ADC原始值 (0~4095)
uint16_t blink_interval = 0;            // 存放计算后的闪烁间隔(ms)
uint32_t last_blink_time = 0;           // 记录上次LED状态翻转的时刻

static uint32_t last_motor_check = 0;
static uint8_t last_openness = 0xFF;   // 上次开度，初始值设为不可能的值
static uint8_t display_openness = 0;    // 屏幕显示的开度（实时计算）

static uint32_t last_key_time = 0;
static uint8_t key_debounce_delay = 50;  // 50ms防抖

static uint32_t last_oled_update = 0;   // OLED刷新时间戳

char bluetooth_tx_buffer[64];// 蓝牙发送缓冲区
uint8_t rx_data;                    // 接收到的单字节数据
char cmd_buffer[16];                // 指令缓存
uint8_t cmd_index = 0;              // 缓存索引

uint8_t infrared_detected = 0;

extern Security_State_t security;

//-----  将OLED四行数据打包并通过蓝牙发送-----
void Bluetooth_SendStatus(uint16_t dist, uint16_t light, uint8_t alarm, uint8_t openness)
{
    // 格式化数据：距离(cm)、光照(ADC值)、报警状态(0/1)、开度(%)
    sprintf(bluetooth_tx_buffer, "Dist:%04d Light:%04d Alarm:%d Open:%d%%\r\n",
            dist, light, alarm, openness);

    // 通过USART2（蓝牙模块）发送
    HAL_UART_Transmit(&huart2, (uint8_t*)bluetooth_tx_buffer,
                      strlen(bluetooth_tx_buffer), 100);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1);
  OLED_Init();
  OLED_Clear();
  OLED_ShowString(1, 1, "Dist:");   // 第1行第1列显示标签
  OLED_ShowString(2, 1, "Light:");      // 第2行第1列显示光照标签（可选）
  OLED_ShowString(3, 1, "Alarm:");      // 第3行显示报警状态（可选）
  OLED_ShowString(4, 1, "Open:");
  Motor_Init();
  Security_Init();
  Ultrasonic_Init();
  HAL_TIM_Base_Start(&htim2);
  HAL_UART_Receive_IT(&huart2, (uint8_t*)&rx_data, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // ===== 1. 红外检测（每次循环执行） =====
	  uint8_t sensorState=HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11);
	  	  if(sensorState==GPIO_PIN_RESET)
	  	  {
	  		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
	  	  }
	  	  else
	  	  {
	  		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	  	  }
	  // ===== 2. ADC读取 + LED闪烁 =====
	  HAL_ADC_Start(&hadc1);
	  if (HAL_ADC_PollForConversion(&hadc1, 100)==HAL_OK)
	  {
		  light_adc = HAL_ADC_GetValue(&hadc1);
		  uint16_t max_interval = 1000;    // 最慢间隔 (ms)
		  uint16_t min_interval = 50;      // 最快间隔 (ms)
		  uint16_t range = max_interval - min_interval;
		  blink_interval = max_interval - (light_adc * range / 4095);
		  // 3. 限制边界值，防止意外
		  if (blink_interval < min_interval) blink_interval = min_interval;
		  if (blink_interval > max_interval) blink_interval = max_interval;
		  // 4. 非阻塞式闪烁控制 (不卡死CPU)
		  if (HAL_GetTick() - last_blink_time >= blink_interval)
		  {
		      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5); // 翻转LED状态
		      last_blink_time = HAL_GetTick();            // 重置时间戳
		  }
	  }
	  // ===== 3. 电机状态更新（非阻塞，每次循环调用） =====
	  Motor_Update();
	  //  4. 安全模块状态更新（非阻塞：处理LED闪烁）
	  Security_Update();
	  //  5. 按键检测（带50ms防抖）
	  if (HAL_GetTick() - last_key_time >= key_debounce_delay) {
	          last_key_time = HAL_GetTick();

	          // ---- PB0: 布防/解防 ----
	          if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
	              Security_ToggleArm();
	              // 等待按键释放（防止连续触发）
	              while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET);
	          }

	          // ---- PB1: 密码数字1 ----
	          if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) {
	              Security_InputPassword(1);
	              while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET);
	          }

	          // ---- PB2: 密码数字2 ----
	          if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
	              Security_InputPassword(2);
	              while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET);
	          }

	          // ---- PB3: 密码数字3 ----
	          if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET) {
	              Security_InputPassword(3);
	              while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET);
	          }
	      }
	  // ===== 6. OLED刷新（每300ms刷新一次） =====
	  static uint32_t last_update = 0;
	  if(HAL_GetTick() - last_oled_update >= 100){
		  last_oled_update = HAL_GetTick();
		  uint16_t dist = Ultrasonic_GetDistance();
		  Security_CheckIntrusion(dist, infrared_detected);
		  OLED_ShowNum(1, 6, dist, 5);
		  OLED_ShowString(1, 11, "cm");
		  OLED_ShowNum(2, 7, light_adc, 5);
		  OLED_ShowString(3, 1, "                ");   // 清空第3行（16个空格）
		  if (security.is_alarming) {
		              OLED_ShowString(3, 1, "Alarm:ON");
		              static uint8_t led_state = 0;
		              led_state = !led_state;
		              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
		              // 报警LED闪烁（每300ms翻转一次）
		              static uint32_t last_alarm_led_time = 0;
		              static uint8_t alarm_led_state = 0;
		              if (HAL_GetTick() - last_alarm_led_time >= 300) {
		            	  last_alarm_led_time = HAL_GetTick();
		              	  alarm_led_state = !alarm_led_state;
		              	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, alarm_led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
		              	  }
		          } else if (!security.is_armed) {
		              OLED_ShowString(3, 1, "Alarm:OFF");
	                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
		          } else {
		              uint32_t elapsed = HAL_GetTick() - security.arm_start_time;
		              if (elapsed < 10000) {
		                  // 密码输入模式：显示 PWD:** 倒计时
		                  uint8_t remain = (10000 - elapsed) / 1000;
		                  if (remain == 0) remain = 1;

		                  OLED_ShowString(3, 1, "PWD:");
		                  for (uint8_t i = 0; i < security.password_index && i < 3; i++) {
		                      OLED_ShowChar(3, 5 + i, '*');
		                  }
		                  for (uint8_t i = security.password_index; i < 3; i++) {
		                      OLED_ShowChar(3, 5 + i, '_');
		                  }
		                  // 倒计时（2位数）
		                  char cnt[4];
		                  cnt[0] = ' ';
		                  cnt[1] = (remain / 10) + '0';
		                  cnt[2] = (remain % 10) + '0';
		                  cnt[3] = 's';
		                  OLED_ShowString(3, 9, cnt);
		              } else {
		                  OLED_ShowString(3, 1, "Alarm:ON");
		                  // 打开蜂鸣器和LED
		                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
		                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
		                  		              }
		          }
		  display_openness = Motor_GetTargetOpenness(light_adc);
		  OLED_ShowNum(4, 7, display_openness, 3);
		  OLED_ShowString(4, 10, "%");
		  Bluetooth_SendStatus(dist, light_adc, security.is_alarming, display_openness);
	  }
	  // ===== 7. 电机控制（每1秒检测一次，有变化才动作） =====
	  if (HAL_GetTick() - last_motor_check >= 2000)
	  {
	      last_motor_check = HAL_GetTick();

	      // 如果电机正在运行，不进行新检测
	      if (!Motor_IsRunning())
	      {
	          // 根据光照计算目标开度
	          uint8_t target = Motor_GetTargetOpenness(light_adc);

	          // 只有开度发生变化时才执行
	          if (target != last_openness)
	          {
	                last_openness = target;
	                Motor_SetOpenness(target);
	          }
	      }
	  }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        // 收到 '\n' 或 '\r' 表示指令结束
        if (rx_data == '\n' || rx_data == '\r') {
            cmd_buffer[cmd_index] = '\0';  // 字符串结束符

            // 解析指令
            if (strcmp(cmd_buffer, "ARM") == 0) {
                Security_ToggleArm();   // 布防
            } else if (strcmp(cmd_buffer, "DISARM") == 0) {
                Security_ToggleArm();   // 解防
            } else if (strcmp(cmd_buffer, "PWD123") == 0) {
                // 模拟输入密码 1-2-3
                Security_InputPassword(1);
                Security_InputPassword(2);
                Security_InputPassword(3);
            }

            cmd_index = 0;  // 重置缓存索引
            memset(cmd_buffer, 0, sizeof(cmd_buffer));  // 清空缓存
        } else {
            // 普通字符，存入缓存
            if (cmd_index < sizeof(cmd_buffer) - 1) {
                cmd_buffer[cmd_index++] = rx_data;
            }
        }

        // 重新开启接收中断，准备接收下一个字节
        HAL_UART_Receive_IT(&huart2, (uint8_t*)&rx_data, 1);
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
