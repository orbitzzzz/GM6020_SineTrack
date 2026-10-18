#include "iwdg.h"
#include "config.h"

/*
 * IWDG：独立看门狗，时钟来自 LSI（F407 约 32kHz）。
 *
 * 超时时间 = 预分频 × (重装载值 + 1) / LSI
 *          = 64 × (999 + 1) / 32000
 *          = 2.0 秒
 *
 * 作业要求：配置文件里默认开启看门狗，所以 IWDG_ENABLE 默认为 1。
 */
IWDG_HandleTypeDef hiwdg;

void MX_IWDG_Init(void)
{
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Reload = IWDG_RELOAD;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
}
