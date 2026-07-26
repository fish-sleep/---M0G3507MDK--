/*******************************************************************************
 * @file                LQ_clock.h
 * @brief               基于龙邱 MSPM0GX_LIB 开源库修改
 * @copyright           原始版权 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         适配嘉立创天猛星 MSPM0G3507 开发板时钟源配置
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 高频时钟源 : 内部 SYSOSC
 *   - 低频时钟源 : 内部 LFOSC
 *   - 系统时钟 : 80MHz
 *
 * 本文件基于龙邱 GPL-3.0 开源协议代码修改。
 *
 * @author              fish_sleeping  (时钟源适配修改)
 * @author              wuwu          (龙邱接口层编写)
 * @author              LQ_012        (龙邱优化与应用层编写)
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#ifndef __LQ_CLOCK_H__
#define __LQ_CLOCK_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

/*========================== HFCLK 源选择 ==========================*/

/*!
 * @brief		时钟源选择：取消注释你需要的配置（仅保留一个）
 */
//#define LQ_CLK_SRC_HFXT_40MHZ			/* 外部 40MHz 晶振 */
//#define LQ_CLK_SRC_HFXT_16MHZ			/* 外部 16MHz 晶振 */
#define LQ_CLK_SRC_SYSOSC				/* 内部 SYSOSC（天猛星默认） */

#if (defined(LQ_CLK_SRC_HFXT_40MHZ) + defined(LQ_CLK_SRC_HFXT_16MHZ) + defined(LQ_CLK_SRC_SYSOSC)) != 1
#error "Select exactly one HFCLK source"
#endif

/*========================== LFCLK 源选择 ==========================*/

//#define LQ_CLK_SRC_LFXT_32KHZ      		/* 外部 32kHz 晶振 */
#define LQ_CLK_SRC_LFOSC_32KHZ    		/* 内部 32kHz 振荡器（默认） */

#if (defined(LQ_CLK_SRC_LFXT_32KHZ) + defined(LQ_CLK_SRC_LFOSC_32KHZ)) != 1
#error "Select exactly one LFCLK source"
#endif

/*=========================== 时钟配置宏 ===========================*/

/* 开启电源后等待稳定的时长 */
#define POWER_STARTUP_DELAY                           			( 16 )

/* 浮点数转整数比例系数(放大 1000 倍, 避免浮点运算) */
#define FLOAT_TO_INT_SCALE										( 1000U )

#ifdef LQ_CLK_SRC_HFXT_40MHZ
#define FCC_EXPECTED_RATIO										( 1000 )						/* FCC 预期频率比值 */
#endif

#ifdef LQ_CLK_SRC_HFXT_16MHZ
#define FCC_EXPECTED_RATIO										( 2500 )
#endif

#ifdef LQ_CLK_SRC_SYSOSC
#define FCC_EXPECTED_RATIO										( 1250 )
#endif

/* FCC频率比值上下限(允许 ±0.3% 误差) */
#define FCC_UPPER_BOUND											( FCC_EXPECTED_RATIO * (1 + 0.003) )
#define FCC_LOWER_BOUND											( FCC_EXPECTED_RATIO * (1 - 0.003) )

/* 主时钟频率 */
#define CPUCLK_FREQ                                           	( 80000000 )

/*  函数声明  */

void LQ_System_Init(void);				// 系统初始化

#endif
