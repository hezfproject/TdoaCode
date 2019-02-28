/**************************************************************************************************
Filename:       med.h
Revised:        $Date: 2012/01/07 02:17:12 $
Revision:       $Revision: 1.1.4.2 $

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
#define APS_CLUSTER_ID          BLAST_CLUSTERID //should be the same to the full-stack app setting, now is MINE_APP_CLUSTERID.
#define APS_PROFILE_ID           MINEAPP_APS_PROFID //should be the same to the full-stack setting.

#define REQ_SPACE 1000
#define FEEDDOG_SPACE   500
#define AUTOREBOOT      1200
#define LIMITCNT        10
    /**************************************************************************************************
    * CONSTANTS
    **************************************************************************************************/

    /* Event IDs */
#define MED_FEEDDOG_EVENT     0x0001
#define MED_REQUEST_EVENT     0x0002
    /***************************************************************************
    * GLOBALS
    ***************************************************************************/
    extern uint8 MED_TaskId;

    /*********************************************************************
    * FUNCTIONS
    */

    /*
    * Task Initialization for the Mac Sample Application
    */
    extern void MED_Init( uint8 task_id );

    /*
    * Task Event Processor for the Mac Sample Application
    */
    extern uint16 MED_ProcessEvent( uint8 task_id, uint16 events );

    /*
    * Handle keys
    */
    // extern void MED_HandleKeys( uint16 keys, uint8 shift );

    /*
    * Handle power saving
    */
    extern void MED_PowerMgr (uint8 mode);

    /*********************************************************************
    *********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MACENDDEVICEAPP_H */
