/**************************************************************************************************
Filename:       CR.h
Revised:        $Date: 2011/01/21 18:32:41 $
Revision:       $Revision: 1.2 $

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
#define SR_KEY_INT_ENABLED     FALSE       
#else
#define SR_KEY_INT_ENABLED     FALSE        
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

#define SR_UART0_READ_EVENT     0x0001
#define SR_UART0_FETCHDATA_EVENT   0x0002
#define SR_UART0_FETCHSTATUS_EVENT   0x0004
#define SR_UART0_SET_EVENT   0x0008

#define SR_UART1_READ_EVENT     0x0100
#define SR_UART1_WRITE_EVENT   0x0200

#define SR_RESET_STATUS_EVENT  0x1000
#define SR_LED_CONTROL_EVENT  0x2000

#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#define SR_FEEDWATCHDOG_EVENT    	0x4000
#endif


#ifndef MBUS_CRC
#define MBUS_CRC
#endif


	//#define SR_POLL_EVENT     0x0008
	//#define SR_URGENT_EVENT 0x0010

	/**************************************************************************************************
	* GLOBALS
	**************************************************************************************************/
	extern uint8 SR_TaskId;

	/*********************************************************************
	* FUNCTIONS
	*/

	/*
	* Task Initialization for the Mac Sample Application
	*/
	extern void SR_Init( uint8 task_id );

	/*
	* Task Event Processor for the Mac Sample Application
	*/
	extern uint16 SR_ProcessEvent( uint8 task_id, uint16 events );

	/*
	* Handle power saving
	*/
	// extern void SR_PowerMgr (uint8 mode);

	/*********************************************************************
	*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MACENDDEVICEAPP_H */
