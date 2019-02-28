/**************************************************************************************************
  Filename:       med_Main.c
  Revised:        $Date: 2011/04/08 02:51:57 $
  Revision:       $Revision: 1.2 $

  Description:    This file contains the main and callback functions

**************************************************************************************************/

/**************************************************************************************************
 *                                           Includes
 **************************************************************************************************/
/* Hal Drivers */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"


/* MAC Application Interface */
#include "mac_api.h"

/* Application */
#include "med.h"

/* OSAL */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OnBoard.h"
#include "OSAL_PwrMgr.h"

/**************************************************************************************************
 * FUNCTIONS
 **************************************************************************************************/
/* This callback is triggered when the timer finish its tick */
void MED_Main_TimerCallBack(uint8 timerId, uint8 channel, uint8 channelMode);

/* This callback is triggered when a key is pressed */
void MED_Main_KeyCallback(uint8 keys, uint8 state);

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
int main(void)
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

  /* Setup Keyboard callback */
  HalKeyConfig(MED_KEY_INT_ENABLED, MED_Main_KeyCallback);

  /* Start OSAL */
  osal_start_system(); // No Return from here

}

/**************************************************************************************************
                                           CALL-BACKS
**************************************************************************************************/


/**************************************************************************************************
 * @fn      MED_KeyCallback
 *
 * @brief   Callback service for keys
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  void
 **************************************************************************************************/
void MED_Main_KeyCallback(uint8 keys, uint8 state)
{
  if ( MED_TaskId != TASK_NO_TASK )
  {
    MED_HandleKeys (keys, state);
  }
}

/**************************************************************************************************
 *
 * @fn      MED_PowerMgr
 *
 * @brief   Enable/Disable and setup power saving related stuff
 *
 * @param   mode - PWRMGR_ALWAYS_ON or PWRMGR_BATTERY
 *
 * @return  void
 *
 **************************************************************************************************/
void MED_PowerMgr(uint8 enable)
{
  /* enable OSAL power management */
  if (enable)
   osal_pwrmgr_device(PWRMGR_BATTERY);
  else
   osal_pwrmgr_device(PWRMGR_ALWAYS_ON);
}

/*************************************************************************************************
**************************************************************************************************/
