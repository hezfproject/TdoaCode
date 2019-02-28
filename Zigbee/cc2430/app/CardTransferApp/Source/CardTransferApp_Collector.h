#ifndef AMMETERAPP_COLLECTOR_H
#define AMMETERAPP_COLLECTOR_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * MACROS
 */
#define CARDTRANSFERAPP_MSG_RTRY_EVT       			0x0001
#define CARDTRANSFERAPP_FEEDWATCHDOG_EVENT      	0x0002
#define CARDTRANSFERAPP_MSG_SEND_EVT       			0x0004
#define CARDTRANSFERAPP_ROUTE_REPORT_EVT       	0x0020
#define CARDTRANSFERAPP_ROUTE_EXPIRE_EVT		0x0040

#define CARDTRANSFERAPP_ROUTE_ALIVE_EVT      0x0200
#define CARDTRANSFERAPP_ROUTE_REPAIR_EVT      0x0400
#ifdef DEBUG_REPORT_NW
#define CARDTRANSFERAPP_REPORT_NW_EVT      0x1000
#endif 
/*********************************************************************
 * GLOBAL VARIABLES
 */
extern byte CardTransferApp_TaskID;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Card Collector Application
 */
extern void CardTransferApp_Init( byte task_id );

/*
 * Task Event Processor for the Card Collector Application
 */
extern UINT16 CardTransferApp_ProcessEvent( byte task_id, UINT16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif

