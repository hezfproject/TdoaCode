/**************************************************************************************************
  Filename:       MineController_global.h
  Revised:        $Date: 2009/10/29 02:06:28 $
  Revision:       $Revision: 1.1 $

  Description:   User definable global Parameters.
**************************************************************************************************/
#ifndef MINECONTROLLER_GLOBAL_H
#define MINECONTROLLER_GLOBAL_H
/*********************************************************************
* INCLUDES
*/
#include "ZDApp.h"
#include "AppProtocolWrapper.h"

/*************************************************************************************************
*CONSTANTS
*/
/*************************************************************************************************
*MACROS
*/

/*Describe a MP status:
*                                                                   ->idle 
* For calling side: idle->calling->calling-wait->|
*                                                                   ->audio->idle
*                                             ->idle
* For called side: idle->called->|
*                                             ->audio->idle
* For Search Nwk: idle->SearchNwk->idle
*/
#define WORK_STATUS_IDLE                                      (0x00)
#define WORK_STATUS_CARDDETECTING                   (0x01)
#define WORK_STATUS_CARDSETTING                       (0x02)
#define WORK_STATUS_STATIONDETECTING              (0x03)
#define WORK_STATUS_STATIONSETTING                  (0x04)
//#define WORK_STATUS_SM_SENDING          (0x05)
//#define WORK_STATUS_WAKE                    (0x07)                    

#define INIT_NWK_TIMEOUT                    20000


/*************************************************************************************************
*MACROS
*/
#define ON_CARDDETECTING() MineApp_JudgeStatus(WORK_STATUS_CARDDETECTING)
#define IS_IDLE() MineApp_JudgeStatus(WORK_STATUS_IDLE)
#define ON_CARDSETTING() MineApp_JudgeStatus(WORK_STATUS_CARDSETTING)
#define ON_SUBSTATIONDETECTING() MineApp_JudgeStatus(WORK_STATUS_STATIONDETECTING)
#define ON_SUBSTATIONSETTING() MineApp_JudgeStatus(WORK_STATUS_STATIONSETTING)
//#define ON_SM_SENDING() MineApp_JudgeStatus(WORK_STATUS_SM_SENDING)
//#define ON_WAKE() MineApp_JudgeStatus(WORK_STATUS_WAKE)

#define SET_ON_CARDDETECTING() MineApp_SetStatus(WORK_STATUS_CARDDETECTING)
#define SET_ON_CARDSETTING() MineApp_SetStatus(WORK_STATUS_CARDSETTING)
#define SET_ON_SUBSTATIONDETECTING() MineApp_SetStatus(WORK_STATUS_STATIONDETECTING)
#define SET_ON_SUBSTATIONSETTING() MineApp_SetStatus(WORK_STATUS_STATIONSETTING)
#define SET_ON_IDLE() MineApp_SetStatus(WORK_STATUS_IDLE)
//#define SET_ON_SM_SENDING() MineApp_SetStatus(WORK_STATUS_SM_SENDING)
//#define SET_ON_WAKE() MineApp_SetStatus(WORK_STATUS_WAKE)

/*********************************************************************
* FUNCTIONS
*/
bool MineApp_JudgeStatus(uint8 WorkStatus);
void MineApp_SetStatus(uint8 WorkStatus);
/*
void MineApp_EndTalk(void);
void MineApp_StartSearchNWK(void);
void MineApp_SearchNWKStopped(void);
*/
#endif
