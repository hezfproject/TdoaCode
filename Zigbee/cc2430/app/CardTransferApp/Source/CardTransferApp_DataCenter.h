#ifndef CARDTRANSFERAPP_DATACENTER_H
#define CARDTRANSFERAPP_DATACENTER_H

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

/* Max buffer length that CardTransferApp can accept from UART */
#define CARDTRANSFERAPP_MAX_RX_BUFF_LEN   10

/*********************************************************************
 * MACROS
 */
#define CARDTRANSFERAPP_FEEDWATCHDOG_EVENT 	0x0001
#define CARDTRANSFERAPP_STATUS_REPORT_EVENT  0x0002
#define CARDTRANSFERAPP_WDG_RESTART_EVENT     0x0004
#define CARDTRANSFERAPP_ROUTE_REQ_EVENT     	0x0008
#define CARDTRANSFERAPP_MSG_SEND_EVT			0x0010
#ifdef DEBUG_ROUTE
#define CARDTRANSFERAPP_REPORT_ROUTE_EVENT    0x0020
#endif
#define CARDTRANSFERAPP_ROUTE_EXPIRE_EVT		0x0080

#define CARDTRANSFERAPP_ROUTE_ALIVE_EVT      	0x0200
#define CARDTRANSFERAPP_ROUTE_CLEAN_EVT		0x0400

#define CARDTRANSFERAPP_PROBE_SPI_EVENT    		0x0800
#define CARDTRANSFERAPP_DETECT_SPI_EVENT			0x1000


/*********************************************************************
 * GLOBAL VARIABLES
 */
extern byte CardTransferApp_TaskID;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Ammeter Collector Application
 */
extern void CardTransferApp_Init( byte task_id );

/*
 * Task Event Processor for the Ammeter Collector Application
 */
extern UINT16 CardTransferApp_ProcessEvent( byte task_id, UINT16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif

