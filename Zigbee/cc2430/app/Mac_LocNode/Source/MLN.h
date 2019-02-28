/**************************************************************************************************
Filename:       MLN.h
Revised:        $Date: 2010/07/05 17:14:52 $
Revision:       $Revision: 1.3 $

Description:    This file contains the the Mac EndDevice Application protypes and definitions
**************************************************************************************************/

#ifndef MACENDDEVICEAPP_H
#define MACENDDEVICEAPP_H

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
#define MLN_KEY_INT_ENABLED     FALSE       
#else
#define MLN_KEY_INT_ENABLED     FALSE        
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
#define APS_CLUSTER_ID          LOCNODE_CLUSTERID //should be the same to the full-stack app setting, now is MINE_APP_CLUSTERID.
#define APS_PROFILE_ID           MINEAPP_APS_PROFID //should be the same to the full-stack setting.


	/**************************************************************************************************
	* CONSTANTS
	**************************************************************************************************/

	/* Event IDs */
//#define MLN_KEY_EVENT      0x0001
#define MLN_ALERT_EVENT   0x0001
#define MLN_BLAST_EVENT   0x0002
#define MLN_SLEEP_EVENT   0x0004
#ifdef USE_STATE_UNINIT
#define MLN_UNINIT_EVENT  0x0008
#endif


	//#define MLN_POLL_EVENT     0x0008
	//#define MLN_URGENT_EVENT 0x0010

	/**************************************************************************************************
	* GLOBALS
	**************************************************************************************************/
	extern uint8 MLN_TaskId;

	/*********************************************************************
	* FUNCTIONS
	*/

	/*
	* Task Initialization for the Mac Sample Application
	*/
	extern void MLN_Init( uint8 task_id );

	/*
	* Task Event Processor for the Mac Sample Application
	*/
	extern uint16 MLN_ProcessEvent( uint8 task_id, uint16 events );

	/*
	* Handle keys
	*/
	extern void MLN_HandleKeys( uint16 keys, uint8 shift );

	/*
	* Handle power saving
	*/
	// extern void MLN_PowerMgr (uint8 mode);

	/*********************************************************************
	*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MACENDDEVICEAPP_H */
