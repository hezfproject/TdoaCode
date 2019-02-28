/**************************************************************************************************
Filename:       hal_key.c
Revised:        $Date: 2011/04/21 23:22:11 $
Revision:       $Revision: 1.1 $
Description:    This file contains the interface to the HAL KEY Service.
Key driver for RRN, there is only one key HAL_KEY_SW_7 enabled

**************************************************************************************************/

/*********************************************************************
NOTE: If polling is used, the hal_driver task schedules the KeyRead()
to occur every 100ms.  This should be long enough to naturally
debounce the keys.  The KeyRead() function remembers the key
state of the previous poll and will only return a non-zero
value if the key state changes.

NOTE: If interrupts are used, the KeyRead() function is scheduled
25ms after the interrupt occurs by the ISR.  This delay is used
for key debouncing.  The ISR disables any further Key interrupt
until KeyRead() is executed.  KeyRead() will re-enable Key
interrupts after executing.  Unlike polling, when interrupts
are enabled, the previous key state is not remembered.  This
means that KeyRead() will return the current state of the keys
(not a change in state of the keys).

NOTE: If interrupts are used, the KeyRead() fucntion is scheduled by
the ISR.  Therefore, the joystick movements will only be detected
during a pushbutton interrupt caused by S1 or the center joystick
pushbutton.

NOTE: When a switch like S1 is pushed, the S1 signal goes from a normally
high state to a low state.  This transition is typically clean.  The
duration of the low state is around 200ms.  When the signal returns
to the high state, there is a high likelihood of signal bounce, which
causes a unwanted interrupts.  Normally, we would set the interrupt
edge to falling edge to generate an interrupt when S1 is pushed, but
because of the signal bounce, it is better to set the edge to rising
edge to generate an interrupt when S1 is released.  The debounce logic
can then filter out the signal bounce.  The result is that we typically
get only 1 interrupt per button push.  This mechanism is not totally
foolproof because occasionally, signal bound occurs during the falling
edge as well.  A similar mechanism is used to handle the joystick
pushbutton on the DB.  For the EB, we do not have independent control
of the interrupt edge for the S1 and center joystick pushbutton.  As
a result, only one or the other pushbuttons work reasonably well with
interrupts.  The default is the make the S1 switch on the EB work more
reliably.
*********************************************************************/

/**************************************************************************************************
*                                            INCLUDES
**************************************************************************************************/
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "osal.h"
#include "hal_key_cfg.h"
/**************************************************************************************************
*                                              MACROS
**************************************************************************************************/

/**************************************************************************************************
*                                            CONSTANTS
**************************************************************************************************/
#define HAL_KEY_BIT0   0x01
#define HAL_KEY_BIT1   0x02
#define HAL_KEY_BIT2   0x04
#define HAL_KEY_BIT3   0x08
#define HAL_KEY_BIT4   0x10
#define HAL_KEY_BIT5   0x20
#define HAL_KEY_BIT6   0x40
#define HAL_KEY_BIT7   0x80
#define HAL_KEY_RISING_EDGE   0
#define HAL_KEY_FALLING_EDGE  1

#define HAL_KEY_PDUP2           0x80
#define HAL_KEY_PDUP1           0x40
#define HAL_KEY_PDUP0           0x20

#define HAL_KEY_DEBOUNCE_VALUE  25
#define HAL_KEY_POLLING_VALUE   100

#if defined (HAL_BOARD_YIRIRRN) || defined(HAL_BOARD_YIRIBFN)
#define HAL_KEY_SW_7_ENABLE   /* YIRRN or BFN key - SW7 is enabled based on key interrupt enable or not - see config */
#define HAL_KEY_SW_7_PORT     P2                      /* Port location of SW7 */
#define HAL_KEY_SW_7_BIT      HAL_KEY_BIT0            /* Bit location of SW7 */
#define HAL_KEY_SW_7_SEL      P2SEL                   /* Port Select Register for SW7 */
#define HAL_KEY_SW_7_DIR      P2DIR                   /* Port Direction Register for SW7 */
#define HAL_KEY_SW_7_INP      P2INP                   /* Port Input Mode Register for SW7 */
#define HAL_KEY_SW_7_IEN      IEN2                    /* Interrupt Enable Register for SW7 */
#define HAL_KEY_SW_7_IENBIT   HAL_KEY_BIT1            /* Interrupt Enable bit for SW7 */
#define HAL_KEY_SW_7_EDGE     HAL_KEY_FALLING_EDGE    /* Type of interrupt for SW7 */
#define HAL_KEY_SW_7_EDGEBIT  HAL_KEY_BIT2            /* EdgeType enable bit SW7 */
#define HAL_KEY_SW_7_ICTL     PICTL                   /* Port Interrupt Control for SW7 */
#define HAL_KEY_SW_7_ICTLBIT  HAL_KEY_BIT5            /* Interrupt enable bit for SW7 */
#define HAL_KEY_SW_7_PXIFG    P2IFG                   /* Port Interrupt Flag for SW7 */

#define HAL_KEY_P0INT_LOW_USED    0    /* smooth compiling error*/
#define HAL_KEY_POINT_HIGH_USED   0                   /* P0 can only be enabled/disabled as group of high or low nibble */
#endif

/**************************************************************************************************
*                                            TYPEDEFS
**************************************************************************************************/

/**************************************************************************************************
*                                        GLOBAL VARIABLES
**************************************************************************************************/

//static uint16 halKeySavedKeys;     /* used to store previous key state in polling mode */
static halKeyCBack_t pHalKeyProcessFunction;
bool Hal_KeyIntEnable;            /* interrupt enable/disable flag */
uint16 halSaveIntKey;              /* used by ISR to save state of interrupt-driven keys */

//static uint8 HalKeyConfigured;



/**************************************************************************************************
*                                        FUNCTIONS - Local
**************************************************************************************************/
void halProcessKeyInterrupt (void);

/**************************************************************************************************
*                                        FUNCTIONS - API
**************************************************************************************************/
/**************************************************************************************************
* @fn      HalKeyInit
*
* @brief   Initilize Key Service
*
* @param   none
*
* @return  None
**************************************************************************************************/
void HalKeyInit( void )
{
#if (HAL_KEY == TRUE)

	/* Initialize previous key to 0 */
	halKeySavedKeys = 0;

#if defined (HAL_KEY_SW_6_ENABLE)
	HAL_KEY_SW_6_SEL &= ~(HAL_KEY_SW_6_BIT);    /* Set pin function to GPIO */
	HAL_KEY_SW_6_DIR &= ~(HAL_KEY_SW_6_BIT);    /* Set pin direction to Input */
	//HAL_KEY_SW_6_INP |= (HAL_KEY_SW_6_BIT);     /* set to tri-state */
	HAL_KEY_SW_6_INP &= ~(HAL_KEY_SW_6_BIT); 	/* set to poll up-down */
#endif
#if defined (HAL_KEY_SW_7_ENABLE)
	HAL_KEY_SW_7_SEL &= ~(HAL_KEY_SW_7_BIT);    /* Set pin function to GPIO */
	HAL_KEY_SW_7_DIR &= ~(HAL_KEY_SW_7_BIT);    /* Set pin direction to Input */
  //HAL_KEY_SW_7_INP |= HAL_KEY_SW_7_BIT;       /* Set pin input mode to tri-state */
    HAL_KEY_SW_7_INP &= ~(HAL_KEY_SW_7_BIT);		/* set to poll up-down */
#endif

/* if set to poll up-down, select as poll up*/
#if defined (HAL_BOARD_YIRIRRN)
	P2INP &= ~(0x01<<7);         /* set port 2 to poll up*/
#endif 
	/* Initialize callback function */
	pHalKeyProcessFunction  = NULL;

	/* Start with key is not configured */
	HalKeyConfigured = FALSE;
#endif /* HAL_KEY */
}

/**************************************************************************************************
* @fn      HalKeyConfig
*
* @brief   Configure the Key serivce
*
* @param   interruptEnable - TRUE/FALSE, enable/disable interrupt
*          cback - pointer to the CallBack function
*
* @return  None
**************************************************************************************************/
void HalKeyConfig (bool interruptEnable, halKeyCBack_t cback)
{
#if (HAL_KEY == TRUE)
	/* Enable/Disable Interrupt or */
	Hal_KeyIntEnable = interruptEnable;
	/* Register the callback fucntion */
	pHalKeyProcessFunction = cback;
	/* Determine if interrupt is enable or not */
	if (Hal_KeyIntEnable)
	{
		/*
		Work around for CC2430DB when interrupt is enabled and SW5 (center joystick)
		is used. This SW5 uses P2 which also has debug lines connected to it. This
		causes contant interruption on P2INT_VECTOR. Disable the usage of P2 interrupt
		will stop this problem.
		*/

#if defined (HAL_KEY_SW_6_ENABLE)
		PICTL &= ~(HAL_KEY_SW_6_EDGEBIT);                 /* Set rising or falling edge */
#if (HAL_KEY_SW_6_EDGE == HAL_KEY_FALLING_EDGE)
		PICTL |= HAL_KEY_SW_6_EDGEBIT;
#endif
		HAL_KEY_SW_6_ICTL |= HAL_KEY_SW_6_ICTLBIT;        /* Set interrupt enable bit */
		HAL_KEY_SW_6_IEN |= HAL_KEY_SW_6_IENBIT;
		HAL_KEY_SW_6_PXIFG &= ~(HAL_KEY_SW_6_BIT);        /* Clear any pending interrupts */
#endif
#if defined (HAL_KEY_SW_7_ENABLE)
		PICTL &= ~(HAL_KEY_SW_7_EDGEBIT);                 /* Set rising or falling edge */
#if (HAL_KEY_SW_7_EDGE == HAL_KEY_FALLING_EDGE)
		PICTL |= HAL_KEY_SW_7_EDGEBIT;
#endif
		HAL_KEY_SW_7_ICTL |= HAL_KEY_SW_7_ICTLBIT;        /* Set interrupt enable bit */
		HAL_KEY_SW_7_IEN |= HAL_KEY_SW_7_IENBIT;
		HAL_KEY_SW_7_PXIFG &= ~(HAL_KEY_SW_7_BIT);        /* Clear any pending interrupts */
#endif
		P2IFG = 0x00;      /* Clear all port 2 interrupts, It must be later than set HAL_KEY_SW_7_EDGE!!!!!*/

		/* Do this only after the hal_key is configured - to work with sleep stuff */
		if (HalKeyConfigured == TRUE)
		{
			osal_stop_timerEx( Hal_TaskID, HAL_KEY_EVENT);  /* Cancel polling if active */
		}
	}
	else    /* Interrupts NOT enabled */
	{
#if defined (HAL_KEY_SW_7_ENABLE)
		HAL_KEY_SW_7_ICTL &= ~(HAL_KEY_SW_7_ICTLBIT);     /* Clear interrupt enable bit */
		HAL_KEY_SW_7_IEN &= ~(HAL_KEY_SW_7_IENBIT);
#endif
#if defined (HAL_KEY_SW_6_ENABLE)
		HAL_KEY_SW_6_ICTL &= ~(HAL_KEY_SW_6_ICTLBIT);     /* Clear interrupt enable bit */
		HAL_KEY_SW_6_IEN &= ~(HAL_KEY_SW_6_IENBIT);
#endif

		osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_POLLING_VALUE);    /* Kick off polling */
	}

	/* Key now is configured */
	HalKeyConfigured = TRUE;
#endif /* HAL_KEY */
}

/**************************************************************************************************
* @fn      HalKeyRead
*
* @brief   Read the current value of a key
*
* @param   None
*
* @return  keys - current keys status
**************************************************************************************************/
uint8 HalKeyRead ( void )
{

	uint16 keys = 0;
#if (HAL_KEY == TRUE)
#if defined (HAL_KEY_SW_6_ENABLE)
	if (!(HAL_KEY_SW_6_PORT & HAL_KEY_SW_6_BIT))    /* Key is active low */
	{
		keys |= HAL_KEY_SW_6;
	}
#endif
#if defined (HAL_KEY_SW_7_ENABLE)
	if (!(HAL_KEY_SW_7_PORT & HAL_KEY_SW_7_BIT))       /* Key is active low */
	{
		keys |= HAL_KEY_SW_7;
	}
#endif


#endif /* HAL_KEY */

	return keys;

}


/**************************************************************************************************
*@fn      HalKeyPoll
*
* @brief   Called by hal_driver to poll the keys
*
* @param   None
*
* @return  None
**************************************************************************************************/
void HalKeyPoll (void)
{
#if (HAL_KEY == TRUE)
	uint16 keys = 0;
#if defined (HAL_KEY_SW_6_ENABLE)
	if (!(HAL_KEY_SW_6_PORT & HAL_KEY_SW_6_BIT))    /* Key is active low */
	{
		keys |= HAL_KEY_SW_6;
	}
#endif
#if defined (HAL_KEY_SW_7_ENABLE)
	if (!(HAL_KEY_SW_7_PORT & HAL_KEY_SW_7_BIT))       /* Key is active low */
	{
		keys |= HAL_KEY_SW_7;
	}
#endif

	/* Exit if polling and no keys have changed */
	if (!Hal_KeyIntEnable)
	{
		if (keys == halKeySavedKeys)
		{
			return;
		}
		halKeySavedKeys = keys;     /* Store the current keys for comparation next time */
	}

	/* Invoke Callback if new keys were depressed */
	if (keys && (pHalKeyProcessFunction))
	{
		(pHalKeyProcessFunction) (keys, HAL_KEY_STATE_NORMAL);
	}
#endif /* HAL_KEY */
}

/**************************************************************************************************
* @fn      halProcessKeyInterrupt
*
* @brief   Checks to see if it's a valid key interrupt, saves interrupt driven key states for
*          processing by HalKeyRead(), and debounces keys by scheduling HalKeyRead() 25ms later.
*
* @param
*
* @return
**************************************************************************************************/
void halProcessKeyInterrupt (void)
{
#if (HAL_KEY == TRUE)
	bool valid = FALSE;
#if defined (HAL_KEY_SW_6_ENABLE)
	if (HAL_KEY_SW_6_PXIFG & HAL_KEY_SW_6_BIT)      /* Interrupt Flag has been set */
	{
		HAL_KEY_SW_6_PXIFG &= ~(HAL_KEY_SW_6_BIT);    /* Clear Interrupt Flag */
		valid = TRUE;
	}
#endif
#if defined (HAL_KEY_SW_7_ENABLE)
	if (HAL_KEY_SW_7_PXIFG & HAL_KEY_SW_7_BIT)      /* Interrupt Flag has been set */
	{
              HAL_KEY_SW_7_PXIFG &= ~(HAL_KEY_SW_7_BIT);    /* Clear Interrupt Flag */
		valid = TRUE;
	}
#endif

	if (valid)
	{
		osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_DEBOUNCE_VALUE);
	}
#endif /* HAL_KEY */
}

/**************************************************************************************************
* @fn      HalKeyEnterSleep
*
* @brief  - Get called to enter sleep mode
*
* @param
*
* @return
**************************************************************************************************/
void HalKeyEnterSleep ( void )
{
	/* Sleep!!! Note that HAL_KEY_SW_5 is shared with CTS pin of RS-232.
	* It was set to tri-state during active state. It needs to be pulled-up.
	*/
//#if defined (HAL_KEY_SW_7_ENABLE)
//	HAL_KEY_SW_7_INP &= ~HAL_KEY_SW_7_BIT;       /* Set pin input mode to pull-up */
//#endif
}

/**************************************************************************************************
* @fn      HalKeyExitSleep
*
* @brief   - Get called when sleep is over
*
* @param
*
* @return  - return saved keys
**************************************************************************************************/
uint8 HalKeyExitSleep ( void )
{
	/* Wakeup!!! Note that HAL_KEY_SW_5 is shared with CTS pin of RS-232.
	* It was pulled up during sleep. It needs to be set to tri-state during active state.
	*/
//#if defined (HAL_KEY_SW_7_ENABLE)
//	HAL_KEY_SW_7_INP |= HAL_KEY_SW_7_BIT;       /* Set pin input mode to tri-state */
//#endif

	/* Wake up and read keys */
	return ( HalKeyRead () );
}
//P0ISR is used not only for keyboard, but LCD/audio. see KeyAudioISR.c for details.

/***************************************************************************************************
*                                    INTERRUPT SERVICE ROUTINE
***************************************************************************************************/

/**************************************************************************************************
* @fn      halKeyPort0Isr
*
* @brief   Port0 ISR
*
* @param
*
* @return
**************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort0Isr, P0INT_VECTOR )
{
	/* P0IF is cleared by HW for CHVER < REV_E */

	halProcessKeyInterrupt();

	if( CHVER >= REV_E )
	{
		/* Make sure that we clear all enabled, but unused P0IFG bits.
		* For P0 we can only enable or disable high or low nibble, not bit by
		* bit. For P1 and P2 enabling of single bits are possible, therefore
		* will not any unused pins generate interrupts on P1 or P2.
		* We could have checked for low and high nibble in P0, but this
		* isn't necessary as long as we only clear unused pin interrupts.
		*/
		P0IFG = (HAL_KEY_P0INT_LOW_USED | HAL_KEY_POINT_HIGH_USED);
		P0IF = 0;
		CLEAR_SLEEP_MODE();
	}
}
/**************************************************************************************************
* @fn      halKeyPort1Isr
*
* @brief   Port1 ISR
*
* @param
*
* @return
**************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort1Isr, P1INT_VECTOR )
{
	if( CHVER <= REV_D )
	{
		P1IF = 0;
	}

        halProcessKeyInterrupt();

	if( CHVER >= REV_E )
	{
		P1IF = 0;
		CLEAR_SLEEP_MODE();
	}
}

/**************************************************************************************************
* @fn      halKeyPort2Isr
*
* @brief   Port2 ISR
*
* @param
*
* @return
**************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort2Isr, P2INT_VECTOR )
{
	if( CHVER <= REV_D )
	{
		P2IF = 0;
	}
#if (defined HAL_BOARD_YIRIRRN) 
  //halProcessKeyInterrupt();
  #if defined (HAL_KEY_SW_7_ENABLE)
  if (HAL_KEY_SW_7_PXIFG & HAL_KEY_SW_7_BIT)      /* Interrupt Flag has been set */
  {
    HAL_KEY_SW_7_PXIFG = ~(HAL_KEY_SW_7_BIT);    /* Clear Interrupt Flag */
    HAL_KEY_SW_7_PXIFG = 0;
  }
  #endif
#else
  halProcessKeyInterrupt();
#endif

	if( CHVER >= REV_E )
	{
		P2IF = 0;
		CLEAR_SLEEP_MODE();
	}
	
#if (defined HAL_BOARD_YIRIRRN)
  #ifdef HAL_KEY_SW_7_ENABLE
  if(!(HAL_KEY_SW_7_PORT& HAL_KEY_SW_7_BIT) )
    {
            (pHalKeyProcessFunction) (HAL_KEY_SW_7, HAL_KEY_STATE_NORMAL);
    }
#endif
  #endif
}

/**************************************************************************************************
**************************************************************************************************/

