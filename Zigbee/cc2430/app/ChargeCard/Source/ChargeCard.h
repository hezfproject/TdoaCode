/**************************************************************************************************
Filename:       ChargeCard.h
Revised:        $Date: 2011/03/15 18:37:07 $
Revision:       $Revision: 1.2 $

Description:    This file contains the the Mac EndDevice Application protypes and definitions
**************************************************************************************************/

#ifndef ChargeCard_H
#define ChargeCard_H

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
#define ChargeCard_KEY_INT_ENABLED     FALSE
#else
#define ChargeCard_KEY_INT_ENABLED     TRUE
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
#define CHARGECARD_ALERT_EVENT   0x0001
#define CHARGECARD_BLAST_EVENT   0x0002
#define CHARGECARD_RETREAT_EVENT   0x0004
#define CHARGECARD_KEY_LONGPRESS_EVENT  0x0008
#define CHARGECARD_RESET_EVENT  0x0010
#define CHARGECARD_OK_PRESS_EVENT  0x0020
#define CHARGECARD_PROCTIMETICK_EVENT 0x0040
#define CHARGECARD_KEYCONFIRM_EVENT 0x0080

#ifdef USE_STATE_UNINIT
#define CHARGECARD_UNINIT_EVENT  0x0100
#endif

    /**************************************************************************************************
    * GLOBALS
    **************************************************************************************************/
    extern uint8 ChargeCard_TaskId;

    /*********************************************************************
    * FUNCTIONS
    */

    /*
    * Task Initialization for the Mac Sample Application
    */
    extern  void ChargeCard_Init( uint8 task_id );

    /*
    * Task Event Processor for the Mac Sample Application
    */
    extern  uint16 ChargeCard_ProcessEvent( uint8 task_id, uint16 events );

    /*
    * Handle keys
    */
    extern void ChargeCard_HandleKeys( uint16 keys, uint8 shift );

    /*
    	* Handle timers
    	*/
    extern void ChargeCard_ProcTimeTick(void);

    /*********************************************************************
    *********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MACENDDEVICEAPP_H */
