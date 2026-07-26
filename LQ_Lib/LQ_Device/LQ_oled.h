/*******************************************************************************
 * @file                LQ_oled.h
 * @brief               基于龙邱 MSPM0GX_LIB 开源库修改，支持 SPI / I2C 双模式切换
 * @copyright           原始版权 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         适配嘉立创天猛星 MSPM0G3507 开发板
 *                      通过宏定义切换 OLED 驱动模式：
 *                        OLED_DRIVER_SPI → 7脚 SPI（龙邱原版）
 *                        OLED_DRIVER_I2C → 4脚 I2C
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 系统时钟 : 80MHz
 *
 * 基于龙邱 GPL-3.0 开源协议代码修改。
 *
 * @author              fish_sleeping  (双模式适配修改)
 * @author              LQ_012        (龙邱原始 SPI OLED 驱动)
 * @email               chiusir@163.com
 * @version             V2.0.0
 *******************************************************************************/
#ifndef __LQ_OLED_H__
#define __LQ_OLED_H__

#include "include.h"

/* ========================================================================
 * 驱动模式选择（二选一，只保留一个取消注释）
 * ======================================================================== */
// #define OLED_DRIVER_SPI       /* 7脚 SPI  OLED（龙邱原版） */
#define OLED_DRIVER_I2C           /* 4脚 I2C OLED            */

/****************************************************************************************************
 * @brief   通用宏定义（两种模式共用）
 ****************************************************************************************************/

#define OLED_WIDTH             128U
#define OLED_HEIGHT            64U
#define OLED_PAGE_COUNT        (OLED_HEIGHT / 8U)

/* ========================================================================
 * SPI 模式配置
 * ======================================================================== */
#ifdef OLED_DRIVER_SPI

#define OLED_COLUMN_OFFSET     2U        // SH1106 132列GRAM偏移

#define OLED_SCL_PIN           GPIO_Pin_A_22   // SPI 时钟 SCK
#define OLED_SDA_PIN           GPIO_Pin_A_15   // SPI 数据 MOSI
#define OLED_CS_PIN            GPIO_Pin_B_22   // SPI 片选 CS
#define OLED_RES_PIN           GPIO_Pin_A_12   // 复位 RES
#define OLED_DC_PIN            GPIO_Pin_A_13   // 数据/命令 DC

#define OLED_SCL_Clr()         LQ_GPIO_WritePin(OLED_SCL_PIN, 0)
#define OLED_SCL_Set()         LQ_GPIO_WritePin(OLED_SCL_PIN, 1)
#define OLED_SDA_Clr()         LQ_GPIO_WritePin(OLED_SDA_PIN, 0)
#define OLED_SDA_Set()         LQ_GPIO_WritePin(OLED_SDA_PIN, 1)
#define OLED_RES_Clr()         LQ_GPIO_WritePin(OLED_RES_PIN, 0)
#define OLED_RES_Set()         LQ_GPIO_WritePin(OLED_RES_PIN, 1)
#define OLED_DC_Clr()          LQ_GPIO_WritePin(OLED_DC_PIN, 0)
#define OLED_DC_Set()          LQ_GPIO_WritePin(OLED_DC_PIN, 1)
#define OLED_CS_Clr()          LQ_GPIO_WritePin(OLED_CS_PIN, 0)
#define OLED_CS_Set()          LQ_GPIO_WritePin(OLED_CS_PIN, 1)

#define OLED_SDA_Read()        0   // SPI 不需要读

/* ========================================================================
 * I2C 模式配置
 * ======================================================================== */
#elif defined(OLED_DRIVER_I2C)

#define OLED_COLUMN_OFFSET     0U        // I2C 模块多为 SSD1306，无列偏移

#define OLED_SCL_PIN           GPIO_Pin_A_22   // I2C 时钟 SCL
#define OLED_SDA_PIN           GPIO_Pin_A_15   // I2C 数据 SDA

#define OLED_I2C_ADDR          0x78           // I2C 8位写地址 (7位地址 0x3C << 1)

#define OLED_SCL_Clr()         LQ_GPIO_WritePin(OLED_SCL_PIN, 0)
#define OLED_SCL_Set()         LQ_GPIO_WritePin(OLED_SCL_PIN, 1)
#define OLED_SDA_Clr()         LQ_GPIO_WritePin(OLED_SDA_PIN, 0)
#define OLED_SDA_Set()         LQ_GPIO_WritePin(OLED_SDA_PIN, 1)
#define OLED_SDA_Read()        LQ_GPIO_ReadPin(OLED_SDA_PIN)

/* I2C 模式无 RES/DC/CS，定义为空宏保持上层代码兼容 */
#define OLED_RES_Clr()
#define OLED_RES_Set()
#define OLED_DC_Clr()
#define OLED_DC_Set()
#define OLED_CS_Clr()
#define OLED_CS_Set()

#else
#error "请在 LQ_oled.h 中选择 OLED 驱动模式：OLED_DRIVER_SPI 或 OLED_DRIVER_I2C"
#endif

/****************************************************************************************************
 * @brief   类型 / 枚举
 ****************************************************************************************************/

typedef enum
{
    OLED_CMD  = 0x00,    // 命令
#ifdef OLED_DRIVER_SPI
    OLED_DATA = 0x01,    // SPI 模式：0x01 表示数据
#else
    OLED_DATA = 0x40,    // I2C 模式：0x40 是 I2C 控制字节
#endif
} LQ_OLED_Instruct_t;

/****************************************************************************************************
 * @brief   函数声明（两种模式接口完全相同）
 ****************************************************************************************************/

void LQ_OLED_Init(void);
void LQ_OLED_ColorTurn(unsigned char i);
void LQ_OLED_DisplayTurn(unsigned char i);
void LQ_OLED_DisPlay_On(void);
void LQ_OLED_DisPlay_Off(void);
void LQ_OLED_Refresh(void);
void LQ_OLED_Clear(void);
void LQ_OLED_DrawPoint(unsigned char x, unsigned char y, unsigned char t);
void LQ_OLED_DrawLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char mode);
void LQ_OLED_DrawCircle(unsigned char x, unsigned char y, unsigned char r);
void LQ_OLED_ShowChar(unsigned char x, unsigned char y, unsigned char chr, unsigned char size1, unsigned char mode);
void LQ_OLED_ShowString(unsigned char y, unsigned char x, unsigned char *chr, unsigned char size1);
unsigned int LQ_OLED_Pow(unsigned char m, unsigned char n);
void LQ_OLED_ShowNum(unsigned char x, unsigned char y, unsigned int num, unsigned char len, unsigned char size1, unsigned char mode);
void LQ_OLED_ShowChinese(unsigned char x, unsigned char y, unsigned char num, unsigned char mode);
void LQ_OLED_ScrollDisplay(unsigned char num, unsigned char space, unsigned char mode);
void LQ_OLED_ShowPicture(unsigned char x, unsigned char y, unsigned char sizex, unsigned char sizey, unsigned char BMP[], unsigned char mode);

#endif
