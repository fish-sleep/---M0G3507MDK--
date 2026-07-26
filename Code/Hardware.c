/* ============================================================================
 * hardware.c — 外设初始化函数存放文件
 *
 * 本文件用于存放各外设的初始化函数，常见外设包括：
 *   - Motor_Init()        电机驱动初始化（GPIO + PWM）
 *   - Servo_PWM_Init()    舵机 PWM 初始化
 *   - UART_Init()         串口通信初始化
 *   - Encoder_Init()      编码器初始化
 *   - Key_Init()          按键 GPIO 初始化
 *   - LED_Init()          LED GPIO 初始化
 *   - Buzzer_Init()       蜂鸣器 GPIO 初始化
 *   - TIMGx_Control_Init()  定时器中断初始化
 *
 * 所有外设初始化函数在 hardware.h 中声明，在 main() 中调用。
 *
 * 当前为预留空文件，后续按需添加具体的初始化代码。
 * ============================================================================ */

#include "headfile.h"
#include "hardware.h"
