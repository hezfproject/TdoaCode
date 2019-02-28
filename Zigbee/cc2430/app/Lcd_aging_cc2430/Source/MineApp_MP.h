/**************************************************************************************************
  Filename:       MineApp_MP.h
  Revised:        $Date: 2011/07/22 17:38:35 $
  Revision:       $Revision: 1.27 $

  Description:    Mine Application of mobile phone.
  **************************************************************************************************/
  
#ifndef MINEAPP_MP_H
#define MINEAPP_MP_H

#include "hal_types.h"
#include "Appprotocol.h"
#if !(defined HAL_AUDIO) || (HAL_AUDIO == FALSE)
#error "ERROR! For MP function, Macro HAL_AUDIO should be set to true first!!"
#endif

/*************************************************************************************************
  *CONSTANTS
  */
/*************************************************************************************************
  *MACROS
  */
#define MINEAPP_PROBENWK_EVENT        0x0001
#define MINEAPP_FEEDDOG_EVENT           0x0004
//#define MINEAPP_JOINNEIGHBOR_EVENT  0x0008 deprecated now.
#define MINEAPP_WAKE_TIMEOUT_EVT     0x0008
#define MINEAPP_INITNWK_TIMEOUT_EVENT 0x0010
#define MINEAPP_DIALUP_RETRY_EVENT 0x0020
/*Ambe may not be  stop normally, need app to help to check and consolidate it.*/
#define MINEAPP_STOP_AUDIO_EVENT 0x0040
#ifdef MENU_RF_DEBUG
#define MINEAPP_RESTART_EVENT 	0x0080
#endif
#define MINE_JOINNOTIFY_EVENT		0x0100
#define MINE_JOINNEIGHTBOR_EVENT  0x0200
#ifdef PACKAGE_INFORMATION
#define MINEAPP_PACKAGE_INFORM_EVENT  0x0400
#endif

#ifndef MINEAPP_STOP_AUDIO_TIMEOUT
#define MINEAPP_STOP_AUDIO_TIMEOUT 10000
#endif


extern uint8 MineApp_TaskID;

/*********************************************************************
* FUNCTIONS
*/
void MineApp_Init( uint8 task_id );
uint16 MineApp_ProcessEvent( uint8 task_id, uint16 events);
void MineApp_SendSignal(uint8 signalLen);
void MineApp_ResetFrameblk(void);
void MineApp_ResetAudioStatus(uint16 timeout);
void MineApp_SetPeerNumber(termNbr_t* pnmbr);

#endif

