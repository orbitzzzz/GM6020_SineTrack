#ifndef VOFA_H
#define VOFA_H

#include <stdint.h>

/*
 * 给 Ozone Data Sampling 用的全局变量（volatile 防止被优化掉）。
 * 上位机 VOFA+ 发的也是同一组数据。
 */
extern volatile float g_dbg_ref_speed;  /* 参考速度 rpm */
extern volatile float g_dbg_act_speed;  /* 实际速度 rpm */
extern volatile float g_dbg_ref_pos;    /* 参考位置 ° */
extern volatile float g_dbg_act_pos;    /* 实际位置 ° */
extern volatile float g_dbg_mode;       /* 0=速度模式 1=位置模式 */

void VOFA_Init(void);
void VOFA_Send(float ref_speed, float act_speed,
               float ref_pos, float act_pos, float mode);

#endif /* VOFA_H */
