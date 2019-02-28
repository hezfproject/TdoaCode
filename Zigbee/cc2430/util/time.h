/**************************************************************************************************
  Filename:       time.h
  Revised:        $Date: 2010/07/10 02:45:14 $
  Revision:       $Revision: 1.4 $
 **************************************************************************************************/

/*********************************************************************
 Statistic time: i.e: a segment code and so on. TO ENSURE THE TIMER YOU SELECT IS FREE.
 Choose Tick frequency/128 and free running mode. So the maxtime is: 
 2^16*128/32/1000 = 256 ms, which decide the most time span we can statistic.

 Update:
 Add global prescale support to get longer time span. The most global prescale is Tick frequency/128.
 So the the most time span is expanded to 256*128ms.
*********************************************************************/
 #ifndef TIME_H
#define TIME_H

#include "hal_timer.h"

/* Timer global clock pre-scaler definitions for HAL_TIMER_0, HAL_TIMER_2, HAL_TIMER_3 */
#define HAL_TIMER_GLOBAL_DIV1     0x00  /* No clock pre-scaling */
#define HAL_TIMER_GLOBAL_DIV2     0x08  /* Clock pre-scaled by 2 */
#define HAL_TIMER_GLOBAL_DIV4     0x10  /* Clock pre-scaled by 4 */
#define HAL_TIMER_GLOBAL_DIV8     0x18  /* Clock pre-scaled by 8 */
#define HAL_TIMER_GLOBAL_DIV16    0x20  /* Clock pre-scaled by 16 */
#define HAL_TIMER_GLOBAL_DIV32    0x28  /* Clock pre-scaled by 32 */
#define HAL_TIMER_GLOBAL_DIV64    0x30  /* Clock pre-scaled by 64 */
#define HAL_TIMER_GLOBAL_DIV128   0x38  /* Clock pre-scaled by 128 */
#define HAL_TIMER_GLOBAL_BITS     0x38  /* Bits 5:3 */

extern bool timeclock;

#define INIT_TIMER(timerid)  \
st (  \
	if (!timeclock) \
	{   \
		timeclock++; \
		HalTimerConfig(timerid, HAL_TIMER_MODE_NORMAL, \
				HAL_TIMER_CHANNEL_SINGLE, \
				HAL_TIMER_CH_MODE_OUTPUT_COMPARE, false, 0);\
		HalTimerStart(timerid, 0);\
	}  \
)
#define GET_TxCNTL(HWtimerid)   T##HWtimerid##CNTL
#define GET_TxCNTH(HWtimerid)  T##HWtimerid##CNTH 
#define RESET_TxCNT(HWtimerid)  st ( T##HWtimerid##CNTL = 0;)
#define GET_TxCNT(HWtimerid)  BUILD_UINT16(\
								GET_TxCNTL(HWtimerid), \
								GET_TxCNTH(HWtimerid)  \
								)
/*unit:ms.*/
#define CAL_TIME(HWtimerid) GET_TxCNT(HWtimerid);// /1000 measure in us

/* From HalTimer.c
 *          HAL_TIMER_0 --> HW Timer 3
 *          HAL_TIMER_2 --> HW Timer 4
 *          HAL_TIMER_3 --> HW Timer 1
 */
#define INIT_HWTIMER1() INIT_TIMER(3)
/*If a time span exceeds 256ms, need to use the follow init macro to expand the timespan.*/
#define INIT_HWTIMER1_PRESCALE_CLKCON(prescale) \
st ( \
	INIT_HWTIMER1();\
	CLKCON &= ~ HAL_TIMER_GLOBAL_BITS;\
	CLKCON |= prescale;   \
)
/*
#define INIT_HWTIMER1_PRESCALE_CLKCON_1() INIT_HWTIMER1_PRESCALE_CLKCON(1)
#define INIT_HWTIMER1_PRESCALE_CLKCON_2() INIT_HWTIMER1_PRESCALE_CLKCON(2)
#define INIT_HWTIMER1_PRESCALE_CLKCON_4() INIT_HWTIMER1_PRESCALE_CLKCON(4)
#define INIT_HWTIMER1_PRESCALE_CLKCON_8() INIT_HWTIMER1_PRESCALE_CLKCON(8)
#define INIT_HWTIMER1_PRESCALE_CLKCON_16() INIT_HWTIMER1_PRESCALE_CLKCON(16)
#define INIT_HWTIMER1_PRESCALE_CLKCON_32() INIT_HWTIMER1_PRESCALE_CLKCON(32)
#define INIT_HWTIMER1_PRESCALE_CLKCON_64() INIT_HWTIMER1_PRESCALE_CLKCON(64)
#define INIT_HWTIMER1_PRESCALE_CLKCON_128() INIT_HWTIMER1_PRESCALE_CLKCON(128)
*/
#define RESET_T1CNT() RESET_TxCNT(1)
#define CAL_HWTIMER1_TIME() CAL_TIME(1)
/*
#define CAL_HWTIMER1_PRESCALE_CLKCON_TIME(prescale) (CAL_TIME(1)*prescale)
#define CAL_HWTIMER1_PRESCALE_CLKCON_1_TIME() CAL_HWTIMER1_PRESCALE_CLKCON_TIME(1)
#define CAL_HWTIMER1_PRESCALE_CLKCON_2_TIME() CAL_HWTIMER1_PRESCALE_CLKCON_TIME(2)
#define CAL_HWTIMER1_PRESCALE_CLKCON_4_TIME() CAL_HWTIMER1_PRESCALE_CLKCON_TIME(4)
#define CAL_HWTIMER1_PRESCALE_CLKCON_8_TIME() CAL_HWTIMER1_PRESCALE_CLKCON_TIME(8)
#define CAL_HWTIMER1_PRESCALE_CLKCON_16_TIME() CAL_HWTIMER1_PRESCALE_CLKCON_TIME(16)
#define CAL_HWTIMER1_PRESCALE_CLKCON_32_TIME() CAL_HWTIMER1_PRESCALE_CLKCON_TIME(32)
#define CAL_HWTIMER1_PRESCALE_CLKCON_64_TIME() CAL_HWTIMER1_PRESCALE_CLKCON_TIME(64)
#define CAL_HWTIMER1_PRESCALE_CLKCON_128_TIME() CAL_HWTIMER1_PRESCALE_CLKCON_TIME(128)
*/
#endif

