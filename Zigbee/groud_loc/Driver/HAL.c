#include "HAL.h"
#include "KeyScan.h"
#include "OSAL.h"
#include "NtrxDrv.h"
#include "hal_uart.h"
#include "OSAL_PwrMgr.h"
#include <stdio.h>
#include "sleep.h"
//#include "Console.h"
//#include "CPU.h"
//#include "BoardConfig.h"
#include "LED.h"

uint8 Hal_TaskId;

void Hal_Init(uint8 taskid)
{
	Hal_TaskId = taskid;
	osal_pwrmgr_task_state(Hal_TaskId, PWRMGR_CONSERVE);
}

//处理按键输入
uint16 Hal_ProcessEvent(uint8 taskid, uint16 events)
{
	if (events & HAL_EVENT_KEY) {

#ifdef ENABLE_KEY
		/* Check for keys */
		HalKeyPoll();

		/* if interrupt disabled, do next polling */
		if (!Hal_KeyIntEnable) {
			osal_start_timerEx( Hal_TaskId, HAL_EVENT_KEY, 100);
		}
#endif // ENABLE_KEY

		return events ^ HAL_EVENT_KEY;
	}
	return 0;
}

void Hal_Poll(void)
{
	HalUARTPoll();
}

#ifdef ENABLE_DEBUG_SLEEP
extern uint32 sleeptime;
#endif

#define WARM_STARTUP_TIME	10
#define SLEEP_RESOLUTION	8  //7.8125ms
#define MAX_SLEEP_TIME		(24ul * 60 * 60 * 1000) //24hr

#ifndef ENABLE_POWERDOWN_FULL
extern NtrxDevAddr ThisDev;
void Hal_Sleep(uint32 ms)
{
 	uint32 start = 0;
	uint32 end = 0;
	uint32 elapsed;
	volatile uint32 time1, time2;
	uint32 ticks;

	NtrxBufferType rtc1[6], rtc2[6];

	if(ms < WARM_STARTUP_TIME + SLEEP_RESOLUTION){
		return;
	}
	ms -= WARM_STARTUP_TIME;

	if(ms >= MAX_SLEEP_TIME) ms = MAX_SLEEP_TIME;

	ticks = (uint32)((float)ms * 0.128 + 0.5);

	NtrxGetRTC(rtc1);

	NtrxPrepWakeupByDio();

	NtrxPowerdownPadMode( ticks ) ;	// 7.8125ms per tick

	//CpuEnterPowerSave() ;
	printf(" Sleep %u ms\n", ms);
    time1 = GetSysClock();

	// Hold off interrupts.
	CpuDisableInterrupt();
	LED1_On() ;
	LED2_On() ;
	Sleep(ms);
	//while (GetSysClock() - time1 < ms);

	LED1_Off() ;	  			//On和Off与AVR板上相反
	LED2_Off() ;
    // Re-enable interrupts.
    CpuEnableInterrupt();

	printf("exit Sleep\n");
    elapsed = ms - (time2 - time1);
    ElapsedTime(elapsed);

//	ElapsedTime(ms);	//补上时间和中断数

#if 0

	NtrxWakeupByDio();

	WatchdogReset() ;

	NtrxWarmInit() ;
#else
	NtrxWakeupByDio();

	WatchdogReset() ;

	NtrxInit(1) ;

#endif
    NtrxSetStaAddress( ThisDev ) ;
	NtrxGetRTC(rtc2);
	time2 = GetSysClock();

	start += (uint32)rtc1[3] << 24;
	start += (uint32)rtc1[2] << 16;
	start += (uint32)rtc1[1] << 8;
	start += (uint32)rtc1[0] ;
	end += (uint32)rtc2[3] << 24;
	end += (uint32)rtc2[2] << 16;
	end += (uint32)rtc2[1] << 8;
	end += (uint32)rtc2[0] ;

	//sysclock stop time
	//elapsed = (uint32)((float)(end - start) / 32.768) - (time2 - time1);
	//elapsed = (uint32)((float)(end - start) / 32.768);


	#ifdef ENABLE_DEBUG_SLEEP
	sleeptime += elapsed;
	#endif

#ifdef ENABLE_DEBUG_PRINT

	ms = (uint32)((float)(end - start) / 32.768);
	con_PutHexNum((uint8)(ms >> 24));
	con_PutHexNum((uint8)(ms >> 16));
	con_PutHexNum((uint8)(ms >> 8));
	con_PutHexNum((uint8)(ms >> 0));
	con_PutReturn();
	ms = (uint32)(time2 - time1);
	con_PutHexNum((uint8)(ms >> 24));
	con_PutHexNum((uint8)(ms >> 16));
	con_PutHexNum((uint8)(ms >> 8));
	con_PutHexNum((uint8)(ms >> 0));
	con_PutReturn();
#endif


}

#else
extern NtrxDevAddr ThisDev;
void Hal_Sleep(uint32 ms)
{
	uint32 ticks;
	uint32 time1, time2;
	uint32 elapsed;

	if(ms < WARM_STARTUP_TIME + SLEEP_RESOLUTION){
		return;
	}
	ms -= WARM_STARTUP_TIME;

	if(ms >= MAX_SLEEP_TIME) ms = MAX_SLEEP_TIME;

	ticks = (uint32)((float)ms * 0.128 + 0.5);

	NtrxPrepWakeupByDio();

	NtrxPowerdownMode( ticks ) ;	// 7.8125ms per tick

printf("Enter sleep\n");
	CpuEnterPowerSave() ;
printf("Exit sleep\n");

#ifdef ENABLE_DEBUG_PRINT
	time1 = GetSysClock();
#endif

	NtrxWakeupByDio();

	WatchdogReset() ;

	NtrxInit(1) ;

	NtrxSetStaAddress( ThisDev ) ;

#ifdef ENABLE_DEBUG_PRINT
	time2 = GetSysClock();
#endif

	//sysclock stop time
	elapsed = ms + WARM_STARTUP_TIME;
	ElapsedTime(elapsed);

	#ifdef ENABLE_DEBUG_SLEEP
	sleeptime += elapsed;
	#endif

#ifdef ENABLE_DEBUG_PRINT
	ms = (uint32)(time2 - time1);
	con_PutHexNum((uint8)(ms >> 24));
	con_PutHexNum((uint8)(ms >> 16));
	con_PutHexNum((uint8)(ms >> 8));
	con_PutHexNum((uint8)(ms >> 0));
	con_PutReturn();
#endif
}

#endif


#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART */
	//  USART_SendData(EVAL_COM2, (uint8_t) ch);
	//
	//  /* Loop until the end of transmission */
	//  while (USART_GetFlagStatus(EVAL_COM1, USART_FLAG_TC) == RESET)
	//  {}
	USART_SendData( USART1, (uint8_t) ch ) ;
	while( USART_GetFlagStatus( USART1, USART_FLAG_TC ) == RESET ) ;
	if (ch == '\n')
		USART_SendData( USART1, ('\r') );
	while( USART_GetFlagStatus( USART1, USART_FLAG_TC ) == RESET ) ;

	return ch;
}

