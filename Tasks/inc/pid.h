#ifndef PID_H
#define PID_H

#include <stdint.h>

typedef struct
{
  float kp;
  float ki;
  float kd;
  float dt;          /* 采样周期，单位秒 */
  float integral;    /* 积分项（已经乘过 ki） */
  float prev_meas;   /* 上一次测量值，用于微分先行 */
  float out_min;
  float out_max;
  float int_min;
  float int_max;
  uint8_t first;     /* 第一次计算时不做微分 */
} PID_t;

void PID_Init(PID_t *pid, float kp, float ki, float kd,
              float dt, float out_min, float out_max,
              float int_min, float int_max);

void PID_Reset(PID_t *pid);

float PID_Calc(PID_t *pid, float ref, float meas);

#endif /* PID_H */
