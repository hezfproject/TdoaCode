/**************************************************************************************************
  Filename:       Station_Main.c
  Revised:        $Date: 2011/04/08 00:19:24 $
  Revision:       $Revision: 1.1 $

  DesStationiption:    This file contains the main and callback functions

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
#include "CardReader.h"

/* OSAL */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OnBoard.h"
#include "OSAL_PwrMgr.h"

#include "hal_board_cfg.h"
/**************************************************************************************************
 * FUNCTIONS
 **************************************************************************************************/
 
/* This callback is triggered when a key is pressed */
void Station_Main_KeyCallback(uint8 keys, uint8 state);
void Station_Main_TimerCallBack( uint8 timerId, uint8 channel, uint8 channelMode);

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
  HalKeyConfig(STATION_KEY_INT_ENABLED, Station_Main_KeyCallback);

  /* Start OSAL */
  osal_start_system(); // No Return from here

  return 0;
}

/**************************************************************************************************
                                           CALL-BACKS
**************************************************************************************************/


/**************************************************************************************************
 * @fn      Station_KeyCallback
 *
 * @brief   Callback service for keys
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  void
 **************************************************************************************************/
void Station_Main_KeyCallback(uint8 keys, uint8 state)
{
  // if(OnBoard_SendKeys(keys, state) != SUCCESS)
  {
    //    SystemReset();
  }
}
/*************************************************************************************************
**************************************************************************************************/
