/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : GM6020 正弦速度/位置跟随 + IWDG 看门狗保护
  ******************************************************************************
  * 目标芯片：STM32F407IGH6（RoboMaster 开发板 C 型，168 MHz）
  * 开发环境：CMake + Ninja + VS Code + arm-none-eabi-gcc + OpenOCD
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "iwdg.h"
#include "tim.h"
#include "gpio.h"
#include "can.h"
#include "usart.h"

/* USER CODE BEGIN Includes */
#include "config.h"
#include "motor.h"
#include "control.h"
#include "vofa.h"
/* USER CODE END Includes */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

int main(void)
{
  /* HAL 初始化：Flash 预取、SysTick 1ms */
  HAL_Init();

  /* 系统时钟：HSE 12MHz -> PLL -> 168MHz */
  SystemClock_Config();

  /* GPIO：板载 LED（PH11） */
  MX_GPIO_Init();

  /*
   * 初始化顺序：
   *   CAN -> 电机对象 -> 串口/VOFA -> 控制参数 -> 看门狗 -> 1ms 定时器
   * 看门狗必须在控制循环启动前使能，控制循环跑起来后才会开始喂狗。
   */
  MX_CAN_Init();
  Motor_Init();

#if VOFA_ENABLE
  MX_USART1_UART_Init();
  VOFA_Init();
#endif

  Control_Init();

#if IWDG_ENABLE
  MX_IWDG_Init();
#endif

  MX_TIM2_Init();
  HAL_TIM_Base_Start_IT(&htim2);   /* 启动 1ms 控制中断 */

  /* 所有实时控制都在 TIM2 中断里完成，主循环空转 */
  while (1)
  {
  }
}

/**
  * @brief 系统时钟：12MHz HSE -> 168MHz
  *
  * PLLM = 6  : 12MHz / 6  = 2MHz  (VCO 输入)
  * PLLN = 168: 2MHz * 168 = 336MHz (VCO)
  * PLLP = 2  : 336 / 2    = 168MHz (SYSCLK)
  * PLLQ = 7  : 336 / 7    = 48MHz  (USB/SDIO 等)
  * AHB = 168MHz, APB1 = 42MHz, APB2 = 84MHz
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState       = RCC_LSI_ON;   /* IWDG 用 LSI */
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM       = 6;
  RCC_OscInitStruct.PLL.PLLN       = 168;
  RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ       = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;   /* 42MHz  */
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;   /* 84MHz  */
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief 出错后停机（调试时可以在这里打断点）。
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif /* USE_FULL_ASSERT */
