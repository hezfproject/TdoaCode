
/**************************************************************************************************
Filename:       Station.h
Revised:        $Date: 2011/05/12 23:18:01 $
Revision:       $Revision: 1.3 $

Description:    This file contains the the Mac EndDevice Application protypes and definitions
**************************************************************************************************/

#ifndef STATION_H
#define STATION_H


/**************************************************************************************************
* INCLUDES
**************************************************************************************************/
#include "hal_types.h"
#include "hal_defs.h"
#include "App_cfg.h"
#include "RadioProto.h"





typedef enum
{
    E_STATION_INIT          = 0,
    E_STATION_STARTED
} eStationState;
typedef struct
{
    /* device information */
    uint16 PanId;
    uint16 ShortAddr;
    uint8 Channel;
    sAddrExt_t extAddr;

    /* status */
    eStationState State;

    /* retreat */
    uint8 retreat_flag;
    uint8 retreat_cnt;
} Station_DevInfo_t;


/**************************************************************************************************
*                                        User's  Defines
**************************************************************************************************/

#ifdef DEBUG
#define STATION_KEY_INT_ENABLED     FALSE
#else
#define STATION_KEY_INT_ENABLED     FALSE
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


#define APS_DST_ENDPOINT      STATION_ENDPOINT //should be the same to the full-stack app setting, now is MINE_APP_ENDPOINT.
#define APS_SRC_ENDPOINT      0x20  //FIXME:Get from sniffer, If not so, the full stack node will reject the pkt.
#define APS_CLUSTER_ID          LOCNODE_CLUSTERID //should be the same to the full-stack app setting, now is MINE_APP_CLUSTERID.
#define APS_PROFILE_ID           STATION_APS_PROFID //should be the same to the full-stack setting.

#define EXCITE_ID               (Station_DevInfo.PanId%100)
extern uint16 stationID;
extern uint16 sendData;
extern uint16 stationIDcrc;
/**************************************************************************************************
* CONSTANTS
**************************************************************************************************/

/* Event IDs */

#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#define STATION_FEEDWATCHDOG_EVENT    	0x0001
#endif
#define STATION_START_EVENT                         0x0002
#define STATION_IO_READ_EVENT                    0x0004
#define STATION_LF_INSPIRE_EVENT                    0x0008
#define STATION_UART_LINK_EVENT                     0x0010
#define STATION_WRITE_TIMEOUT_EVENT		    	    0x0020
#define STATION_STOP_BUZZER_EVENT                   0x0040
#define STATION_RESET_SAVEDEVID_EVENT               0x0080
#define STATION_REPORT_VERSION_EVENT                0x0100
#define STATION_REPORT_REMOTE_EVENT                 0x0200
#define STATION_SENDDATA_EVENT             			0x0400


/**************************************************************************************************
* GLOBALS
**************************************************************************************************/
extern uint8 Station_TaskId;
extern Station_DevInfo_t Station_DevInfo;

/*********************************************************************
* FUNCTIONS
*/

/*
* Task Initialization for the Mac Sample Application
*/
extern void Station_Init( uint8 task_id );

/*
* Task Event Processor for the Mac Sample Application
*/
extern uint16 Station_ProcessEvent( uint8 task_id, uint16 events );

/*
* Handle keys
*/
extern void Station_HandleKeys( uint16 keys, uint8 shift );

extern uint8 Get_8bits_for_LF(void);
extern uint16 Get_LFdata_Len(void);
extern uint16 Manchester_Encoding(uint8 uncode);

extern bool sentFlag;

/*
* UART state
*/
#define UART_IDLE                          0x01
#define UART_SYNC_Y                        0x02
#define UART_SYNC_I                        0x03
#define UART_SYNC_R                        0x04
#define UART_SYNC_i                        0x05

#define UART_MSG_T                         0x06
#define UART_EDEV_T                        0x07
#define UART_EDEV_ID_F                     0x08
#define UART_EDEV_ID_S                     0x09
#define UART_MDEV_ID_F                     0x0a
#define UART_MDEV_ID_S                     0x0b
#define UART_DATA_LEN_F                    0x0c
#define UART_DATA_LEN_S                    0x0d

#define UART_DATA_RX                       0x0e

#define UART_PADDING_F                     0x0f
#define UART_PADDING_S                     0x10
#define UART_CRC_F                    	   0x11
#define UART_CRC_S                    	   0x12

// extern void CR_PowerMgr (uint8 mode);

/*********************************************************************
*********************************************************************/


#endif /* MACENDDEVICEAPP_H */
