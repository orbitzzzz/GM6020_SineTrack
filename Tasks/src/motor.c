#include "motor.h"
#include "can.h"

#if (MOTOR_ID < 1) || (MOTOR_ID > 7)
#error "MOTOR_ID must be 1..7 (GM6020 支持 ID 1~7)"
#endif

MotorState_t g_motor;

/*
 * 初始化 CAN 滤波器并启动 CAN。
 * 滤波器全 0 = 接收总线上所有标准帧，最简单的做法。
 */
void Motor_Init(void)
{
  CAN_FilterTypeDef filter = {0};

  filter.FilterBank = 0;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterIdHigh = 0x0000;
  filter.FilterIdLow = 0x0000;
  filter.FilterMaskIdHigh = 0x0000;
  filter.FilterMaskIdLow = 0x0000;
  filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  filter.FilterActivation = ENABLE;
  filter.SlaveStartFilterBank = 14;

  if (HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_CAN_Start(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    Error_Handler();
  }
}

/*
 * 给 GM6020 发电压控制帧。
 *
 * ID 1~4：标准帧 0x1FF，8 字节里依次放 4 个 int16（小端）
 * ID 5~7：标准帧 0x2FF，同理
 * 每个 int16 的范围 -30000~30000，对应 -24V~+24V。
 */
void Motor_SendVoltage(int16_t voltage)
{
  CAN_TxHeaderTypeDef tx_header = {0};
  uint8_t data[8] = {0};
  uint32_t mailbox = 0;

  int32_t v = voltage;
  if (v > 30000)  v = 30000;
  if (v < -30000) v = -30000;

  uint8_t index;
  if (MOTOR_ID >= 1 && MOTOR_ID <= 4)
  {
    tx_header.StdId = 0x1FF;
    index = (uint8_t)((MOTOR_ID - 1) * 2);
  }
  else
  {
    tx_header.StdId = 0x2FF;
    index = (uint8_t)((MOTOR_ID - 5) * 2);
  }

  data[index]     = (uint8_t)(v & 0xFF);
  data[index + 1] = (uint8_t)((v >> 8) & 0xFF);

  tx_header.ExtId = 0;
  tx_header.IDE = CAN_ID_STD;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.DLC = 8;
  tx_header.TransmitGlobalTime = DISABLE;

  if (HAL_CAN_AddTxMessage(&hcan, &tx_header, data, &mailbox) != HAL_OK)
  {
    g_motor.err_count++;
  }
}

/*
 * CAN 接收中断回调：解析 GM6020 反馈帧。
 *
 * 反馈帧 ID = 0x204 + 电机 ID（ID1 -> 0x205）
 * 数据格式（大端！）：
 *   data[0..1] 机械角度 0~8191
 *   data[2..3] 转速 rpm（int16）
 *   data[4..5] 实际转矩电流（int16）
 *   data[6]    温度
 *   data[7]    保留
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan_ptr)
{
  CAN_RxHeaderTypeDef rx_header;
  uint8_t data[8];

  while (HAL_CAN_GetRxFifoFillLevel(hcan_ptr, CAN_RX_FIFO0) > 0)
  {
    if (HAL_CAN_GetRxMessage(hcan_ptr, CAN_RX_FIFO0, &rx_header, data) != HAL_OK)
    {
      continue;
    }
    if (rx_header.IDE != CAN_ID_STD)
    {
      continue;
    }

    uint32_t std_id = rx_header.StdId;
    if (std_id >= 0x205 && std_id <= 0x20B)
    {
      uint8_t motor_id = (uint8_t)(std_id - 0x204);

      /* 调试信息：不管 ID 是不是目标电机，都记录总线上有反馈 */
      g_motor.any_rx_count++;
      g_motor.last_rx_id = motor_id;

      if (motor_id != MOTOR_ID)
      {
        continue;
      }

      g_motor.angle       = (uint16_t)((data[0] << 8) | data[1]);
      g_motor.speed_rpm   = (int16_t)((data[2] << 8) | data[3]);
      g_motor.current     = (int16_t)((data[4] << 8) | data[5]);
      g_motor.temperature = data[6];
      g_motor.last_rx_tick = HAL_GetTick();
      g_motor.rx_count++;
    }
  }
}

uint8_t Motor_IsOnline(void)
{
  if (g_motor.rx_count == 0)
  {
    return 0;
  }
  return (HAL_GetTick() - g_motor.last_rx_tick) < MOTOR_LOST_TIMEOUT_MS;
}
