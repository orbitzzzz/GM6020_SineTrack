#include "vofa.h"
#include "usart.h"
#include "config.h"
#include <string.h>

volatile float g_dbg_ref_speed = 0.0f;
volatile float g_dbg_act_speed = 0.0f;
volatile float g_dbg_ref_pos   = 0.0f;
volatile float g_dbg_act_pos   = 0.0f;
volatile float g_dbg_mode      = 0.0f;

/*
 * VOFA+ 的 JustFloat 协议：
 *   5 个 float（小端，20 字节）+ 帧尾 00 00 80 7F，共 24 字节。
 *   VOFA+ 里选择协议 JustFloat，通道数 5 即可。
 */
#define VOFA_CHANNELS   5
#define VOFA_TAIL_LEN   4
#define VOFA_FRAME_LEN  (VOFA_CHANNELS * 4 + VOFA_TAIL_LEN)

static uint8_t s_tx_buf[VOFA_FRAME_LEN];
static volatile uint8_t s_busy = 0;

void VOFA_Init(void)
{
  s_busy = 0;
}

void VOFA_Send(float ref_speed, float act_speed,
               float ref_pos, float act_pos, float mode)
{
#if VOFA_ENABLE
  if (s_busy)
  {
    return;   /* 上一帧还没发完，丢掉这一帧，绝不阻塞控制循环 */
  }

  float values[VOFA_CHANNELS] = {ref_speed, act_speed, ref_pos, act_pos, mode};
  memcpy(s_tx_buf, values, sizeof(values));

  s_tx_buf[20] = 0x00;
  s_tx_buf[21] = 0x00;
  s_tx_buf[22] = 0x80;
  s_tx_buf[23] = 0x7F;

  s_busy = 1;
  if (HAL_UART_Transmit_IT(&huart1, s_tx_buf, VOFA_FRAME_LEN) != HAL_OK)
  {
    s_busy = 0;
  }
#endif
}

/* 串口发送完成回调 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    s_busy = 0;
  }
}

/*
 * 串口出错时也要清 busy 标志。
 * 否则一次错误会让 busy 永远为 1，之后再也发不出波形。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    s_busy = 0;
  }
}
