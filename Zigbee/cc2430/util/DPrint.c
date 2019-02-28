/**************************************************************************************************
Filename:       StringUtil.c
Revised:        $Date: 2010/08/02 19:53:52 $
Revision:       $Revision: 1.2 $

Description:    String utils. 
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "DPrint.h"
#include "Hal_uart.h"
#include "OnBoard.h"
//#include "ChargeED.h"
/*********************************************************************
* INCLUDES
*/
static uint32 med_test_time[100];
static uint8  med_test_cnt = 0;

static uint32 GetTimeInterval(void);
/*********************************************************************
* INCLUDES
*/
void DPrint_init(void)
{
#undef HAL_UART
#define  HAL_UART	TRUE

	HalUARTInit();

	halUARTCfg_t uartConfig;
	uartConfig.configured           = TRUE;              // 2430 don't care.
	uartConfig.baudRate             = HAL_UART_BR_115200;
	uartConfig.flowControl          = FALSE;
	uartConfig.flowControlThreshold = 48;
	uartConfig.rx.maxBufSize        = 255;
	uartConfig.tx.maxBufSize        = 255;
	uartConfig.idleTimeout          = 6;   // 2430 don't care.
	uartConfig.intEnable            = TRUE;              // 2430 don't care.
	uartConfig.callBackFunc         = NULL;

	HalUARTOpen (HAL_UART_PORT_0, &uartConfig);
}

void DPrint(char* p)
{
	HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, osal_strlen(p));
}

uint32  DPrint_TimeUs(void)
{
	static uint32 time;
	
	time += GetTimeInterval();

	char p[16];
	_ltoa(time, (unsigned char *)p, 10);

	HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, osal_strlen(p));
	p[0] = 0x0D;
	p[1] = 0x0A;
	p[2] = 0x00;
	HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, osal_strlen(p));
	return time;
}
uint32  DPrint_TimeMs(void)
{
	static uint32 time_ms;
	static uint32 time_us;

	time_us += GetTimeInterval();
	
	time_ms += time_us/1000;
	time_us = time_us%1000;
	
	char p[16];
	_ltoa(time_ms, (unsigned char *)p, 10);

	HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, osal_strlen(p));
	p[0] = 0x0D;
	p[1] = 0x0A;
	p[2] = 0x00;
	HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, osal_strlen(p));
	return time_ms;
}
uint32  DPrint_OsalTimeMs(void)
{
	uint32 time_ms = osal_GetSystemClock();
	
	char p[16];
	_ltoa(time_ms, (unsigned char *)p, 10);

	HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, osal_strlen(p));
	p[0] = 0x0D;
	p[1] = 0x0A;
	p[2] = 0x00;
	HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, osal_strlen(p));
	return time_ms;
}

void DPrint_TimeInterVal(void)
{	

	if(med_test_cnt<99)
	{
		med_test_cnt++;
	}
	else
	{
		med_test_cnt = 0;
	}

	med_test_time[med_test_cnt] = GetTimeInterval();

	char p[16];
	_ltoa(med_test_time[med_test_cnt], (unsigned char *)p, 10);
	//_itoa(med_test_time[med_test_cnt-1], p, 10);		
	HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, osal_strlen(p));
	p[0] = 0x0D;
	p[1] = 0x0A;
	p[2] = 0x00;
	HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, osal_strlen(p));
}
uint32 GetTimeInterval()
{
	uint32 ticks,tick_delta;
	static uint32 ticks_old; 
	/* read the sleep timer; ST0 must be read first */
	((uint8 *) &ticks)[0] = ST0;
	((uint8 *) &ticks)[1] = ST1;
	((uint8 *) &ticks)[2] = ST2;
	((uint8 *) &ticks)[3] = 0;

	if(ticks_old == 0)
	{
		tick_delta = 0;
	}

	else
	{
		if(ticks > ticks_old)
		{
			tick_delta = ticks - ticks_old;
		}
		else
		{
			tick_delta = ticks + 0x01000000 -ticks_old;
		}
	}
	
	uint32 time=0;
	time = tick_delta*125UL*125UL/512UL;  //unit us;
	ticks_old = ticks;
	return time;
}
