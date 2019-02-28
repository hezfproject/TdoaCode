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
#include "Station.h"

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
//void Station_Main_KeyCallback(uint8 keys, uint8 state);
void Station_Main_Timer0CallBack( uint8 timerId, uint8 channel, uint8 channelMode);

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

  HalTimerConfig ( HAL_TIMER_0,                         // 16bit timer3
                   HAL_TIMER_MODE_CTC,                 // Clear Timer on Compare
                   HAL_TIMER_CHANNEL_SINGLE,           // Channel 1 - default
                   HAL_TIMER_CH_MODE_OUTPUT_COMPARE,   // Output Compare mode
                   TRUE,                              // Use interrupt
                   Station_Main_Timer0CallBack);            // Channel Mode

  /* Initialize the operating system */
  osal_init_system();

  /* Enable interrupts */
  HAL_ENABLE_INTERRUPTS();

  /* Setup Keyboard callback */
  //HalKeyConfig(STATION_KEY_INT_ENABLED, Station_Main_KeyCallback);

  /* Setup OSAL Timer */

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
/*
void Station_Main_KeyCallback(uint8 keys, uint8 state)
{
  if(OnBoard_SendKeys(keys, state) != SUCCESS)
  {
        SystemReset();
  }
}
*/

void Station_Main_Timer0CallBack( uint8 timerId, uint8 channel, uint8 channelMode)
{
#define  PREAMBLE_LEN     17
    static uint8 save8bits = 0;
    static uint8 idx = 0;
    static uint8 pCount =PREAMBLE_LEN;
    static uint16 sentCount =0;

    if(!sentFlag)
    {
        P0_3 = 0;
        return;
    }

    if(pCount)
    {
        pCount--;
        if(pCount <= 8)
        {
            if((pCount >= 4)&&(pCount <= 7))
            {
                P0_3 = 1;
            }
            else
            {
                P0_3 = 0;
            }
        }
        else
        {
            P0_3 = 1;
        }
    }
    else
    {
        if(idx == 0)
        {
            save8bits =Get_8bits_for_LF();
            sentCount ++;
        }

        if(save8bits&(0x80>>(idx ++)))
        {
                P0_3 = 1;
        }
        else
        {
                P0_3 = 0;
        }
        if(idx >= 8)
        {
            idx = 0;

            if(sentCount >= Get_LFdata_Len())
            {
                sentCount = 0;
                pCount =PREAMBLE_LEN;
                sentFlag = false;
            }
            save8bits = 0;
        }

    }
}

/*************************************************************************************************
**************************************************************************************************/
