/**************************************************************************************************
  Filename:       MineApp_global.h
  Revised:        $Date: 2011/01/12 00:32:55 $
  Revision:       $Revision: 1.15 $

  Description:   User definable global Parameters.
**************************************************************************************************/
#ifndef MINEAPP_GLOBAL_H
#define MINEAPP_GLOBAL_H
/*********************************************************************
* INCLUDES
*/
#include "ZDApp.h"
#include "AppProtocolWrapper.h"
#include "mac_radio_defs.h"
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
#define WORK_STATUS_IDLE                      (0x00)
#define WORK_STATUS_AUDIO                   (0x01)
#define WORK_STATUS_CALLING                (0x02)
#define WORK_STATUS_CALLINGWAIT         (0x03)
#define WORK_STATUS_CALLED                  (0x04)
#define WORK_STATUS_SM_SENDING          (0x05)
#define WORK_STATUS_WAKE                    (0x07)                    

#define INIT_NWK_TIMEOUT                    20000


#define NMBRSIZE 4 //the number digit of mp number.

#define MINEAPP_PARAM_ADDR 0xFEF0
typedef struct 
{
	bool abnormalRst;
	bool backLightOn;
}StoreParam_t;


/*************************************************************************************************
*MACROS
*/
#define ON_AUDIO() MineApp_JudgeStatus(WORK_STATUS_AUDIO)
#define IS_IDLE() MineApp_JudgeStatus(WORK_STATUS_IDLE)
#define ON_CALLING() MineApp_JudgeStatus(WORK_STATUS_CALLING)
#define ON_CALLINGWAIT() MineApp_JudgeStatus(WORK_STATUS_CALLINGWAIT)
#define ON_CALLED() MineApp_JudgeStatus(WORK_STATUS_CALLED)
#define ON_SM_SENDING() MineApp_JudgeStatus(WORK_STATUS_SM_SENDING)
#define ON_WAKE() MineApp_JudgeStatus(WORK_STATUS_WAKE)

#define SET_ON_AUDIO() MineApp_SetStatus(WORK_STATUS_AUDIO)
#define SET_ON_CALLING() MineApp_SetStatus(WORK_STATUS_CALLING)
#define SET_ON_CALLINGWAIT() MineApp_SetStatus(WORK_STATUS_CALLINGWAIT)
#define SET_ON_CALLED() MineApp_SetStatus(WORK_STATUS_CALLED)
#define SET_ON_IDLE() MineApp_SetStatus(WORK_STATUS_IDLE)
#define SET_ON_SM_SENDING() MineApp_SetStatus(WORK_STATUS_SM_SENDING)
#define SET_ON_WAKE() MineApp_SetStatus(WORK_STATUS_WAKE)


#define RSSI_MIN     -81 //(MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
#define RSSI_MAX    10 //MAC_RADIO_RECEIVER_SATURATION_DBM


#define CONV_LQI_TO_RSSI( lqi )  (int8)(( lqi*((int16)RSSI_MAX - (int16)RSSI_MIN)/MAC_SPEC_ED_MAX + (int16)RSSI_MIN))

/*********************************************************************
* FUNCTIONS
*/
bool MineApp_JudgeStatus(uint8 WorkStatus);
void MineApp_SetStatus(uint8 WorkStatus);
void MineApp_StartTalk(void);
void MineApp_EndTalk(void);
void MineApp_ResetAudio(void);
void MineApp_StartSearchNWK(void);
void MineApp_StartSearchNWK2(uint16 panid);
void MineApp_SearchNWKStopped(void);
void MineApp_set_event(byte taskID, uint16 event_id);
void MineApp_start_timerEx( byte taskID, uint16 event_id, uint16 timeout_value );
void MineApp_SendSignal(uint8 signalLen);

/* remove from stringutil to reduce codesize */
uint16 MineaApp_atoul(uint8 *str);

#ifdef MENU_RF_DEBUG
void MineApp_LeaveNWK(uint8* macAddr);
void MineApp_Restart(void);
#endif

#endif

