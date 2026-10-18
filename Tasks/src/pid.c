#include "pid.h"

void PID_Init(PID_t *pid, float kp, float ki, float kd,
              float dt, float out_min, float out_max,
              float int_min, float int_max)
{
  pid->kp = kp;
  pid->ki = ki;
  pid->kd = kd;
  pid->dt = dt;
  pid->out_min = out_min;
  pid->out_max = out_max;
  pid->int_min = int_min;
  pid->int_max = int_max;
  PID_Reset(pid);
}

void PID_Reset(PID_t *pid)
{
  pid->integral = 0.0f;
  pid->prev_meas = 0.0f;
  pid->first = 1;
}

/*
 * 位置式 PID + 积分限幅 + 输出饱和时停止积分（抗积分饱和）
 * 微分项使用“测量值微分”，避免参考值突变时输出冲击。
 */
float PID_Calc(PID_t *pid, float ref, float meas)
{
  float err = ref - meas;
  float p_term = pid->kp * err;

  /* 积分 */
  pid->integral += pid->ki * err * pid->dt;
  if (pid->integral > pid->int_max) pid->integral = pid->int_max;
  if (pid->integral < pid->int_min) pid->integral = pid->int_min;

  /* 微分（第一次不计算） */
  float d_term = 0.0f;
  if (!pid->first)
  {
    d_term = -pid->kd * (meas - pid->prev_meas) / pid->dt;
  }
  pid->prev_meas = meas;
  pid->first = 0;

  float out = p_term + pid->integral + d_term;

  /* 输出限幅 + 抗饱和：已经顶到限幅就不要再往同方向积分 */
  if (out > pid->out_max)
  {
    out = pid->out_max;
    if (err > 0.0f) pid->integral -= pid->ki * err * pid->dt;
  }
  else if (out < pid->out_min)
  {
    out = pid->out_min;
    if (err < 0.0f) pid->integral -= pid->ki * err * pid->dt;
  }

  return out;
}
