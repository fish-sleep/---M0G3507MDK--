/*******************************************************************************
 * @file                LQ_oled.c
 * @brief               基于龙邱 MSPM0GX_LIB 开源库修改，支持 SPI / I2C 双模式
 * @copyright           原始版权 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         适配嘉立创天猛星 MSPM0G3507 开发板
 *                      在 LQ_oled.h 中通过 OLED_DRIVER_SPI / OLED_DRIVER_I2C 切换
 *
 * 接线方式：
 *   [SPI 7脚]  SCL=PA22  SDA=PA15  RES=PA12  DC=PA13  CS=PB22  VCC  GND
 *   [I2C 4脚]  SCL=PA22  SDA=PA15  VCC  GND
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 系统时钟 : 80MHz
 *
 * 基于龙邱 GPL-3.0 开源协议代码修改。
 *
 * @author              fish_sleeping  (I2C 适配 & 双模式合并)
 * @author              wuwu          (龙邱 SPI OLED 原始编写)
 * @author              LQ_012        (龙邱优化与应用层编写)
 * @email               chiusir@163.com
 * @version             V2.0.0
 *******************************************************************************/
#include "LQ_oled.h"
#include "LQ_font.h"

#ifdef OLED_DRIVER_SPI
#include "LQ_spi.h"
#elif defined(OLED_DRIVER_I2C)
#include "LQ_soft_i2c.h"
#endif

/* ========================================================================
 * I2C 模式静态配置
 * ======================================================================== */
#ifdef OLED_DRIVER_I2C
static LQConfig_SoftI2C_InitTypeDef_t oled_i2c = {
    .scl  = OLED_SCL_PIN,
    .sda  = OLED_SDA_PIN,
    .addr = (OLED_I2C_ADDR >> 1),  // 右移1位得到 7-bit 地址 0x3C
};
#endif

/* ========================================================================
 * OLED 写字节 — SPI / I2C 双实现
 * ======================================================================== */
#ifdef OLED_DRIVER_SPI
/*************************************************************************
 * @brief       OLED写入一个字节（SPI 模拟时序）
 * @param       dat : 待写入的数据
 * @param       cmd : 1=命令，0=数据
 *
 * @note        DC 引脚区分命令/数据，CS 选通，SCL/SDA 模拟 SPI
 *************************************************************************/
static void LQ_OLED_WR_Byte(unsigned char dat, unsigned char cmd)
{
    unsigned char i;
    if (cmd) OLED_DC_Set();
    else     OLED_DC_Clr();
    OLED_CS_Clr();

    for (i = 0; i < 8; i++) {
        OLED_SCL_Clr();
        if (dat & 0x80) OLED_SDA_Set();
        else            OLED_SDA_Clr();
        OLED_SCL_Set();
        dat <<= 1;
    }

    OLED_CS_Set();
    OLED_DC_Set();
}

#elif defined(OLED_DRIVER_I2C)
/*************************************************************************
 * @brief       OLED写入一个字节（I2C 总线）
 * @param       dat : 待写入的数据
 * @param       cmd : OLED_CMD(0x00)=命令, OLED_DATA(0x40)=数据
 *
 * @note        I2C 写流程：Start → 设备地址 → 控制字节 → 数据字节 → Stop
 *************************************************************************/
static void LQ_OLED_WR_Byte(unsigned char dat, unsigned char cmd)
{
    LQ_Soft_I2C_Start(&oled_i2c);
    LQ_Soft_I2C_WriteData(&oled_i2c, OLED_I2C_ADDR);
    LQ_Soft_I2C_Wait_ACK(&oled_i2c);
    LQ_Soft_I2C_WriteData(&oled_i2c, cmd);
    LQ_Soft_I2C_Wait_ACK(&oled_i2c);
    LQ_Soft_I2C_WriteData(&oled_i2c, dat);
    LQ_Soft_I2C_Wait_ACK(&oled_i2c);
    LQ_Soft_I2C_Stop(&oled_i2c);
}
#endif

/* ========================================================================
 * 显存 GRAM（两种模式共用）
 * ======================================================================== */
static unsigned char OLED_GRAM[144][8];

/* ========================================================================
 * 以下所有函数两种模式通用，通过宏切换底层 LQ_OLED_WR_Byte 实现
 * ======================================================================== */

void LQ_OLED_ColorTurn(unsigned char i)
{
    if (i == 0) LQ_OLED_WR_Byte(0xA6, OLED_CMD);
    if (i == 1) LQ_OLED_WR_Byte(0xA7, OLED_CMD);
}

void LQ_OLED_DisplayTurn(unsigned char i)
{
    if (i == 0) {
        LQ_OLED_WR_Byte(0xC8, OLED_CMD);
        LQ_OLED_WR_Byte(0xA1, OLED_CMD);
    }
    if (i == 1) {
        LQ_OLED_WR_Byte(0xC0, OLED_CMD);
        LQ_OLED_WR_Byte(0xA0, OLED_CMD);
    }
}

void LQ_OLED_DisPlay_On(void)
{
    LQ_OLED_WR_Byte(0xAD, OLED_CMD);
    LQ_OLED_WR_Byte(0x8B, OLED_CMD);
    LQ_OLED_WR_Byte(0xAF, OLED_CMD);
}

void LQ_OLED_DisPlay_Off(void)
{
    LQ_OLED_WR_Byte(0xAE, OLED_CMD);
    LQ_OLED_WR_Byte(0xAD, OLED_CMD);
    LQ_OLED_WR_Byte(0x8A, OLED_CMD);
}

void LQ_OLED_Refresh(void)
{
    unsigned char i, n;
    for (i = 0; i < OLED_PAGE_COUNT; i++) {
        LQ_OLED_WR_Byte(0xb0 + i, OLED_CMD);
        LQ_OLED_WR_Byte(0x00 | (OLED_COLUMN_OFFSET & 0x0F), OLED_CMD);
        LQ_OLED_WR_Byte(0x10 | (OLED_COLUMN_OFFSET >> 4), OLED_CMD);
        for (n = 0; n < OLED_WIDTH; n++)
            LQ_OLED_WR_Byte(OLED_GRAM[n][i], OLED_DATA);
    }
}

void LQ_OLED_Clear(void)
{
    unsigned char i, n;
    for (i = 0; i < OLED_PAGE_COUNT; i++)
        for (n = 0; n < OLED_WIDTH; n++)
            OLED_GRAM[n][i] = 0;
    LQ_OLED_Refresh();
}

void LQ_OLED_DrawPoint(unsigned char x, unsigned char y, unsigned char t)
{
    unsigned char i, m, n;
    i = y / 8;
    m = y % 8;
    n = 1 << m;
    if (t) { OLED_GRAM[x][i] |= n; }
    else {
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
        OLED_GRAM[x][i] |= n;
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
    }
}

void LQ_OLED_DrawLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char mode)
{
    unsigned int t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;
    if (delta_x > 0) incx = 1;
    else if (delta_x == 0) incx = 0;
    else { incx = -1; delta_x = -delta_x; }
    if (delta_y > 0) incy = 1;
    else if (delta_y == 0) incy = 0;
    else { incy = -1; delta_y = -delta_y; }
    if (delta_x > delta_y) distance = delta_x;
    else distance = delta_y;
    for (t = 0; t < distance + 1; t++) {
        LQ_OLED_DrawPoint(uRow, uCol, mode);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) { xerr -= distance; uRow += incx; }
        if (yerr > distance) { yerr -= distance; uCol += incy; }
    }
}

void LQ_OLED_DrawCircle(unsigned char x, unsigned char y, unsigned char r)
{
    int a, b, num;
    a = 0;
    b = r;
    while (2 * b * b >= r * r) {
        LQ_OLED_DrawPoint(x + a, y - b, 1);
        LQ_OLED_DrawPoint(x - a, y - b, 1);
        LQ_OLED_DrawPoint(x - a, y + b, 1);
        LQ_OLED_DrawPoint(x + a, y + b, 1);
        LQ_OLED_DrawPoint(x + b, y + a, 1);
        LQ_OLED_DrawPoint(x + b, y - a, 1);
        LQ_OLED_DrawPoint(x - b, y - a, 1);
        LQ_OLED_DrawPoint(x - b, y + a, 1);
        a++;
        num = (a * a + b * b) - r * r;
        if (num > 0) { b--; a--; }
    }
}

void LQ_OLED_ShowChar(unsigned char x, unsigned char y, unsigned char chr, unsigned char size1, unsigned char mode)
{
    unsigned char i, m, temp, size2, chr1;
    unsigned char x0 = x, y0 = y;
    if (size1 == 8) size2 = 6;
    else size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2);
    chr1 = chr - ' ';
    for (i = 0; i < size2; i++) {
        if (size1 == 8)      { temp = asc2_0806[chr1][i]; }
        else if (size1 == 12) { temp = asc2_1206[chr1][i]; }
        else return;
        for (m = 0; m < 8; m++) {
            if (temp & 0x01) LQ_OLED_DrawPoint(x, y, mode);
            else             LQ_OLED_DrawPoint(x, y, !mode);
            temp >>= 1;
            y++;
        }
        x++;
        if ((size1 != 8) && ((x - x0) == size1 / 2)) { x = x0; y0 = y0 + 8; }
        y = y0;
    }
}

void LQ_OLED_ShowString(unsigned char y, unsigned char x, unsigned char *chr, unsigned char size1)
{
    char mode = 1;
    while ((*chr >= ' ') && (*chr <= '~')) {
        if (size1 == 8)  LQ_OLED_ShowChar(x, y * 8, *chr, size1, mode);
        if (size1 == 12) LQ_OLED_ShowChar(x, y * 16, *chr, size1, mode);
        if (size1 == 8)  x += 6;
        else             x += size1 / 2;
        chr++;
    }
}

unsigned int LQ_OLED_Pow(unsigned char m, unsigned char n)
{
    unsigned int result = 1;
    while (n--) result *= m;
    return result;
}

void LQ_OLED_ShowNum(unsigned char x, unsigned char y, unsigned int num, unsigned char len, unsigned char size1, unsigned char mode)
{
    unsigned char t, temp, m = 0;
    if (size1 == 8) m = 2;
    for (t = 0; t < len; t++) {
        temp = (num / LQ_OLED_Pow(10, len - t - 1)) % 10;
        if (temp == 0)
            LQ_OLED_ShowChar(x + (size1 / 2 + m) * t, y, '0', size1, mode);
        else
            LQ_OLED_ShowChar(x + (size1 / 2 + m) * t, y, temp + '0', size1, mode);
    }
}

void LQ_OLED_ShowChinese(unsigned char y, unsigned char x, unsigned char num, unsigned char mode)
{
    y *= 16;
    unsigned char m, temp;
    unsigned char x0 = x, y0 = y;
    unsigned int i, size3 = (14 / 8 + ((14 % 8) ? 1 : 0)) * 14;
    for (i = 0; i < size3; i++) {
        temp = Hzk1[num][i];
        for (m = 0; m < 8; m++) {
            if (temp & 0x01) LQ_OLED_DrawPoint(x, y, mode);
            else             LQ_OLED_DrawPoint(x, y, !mode);
            temp >>= 1;
            y++;
        }
        x++;
        if ((x - x0) == 14) { x = x0; y0 = y0 + 8; }
        y = y0;
    }
}

void LQ_OLED_ScrollDisplay(unsigned char num, unsigned char space, unsigned char mode)
{
    unsigned char i, n, t = 0, m = 0, r;
    while (1) {
        if (m == 0) {
            LQ_OLED_ShowChinese(128, 24, t, mode);
            t++;
        }
        if (t == num) {
            for (r = 0; r < 16 * space; r++) {
                for (i = 1; i < 144; i++)
                    for (n = 0; n < 8; n++)
                        OLED_GRAM[i - 1][n] = OLED_GRAM[i][n];
                LQ_OLED_Refresh();
            }
            t = 0;
        }
        m++;
        if (m == 16) { m = 0; }
        for (i = 1; i < 144; i++)
            for (n = 0; n < 8; n++)
                OLED_GRAM[i - 1][n] = OLED_GRAM[i][n];
        LQ_OLED_Refresh();
    }
}

void LQ_OLED_ShowPicture(unsigned char x, unsigned char y, unsigned char sizex, unsigned char sizey, unsigned char BMP[], unsigned char mode)
{
    unsigned int j = 0;
    unsigned char i, n, temp, m;
    unsigned char x0 = x, y0 = y;
    sizey = sizey / 8 + ((sizey % 8) ? 1 : 0);
    for (n = 0; n < sizey; n++) {
        for (i = 0; i < sizex; i++) {
            temp = BMP[j];
            j++;
            for (m = 0; m < 8; m++) {
                if (temp & 0x01) LQ_OLED_DrawPoint(x, y, mode);
                else             LQ_OLED_DrawPoint(x, y, !mode);
                temp >>= 1;
                y++;
            }
            x++;
            if ((x - x0) == sizex) { x = x0; y0 = y0 + 8; }
            y = y0;
        }
    }
}

/* ========================================================================
 * OLED 初始化 — SPI / I2C 双实现
 * ======================================================================== */
void LQ_OLED_Init(void)
{
#ifdef OLED_DRIVER_SPI
    /* ---- SPI 模式：初始化5个GPIO ---- */
    LQConfig_GPIO_InitTypeDef_t oled_gpio = {
        .Mode  = GPIO_MODE_OUTPUT_PP,
        .Pull  = GPIO_RESISTOR_NO_PULL,
        .Speed = GPIO_SPEED_HIGH,
    };
    LQ_GPIO_Init(OLED_SCL_PIN, &oled_gpio);
    LQ_GPIO_Init(OLED_SDA_PIN, &oled_gpio);
    LQ_GPIO_Init(OLED_CS_PIN,  &oled_gpio);
    LQ_GPIO_Init(OLED_RES_PIN, &oled_gpio);
    LQ_GPIO_Init(OLED_DC_PIN,  &oled_gpio);

    delay_ms(10);
    OLED_RES_Clr();
    delay_ms(50);
    OLED_RES_Set();

#elif defined(OLED_DRIVER_I2C)
    /* ---- I2C 模式：初始化 GPIO 模拟 I2C ---- */
    LQ_Soft_I2C_Init(&oled_i2c);
    /* I2C 模块 RES 内部上拉，等待上电稳定 */
    delay_ms(50);

#endif

    /* ---- 初始化命令序列（SPI / I2C 共用）---- */
    LQ_OLED_WR_Byte(0xAE, OLED_CMD); // display off
    LQ_OLED_WR_Byte(0x02, OLED_CMD); // set low column address
    LQ_OLED_WR_Byte(0x10, OLED_CMD); // set high column address
    LQ_OLED_WR_Byte(0x40, OLED_CMD); // set start line address
    LQ_OLED_WR_Byte(0x81, OLED_CMD); // set contrast control
    LQ_OLED_WR_Byte(0x80, OLED_CMD); // contrast value
    LQ_OLED_WR_Byte(0xA1, OLED_CMD); // segment remap
    LQ_OLED_WR_Byte(0xC8, OLED_CMD); // COM scan direction
    LQ_OLED_WR_Byte(0xA6, OLED_CMD); // normal display
    LQ_OLED_WR_Byte(0xA8, OLED_CMD); // set multiplex ratio
    LQ_OLED_WR_Byte(0x3F, OLED_CMD); // 1/64 duty
    LQ_OLED_WR_Byte(0xD3, OLED_CMD); // set display offset
    LQ_OLED_WR_Byte(0x00, OLED_CMD); // no offset
    LQ_OLED_WR_Byte(0xD5, OLED_CMD); // set display clock divide
    LQ_OLED_WR_Byte(0x80, OLED_CMD); // default ratio
    LQ_OLED_WR_Byte(0xD9, OLED_CMD); // set pre-charge period
    LQ_OLED_WR_Byte(0x1F, OLED_CMD); // SH1106 / SSD1306
    LQ_OLED_WR_Byte(0xDA, OLED_CMD); // set COM pins config
    LQ_OLED_WR_Byte(0x12, OLED_CMD);
    LQ_OLED_WR_Byte(0xDB, OLED_CMD); // set VCOMH
    LQ_OLED_WR_Byte(0x40, OLED_CMD); // VCOM deselect level
    LQ_OLED_WR_Byte(0xAD, OLED_CMD); // DC-DC control
    LQ_OLED_WR_Byte(0x8B, OLED_CMD); // enable internal DC-DC
    LQ_OLED_WR_Byte(0xA4, OLED_CMD); // display follows RAM
    LQ_OLED_WR_Byte(0xA6, OLED_CMD); // normal (not inverse)
    LQ_OLED_Clear();
    LQ_OLED_WR_Byte(0xAF, OLED_CMD); // display on
    LQ_OLED_Clear();
}
