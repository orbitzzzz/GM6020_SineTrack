#include "control.h"
#include "config.h"
#include "pid.h"
#include "motor.h"
#include "vofa.h"
#include "iwdg.h"
#include <math.h>

#define PI_F  3.14159265358979f

static PID_t s_speed_pid;   /* 速度环：rpm -> 电压值 */
static uint32_t s_tick = 0;         /* 1ms 计数 */
static uint8_t  s_mode = MODE_SPEED;
static uint32_t s_mode_start = 0;   /* 当前模式开始时的 tick */

/* 把角度误差折算到 -4096 ~ +4096，处理 0/8191 的圈边界 */
static float wrap_counts(float x)
{
  while (x > 4096.0f)
  {
    x -= GM6020_ENC_PER_REV;
  }
  while (x < -4096.0f)
  {
    x += GM6020_ENC_PER_REV;
  }
  return x;
}

static float clampf(float x, float lo, float hi)
{
  if (x > hi) return hi;
  if (x < lo) return lo;
  return x;
}

void Control_Init(void)
{
  PID_Init(&s_speed_pid,
           SPEED_KP, SPEED_KI, SPEED_KD,
           0.001f,
           -VOLTAGE_LIMIT, VOLTAGE_LIMIT,
           -VOLTAGE_LIMIT, VOLTAGE_LIMIT);

  s_tick = 0;
  s_mode_start = 0;

#if (CONTROL_MODE == MODE_AUTO)
  s_mode = MODE_SPEED;
#else
  s_mode = CONTROL_MODE;
#endif
}

/*
 * 每 1ms 调用一次（TIM2 更新中断）。
 * 1. 产生正弦参考
 * 2. 跑 PID
 * 3. 发 CAN 控制帧
 * 4. 每 10ms 发一帧 VOFA+ 数据
 * 5. 条件喂狗
 */
void Control_1ms(void)
{
  float speed_ref;
  float voltage;
  float pos_ref_deg = 0.0f;
  float pos_act_deg = 0.0f;
  float speed_act = (float)g_motor.speed_rpm;

  s_tick++;

#if (CONTROL_MODE == MODE_AUTO)
  /* 自动模式：15 秒换一次，方便一次运行截两张图 */
  if ((s_tick - s_mode_start) >= MODE_SWITCH_MS)
  {
    s_mode = (s_mode == MODE_SPEED) ? MODE_POSITION : MODE_SPEED;
    s_mode_start = s_tick;
    PID_Reset(&s_speed_pid);   /* 切换模式时清掉积分，避免残留 */
  }
#endif

  float t = (float)(s_tick - s_mode_start) * 0.001f;

  if (s_mode == MODE_SPEED)
  {
    /* ---------------- 速度正弦跟随 ---------------- */
    float w = 2.0f * PI_F * SPEED_SINE_FREQ;
    speed_ref = SPEED_SINE_AMP * sinf(w * t);

    /*
     * 前馈：按电机转速常数估算“维持这个转速大概需要多少电压”，
     * PID 只负责修正模型不准/负载变化带来的误差。
     */
    float feedforward = SPEED_FF_GAIN * speed_ref;
    voltage = feedforward + PID_Calc(&s_speed_pid, speed_ref, speed_act);

    /* 速度模式下位置不受控，位置通道置 0，让曲线图更干净 */
    pos_ref_deg = 0.0f;
    pos_act_deg = 0.0f;
  }
  else
  {
    /* ---------------- 位置正弦跟随（位置环 + 速度环串级） ---------------- */
    float w = 2.0f * PI_F * POS_SINE_FREQ;
    float phase = w * t;
    float s = sinf(phase);
    float c = sinf(phase + 0.5f * PI_F);   /* cos(phase)，复用 sinf 省空间 */

    float pos_ref = POS_CENTER + POS_SINE_AMP * s;

    /* 角度误差要过圈边界处理 */
    float pos_err = wrap_counts(pos_ref - (float)g_motor.angle);

    /* 位置参考本身的速度（rpm），作为速度环前馈 */
    float speed_feedforward =
        POS_SINE_AMP * w * c * (60.0f / GM6020_ENC_PER_REV);

    speed_ref = POS_KP * POS_DIR * pos_err + POS_DIR * speed_feedforward;
    speed_ref = clampf(speed_ref, -SPEED_REF_LIMIT, SPEED_REF_LIMIT);

    voltage = SPEED_FF_GAIN * speed_ref
            + PID_Calc(&s_speed_pid, speed_ref, speed_act);

    pos_ref_deg = pos_ref * (360.0f / GM6020_ENC_PER_REV);
    pos_act_deg = (float)g_motor.angle * (360.0f / GM6020_ENC_PER_REV);
  }

  /* 最终电压限幅 */
  voltage = clampf(voltage, -VOLTAGE_LIMIT, VOLTAGE_LIMIT);

  /*
   * 电机没上线时不输出电压，避免乱转。
   * 同时清掉速度环积分：否则电机没接时积分会一直累积，
   * 等电机上线瞬间会给出一个饱和电压，电机猛冲。
   */
  if (!Motor_IsOnline())
  {
    voltage = 0.0f;
    PID_Reset(&s_speed_pid);
  }

  /* 发 GM6020 电压控制帧（每 1ms 一帧） */
  Motor_SendVoltage((int16_t)voltage);

  /* 记录给 Ozone / VOFA+ 的调试变量 */
  g_dbg_ref_speed = speed_ref;
  g_dbg_act_speed = speed_act;
  g_dbg_ref_pos   = pos_ref_deg;
  g_dbg_act_pos   = pos_act_deg;
  g_dbg_mode      = (float)s_mode;

#if VOFA_ENABLE
  /* 每 10ms 发一帧，100Hz，足够画出 0.3Hz 的正弦 */
  if ((s_tick % VOFA_SEND_DIV) == 0u)
  {
    VOFA_Send(g_dbg_ref_speed, g_dbg_act_speed,
              g_dbg_ref_pos, g_dbg_act_pos, g_dbg_mode);
  }
#endif

#if IWDG_ENABLE
  /*
   * 喂狗条件：
   *   1) 调试时允许无条件喂（IWDG_FEED_ALWAYS=1）
   *   2) 上电头 2 秒，等电机上线
   *   3) 电机反馈正常（100ms 内收到过 CAN 帧）
   * 否则不喂，2 秒后 IWDG 复位。
   */
  if (IWDG_FEED_ALWAYS ||
      (s_tick < MOTOR_STARTUP_GRACE_MS) ||
      Motor_IsOnline())
  {
    HAL_IWDG_Refresh(&hiwdg);
  }
#endif

  /* 板载 LED 每 500ms 翻转一次，表示程序还活着 */
  if ((s_tick % 500u) == 0u)
  {
    HAL_GPIO_TogglePin(STATUS_LED_GPIO_PORT, STATUS_LED_PIN);
  }
}

/* TIM2 1ms 更新中断回调 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    Control_1ms();
  }
}
