#ifndef __MENU_H__
#define __MENU_H__

#include <stdint.h>

/**
 * @brief 主界面 OLED 显示（循迹调试信息）
 * @note  每行 21 字符，8 行 OLED 128x64
 *        0: 右轮速度 PID
 *        1: 左轮速度 PID
 *        2: 循迹 PID
 *        3: 跟踪误差
 *        4: 循迹输出
 *        5: 目标/实际速度
 *        6: PWM 输出
 *        7: OUT1~OUT5 黑白状态
 */
void Menu_DisplayMain(void);

#endif /* __MENU_H__ */
