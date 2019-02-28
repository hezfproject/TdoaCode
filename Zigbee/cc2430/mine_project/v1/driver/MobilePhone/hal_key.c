/**************************************************************************************************
  Filename:       hal_key.c
  Revised:        $Date: 2010/10/31 00:34:25 $
  Revision:       $Revision: 1.1 $
  Description:    This file contains the interface to the HAL KEY Service.
  For RRN board, there are two kinds of keys. one kind is controlled by ADC, the other one
  is controlled by IO port directly. They are mutually exclusive. that means, they will not 
  exist at the same time.
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
#include "key.h"
#include "osal.h"
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


/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/

static halKeyCBack_t pHalKeyProcessFunction;
bool Hal_KeyIntEnable;            /* interrupt enable/disable flag */

static uint8 HalKeyConfigured;


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

  InitialKey();
	
  /* Initialize callback function */
  pHalKeyProcessFunction  = NULL;

  /* Start with key is configured */
  HalKeyConfigured = TRUE;
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
    /* Do this only after the hal_key is configured - to work with sleep stuff */
    if (HalKeyConfigured == TRUE)
    {
      osal_stop_timerEx( Hal_TaskID, HAL_KEY_EVENT);  /* Cancel polling if active */
    }
  }
  else    /* Interrupts NOT enabled */
  {
    osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_POLLING_VALUE);    /* Kick off polling */
  }
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
//#if defined (HAL_KEY_ADC_ENABLE)
  keys = GetKey();
//#endif
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
//#if defined (HAL_KEY_ADC_ENABLE)
  keys = GetKey();
//#endif

  /* Exit if polling and no keys have changed */
  if (!Hal_KeyIntEnable)
  {
    if (keys == 0)
    {
      return;
    }
  }

  /* Invoke Callback if new keys were depressed */
  if (keys &&keys!=HAL_KEY_INVALID&& (pHalKeyProcessFunction))
  {
    (pHalKeyProcessFunction) (keys, HAL_KEY_STATE_NORMAL);
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
  /* Wake up and read keys */
  return ( HalKeyRead () );
}

//P0ISR is used not only for keyboard, but LCD/audio. see KeyAudioISR.c for details.

/**************************************************************************************************
**************************************************************************************************/

