#ifndef AMMETERAPP_DATACENTER_H
#define AMMETERAPP_DATACENTER_H

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

/* Max buffer length that AmmeterApp can accept from UART */
#define AMMETERAPP_MAX_RX_BUFF_LEN   10

/*********************************************************************
 * MACROS
 */
#define AMMETERAPP_FEEDWATCHDOG_EVENT 	0x0001
#define AMMETERAPP_STATUS_REPORT_EVENT  0x0002
#define AMMETERAPP_WDG_RESTART_EVENT     0x0004
#define AMMETERAPP_ROUTE_REQ_EVENT     	0x0008
#define AMMETERAPP_MSG_SEND_EVT			0x0010
#ifdef DEBUG_ROUTE
#define AMMETERAPP_REPORT_ROUTE_EVENT    0x0020
#define AMMETERAPP_SEND_ROUTE_EVENT   	0x0040
#endif
#define AMMETERAPP_ROUTE_EXPIRE_EVT		0x0080
#ifdef DEBUG_AMMETER
#define AMMETERAPP_TX_TEST_EVT			0x0100
#endif
#define AMMETERAPP_ROUTE_ALIVE_EVT      	0x0200
#define AMMETERAPP_ROUTE_CLEAN_EVT		0x0400

/*FIXME, too many events*/
#if (defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_TRANSPARENT)
#define AMMETERAPP_UART_READ_EVT                0x1000
#define  AMMETERAPP_ACK_SEND_EVT                 0x2000
#endif 

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

