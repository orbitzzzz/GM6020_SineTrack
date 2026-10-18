#include "gpio.h"

/*
 * 开发板 C 型板载 RGB LED：
 *   LED_R = PH12, LED_G = PH11, LED_B = PH10
 * 这里只用绿色 LED 做 500ms 心跳闪烁。
 * 即使板上是低电平点亮，也只会变成反相闪烁，不影响功能。
 */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOH_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOH, GPIO_PIN_11, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
}
