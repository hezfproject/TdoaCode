/**************************************************************************************************
  Filename:       MineController.h
  Revised:        $Date: 2009/10/29 22:11:43 $
  Revision:       $Revision: 1.4 $

  Description:    Controller of mine devices.
  **************************************************************************************************/
  
#ifndef MINE_CONTROLLER_H
#define MINE_CONTROLLER_H

#include "hal_types.h"

/*************************************************************************************************
  *CONSTANTS
  */
  #define MINECONTROLLER_POWEROFF_EVENT          0x0001
  #define MINECONTROLLER_PERIOD_SLEEP_EVENT     0x0002
  #define MINECONTROLLER_UPDATE_EVENT               0x0004
  #define MINECONTROLLER_MENULIB_EVENT             0x0008
  #if 1
  #define MINECONTROLLER_CARDSETFINISH_EVENT             0x0010
  #endif
/*************************************************************************************************
  *MACROS
  */

/*********************************************************************
* FUNCTIONS
*/
void MineController_Init( uint8 task_id );
uint16 MineController_ProcessEvent( uint8 task_id, uint16 events);
void MineController_StartMenuLibEvt (uint16 timeout);
void MineController_StopMenuLibEvt (void);
void  MineController_JoinNWK(uint8 LogicChannel);
#endif

