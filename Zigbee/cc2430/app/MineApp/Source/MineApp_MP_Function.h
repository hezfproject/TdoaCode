/**************************************************************************************************
  Filename:       MineApp_MP_fuction.h
  Revised:        $Date: 2011/04/20 18:54:11 $
  Revision:       $Revision: 1.13 $

  Description:    A Mine Application task except voice function of mobile phone.
  **************************************************************************************************/

#ifndef MINEAPP_MP_FUCTION_H
#define MINEAPP_MP_FUCTION_H

#include "hal_types.h"
#include "mac_api.h"
#include "NLMEDE.h"
/*************************************************************************************************
  *CONSTANTS
  */
extern   uint8 MineApp_Function_TaskID;
/*************************************************************************************************
  *MACROS
  */
#define MINEAPP_POWEROFF_EVENT         0x0001
#define MINEAPP_UPDATE_EVENT           0x0002
#define MINEAPP_MENULIB_EVENT          0x0004
#define MINEAPP_PERIOD_SLEEP_EVENT     0x0008
#define MINEAPP_RESET_COMM_EVENT       0x0010
#define MINEAPP_POWERON_EVENT          0x0020
#define MINEAPP_CONTINUESPRESS_TEST_EVENT          0x0040
#define MINEAPP_ENERGYSCAN_PREP_EVENT   0x0080
#define MINEAPP_ENERGYSCAN_EVENT        0x0100
#define MINEAPP_POLL_EVENT			    0x0200
#define MINEAPP_CELL2_LEAVE_EVENT	    0x0400
#define MINEAPP_CELL2_JOIN_EVENT        0x0800
#define MINEAPP_BLAST_EVENT                 0x1000
#define MINEAPP_CELL_TIMEOUT_EVENT   	0x2000
/*********************************************************************
* FUNCTIONS
*/
void MineApp_Function_Init( uint8 task_id );
uint16 MineApp_Function_ProcessEvent( uint8 task_id, uint16 events);

void MineApp_StartMenuLibEvt (uint16 timeout);
void MineApp_StopMenuLibEvt (void);

void  MineApp_SetEnergyScanEn(bool en);
bool  MineApp_getEnergyScanEn(void);
void  MineApp_NetworkDiscoveryConfirmCB(uint8 ResultCount, networkDesc_t *NetworkList );

#endif

