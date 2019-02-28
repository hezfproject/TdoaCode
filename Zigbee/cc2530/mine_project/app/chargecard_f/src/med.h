/**************************************************************************************************
Filename:       med.h
Revised:        $Date: 2011/08/25 03:20:59 $
Revision:       $Revision: 1.3 $

Description:    This file contains the the Mac EndDevice Application protypes and definitions
**************************************************************************************************/

#ifndef MACENDDEVICEAPP_H
#define MACENDDEVICEAPP_H


/**************************************************************************************************
* INCLUDES
**************************************************************************************************/
#include "hal_types.h"
#include "hal_defs.h"
#include "App_cfg.h"
#include "AppProtocol.h"
/**************************************************************************************************
*                                        User's  Defines AND Constant
**************************************************************************************************/
#define MED_KEY_SW_6_PXIFG    P1IFG
#define MED_KEY_SW_6_PORT     P1
#define MED_KEY_SW_6_BIT      BV(6)

#ifdef DEBUG
#define MED_KEY_INT_ENABLED     FALSE
#define MED_HALL_INT_ENABLED    FALSE
#else
#define MED_KEY_INT_ENABLED     TRUE
#define MED_HALL_INT_ENABLED    TRUE
#endif

#define APS_DST_ENDPOINT      MINEAPP_ENDPOINT //should be the same to the full-stack app setting, now is MINE_APP_ENDPOINT.
#define APS_SRC_ENDPOINT      0x20  //FIXME:Get from sniffer, If not so, the full stack node will reject the pkt.
#define APS_CLUSTER_ID          CHARGEED_CLUSTERID //should be the same to the full-stack app setting, now is MINE_APP_CLUSTERID.
#define APS_PROFILE_ID           MINEAPP_APS_PROFID //should be the same to the full-stack setting.

/* Period of wait data when URGENT*/
#define MED_URGENT_POLL_TIMEOUT 50

/* Period of wait ALERTACK */
#define MED_ALERT_TIMEOUT 100

/* sleep length in ms */
#define MED_URGENT_SLEEP_PERIOD 1000

/* Led flash time when no power*/
#define MED_LED_FLASH_TIME_LOWBATTERY 300

/* Setting beacon order to 15 will disable the beacon */
#define MED_MAC_BEACON_ORDER 15

/* Setting superframe order to 15 will disable the superframe */
#define MED_MAC_SUPERFRAME_ORDER 15

/* (VDD/3)/(VREF/(2e7-1)) (VDD~=2.28V,VREF=1.15V) : VDD = 3.45 * LIMT / 127 */
#define MED_VDD_LIMT            0x23        /* 0x23 == 3.5V 0x54 = 2.28v, 0x55 = 2.309 */

/* Confirm receive RETREAT signal */
#define MED_RECVRETREAT_MAX     5

/* Alert period  in second */
#define MED_ALERT_TIME          10

/* Confirm time: 10min = 120 x 5s */
#define MED_MAX_CONFIRM_CNT     120

/* MED_REPORT_PERIOD * MED_REPORT_TICK equals 10 Minutes */
#define MED_REPORT_PERIOD       40

/* Not received reply maximum wait times */
#define MED_NOSIGNAL            1//3

/* stop POLL, Wait 1 minute = 12 * 5s*/
#define MED_SLEEP_POLL          3//12

#if (MED_NOSIGNAL >= MED_SLEEP_POLL)
#error "MED_NOSIGNAL >= MED_SLEEP_POLL"
#endif

/* two OUT + a POLL , (OUT__5s__OUT__5s__POLL) */
#define MED_GOTO_POLL           (MED_NOSIGNAL - 1)

/* mac payload length by datasheet. */
#define MED_MAC_PAYLOAD_LEN          127

/* Short Address of PAN Coordinator, will always be 0x0000 */
#define MED_SHORTADDR_PANCOORD       0x0000

/* Short Address of all Devs*/
#define MED_BROADCAST_SHORTADDR_DEVALL      0xFFFF

/* Application Work State */
/* The priority is: Alert > Urgent > Normal */
#define MED_STATE_NORMAL        0
#define MED_STATE_URGENT        1
#define MED_STATE_ALERT         2
#define MED_STATE_HALL          3

#define MED_UNICAST             0
#define MED_BROADCAST           1

//#define MED_VER               0
//#define MED_SHORTADDR         1
#define MED_EXITADDR            2

//0x1E00-0x1D00 are used to store paramters
#define MED_PARAM_ADDR          0x1E00
#define MED_DEVINFO_ADDR        0x780C

#define MED_LED_BLUE            HAL_LED_1
#define MED_LED_RED             HAL_LED_2

#define MED_VERSION_LEN         15

/* Event IDs */
//#define MED_KEY_EVENT      0x0001
#define MED_ALERT_EVENT   0x0001
#define MED_BLAST_EVENT   0x0002
#define MED_NOMAL_EVENT   0x0004

#define MED_SEND_RECVRETREAT_EVT   0x0010
#define MED_SEND_KEYCONFIRM_EVT   0x0020
#define MED_TIME_VERIFY_EVENT   0x0040
#define MED_HALL_EVENT      0x0080

/*
** Constant
*******************************************************************************/

/*********************************************************************
* TYPEDEFS
*/
typedef uint8 report_buff[30];
typedef uint8 P_Addr[8];

typedef struct
{
    bool bState;
    uint32 u32Delay;
} stTimeControl;

typedef enum
{
    POLL_WAIT,
    BLAST_WAIT,
    DELAYSTATELEN
} enDelayState;

typedef struct
{
    sAddrExt_t ExitAddr;
} Dev_Info_t;

typedef struct
{
    uint8 med_WorkState;
    uint8 med_WorkState_bk;

    bool med_IsBatteryLow;

    uint8 med_blast_cnt ;

    uint16 med_seqnum;

    bool med_AlertSuccess;

    uint8 med_AlertCnt;
    uint8 med_urgent_cnt;

    bool med_Confirm;

    uint8 med_ConfirmCnt;
    uint8 med_HallConfirmCnt;
} Dev_Param_t;

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
extern void     MED_Init(uint8 task_id);

/*
* Task Event Processor for the Mac Sample Application
*/
extern uint16   MED_ProcessEvent(uint8 task_id, uint16 events);

/*
* Handle keys
*/
extern void     MED_HandleKeys(uint8 keys, uint8 shift);

/*
* Handle Hall element
*/
extern void     MED_HandleHall(void);

/*
* Handle power saving
*/
extern void     MED_PowerMgr(uint8 mode);

/*********************************************************************
*********************************************************************/

#endif /* MACENDDEVICEAPP_H */
