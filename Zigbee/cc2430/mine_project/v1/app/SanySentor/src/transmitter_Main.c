/**************************************************************************************************
  Filename:       transmitter_Main.c
  Revised:        $Date: 2011/08/31 10:36:11 $
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
#include "hal_led.h"


/* MAC Application Interface */
#include "mac_api.h"

/* Application */
#include "transmitter.h"
//#include "msa.h"

/* OSAL */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OnBoard.h"
#include "OSAL_PwrMgr.h"
#include "hal_board_cfg.h"
#include "OSAL_Nv.h"
/**************************************************************************************************
 * FUNCTIONS
 **************************************************************************************************/
 
/* This callback is triggered when a key is pressed */
void Sany_Main_KeyCallback(uint16 keys, uint8 state);

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

    osal_nv_init( NULL );
  /* Initialize the operating system */
  osal_init_system();

  /* Enable interrupts */
  HAL_ENABLE_INTERRUPTS();  

  /* Setup Keyboard callback */
    // HalKeyConfig(FALSE, Sany_Main_KeyCallback);

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
void Sany_Main_KeyCallback(uint16 keys, uint8 state)
{

}
/**************************************************************************************************
 *
 * @fn      Sany_PowerMgr
 *
 * @brief   Enable/Disable and setup power saving related stuff
 *
 * @param   mode - PWRMGR_ALWAYS_ON or PWRMGR_BATTERY
 *
 * @return  void
 *
 **************************************************************************************************/
void Sany_PowerMgr(uint8 enable)
{
    /* enable OSAL power management */
    if (enable)
        osal_pwrmgr_device(PWRMGR_BATTERY);
    else
        osal_pwrmgr_device(PWRMGR_ALWAYS_ON);
}

/*************************************************************************************************
**************************************************************************************************/
