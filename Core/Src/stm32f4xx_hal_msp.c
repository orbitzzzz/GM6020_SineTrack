#include "main.h"

/**
  * @brief 全局 MSP 初始化。
  *        HAL_Init() 会自动调用它，早于 SystemClock_Config()。
  */
void HAL_MspInit(void)
{
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  /* 4 位抢占优先级，0 最高。TIM2 控制中断用 0，CAN 用 1，串口用 2。 */
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}
