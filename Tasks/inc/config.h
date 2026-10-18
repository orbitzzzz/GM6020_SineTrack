#ifndef CONFIG_H
#define CONFIG_H

/*
 * ============================================================================
 *  所有需要改的参数都在这个文件里。
 *  改完以后重新编译、重新下载即可，不需要动其它代码。
 *
 *  硬件：RoboMaster 开发板 C 型（STM32F407IGH6，168MHz）
 *        GM6020 接 CAN1（PD0/PD1），电机 ID 默认 1
 *        CAN 1Mbps，控制周期 1ms
 * ============================================================================
 */

/* ------------------------- 1. 运行模式 ------------------------- */
#define MODE_SPEED       0   /* 只做速度正弦跟随 */
#define MODE_POSITION    1   /* 只做位置正弦跟随 */
#define MODE_AUTO        2   /* 自动交替：先速度、再位置，循环 */

/*
 * 默认用 AUTO：下载一次程序，15 秒速度模式 + 15 秒位置模式自动切换，
 * 方便一次运行就把两张截图都拍下来。
 * 考核时如果想固定某一种模式，把下面改成 MODE_SPEED 或 MODE_POSITION。
 */
#define CONTROL_MODE        MODE_AUTO
#define MODE_SWITCH_MS      15000u   /* AUTO 模式下每种模式持续 15 秒 */

/* ------------------------- 2. GM6020 参数 ------------------------- */
/*
 * 电机 ID 由电机上的拨码/调参软件决定，默认 1。
 * 如果实验室的电机 ID 不是 1，只改这里即可。
 */
#define MOTOR_ID            1

/* GM6020 反馈的机械角度范围是 0~8191，对应 0~360° */
#define GM6020_ENC_PER_REV  8192.0f

/* ------------------------- 3. 正弦曲线参数 ------------------------- */
/* 速度正弦：幅值 200 rpm，频率 0.3 Hz（约 3.3 秒一个周期） */
#define SPEED_SINE_AMP      200.0f    /* rpm */
#define SPEED_SINE_FREQ     0.3f      /* Hz */

/*
 * 位置正弦：幅值 2000 个编码器计数（约 88°），中心 4096（180°）。
 * 4096 ± 2000 不会跨过 0/8191 的圈边界，曲线不会跳变。
 */
#define POS_SINE_AMP        2000.0f   /* 编码器计数 */
#define POS_SINE_FREQ       0.3f      /* Hz */
#define POS_CENTER          4096.0f   /* 编码器计数 */

/* ------------------------- 4. PID 参数 ------------------------- */
/*
 * 速度环：输入 rpm，输出 -30000~30000 的电压值。
 * 前馈系数 90 是按 GM6020 大约 13.3 rpm/V 估算的：
 *   30000 / 24V / 13.3rpm/V ≈ 94，取 90。
 * 前馈负责“大致给到位”，PID 只负责修正误差。
 *
 * 如果实际曲线超调/抖动：先减小 SPEED_KP；如果跟随太慢/幅值不够：增大 SPEED_KI。
 */
#define SPEED_KP            150.0f
#define SPEED_KI            150.0f
#define SPEED_KD            0.0f
#define SPEED_FF_GAIN       90.0f     /* 前馈：电压值 / rpm */

/*
 * 位置环：输入编码器计数误差，输出速度指令（rpm）。
 * 位置环只用一个 P，速度环负责动态响应。
 */
#define POS_KP              0.30f

/* 位置环输出的速度指令限幅，防止切换模式时猛冲 */
#define SPEED_REF_LIMIT     250.0f    /* rpm */

/*
 * 位置环方向。正常情况是 +1。
 * 万一上电后发现电机朝远离目标的方向跑（位置环变成正反馈），
 * 立刻断电，把这个值改成 -1 即可。
 */
#define POS_DIR             1.0f
/* 最终电压输出限幅，30000 对应 24V；先限制到 24000 更安全 */
#define VOLTAGE_LIMIT       24000.0f

/* ------------------------- 5. 看门狗 ------------------------- */
/*
 * 作业要求：配置文件里默认开启看门狗。
 * IWDG_ENABLE 默认为 1，即上电就启用。
 *
 * F407 的 LSI 约 32kHz（F103 是 40kHz，这点不一样）：
 *   超时 = 64 × (999 + 1) / 32000 = 2.0 秒
 *
 * 喂狗策略：
 *   只有“控制循环在跑 + 电机反馈正常”时才喂狗。
 *   如果 CAN 线断了 / 电机掉电 / 程序卡死，2 秒后芯片自动复位，
 *   复位后电机收不到控制帧，GM6020 自己也会停转，形成双重保护。
 */
#define IWDG_ENABLE             1
#define IWDG_PRESCALER          64
#define IWDG_RELOAD             999
#define MOTOR_LOST_TIMEOUT_MS   100u   /* 100ms 没收到反馈就算离线 */
#define MOTOR_STARTUP_GRACE_MS  2000u  /* 上电后给 2 秒时间等电机上线 */

/*
 * 调试用：没有接电机时，如果不想一直复位，把下面改成 1。
 * 正常考核/演示时保持 0，才能体现“失联复位”的保护效果。
 */
#define IWDG_FEED_ALWAYS        0

/* ------------------------- 6. VOFA+ 串口输出 ------------------------- */
/*
 * 开发板 C 型 4-pin UART（外壳丝印 UART2）= USART1：
 *   PA9 = TX，PB7 = RX，波特率 115200。
 * 每 10ms 发一帧 JustFloat 数据：
 *   [参考速度, 实际速度, 参考位置(°), 实际位置(°), 模式(0=速度 1=位置)]
 *
 * 接线：C 板 UART2 的 TX -> USB-TTL 的 RX，GND 共地。
 * 没有 USB-TTL 时，把 VOFA_ENABLE 改成 0，用 Ozone 的 Data Sampling
 * 采样下面这些全局变量：
 *   g_dbg_ref_speed, g_dbg_act_speed
 *   g_dbg_ref_pos,   g_dbg_act_pos
 *   g_dbg_mode
 */
#define VOFA_ENABLE         1
#define VOFA_UART_BAUD      115200
#define VOFA_SEND_DIV       10        /* 1kHz / 10 = 100Hz */

/* ------------------------- 7. 板载 LED ------------------------- */
/* C 板 RGB LED：LED_G = PH11。用于 500ms 心跳闪烁。 */
#define STATUS_LED_GPIO_PORT  GPIOH
#define STATUS_LED_PIN        GPIO_PIN_11

#endif /* CONFIG_H */
