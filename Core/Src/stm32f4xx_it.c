/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
#include "can.h"
#include "usart.h"

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;
extern CAN_HandleTypeDef hcan;
extern UART_HandleTypeDef huart1;

/******************************************************************************/
/* Cortex-M4 Processor Interruption and Exception Handlers                    */
/******************************************************************************/

void NMI_Handler(void)
{
  while (1)
  {
  }
}

void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

/**
  * @brief TIM2 更新中断：1ms 控制周期
  */
void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim2);
}

/**
  * @brief CAN1 RX0 中断：接收 GM6020 反馈帧
  * @note  F4 上 CAN1_RX0 有独立的中断向量，直接叫 CAN1_RX0_IRQHandler。
  */
void CAN1_RX0_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan);
}

/**
  * @brief USART1 中断：VOFA+ 发送完成
  */
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}
