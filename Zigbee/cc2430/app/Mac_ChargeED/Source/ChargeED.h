/**************************************************************************************************
Filename:       ChargeED.h
Revised:        $Date: 2010/08/29 04:14:59 $
Revision:       $Revision: 1.8 $

Description:    This file contains the the Mac EndDevice Application protypes and definitions
**************************************************************************************************/

#ifndef CHARGEED_H
#define CHARGEED_H

#ifdef __cplusplus
extern "C"
{
#endif

	/**************************************************************************************************
	* INCLUDES
	**************************************************************************************************/
#include "hal_types.h"
#include "hal_defs.h"
#include "App_cfg.h"
#include "AppProtocol.h"
	/**************************************************************************************************
	*                                        User's  Defines
	**************************************************************************************************/  

#ifdef DEBUG
#define CHARGEED_KEY_INT_ENABLED     FALSE       
#else
#define CHARGEED_KEY_INT_ENABLED     TRUE     
#endif
	/*
	* FALSE = Key Polling
	* TRUE  = Key interrupt
	*
	* Notes: Key interrupt will not work well with 2430 EB because
	*        all the operations using up/down/left/right switch will
	*        no longer work. Normally S1 + up/down/left/right is used
	*        to invoke the switches but on the 2430 EB board,  the
	*        GPIO for S1 is used by the LCD.
	*/


#define APS_DST_ENDPOINT      MINEAPP_ENDPOINT //should be the same to the full-stack app setting, now is MINE_APP_ENDPOINT.
#define APS_SRC_ENDPOINT      0x20  //FIXME:Get from sniffer, If not so, the full stack node will reject the pkt.
#define APS_CLUSTER_ID          CHARGEED_CLUSTERID //should be the same to the full-stack app setting, now is MINE_APP_CLUSTERID.
#define APS_PROFILE_ID           MINEAPP_APS_PROFID //should be the same to the full-stack setting.


	/**************************************************************************************************
	* CONSTANTS
	**************************************************************************************************/

	/* Event IDs */
#define CHARGEED_ALERT_EVENT   0x0001
#define CHARGEED_BLAST_EVENT   0x0002
#define CHARGEED_RETREAT_EVENT   0x0004
#define CHARGEED_KEY_LONGPRESS_EVENT  0x0008
#define CHARGEED_NEXTTICK_EVENT  0x0010
#define CHARGEED_PROCTIMETICK_EVENT  0x0020
#define CHARGEED_RESET_EVENT  0x0040
#define CHARGEED_OK_PRESS_EVENT  0x0080

#ifdef USE_STATE_UNINIT
#define CHARGEED_UNINIT_EVENT  0x0100
#endif

	//#define CHARGEED_POLL_EVENT     0x0008
	//#define CHARGEED_URGENT_EVENT 0x0010

	/**************************************************************************************************
	* GLOBALS
	**************************************************************************************************/
	extern uint8 ChargeED_TaskId;

	/*********************************************************************
	* FUNCTIONS
	*/

	/*
	* Task Initialization for the Mac Sample Application
	*/
	extern  void ChargeED_Init( uint8 task_id );

	/*
	* Task Event Processor for the Mac Sample Application
	*/
	extern  uint16 ChargeED_ProcessEvent( uint8 task_id, uint16 events );

	/*
	* Handle keys
	*/
	extern void ChargeED_HandleKeys( uint16 keys, uint8 shift );
        
	/*
	* Handle timers
	*/        
        extern void ChargeED_ProcTimeTick(void);
	/*
	* Handle power saving
	*/
	// extern void ChargeED_PowerMgr (uint8 mode);

	/*********************************************************************
	*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MACENDDEVICEAPP_H */
