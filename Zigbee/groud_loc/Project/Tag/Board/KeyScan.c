/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/
//#ifdef __AVR__
//#include <avr/interrupt.h>
//#endif

#include "Config.h"
#include "KeyScan.h"
#include "HAL.h"
#include "OSAL.h"
#include "App.h"

#ifdef ENABLE_KEY

static uint8 KS_PrevCode = 0 ;
static uint8 KS_KeyCount = 0 ;

static uint8 halKeySavedKeys;     /* used to store previous key state in polling mode */
static halKeyCBack_t pHalKeyProcessFunction;
static uint8 HalKeyConfigured;
Bool Hal_KeyIntEnable;            /* interrupt enable/disable flag */

void halProcessKeyInterrupt(void);

void KeyScanProcess( void )
{
	uint8 r ;
	r = GetKeyStatus() ;
	if( r == 0 )
	{
		KS_PrevCode = 0 ;
		KS_KeyCount = 0 ;
	}
	else if( r != KS_PrevCode )
	{
		KS_PrevCode = r ;
		KS_KeyCount = 1 ;
	}
	else
	{
		KS_KeyCount ++ ;
		if( KS_KeyCount == 100/BASE_TIME_TICK )			// 至少稳定100ms才认为按键有效
		{
			KeyCommand = r ;
			KeyStatus = KeyPress ;
		}
		else if( KS_KeyCount >= 500/BASE_TIME_TICK )	// 至少按键500ms后才是重复键
		{
			KeyStatus |= KeyRepeat ;
			KS_KeyCount = (500-100)/BASE_TIME_TICK ;	// 重复键间隔100ms
		}
	}
}

void InitKeyScan( void )
{
	HalKeyInit();
}

//=============================================================================


#define HAL_KEY_DEBOUNCE_VALUE  25
#define HAL_KEY_POLLING_VALUE   100

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void halProcessKeyInterrupt(void);

void HalKeyInit( void )
{
  /* Initialize previous key to 0 */
  halKeySavedKeys = 0;

  /* Initialize callback function */
  pHalKeyProcessFunction  = 0;

  /* Start with key is not configured */
  HalKeyConfigured = FALSE;
}


void HalKeyConfig (Bool interruptEnable, halKeyCBack_t cback)
{
  	/* Enable/Disable Interrupt or */
  	Hal_KeyIntEnable = interruptEnable;

  	/* Register the callback fucntion */
  	pHalKeyProcessFunction = cback;

  	/* Determine if interrupt is enable or not */
	if (Hal_KeyIntEnable) {
    	/* Rising/Falling edge configuratinn */
//#ifdef __AVR__
//		PCICR &= ~(1<<PCIE0); //disable interrupt
//		PCMSK0 = (1<<PCINT2) | (1<<PCINT3) | (1<<PCINT4) | (1<<PCINT5) | (1<<PCINT6);
//		PCIFR |= (1<<PCIF0);	//clear flag
//		PCICR |= (1<<PCIE0);	//enable interrupt
//#endif

//#ifdef __TARGET_CPU_CORTEX_M3
		{/* Falling edge */
			EXTI_InitTypeDef EXTI_InitStructure ;

			GPIO_EXTILineConfig( KEY_SW1_PORTSRC, KEY_SW1_PINSRC ) ;
			GPIO_EXTILineConfig( KEY_SW2_PORTSRC, KEY_SW2_PINSRC ) ;
			GPIO_EXTILineConfig( KEY_SW3_PORTSRC, KEY_SW3_PINSRC ) ;
			GPIO_EXTILineConfig( KEY_SW4_PORTSRC, KEY_SW4_PINSRC ) ;
			GPIO_EXTILineConfig( KEY_SW5_PORTSRC, KEY_SW5_PINSRC ) ;
			EXTI_InitStructure.EXTI_Line = KEY_SW1_EXTI_LINE | KEY_SW2_EXTI_LINE | KEY_SW3_EXTI_LINE | KEY_SW4_EXTI_LINE | KEY_SW5_EXTI_LINE;
			EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt ;
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling ;
			EXTI_InitStructure.EXTI_LineCmd = ENABLE ;
			EXTI_Init( &EXTI_InitStructure ) ;
		}
//#endif

    	/* Do this only after the hal_key is configured - to work with sleep stuff */
    	if (HalKeyConfigured == TRUE) {
			osal_stop_timerEx( Hal_TaskId, HAL_EVENT_KEY);  /* Cancel polling if active */
		}
	} else {
		/* Interrupts NOT enabled */
//#ifdef __AVR__
//		PCICR &= ~(1<<PCIE0); //disable interrupt
//#endif
//#ifdef __TARGET_CPU_CORTEX_M3
		{
			EXTI_InitTypeDef EXTI_InitStructure ;

			EXTI_InitStructure.EXTI_Line = KEY_SW1_EXTI_LINE | KEY_SW2_EXTI_LINE | KEY_SW3_EXTI_LINE | KEY_SW4_EXTI_LINE | KEY_SW5_EXTI_LINE;
			EXTI_InitStructure.EXTI_LineCmd = DISABLE ;
			EXTI_Init( &EXTI_InitStructure ) ;
		}
//#endif
		osal_start_timerEx (Hal_TaskId, HAL_EVENT_KEY, HAL_KEY_POLLING_VALUE);    /* Kick off polling */
	}

	/* Key now is configured */
	HalKeyConfigured = TRUE;
}


uint8 HalKeyRead ( void )
{
	return GetKeyStatus() ;
}


void HalKeyPoll (void)
{
	uint8 keys = GetKeyStatus();

	/* If interrupts are not enabled, previous key status and current key status
	 * are compared to find out if a key has changed status.
	 */
	if (!Hal_KeyIntEnable) {
		if (keys == halKeySavedKeys) {
			/* Exit - since no keys have changed */
			return;
		}
		/* Store the current keys for comparation next time */
		halKeySavedKeys = keys;
	} else {
		/* Key interrupt handled here */
	}

	/* Invoke Callback if new keys were depressed */
	if (keys && (pHalKeyProcessFunction)) {
		(pHalKeyProcessFunction) (keys, 0);
	}
}


void halProcessKeyInterrupt (void)
{
	osal_start_timerEx (Hal_TaskId, HAL_EVENT_KEY, HAL_KEY_DEBOUNCE_VALUE);
}

void HalKeyEnterSleep ( void )
{
}

uint8 HalKeyExitSleep ( void )
{
	/* Wake up and read keys */
	return ( HalKeyRead () );
}

//#ifdef __AVR__
//SIGNAL( SIG_PIN_CHANGE0 )
//{
//	halProcessKeyInterrupt();
//}
//#endif

//#ifdef __TARGET_CPU_CORTEX_M3
void Key_NVIC_Configuration( void )
{
	NVIC_InitTypeDef NVIC_InitStructure ;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1 ;
	NVIC_Init( &NVIC_InitStructure ) ;
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn ;
	NVIC_Init( &NVIC_InitStructure ) ;
}

static void EXTI15_5_IRQHandler(void)
{
	EXTI_ClearITPendingBit(KEY_SW1_EXTI_LINE | KEY_SW2_EXTI_LINE | KEY_SW3_EXTI_LINE | KEY_SW4_EXTI_LINE | KEY_SW5_EXTI_LINE);
	halProcessKeyInterrupt();
}

void EXTI9_5_IRQHandler(void)
{
	EXTI15_5_IRQHandler();
}

void EXTI15_10_IRQHandler(void)
{
	EXTI15_5_IRQHandler();
}
//#endif


#else

void InitKeyScan( void ) {}

void HalKeyInit(void){}
void HalKeyConfig(Bool interruptEnable, halKeyCBack_t cback){}
uint8 HalKeyRead(void){ return 0;}
void HalKeyPoll(void){}

#endif /* ENABLE_KEY */
//=============================================================================
