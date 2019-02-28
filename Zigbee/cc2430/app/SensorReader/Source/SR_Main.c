/**************************************************************************************************
  Filename:       SR_Main.c
  Revised:        $Date: 2011/01/18 04:37:38 $
  Revision:       $Revision: 1.1 $

  Description:    This file contains the main and callback functions

**************************************************************************************************/

/**************************************************************************************************
 *                                           Includes
 **************************************************************************************************/
/* Hal Drivers */
#include "hal_types.h"
#include "hal_timer.h"
#include "hal_drivers.h"


/* MAC Application Interface */
#include "mac_api.h"

/* Application */
#include "SensorReader.h"

/* OSAL */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OnBoard.h"
#include "OSAL_PwrMgr.h"

/**************************************************************************************************
 * FUNCTIONS
 **************************************************************************************************/
/* This callback is triggered when the timer finish its tick */
__near_func void SR_Main_TimerCallBack(uint8 timerId, uint8 channel, uint8 channelMode);

/**************************************************************************************************
 * @fn          main
 *
 * @brief       Start of application.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
__near_func int main(void)
{
  /* Initialize hardware */
  HAL_BOARD_INIT();

  /* Initialze the HAL driver */
  HalDriverInit();

  /* Initialize MAC */
  MAC_Init();

  /* Initialize the operating system */
  osal_init_system();

  /* Enable interrupts */
  HAL_ENABLE_INTERRUPTS();  

  /* Setup OSAL Timer 
  HalTimerConfig ( OSAL_TIMER,                         // 8bit timer3
                   HAL_TIMER_MODE_CTC,                 // Clear Timer on Compare
                   HAL_TIMER_CHANNEL_SINGLE,           // Channel 1 - default
                   HAL_TIMER_CH_MODE_OUTPUT_COMPARE,   // Output Compare mode
                   FALSE,                              // Use interrupt
                   SR_Main_TimerCallBack);            // Channel Mode
*/
  /* Start OSAL */
  osal_start_system(); // No Return from here

  return 0;
}

/**************************************************************************************************
                                           CALL-BACKS
**************************************************************************************************/


/**************************************************************************************************
 *
 * @fn      Osal_TimerCallBack()
 *
 * @brief   Update the timer per tick
 *
 * @param   timerId - The id of the timer that triggers the callback
 *          channel - the channel of the timer
 *          channelMode - the mode of the timer
 *
 * @return  local clock in milliseconds
 *
 **************************************************************************************************/
__near_func void SR_Main_TimerCallBack ( uint8 timerId, uint8 channel, uint8 channelMode)
{
  /* Update OSAL timer tick if it's OSAL_TIMER */
  //if ((timerId == OSAL_TIMER))
    //osal_update_timers();
}

/**************************************************************************************************
 *
 * @fn      SR_PowerMgr
 *
 * @brief   Enable/Disable and setup power saving related stuff
 *
 * @param   mode - PWRMGR_ALWAYS_ON or PWRMGR_BATTERY
 *
 * @return  void
 *
 **************************************************************************************************/
void SR_PowerMgr(uint8 enable)
{
  /* enable OSAL power management */
  if (enable)
   osal_pwrmgr_device(PWRMGR_BATTERY);
  else
   osal_pwrmgr_device(PWRMGR_ALWAYS_ON);
}

/*************************************************************************************************
**************************************************************************************************/
