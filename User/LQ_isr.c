/*******************************************************************************
 * @file                LQ_isr.c
 * @brief               ļ LQ_MSPM0GX_LIB Դļһ
 * @copyright           Ȩ (C) 2025-2026 Ƽ޹˾
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         Ƽ MSPM0G3507 İ
 *
 * :
 *   - ʹû : Keil5
 *   - ĿоƬ : MSPM0G3507
 *   - þ : 16.000MHz
 *   - ϵͳʱ : 80MHz
 *
 * ļѭGPL-3.0ԴЭ鷢ּΪ MSPM0G3507 оƬǶʽϵͳṩֿ MSPM0G3507 ӦóĲοʵ
 * ҵ;λʹãǰϵ߻Ȩ
 *
 * GPL-3.0 ֤ժҪ:
 * 1. ʹá޸ġַ
 * 2. ַ޸ĺİ汾ʱ֤ͬ
 * 3. 뱣ԭʼȨ֤Ϣ
 * 4. ṩκεʹ÷Ը
 * 5. ЭıμĿĿ¼ LICENSE ļ
 *
 * @author				wuwu	(ӿڲд)
 * @author              LQ_012	(ŻӦòд)
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026424
 *******************************************************************************/
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_exti.h"
#include "LQ_common.h"

#include "include.h"
#include "LQ_device.h"
#include "hardware.h"

/* UART0 жϷ */
void UART0_IRQHandler(void)
{
	// 接收中断（每收到一个字节触发一次）
	if (DL_UART_getPendingInterrupt(UART0) == DL_UART_IIDX_RX)
	{
		if (LQ_UART_IT_CallBack[LQ_UART0] != NULL)
		{
			LQ_UART_IT_CallBack[LQ_UART0]();
		}
	}
}

/* UART1 жϷ */
void UART1_IRQHandler(void)
{
	// ڽж
	if (DL_UART_getPendingInterrupt(UART1) == DL_UART_IIDX_RX)
	{
		if (LQ_UART_IT_CallBack[LQ_UART1] != NULL)
		{
			LQ_UART_IT_CallBack[LQ_UART1]();
		}
	}
}

/* UART2 жϷ */
void UART2_IRQHandler(void)
{
	// ڽж
	if (DL_UART_getPendingInterrupt(UART2) == DL_UART_IIDX_RX)
	{
		if (LQ_UART_IT_CallBack[LQ_UART2] != NULL)
		{
			LQ_UART_IT_CallBack[LQ_UART2]();
		}
	}
}

/* UART3 жϷ */
void UART3_IRQHandler(void)
{
	// ڽж
	if (DL_UART_getPendingInterrupt(UART3) == DL_UART_IIDX_RX)
	{
		if (LQ_UART_IT_CallBack[LQ_UART3] != NULL)
		{
			LQ_UART_IT_CallBack[LQ_UART3]();
		}
	}
}

/* TIMA0 жϷ */
void TIMA0_IRQHandler(void)
{
	// öʱѱռãҪʹøöʱ
	if (DL_TimerA_getPendingInterrupt(TIMA0) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERA_0] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERA_0]();
		}
	}
}

/* TIMA1 жϷ */
void TIMA1_IRQHandler(void)
{
    if (DL_TimerA_getPendingInterrupt(TIMA1) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERA_1] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERA_1]();
		}
	}
}

/* TIMG0 жϷ */
void TIMG0_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG0) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_0] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_0]();
		}
	}
}

/* TIMG6 жϷ */
void TIMG6_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG6) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_6] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_6]();
		}
	}
}

/* TIMG7 жϷ */
void TIMG7_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG7) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_7] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_7]();
		}
	}
}

/* TIMG8 жϷ */
void TIMG8_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG8) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_8] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_8]();
		}
	}
}

/* TIMG12 жϷ */
void TIMG12_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG12) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_12] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_12]();
		}
	}
}

/* 外部中断服务函数 */
void GROUP0_IRQHandler(void)
{
    LQ_Encoder_IRQHandlers();
}

void GROUP1_IRQHandler(void)
{
    LQ_Encoder_IRQHandlers();
}

/* ADC0 жϷ */
void ADC0_IRQHandler(void)
{
	//ѯADCж
	switch (DL_ADC12_getPendingInterrupt(ADC0))
	{	
		case DL_ADC12_IIDX_MEM0_RESULT_LOADED:		/* ͨ0ת */

			break;

		case DL_ADC12_IIDX_MEM1_RESULT_LOADED:		/* ͨ1ת */

			break;

		case DL_ADC12_IIDX_MEM2_RESULT_LOADED:		/* ͨ2ת */
		
			break;

		case DL_ADC12_IIDX_MEM3_RESULT_LOADED:		/* ͨ3ת */
		
			break;

		case DL_ADC12_IIDX_MEM4_RESULT_LOADED:		/* ͨ4ת */
		
			break;

		case DL_ADC12_IIDX_MEM5_RESULT_LOADED:		/* ͨ5ת */
		
			break;

		case DL_ADC12_IIDX_MEM6_RESULT_LOADED:		/* ͨ6ת */
		
			break;

		case DL_ADC12_IIDX_MEM7_RESULT_LOADED:		/* ͨ7ת */
		
			break;

		case DL_ADC12_IIDX_MEM8_RESULT_LOADED:		/* ͨ8ת */
			
			break;
		
		default:
			break;
	}
}

/* ADC1 жϷ */
void ADC1_IRQHandler(void)
{
	//ѯADCж
	switch (DL_ADC12_getPendingInterrupt(ADC1))
	{	
		case DL_ADC12_IIDX_MEM0_RESULT_LOADED:		/* ͨ0ת */
			
			break;

		case DL_ADC12_IIDX_MEM1_RESULT_LOADED:		/* ͨ1ת */
			
			break;

		case DL_ADC12_IIDX_MEM2_RESULT_LOADED:		/* ͨ2ת */
			
			break;

		case DL_ADC12_IIDX_MEM3_RESULT_LOADED:		/* ͨ3ת */
			
			break;

		case DL_ADC12_IIDX_MEM4_RESULT_LOADED:		/* ͨ4ת */
			
			break;

		case DL_ADC12_IIDX_MEM5_RESULT_LOADED:		/* ͨ5ת */
			
			break;

		case DL_ADC12_IIDX_MEM6_RESULT_LOADED:		/* ͨ6ת */
			
			break;

		case DL_ADC12_IIDX_MEM7_RESULT_LOADED:		/* ͨ7ת */
			
			break;

		case DL_ADC12_IIDX_MEM8_RESULT_LOADED:		/* ͨ8ת */
			
			break;
		
		default:
			break;
	}
}

/* DMA жϷ */
void DMA_IRQHandler(void)
{
    
}