/**************************************************************************************************
Filename:       MPDep.c
Revised:        $Date: 2011/01/12 00:42:07 $
Revision:       $Revision: 1.9 $

Description:   Dependend interface for MP and so on.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "MPDep.h"

#include "WatchDogUtil.h"

#include "hal_mcu.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "mac_radio_defs.h"

#include "App_cfg.h"
#include "Delay.h"
#include "WatchDogUtil.h"
#include "hal_types.h"
#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE)) || (defined MINE_CONTROLLER)
#include "hal_audio.h"
#include "lcd_interface.h"
#endif
#include "SleepUtil.h"
/*************************************************************************************************
*CONSTANTS
*/
/*************************************************************************************************
*MACROS
*/
#define MAX_VDD_SAMPLES       3
#define VDD_LIMIT   HAL_ADC_VDD_LIMIT_3			/* 3.6v */

/*************************************************************************************************
*MACROS
*/
/* sleep and external interrupt port masks */
#define STIE_BV                             BV(5)
#define P0IE_BV                             BV(5)
#define P1IE_BV                             BV(4)
#define P2IE_BV                             BV(1)
#define IsNumberEqual(termNbr_t1, termNbr_t2) (osal_strcmp(termNbr_t1, termNbr_t2) == 0)

/*********************************************************************
* GLOBAL VARIABLES
*/

/*********************************************************************
* LOCAL VARIABLES
*/
static __idata bool MineApp_PowerOn = TRUE;

static inline bool MP_InTestLongPress(uint16 key,uint16 TimeOut);
static inline void MP_SleepSetTimer(uint32 timeout);

void MP_PowerOFF(void)
{    
	HAL_DISABLE_INTERRUPTS();
#if (defined WATCHDOG) &&(WATCHDOG == TRUE)
	FeedWatchDog();
#endif
	//StopAudio();
	AudioIntoSleep();
	MAC_RADIO_TURN_OFF_POWER();   
	BACKLIGHT_CLOSE();
	LCDIntoSleep();
	//KeyIntoSleep();
	MP_ShutDown();
}

void MP_ShutDown(void )
{
	MineApp_PowerOn = FALSE;
	volatile static __idata bool longpress = FALSE;
	volatile static __idata uint32 timeout_us;
	
	while(1)
	{
		HAL_DISABLE_INTERRUPTS();
		//InitialKey();
		KeyEnable();
        	WaitKeySleep(5000);
		IEN0 = STIE_BV; /* open STIE, disable all others */ 
		IEN1 = 0; /* open  P0IE, disable all others */ 
		IEN2 = (P1IE_BV); /* open P1IE, P2IE, disable all others*/
        	P1IEN |= 0x10;   // open KEY_SDA IE

#if 0
		/* set main clock source to RC oscillator for Rev B and Rev D */
		SLEEP &= ~0x04;          /* turn on both oscs */     
		while(!(SLEEP & 0x20));  /* wait for RC osc */       
		asm("NOP");                                          
		CLKCON = (0x49 | OSC_32KHZ); /* select RC osc */     
		/* wait for requested settings to take effect */     
		while (CLKCON != (0x49 | OSC_32KHZ));                
		SLEEP |= 0x04;         /* turn off XOSC */
#endif
		 timeout_us = HAL_SLEEP_MS_TO_320US(10000);
		MP_SleepSetTimer(timeout_us);
		
		SLEEP &= ~0x03;        /* clear mode bits */               
		SLEEP |= (0x02);   /* Enter PM2 sleep */               
		asm("NOP");                                          
		asm("NOP");                                          
		asm("NOP");              

		HAL_ENABLE_INTERRUPTS();   
		if( SLEEP & 0x03 )    
		{
			PCON |= 0x01;  /* enable mode */                   
			asm("NOP");    /* first instruction after sleep*/  
		}
#if(defined WATCHDOG) &&(WATCHDOG == TRUE)
		FeedWatchDog();
#endif
		HAL_DISABLE_INTERRUPTS();
#if 0
		/* set main clock source to crystal for Rev B and Rev D only */
		SLEEP &= ~0x04;          /* turn on both oscs */ 
		while(!(SLEEP & 0x40));  /* wait for XOSC */     
		asm("NOP");                                      
		CLKCON = (0x00 | OSC_32KHZ);   /* 32MHx XOSC */  
		while (CLKCON != (0x00 | OSC_32KHZ));            
		SLEEP |= 0x04;        /* turn off 16MHz RC */
#endif 
		longpress = MP_InTestLongPress(HAL_KEY_POWER,MINEAPP_POWER_LONGPRESS_TIMEOUT);  
		if(longpress)
		{
			STARTWATCHDOG(DOGTIMER_INTERVAL_2MS);
			DelayMs(100);
		}
	}

}

inline bool MP_InTestLongPress(uint16 key,uint16 TimeOut)
{
	 uint16 testInterval = 100;  
	 uint16 testnum = TimeOut/testInterval;
	for( uint16 i=0; i<testnum;i++)
	{
		 uint8 key_tmp = GetKey();

#if(defined WATCHDOG) &&(WATCHDOG == TRUE)
		FeedWatchDog();
#endif
		if(key_tmp != key)  
		{
			return false;
		}
		DelayMs(testInterval);
	}
	return true;
}
bool MP_TestLongPress(uint16 key,uint16 TimeOut)
{
	return MP_InTestLongPress(key,TimeOut);
}

bool MP_vdd_check( void )
{
#ifdef MP_VERSION_1_1
	return TRUE;
#else
	for ( uint8 i=0;i < MAX_VDD_SAMPLES;i++ )
	{
		if ( HalAdcPhoneCheckVdd (VDD_LIMIT) )
			return TRUE;
		DelayMs(10); 
	}
	return  FALSE;
#endif
}

void MP_LongDelay(uint16 timeout, uint8 cnt)
{
	for(uint8 i=0;i<cnt;i++)
	{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
		FeedWatchDog();
#endif
		DelayMs(timeout);
	}
}

bool MP_IsPowerOn(void)
{
	return MineApp_PowerOn;
}

void MP_SetPowerOn(bool powerOn)
{
	MineApp_PowerOn = powerOn;
}

inline void MP_SleepSetTimer(uint32 timeout)
{
	uint32 ticks;

	/* read the sleep timer; ST0 must be read first */
	((uint8 *) &ticks)[0] = ST0;
	((uint8 *) &ticks)[1] = ST1;
	((uint8 *) &ticks)[2] = ST2;
	((uint8 *) &ticks)[3] = 0;


	/* Compute sleep timer compare value.  The ratio of 32 kHz ticks to 320 usec ticks
	* is 32768/3125 = 10.48576.  This is nearly 671/64 = 10.484375.
	*/
	ticks += (timeout * 671) / 64;

	/* CC2430 Rev. B bug:  compare value must not be set higher than 0xFFFF7F */
	if((ticks & 0xFFFFFF) > 0xFFFF7F)
	{
		ticks = 0x000001;
	}

	/* set sleep timer compare; ST0 must be written last */
	ST2 = ((uint8 *) &ticks)[2];
	ST1 = ((uint8 *) &ticks)[1];
	ST0 = ((uint8 *) &ticks)[0];

	/* Wait for ST0 jumped for a hop*/
	while(ST0 == ((uint8 *) &ticks)[0]);
}

