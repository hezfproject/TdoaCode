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
#define AMMETERAPP_MSG_RTRY_EVT       			0x0001
#define AMMETERAPP_FEEDWATCHDOG_EVENT      	0x0002
#define AMMETERAPP_MSG_SEND_EVT       			0x0004
#define AMMETERAPP_ACK_SEND_EVT       			0x0008
#define AMMETERAPP_UART_READ_EVT       			0x0010
#define AMMETERAPP_ROUTE_REPORT_EVT       	0x0020
#define AMMETERAPP_ROUTE_EXPIRE_EVT		0x0040

#ifdef DEBUG_AMMETER
#define AMMETERAPP_TX_RTRY_EVT      0x0080
#define AMMETERAPP_RX_RTRY_EVT      0x0100
#endif
#define AMMETERAPP_ROUTE_ALIVE_EVT      0x0200
#define AMMETERAPP_ROUTE_REPAIR_EVT      0x0400

#define AMMETERAPP_PORTBUSY_TIMEOUT_EVT      0x0800

#ifdef DEBUG_REPORT_NW
#define AMMETERAPP_REPORT_NW_EVT      0x1000
#endif 

#if (defined CFG_AMMETER_IEC) || (defined DEBUG_REPORT_NW)
#define AMMETERAPP_UART_READ2_EVT     0x2000
#endif

//#ifndef MAX_BROADCAST_FREQ				
//#define MAX_BROADCAST_FREQ			10
//#endif


/*********************************************************************
 * GLOBAL VARIABLES
 */
extern byte AmmeterApp_TaskID;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Ammeter Collector Application
 */
extern void AmmeterApp_Init( byte task_id );

/*
 * Task Event Processor for the Ammeter Collector Application
 */
extern UINT16 AmmeterApp_ProcessEvent( byte task_id, UINT16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif

