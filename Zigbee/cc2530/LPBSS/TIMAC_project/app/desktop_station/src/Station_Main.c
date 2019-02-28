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





uint16 sendData = 0;    //0x4E20
uint16 stationID = 0;    //0x4E20
uint16 stationIDcrc = 0;    //0x4E20
uint8  state = 0;
uint8   PreamblePoll = 0;
uint8   PatternPoll = 0;
uint8   DataPoll = 0;
uint8  sendCount = 0;
uint8  PatternCount = 0;

#define IDLE   					0
#define CARRIER_BURST 	1
#define SEPARATION_BIT  2
#define PREAMBLE 				3
#define PATTERN  				4
#define DATA   					5






unsigned int  pattern = 0x9669;
unsigned int  pIndex = 0x8000;
unsigned int  dIndex = 0x8000;


/**************************************************************************************************
 * FUNCTIONS
 **************************************************************************************************/
/* This callback is triggered when a key is pressed */
//void Station_Main_KeyCallback(uint8 keys, uint8 state);
void Station_Main_Timer3CallBack( uint8 timerId, uint8 channel, uint8 channelMode);
/**************************************************************************************************
 * @fn          main002378
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

  HalTimerConfig ( HAL_TIMER_3,                         // 16bit timer3
                   HAL_TIMER_MODE_CTC,                 // Clear Timer on Compare
                   HAL_TIMER_CHANNEL_SINGLE,           // Channel 1 - default
                   HAL_TIMER_CH_MODE_OUTPUT_COMPARE,   // Output Compare mode
                   TRUE,                              // Use interrupt
                   Station_Main_Timer3CallBack);            // Channel Mode

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


void Station_Main_Timer3CallBack( uint8 timerId, uint8 channel, uint8 channelMode)
{

	switch(state)
	{
		case CARRIER_BURST:
			T1CC0L=0xee;   // 3mS的AGC稳定时间
			T1CC0H=0x2;
			P0_3 = 1;
			T1CNTL=0;
			T1CNTH=0;
			state = SEPARATION_BIT;
			break;
		case SEPARATION_BIT:
			T1CC0L=0x5b;  	   // 370uS间隙
			T1CC0H=0x0;
            P0_3 = 0;
			T1CNTL=0;
			T1CNTH=0;
			state = PREAMBLE;
			break;

		case PREAMBLE:
			if(PreamblePoll<6)
			{
				switch(PreamblePoll%2)
				{
					case 1:
						T1CC0L=0x5b;  //370us高
					    T1CC0H=0x0;
                        P0_3 = 0;
						T1CNTL=0;
						T1CNTH=0;
						break;
					case 0:
						T1CC0L=0x5b; 	  //370us低
						T1CC0H=0x0;
                        P0_3 = 1;
						T1CNTL=0;
						T1CNTH=0;
						break;
				}
				PreamblePoll++;
			}
			if(PreamblePoll==6)
			{
				state = PATTERN;
				PreamblePoll=0;
			}
			break;


		case PATTERN:
			if(pattern & pIndex)
			{

              T1CC0L=0x5b; 	  //370us高
              T1CC0H=0x0;
              P0_3 = 1;
			  T1CNTL=0;
			  T1CNTH=0;
			  pIndex >>= 1;
			}
			else
			{
			  T1CC0L=0x5b; 	  //370us低
			  T1CC0H=0x0;
              P0_3 = 0;
			  T1CNTL=0;
			  T1CNTH=0;
			  pIndex >>= 1;
			}
			if(!pIndex)
			{
				pIndex = 0x8000;
				PatternCount++;
			}
			if(PatternCount==2)
			{
				state = DATA;
				PatternCount=0;
			}
			break;

		case DATA:								   //send data:stationID
            if(sendData & dIndex)
			{
			  T1CC0L=0x5b;	//370us高
		      T1CC0H=0x0;
              P0_3 = 1;
			  T1CNTL=0;
			  T1CNTH=0;
			  dIndex >>= 1;
			}
			else
			{
			  T1CC0L=0x5b; 	  //370us低
			  T1CC0H=0x0;
              P0_3 = 0;
			  T1CNTL=0;
			  T1CNTH=0;
			  dIndex >>= 1;
			}
			if(!dIndex)
			{
				dIndex = 0x8000;
				sendCount++;
                                sendData=stationIDcrc;
			}
			if(sendCount==2)
			{
				state = IDLE;
				sendCount=0;
                                sendData=stationID;
			}
			break;
		case IDLE:
			T1CC0L=0x98;	 // 3mS的AGC稳定时间
			T1CC0H=0x3a;
			T1CNTL=0;
			T1CNTH=0;
            P0_3 = 0;
			state = CARRIER_BURST;
			break;

		default:
			state = IDLE;
			T1CC0L=0x98;
			T1CC0H=0x3a;//2100us 低
			T1CNTL=0;
			T1CNTH=0;
			P0_3 = 0;
			break;

	}

}







/*************************************************************************************************
**************************************************************************************************/
