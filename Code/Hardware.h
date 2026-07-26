/* ============================================================================
 * hardware.h — 外设初始化函数声明
 *
 * 本文件声明各外设初始化函数，供 main.c 调用。
 *
 * 常见外设声明：
 *   void Motor_Init(void);              // 电机驱动初始化
 *   void Servo_PWM_Init(void);          // 舵机 PWM 初始化
 *   void UART_Init(void);               // 串口通信初始化
 *   void Encoder_Init(void);            // 编码器初始化
 *   void Key_Init(void);                // 按键 GPIO 初始化
 *   void Key_Scan(void);                // 按键扫描
 *   void LED_Init(void);                // LED GPIO 初始化
 *   void Buzzer_Init(void);             // 蜂鸣器 GPIO 初始化
 *   void TIMG6_Control_Init(uint32_t);  // TIMG6 定时中断初始化
 *   void TIMG7_Tracking_Init(uint32_t); // TIMG7 灰度采样定时器初始化
 *
 *   // 中断回调函数（在 main.c 或其他源文件中实现）
 *   void Encoder_Speed_Handler(void);   // 编码器速度定时中断回调
 *   void Control_Loop_Handler(void);    // 主控制循环定时中断回调
 *   void Tracking_Sampling_Handler(void); // 灰度采样定时中断回调
 *   void UART_RX_Callback(void);        // 串口接收中断回调
 *
 * 当前为预留模板，后续按需取消对应函数的注释。
 * ============================================================================ */

#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "stdint.h"

/* ---- PID 参数结构体（可复用的通用定义） ---- */
typedef struct {
    float Kp;
    float Ki;
    float Kd;
} PID_Param_t;

#endif /* __HARDWARE_H__ */
