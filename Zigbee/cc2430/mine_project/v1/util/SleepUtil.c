
#include "hal_types.h"
#include "hal_mcu.h"
#include "hal_key.h"
#include "hal_led.h"

#include "mac_api.h"
#include "Mac_radio_defs.h"
#include "SleepUtil.h"
 #include "ZComDef.h"
#include "OSAL_Timers.h"
#include "WatchdogUtil.h"
#include "OnBoard.h"

/* ------------------------------------------------------------------------------------------------
*                                      defines
* -----------------------------------------------------------------------------------------------*/

/* sleep and external interrupt port masks */
#define STIE_BV                             BV(5)
#define P0IE_BV                             BV(5)
#define P1IE_BV                             BV(4)
#define P2IE_BV                             BV(1)

/* sleep timer interrupt control */
#define HAL_SLEEP_TIMER_ENABLE_INT()        st(IEN0 |= STIE_BV;)     /* enable sleep timer interrupt */
#define HAL_SLEEP_TIMER_DISABLE_INT()       st(IEN0 &= ~STIE_BV;)    /* disable sleep timer interrupt */
#define HAL_SLEEP_TIMER_CLEAR_INT()         st(IRCON &= ~0x80;)      /* clear sleep interrupt flag */



/* for optimized indexing of uint32's */
#if HAL_MCU_LITTLE_ENDIAN()
#define UINT32_NDX0   0
#define UINT32_NDX1   1
#define UINT32_NDX2   2
#define UINT32_NDX3   3
#else
#define UINT32_NDX0   3
#define UINT32_NDX1   2
#define UINT32_NDX2   1
#define UINT32_NDX3   0
#endif
#define HAL_SLEEP_ADJ_TICKS   (9 + 35)/2

/* backup interrupt enable registers before sleep */
#define HAL_SLEEP_IE_BACKUP_AND_DISABLE(ien0, ien1, ien2) st(ien0  = IEN0;    /* backup IEN0 register */ \
	ien1  = IEN1;    /* backup IEN1 register */ \
	ien2  = IEN2;    /* backup IEN2 register */ \
	IEN0 &= STIE_BV; /* disable IEN0 except STIE */ \
	IEN1 &= P0IE_BV; /* disable IEN1 except P0IE */ \
	IEN2 &= (P1IE_BV|P2IE_BV);) /* disable IEN2 except P1IE, P2IE */

/* restore interrupt enable registers before sleep */
#define HAL_SLEEP_IE_RESTORE(ien0, ien1, ien2) st(IEN0 = ien0;   /* restore IEN0 register */ \
	IEN1 = ien1;   /* restore IEN1 register */ \
	IEN2 = ien2;)  /* restore IEN2 register */

#define HAL_KEY_SW_7_IEN      IEN2                    /* Interrupt Enable Register for SW7 */
#define HAL_KEY_SW_7_IENBIT   0x02            /* Interrupt Enable bit for SW7 */
#define HAL_KEY_SW_7_ICTL     PICTL                   /* Port Interrupt Control for SW7 */
#define HAL_KEY_SW_7_ICTLBIT  0x20             /* Interrupt enable bit for SW7 */


#define MAX_SLEEP_TIME                   510000             /* maximum time to sleep allowed by ST */
#define PM_MIN_SLEEP_TIME             50                 /* default to minimum safe sleep time for CC2430 Rev B */

/* ------------------------------------------------------------------------------------------------
*                                       Global vribals
* -----------------------------------------------------------------------------------------------*/

static uint32 halSleepTimerStart;


/* ------------------------------------------------------------------------------------------------
*                                      Functions 
* -----------------------------------------------------------------------------------------------*/
/* Functions */
static void UtilSleepSetTimerUs(uint32 timeout_us);

static void UtilSleep_adjust_timers( void );


//static  uint32 UtilTimerElapsed( void );
static uint32 UtilTimerElapsedUs( void );
inline void UtilSleepPutIntoSleep(uint8 sleep_mode, uint16 timeout_320USec);

/* Remember you should do MAC off and on yourself !!*/
void UtilSleep(uint8 sleep_mode, uint16 timeout_ms)
{

	uint32        timeout;
	/* get next OSAL timer expiration converted to 320 usec units */
	if(sleep_mode==CC2430_PM3 ||  (timeout_ms > PM_MIN_SLEEP_TIME && timeout_ms < MAX_SLEEP_TIME))
	{
		timeout = HAL_SLEEP_MS_TO_320US(timeout_ms);
		UtilSleepPutIntoSleep(sleep_mode,timeout);
	}
}

/* sleep with mac off  */
void UtilSleepWithMac(uint8 sleep_mode, uint16 timeout_ms)
{
	uint32        timeout;

	/* process key and led */
	//HalKeyEnterSleep();
	HalLedEnterSleep();
	
	/* turn off radio first */
	bool vFalse = false;
	bool rx_on_idle;
	
	MAC_MlmeGetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);	
	MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &vFalse);
	if(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP)!=MAC_SUCCESS)
	{
		MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);
		return;
	}
	

	/* get next OSAL timer expiration converted to 320 usec units */
	timeout = HAL_SLEEP_MS_TO_320US(timeout_ms);
	
#if 0
	if (timeout == 0)
	{
		timeout = MAC_PwrNextTimeout();
	}
	else
	{
		/* get next MAC timer expiration */
		macTimeout = MAC_PwrNextTimeout();

		/* get lesser of two timeouts */
		if ((macTimeout != 0) && (macTimeout < timeout))
		{
			timeout = macTimeout;  
		}
	}
#endif

	if(timeout > HAL_SLEEP_MS_TO_320US(PM_MIN_SLEEP_TIME) &&  timeout < HAL_SLEEP_MS_TO_320US(MAX_SLEEP_TIME))
	{
            UtilSleepPutIntoSleep(sleep_mode, timeout);
	}

	
	/*	open mac	*/
	MAC_PwrOnReq(); 
	MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);

	/* process key and led */
	//HalKeyExitSleep();
	HalLedExitSleep();
	
}


void UtilSleepUs(uint8 sleep_mode, uint32 timeout_us)
{

	//uint32        timeout;
	halIntState_t intState, ien0, ien1, ien2;


	/*if the sleep time is too small or too large */
	if(timeout_us < PM_MIN_SLEEP_TIME*1000UL || timeout_us > MAX_SLEEP_TIME*1000UL)
	{
		return;
	}

	HAL_ENTER_CRITICAL_SECTION(intState);
	{
		/* set sleep timer */
		if(sleep_mode == CC2430_PM2 ||sleep_mode == CC2430_PM1)
		{
			UtilSleepSetTimerUs(timeout_us);
			/* set up sleep timer interrupt */
			HAL_SLEEP_TIMER_CLEAR_INT();
			HAL_SLEEP_TIMER_ENABLE_INT();
		}

		HAL_SLEEP_IE_BACKUP_AND_DISABLE(ien0, ien1, ien2);

		SLEEP &= ~0x03;        /* clear mode bits */               
		SLEEP |= (sleep_mode & 0x03);   /* set mode bits   */               
		asm("NOP");                                          
		asm("NOP");                                          
		asm("NOP");              

		HAL_EXIT_CRITICAL_SECTION(intState);    
		if( SLEEP & 0x03 )    
		{
			PCON |= 0x01;  /* enable mode */                   
			asm("NOP");    /* first instruction after sleep*/  
		}

		HAL_ENTER_CRITICAL_SECTION(intState);

		/* disable sleep timer interrupt */
		HAL_SLEEP_TIMER_DISABLE_INT();
		HAL_SLEEP_IE_RESTORE(ien0, ien1, ien2);
		HAL_EXIT_CRITICAL_SECTION(intState);

	}

	UtilSleep_adjust_timers();

}


inline void UtilSleepPutIntoSleep(uint8 sleep_mode, uint16 timeout_320USec)
{
	halIntState_t intState, ien0, ien1, ien2;
	HAL_ENTER_CRITICAL_SECTION(intState);

	{

#if 1
		/* set main clock source to RC oscillator for Rev B and Rev D */
		SLEEP &= ~0x04;          /* turn on both oscs */     
		while(!(SLEEP & 0x20));  /* wait for RC osc */       
		asm("NOP");                                          
		CLKCON = (0x49 | OSC_32KHZ); /* select RC osc */     
		/* wait for requested settings to take effect */     
		while (CLKCON != (0x49 | OSC_32KHZ));                
		SLEEP |= 0x04;         /* turn off XOSC */
#endif
		/* set sleep timer */
		if(sleep_mode == CC2430_PM2 ||sleep_mode == CC2430_PM1)
		{
			UtilSleepSetTimer(timeout_320USec);
			/* set up sleep timer interrupt */
			HAL_SLEEP_TIMER_CLEAR_INT();
			//HAL_SLEEP_TIMER_DISABLE_INT();
			HAL_SLEEP_TIMER_ENABLE_INT();
		}

		HAL_SLEEP_IE_BACKUP_AND_DISABLE(ien0, ien1, ien2);

		/* Changed by rongtao, close the int in ISR and reopen it in when entering sleep */
		//HAL_KEY_SW_7_ICTL |= HAL_KEY_SW_7_ICTLBIT;        /* Set interrupt enable bit */
		//HAL_KEY_SW_7_IEN |= HAL_KEY_SW_7_IENBIT;

		//HAL_SLEEP_SET_POWER_MODE(halPwrMgtMode);
		SLEEP &= ~0x03;        /* clear mode bits */               
		SLEEP |= (sleep_mode & 0x03);   /* set mode bits   */               
		asm("NOP");                                          
		asm("NOP");                                          
		asm("NOP");              

		HAL_EXIT_CRITICAL_SECTION(intState);    
		if( SLEEP & 0x03 )    
		{
			PCON |= 0x01;  /* enable mode */                   
			asm("NOP");    /* first instruction after sleep*/  
		}

#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
		FeedWatchDog();
#endif
		HAL_ENTER_CRITICAL_SECTION(intState);

		/* disable sleep timer interrupt */
		HAL_SLEEP_TIMER_DISABLE_INT();
		HAL_SLEEP_IE_RESTORE(ien0, ien1, ien2);
#if 1 
		/* set main clock source to crystal for Rev B and Rev D only */
		//HAL_SLEEP_SET_MAIN_CLOCK_CRYSTAL();
		SLEEP &= ~0x04;          /* turn on both oscs */ 
		while(!(SLEEP & 0x40));  /* wait for XOSC */     
		asm("NOP");                                      
		//halSleepWait(63);        /* required for Rev B */
		CLKCON = (0x00 | OSC_32KHZ);   /* 32MHx XOSC */  
		while (CLKCON != (0x00 | OSC_32KHZ));            
		SLEEP |= 0x04;        /* turn off 16MHz RC */
#endif 
		HAL_EXIT_CRITICAL_SECTION(intState);

	}

	UtilSleep_adjust_timers();
}

/* set timer by 320 usec ticks */
void UtilSleepSetTimer(uint32 timeout)
{
	uint32 ticks;

	/* read the sleep timer; ST0 must be read first */
	((uint8 *) &ticks)[UINT32_NDX0] = ST0;
	((uint8 *) &ticks)[UINT32_NDX1] = ST1;
	((uint8 *) &ticks)[UINT32_NDX2] = ST2;
	((uint8 *) &ticks)[UINT32_NDX3] = 0;


	/* store value for later */
	halSleepTimerStart = ticks;

	/* Compute sleep timer compare value.  The ratio of 32 kHz ticks to 320 usec ticks
	* is 32768/3125 = 10.48576.  This is nearly 671/64 = 10.484375.
	*/
	ticks += (timeout * 671) / 64;

	/* subtract the processing time spent in function halSleep() */
	ticks -= HAL_SLEEP_ADJ_TICKS;

	/* CC2430 Rev. B bug:  compare value must not be set higher than 0xFFFF7F */
	if((ticks & 0xFFFFFF) > 0xFFFF7F)
	{
		ticks = 0x000001;
	}

	/* set sleep timer compare; ST0 must be written last */
	ST2 = ((uint8 *) &ticks)[UINT32_NDX2];
	ST1 = ((uint8 *) &ticks)[UINT32_NDX1];
	ST0 = ((uint8 *) &ticks)[UINT32_NDX0];

	/* Wait for ST0 jumped for a hop*/
	while(ST0 == ((uint8 *) &ticks)[UINT32_NDX0]);
}

void UtilSleepSetTimerUs(uint32 timeout_us)
{
	uint32 ticks;

	/* read the sleep timer; ST0 must be read first */
	((uint8 *) &ticks)[UINT32_NDX0] = ST0;
	((uint8 *) &ticks)[UINT32_NDX1] = ST1;
	((uint8 *) &ticks)[UINT32_NDX2] = ST2;
	((uint8 *) &ticks)[UINT32_NDX3] = 0;

	halSleepTimerStart = ticks;
	
	/* Compute sleep timer compare value.  The ratio of 32 kHz ticks to us is 32768/1000/1000 = 512/(125*125)

	*/
	ticks += (timeout_us * 512) / (125*125);

	/* subtract the processing time spent in function halSleep() */
	ticks -= HAL_SLEEP_ADJ_TICKS;

	/* CC2430 Rev. B bug:  compare value must not be set higher than 0xFFFF7F */
	if((ticks & 0xFFFFFF) > 0xFFFF7F)
	{
		ticks = 0x000001;
	}

	/* set sleep timer compare; ST0 must be written last */
	ST2 = ((uint8 *) &ticks)[UINT32_NDX2];
	ST1 = ((uint8 *) &ticks)[UINT32_NDX1];
	ST0 = ((uint8 *) &ticks)[UINT32_NDX0];

	/* Wait for ST0 jumped for a hop*/
	while(ST0 == ((uint8 *) &ticks)[UINT32_NDX0]);
}
 

uint32 UtilTimerElapsedUs( void )
{
	uint32 ticks;

	/* read the sleep timer; ST0 must be read first */
	((uint8 *) &ticks)[UINT32_NDX0] = ST0;
	((uint8 *) &ticks)[UINT32_NDX1] = ST1;
	((uint8 *) &ticks)[UINT32_NDX2] = ST2;

	/* set bit 24 to handle wraparound */
	((uint8 *) &ticks)[UINT32_NDX3] = 0x01;

	/* calculate elapsed time */
	ticks -= halSleepTimerStart;

	/* add back the processing time spent in function halSleep() */
	//ticks += HAL_SLEEP_ADJ_TICKS;

	/* mask off excess if no wraparound */
	ticks &= 0x00FFFFFF;

	/* Convert to us */
	return ticks*125UL*125UL/512UL;  
}

void UtilSleep_adjust_timers( void )
{
#if 0
	uint16 eTime;
	// Compute elapsed time (msec)
	eTime = (uint16)(UtilTimerElapsed() /  TICK_COUNT);
#endif
	uint16 eTime;
	static uint32 time_us;
	time_us += UtilTimerElapsedUs();
	eTime = time_us/1000;
	time_us %= 1000;
	
	if ( eTime )
		osalTimerUpdate( eTime );
}

HAL_ISR_FUNCTION(halSleepTimerIsr, ST_VECTOR)
{
    HAL_SLEEP_TIMER_CLEAR_INT();
}

