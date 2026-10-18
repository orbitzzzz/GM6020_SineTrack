#ifndef MOTOR_H
#define MOTOR_H

#include "main.h"
#include "config.h"

/* GM6020 反馈数据（都在 CAN 接收中断里更新） */
typedef struct
{
  volatile uint16_t angle;       /* 机械角度 0~8191 */
  volatile int16_t  speed_rpm;   /* 转速，单位 rpm */
  volatile int16_t  current;     /* 实际转矩电流值 */
  volatile uint8_t  temperature; /* 温度，摄氏度 */
  volatile uint32_t last_rx_tick;/* 最后一次收到反馈的时刻 */
  volatile uint32_t rx_count;    /* 收到多少帧，用来确认通信正常 */
  volatile uint32_t err_count;   /* 发送失败/错误计数 */
  volatile uint32_t any_rx_count;/* 总线上任意 GM6020 反馈帧数（调试用） */
  volatile uint8_t  last_rx_id;  /* 最近一帧反馈的电机 ID（调试用） */
} MotorState_t;

extern MotorState_t g_motor;

void Motor_Init(void);
void Motor_SendVoltage(int16_t voltage);
uint8_t Motor_IsOnline(void);

#endif /* MOTOR_H */
