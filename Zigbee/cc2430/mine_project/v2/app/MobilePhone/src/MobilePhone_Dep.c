/**************************************************************************************************
Filename:       MPDep.c
Revised:        $Date: 2011/01/29 00:35:46 $
Revision:       $Revision: 1.3 $

Description:   Dependend interface for MP and so on.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "MobilePhone_Dep.h"
#include "MobilePhone_global.h"

#include "WatchDogUtil.h"

#include "hal_mcu.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "key.h"
#include "mac_radio_defs.h"
#include "delay.h"

#include "App_cfg.h"
#include "Delay.h"
#include "WatchDogUtil.h"
#include "hal_types.h"
#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE))
#include "hal_audio.h"
#endif
#include "SleepUtil.h"
#include "lcd_serial.h"
#include "hal_adc.h"
/*************************************************************************************************
*CONSTANTS
*/
/*************************************************************************************************
*MACROS
*/

#define HAL_VCC40V  24496
#define HAL_VCC39V  23888
#define HAL_VCC38V  23328
#define HAL_VCC37V  22704
#define HAL_VCC36V  21984
#define HAL_VCC35V  21552
#define HAL_VCC34V  20868
#define HAL_VCC33V  20360

static __code const uint16 HalAdcVccTable[]=
{
	HAL_VCC36V>>4,  /* Shut Down VDD 3.6v */
	22888>>4,       /*  VDD Limit - 3.70v  */
	23268>>4,       /*  VDD Limit - 3.76v  */
	23576>>4,       /*  VDD Limit - 3.82v  */
	24348>>4,       /*  VDD Limit - 3.95v  */
};

/*************************************************************************************************
*MACROS
*/
/* sleep and external interrupt port masks */
#define STIE_BV                             BV(5)
#define P0IE_BV                             BV(5)
#define P1IE_BV                             BV(4)
#define P2IE_BV                             BV(1)

/*********************************************************************
* GLOBAL VARIABLES
*/

/*********************************************************************
* LOCAL VARIABLES
*/
static __idata bool MineApp_PowerOn = TRUE;

/*********************************************************************
* function definations 
*/
static void MP_ShutDown(void);
static inline bool MP_InTestLongPress(uint16 key,uint16 TimeOut);
static inline void MP_SleepSetTimer(uint32 timeout);
static int8  MP_AdcVdd2Level(uint16 vdd);

/*********************************************************************
* functions 
*/
void MP_PowerOFF(void)
{    
	HAL_DISABLE_INTERRUPTS();
#if (defined WATCHDOG) &&(WATCHDOG == TRUE)
	FeedWatchDog();
#endif

	/* set normal reset flag */
	StoreParam_t param = *(StoreParam_t*)MP_STOREPARAM_ADDR;
	param.abnormalRst = FALSE;
	*(StoreParam_t*)MP_STOREPARAM_ADDR = param;

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
	    DelayMs(testInterval);
        uint8 key_tmp = GetKey();

#if(defined WATCHDOG) &&(WATCHDOG == TRUE)
		FeedWatchDog();
#endif
		if(key_tmp != key)  
		{
			return false;
		}	
	}
	return true;
}
bool MP_TestLongPress(uint16 key,uint16 TimeOut)
{
	return MP_InTestLongPress(key,TimeOut);
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

bool MP_IsPowerOn(void)
{
	return MineApp_PowerOn;
}

void MP_SetPowerOn(bool powerOn)
{
	MineApp_PowerOn = powerOn;
}


/* -1: need power off */
/* 0-4  vdd level */
static int8  MP_AdcVdd2Level(uint16 vdd)
{
	int8 len = sizeof(HalAdcVccTable)/sizeof(HalAdcVccTable[0]);
	int8 i;

	for(i=0;i<len;i++)
	{
		if(vdd < HalAdcVccTable[i])
		{
			return i-1;
		}
	}
	return len-1;
}

int8 MP_CheckVddLevel(void)
{
	int8 batteryLevel;
	uint16 adcvalue;
	adcvalue	= HalAdcRead2(HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_12,HAL_ADC_REF_125V);
	batteryLevel = MP_AdcVdd2Level(adcvalue);
	return batteryLevel;
}
