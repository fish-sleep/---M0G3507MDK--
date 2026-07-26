/*******************************************************************************
 * @file                main.c
 * @brief               fish_sleeping 基于龙邱开源库适配天猛星的工程模板
 * @copyright           原始版权 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 *
 * @description         基于龙邱科技 MSPM0G3507 开源库修改，适配嘉立创天猛星开发板。
 *                      将此工程套用到新开发板时，只需修改 hardware.c 中的外设初始化
 *                      和 main() 中添加自己的功能模块即可。
 *
 * @author              fish_sleeping  (天猛星适配修改)
 *******************************************************************************/

#include "headfile.h"
#include "hardware.h"

int main(void)
{
    LQ_System_Init();

    /* ==================== 外设初始化 ==================== */
    /* 在此调用 hardware.c 中对应的外设初始化函数，例如：
     *
     *   Motor_Init();
     *   UART_Init();
     *   Encoder_Init();
     *   LQ_OLED_Init();
     *   LQ_LSM6DSR_Init();
     *   Key_Init();
     *   LED_Init();
     *   Buzzer_Init();
     *   Tracking_Init();
     *   TIMG6_Control_Init(20);
     *   TIMG7_Tracking_Init(10);
     */

    while (1)
    {
        /* ==================== 主循环 ==================== */
        /* 在此添加你的功能逻辑，例如：
         *
         *   Key_Scan();
         *   OLED_Display();
         *   遥测上发();
         */

        delay_ms(10);
    }
}
