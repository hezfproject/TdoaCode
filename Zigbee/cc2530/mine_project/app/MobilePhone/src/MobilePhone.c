/**************************************************************************************************
Filename:       MobilePhone.c
Revised:        $Date: 2011/08/16 17:53:24 $
Revision:       $Revision: 1.22 $

Description:    This file contains the sample application that can be use to test
the functionality of the MAC, HAL and low level.

**************************************************************************************************/

/**************************************************************************************************

Description:

**************************************************************************************************/


/**************************************************************************************************
*                                           Includes
**************************************************************************************************/

/* Hal Driver includes */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_lcd.h"
#include "hal_audio.h"
#include "hal_assert.h"
#include "hal_alarm.h"
#include "hal_drivers.h"
#include "hal_flash.h"

/* OS includes */
#include "ZComdef.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"
#include "mac_radio_defs.h"
#include "mac_spec.h"
#include "Mac_api.h"

#include "App_cfg.h"
#include "timeUtil.h"
//#include "stringUtil.h"
#include "WatchDogUtil.h"
#include "MacUtil.h"
#include "MenuLib_Nv.h"

/* Application */
#include "MobilePhone.h"
#include "MobilePhone_Function.h"
#include "MobilePhone_global.h"
#include "MobilePhone_Menulib.h"

/**************************************************************************************************
*                                           Constant
**************************************************************************************************/
extern uint8 contact_item_L[MAX_CONTACT_NUM];
bool  Menu_rf_debug=false;
//bool  Rssi_information=false;
//bool  Packge_information=false;
bool  ismore_contact=true;

/* Size table for MAC structures */
const CODE uint8 MP_cbackSizeTable [] =
{
    0,                                   /* unused */
    sizeof(macMlmeAssociateInd_t),       /* MAC_MLME_ASSOCIATE_IND */
    sizeof(macMlmeAssociateCnf_t),       /* MAC_MLME_ASSOCIATE_CNF */
    sizeof(macMlmeDisassociateInd_t),    /* MAC_MLME_DISASSOCIATE_IND */
    sizeof(macMlmeDisassociateCnf_t),    /* MAC_MLME_DISASSOCIATE_CNF */
    sizeof(macMlmeBeaconNotifyInd_t),    /* MAC_MLME_BEACON_NOTIFY_IND */
    sizeof(macMlmeOrphanInd_t),          /* MAC_MLME_ORPHAN_IND */
    sizeof(macMlmeScanCnf_t),            /* MAC_MLME_SCAN_CNF */
    sizeof(macMlmeStartCnf_t),           /* MAC_MLME_START_CNF */
    sizeof(macMlmeSyncLossInd_t),        /* MAC_MLME_SYNC_LOSS_IND */
    sizeof(macMlmePollCnf_t),            /* MAC_MLME_POLL_CNF */
    sizeof(macMlmeCommStatusInd_t),      /* MAC_MLME_COMM_STATUS_IND */
    sizeof(macMcpsDataCnf_t),            /* MAC_MCPS_DATA_CNF */
    sizeof(macMcpsDataInd_t),            /* MAC_MCPS_DATA_IND */
    sizeof(macMcpsPurgeCnf_t),           /* MAC_MCPS_PURGE_CNF */
    sizeof(macEventHdr_t)                /* MAC_PWR_ON_CNF */
};

/* TRUE and FALSE value */
bool          MP_MACTrue = TRUE;
bool          MP_MACFalse = FALSE;

/**************************************************************************************************
*                                        extern  Variables
**************************************************************************************************/

/* Task ID */
uint8 MP_TaskId;
uint8 MP_RFState = 0;
/**************************************************************************************************
*                                         Typedefs
**************************************************************************************************/

#if   1//def PACKAGE_INFORMATION
typedef struct
{
    uint16 err_cnt;
    uint16 recv_cnt;
    uint16 send_cnt;
} MP_packet_info_t;
#endif

/**************************************************************************************************
*                                        Local Variables
**************************************************************************************************/

static MP_packet_info_t MP_packet_info;

static uint8   MP_RejoinTimes;
uint8  signal_update_count=10;

static bool Version_isGet=TRUE;
static uint8  version[20];
/*************************************************************************************************
*MACROS
*/

/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
static void MP_DeviceStartup(void);
static uint8  MP_ReadExtAddr(void);
static byte MP_ParseAppFrame(const sData_t data, uint16 panid, uint16 shortAddr , int8 rssi);
static void MP_ParseVoice(const app_Voice_t*pVoice, int8 rssi);
static void MP_ParseCmd(const app_CmdDown_t* pCmd);
static void MP_ParseSms(app_SMS_t*pSms);
static void MP_ParseJoin(app_Timac_JoinNwk_t*p, uint16 panid, int8 rssi);
static void MP_ParseTime(app_TimessInd_t*p);
static void MP_ParsePoll(app_MP_Poll_Ack_t*p, int8 rssi);
static bool MP_ParseScan(app_Timac_Mp_Scan_t* pScan, uint16 panid, int8 rssi);
static void MP_ParseRejoin(app_RejoinNwk_t *p);
static void MP_ParseVersion(app_Report_t *p);
static int8  MP_SearchCellInfo(uint16 panid, int8 rssi);
static void MP_ReadR_or_DInfo(void);
static void MP_Demand_Version(void);
uint8  MP_VersionisGet(void);
void MP_GetVersion(uint8 *p);
void MP_SetR_or_DInfo(void);
void MP_to_release_vesion(void);
/**************************************************************************************************
*
* @fn          MP_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void MP_Init(uint8 taskId)
{
    Menu_handle_msg(MSG_POWERON_ANIM, NULL, 0);

    MP_Demand_Version();

    /* Initialize the task id */
    MP_TaskId = taskId;

    /* initialize MAC features */
    MAC_Init();
    MAC_InitDevice();

    /* Reset the MAC */
    MAC_MlmeResetReq(TRUE);

    /*inittial variables */
    MP_DevInfo.Channel = 0x0B;
    MP_DevInfo.CoordPanID = 0xFFFF;
    MP_DevInfo.currentRssi = MP_MIN_RSSI;
    MP_DevInfo.DesireCoordPanID = 0xFFFF;

    MP_AudioInfo.peernmbr = 0;
    MP_AudioInfo.seqnum = 0;
    MP_AudioInfo.cmdseqnum = 0;

    MP_NwkInfo.nwkState = NWK_DETAIL_INIT;

    MP_ReadR_or_DInfo();
    MP_ReadExtAddr();
    MP_DeviceStartup();

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
//    StartWatchDog(DOGTIMER_INTERVAL_1S);
    MP_set_event(MP_TaskId, MP_FEEDDOG_EVENT);
#endif

    /* Audio */
    /* DO NOT put it in HalDriverInit, because it needs memalloc */
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
    HalAudioInit();
#endif

    MP_RFState = MP_RF_OK;
//#ifdef PACKAGE_INFORMATION
    if(Menu_rf_debug==true)
    {
        MP_start_timerEx(MP_TaskId, MP_PACKAGE_INFORM_EVENT, 1000);
    }
//#endif

}

void MP_Demand_Version()
{
    Version_isGet=false;
}

uint8 MP_VersionisGet(void)
{
    return  Version_isGet;
}

void MP_GetVersion(uint8* p)
{
    strcpy ((char*)p, (char *)version);
}

void MP_to_release_vesion()
{
    uint8 Param=0xff;
    osal_nv_write(MP_NV_R_OR_D_ITEM, 0, sizeof(uint8), &Param);
    Menu_rf_debug=false;
    //Rssi_information=false;
    //Packge_information=false;
}

void MP_ReadR_or_DInfo()
{
    //MP_SetR_or_DInfo();
    uint8 Param,pos;

    Param = osal_nv_item_init(MP_NV_R_OR_D_ITEM, sizeof(uint8), NULL);
    if(Param == NV_ITEM_UNINIT)
    {
        pos = 0xff;
        osal_nv_write(MP_NV_R_OR_D_ITEM,0, sizeof(uint8), &pos);
    }

    osal_nv_read(MP_NV_R_OR_D_ITEM, 0, sizeof(uint8), &Param);//* (storeParam_t *)(MP_R_or_D_INFO_ADDR);
    if(!Param)
    {
        Menu_rf_debug=true;
        //Rssi_information=true;
        //Packge_information=true;
    }
    else
    {
        Menu_rf_debug=false;
        //Rssi_information=false;
        //Packge_information=false;
    }
}
void MP_SetR_or_DInfo()
{
    //storeParam_t Param=*(storeParam_t *)(MP_R_or_D_INFO_ADDR);
    uint8 Param;
    osal_nv_read(MP_NV_R_OR_D_ITEM, 0, sizeof(uint8), &Param);
    if(!Param)
        Param=0xff;
    else Param=0x0;
    osal_nv_write(MP_NV_R_OR_D_ITEM, 0, sizeof(uint8), &Param);
}
/**************************************************************************************************
*
* @fn          MP_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 MP_ProcessEvent(uint8 taskId, uint16 events)
{
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    if(events & MP_FEEDDOG_EVENT)
    {
        if(NO_TIMER_AVAIL == osal_start_timerEx(MP_TaskId, MP_FEEDDOG_EVENT, 300))
        {
            SystemReset();
        }
        FeedWatchDog();
        return events ^ MP_FEEDDOG_EVENT;
    }
#endif

    if(events & SYS_EVENT_MSG)
    {
        uint8 *pMsg;
        while((pMsg = osal_msg_receive(MP_TaskId)) != NULL)
        {
            macCbackEvent_t *pData = (macCbackEvent_t *)pMsg;
            switch(*pMsg)
            {
            case MAC_MLME_COMM_STATUS_IND:
                break;

            case MAC_MCPS_DATA_CNF:
                pData = (macCbackEvent_t *) pMsg;
                mac_msg_deallocate((uint8 **)&pData->dataCnf.pDataReq);
                break;

            case MAC_MCPS_DATA_IND:
            {
                /* do not check the indication status */
                //if(pData->hdr.status == SUCCESS)
                {
                    pData = (macCbackEvent_t *)pMsg;
                    MP_ParseAppFrame(pData->dataInd.msdu, pData->dataInd.mac.srcPanId, pData->dataInd.mac.srcAddr.addr.shortAddr, pData->dataInd.mac.rssi);
                }

                break;
            }
            }

            /* Deallocate */
            mac_msg_deallocate((uint8 **)&pMsg);
        }

        return events ^ SYS_EVENT_MSG;
    }
    /* resend dialup command */
    if(events & MP_DIALUP_RETRY_EVENT)
    {
        /* continues to send dial up until found, accepted, or start voice  */
        if(MP_AudioInfo.peernmbr != MP_SHORT_INVALIDNMMBR
                && (ON_CALLING() || ON_CALLINGWAIT()))
        {
            MP_AudioInfo.retrying_bitmap |= MP_DIALUP_RETRY_BIT;
            MP_SendCmd(CMD_UP_DIALUP, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
            MP_start_timerEx(MP_TaskId, MP_DIALUP_RETRY_EVENT, MP_SIGNAL_RETRY_TIME);
        }
        else
        {
            MP_AudioInfo.retrying_bitmap &= ~MP_DIALUP_RETRY_BIT;
        }
        return (events ^ MP_DIALUP_RETRY_EVENT);
    }

    /* resend accept command */
    if(events & MP_ACCEPT_RETRY_EVENT)
    {
        if(++MP_AudioInfo.accept_cnt < 3 && MP_AudioInfo.peernmbr != MP_SHORT_INVALIDNMMBR)
        {
            MP_SendCmd(CMD_UP_ACCEPT, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
            MP_start_timerEx(MP_TaskId, MP_ACCEPT_RETRY_EVENT, 200);
            MP_AudioInfo.retrying_bitmap |= MP_ACCEPT_RETRY_BIT;
        }
        else
        {
            MP_AudioInfo.accept_cnt = 0;
            MP_AudioInfo.retrying_bitmap &= ~MP_ACCEPT_RETRY_BIT;
        }
        return (events ^ MP_ACCEPT_RETRY_EVENT);
    }

    /* resend close command */
    if(events & MP_CLOSE_RETRY_EVENT)
    {
        if(++MP_AudioInfo.close_cnt < 3 && MP_AudioInfo.peernmbr != MP_SHORT_INVALIDNMMBR)
        {
            MP_SendCmd(CMD_UP_CLOSE, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
            MP_start_timerEx(MP_TaskId, MP_CLOSE_RETRY_EVENT, 100);
            MP_AudioInfo.retrying_bitmap |= MP_CLOSE_RETRY_BIT;
        }
        else
        {
            MP_AudioInfo.close_cnt = 0;
            MP_AudioInfo.retrying_bitmap &= ~MP_CLOSE_RETRY_BIT;
        }
        return (events ^ MP_CLOSE_RETRY_EVENT);
    }

    if(events & MP_PROBENWK_EVENT)
    {
        if(++MP_RejoinTimes > MP_REJOINTIMES)
        {
            MP_RejoinTimes = 0;

            MP_NwkInfo.nwkState = NWK_DETAIL_INIT;
            MP_DevInfo.CoordPanID = 0xFFFF;
            MP_DevInfo.currentRssi = MP_MIN_RSSI;

            if(ON_AUDIO()) /* if on audio, jump to searching nwk */
            {
                HalAudioClose();
                Menu_handle_msg(MSG_INIT_NWK, NULL, 0);
                HAL_AlarmSet(MP_ALARM_INITNWK, MP_INIT_NWK_TIMEOUT);
            }
            else     /* Update the loggo to NO NETWORK when on Main */
            {
                Menu_RefreshNwkDisp();
            }
        }
        else
        {
            if(MP_RejoinTimes > MP_REJOINTIMES - 1)
            {
                Menu_UpdateSignal(0);
            }
            MP_start_timerEx(MP_TaskId, MP_PROBENWK_EVENT, MP_NWKTOLERANCE_TIME * 1000);
        }
        return events ^ MP_PROBENWK_EVENT;
    }

    if(events & MP_STOP_AUDIO_EVENT)
    {
        if(ON_AUDIO())
        {
            MP_SetPeerNum(NULL);
            MP_ResetAudio();
            Menu_handle_msg(MSG_INIT_MAIN, 0, 0);
        }
        return events ^ MP_STOP_AUDIO_EVENT;

    }
    if(events & MP_SCAN_AUDIO_EVENT)
    {
        if(ON_AUDIO())
        {
            MP_DevInfo.currentRssi = -120 ;
            MP_DevInfo.hascoordlink = false;
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SCAN_EVENT, 5);
        }
        return events ^ MP_SCAN_AUDIO_EVENT;
    }
    if(events & MP_PACKAGE_INFORM_EVENT)
    {
        if(Menu_rf_debug==true)
        {
            Menu_UpdatePackage(MP_packet_info.recv_cnt, MP_packet_info.err_cnt);
            if(NO_TIMER_AVAIL == osal_start_timerEx(MP_TaskId, MP_PACKAGE_INFORM_EVENT, 1000))
            {
                SystemReset();
            }
        }
        return events ^ MP_PACKAGE_INFORM_EVENT;
    }
    return 0;
}

/**************************************************************************************************
*
* @fn          MAC_CbackEvent
*
* @brief       This callback function sends MAC events to the application.
*              The application must implement this function.  A typical
*              implementation of this function would allocate an OSAL message,
*              copy the event parameters to the message, and send the message
*              to the application's OSAL event handler.  This function may be
*              executed from task or interrupt context and therefore must
*              be reentrant.
*
* @param       pData - Pointer to parameters structure.
*
* @return      None.
*
**************************************************************************************************/
void MAC_CbackEvent(macCbackEvent_t *pData)
{

    macCbackEvent_t *pMsg = NULL;

    uint8 len = MP_cbackSizeTable[pData->hdr.event];

    switch(pData->hdr.event)
    {
    case MAC_MLME_BEACON_NOTIFY_IND:

        len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
               MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
        if((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            /* Copy data over and pass them up */
            osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
            pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *)((uint8 *) pMsg + sizeof(macMlmeBeaconNotifyInd_t));
            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc, pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));
            pMsg->beaconNotifyInd.pSdu = (uint8 *)(pMsg->beaconNotifyInd.pPanDesc + 1);
            osal_memcpy(pMsg->beaconNotifyInd.pSdu, pData->beaconNotifyInd.pSdu, pData->beaconNotifyInd.sduLength);
        }
        break;

    case MAC_MCPS_DATA_IND:
        pMsg = pData;
        break;

    default:
        if((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            osal_memcpy(pMsg, pData, len);
        }
        break;
    }

    if(pMsg != NULL)
    {
        osal_msg_send(MP_TaskId, (uint8 *) pMsg);
    }
}

/**************************************************************************************************
*
* @fn      MAC_CbackCheckPending
*
* @brief   Returns the number of indirect messages pending in the application
*
* @param   None
*
* @return  Number of indirect messages in the application
*
**************************************************************************************************/
uint8 MAC_CbackCheckPending(void)
{
    return (0);
}

/**************************************************************************************************
*
* @fn      MP_DeviceStartup()
*
* @brief   Update the timer per tick
*
* @param   beaconEnable: TRUE/FALSE
*
* @return  None
*
**************************************************************************************************/
void MP_DeviceStartup()
{
    /* Setup exitAddr */
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &MP_DevInfo.ExitAddr);

    /* set channel */
    MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &MP_DevInfo.Channel);

    /* Setup shortAddr */
    uint16 ShortAddr =  MP_DevInfo.nmbr;
    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &ShortAddr);

    /* Setup PAN ID */
    uint16 MobilePanID = MOBILEPHONE_NWK_ADDR;
    MAC_MlmeSetReq(MAC_PAN_ID, &MobilePanID);

    /* This device is setup for Direct Message */
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &MP_MACTrue);

    /* Setup Coordinator short address */
    uint16 tmp = 0;
    MAC_MlmeSetReq(MAC_COORD_SHORT_ADDRESS, &tmp);

    uint8 SuperFrameOrder = 15;
    MAC_MlmeSetReq(MAC_SUPERFRAME_ORDER, &SuperFrameOrder);

    uint8 BeaconOrder = 15;
    MAC_MlmeSetReq(MAC_BEACON_ORDER, &BeaconOrder);

    /* change CCA param */
    uint8 maxFrameRetries = 4;
    MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);

    uint8 maxCsmaBackoff  = 5;
    MAC_MlmeSetReq(MAC_MAX_CSMA_BACKOFFS, &maxCsmaBackoff);

    uint8 minBe = 4;
    MAC_MlmeSetReq(MAC_MIN_BE, &minBe);

    uint8 maxBe = 6;
    MAC_MlmeSetReq(MAC_MAX_BE, &maxBe);

    /* set associated */
    bool associated = true;
    MAC_MlmeSetReq(MAC_ASSOCIATED_PAN_COORD, &associated);

    /* set src match */
    sAddr_t addr;
    addr.addrMode = SADDR_MODE_SHORT;
    addr.addr.shortAddr = 0;
    uint16 panId = 0xFFFF;

    MAC_SrcMatchEnable(SADDR_MODE_SHORT, 1);
    MAC_SrcMatchAddEntry(&addr, panId);

}

/**************************************************************************************************
*
* @fn      ParseAppFrame(const afIncomingMSGPacket_t* MSGpkt)
*
* @brief   Parse AppData. Parse the app data from radio.
*
* @param   MSGpkt -the incoming msg.
*
* @return  status.
*
**************************************************************************************************/
byte MP_ParseAppFrame(const sData_t data, uint16 panid, uint16 shortAddr, int8 rssi)
{
    if(data.p == NULL || data.len <= MAC_UTIL_ZIGBEEHDR_LEN) //min length is zigbee header + MSGType
    {
        return FAILURE;
    }
    uint8 clusterid = MAC_UTIL_GetClusterID(data); // mobile only process clusterid== mineapp_clusterid
    if(clusterid != MINEAPP_CLUSTERID)
    {
        return FAILURE;
    }
    sData_t pPayload = MAC_UTIL_RemoveHeader(data);

    if(pPayload.p == NULL || pPayload.len == 0)
    {
        return FAILURE;
    }

    switch(*(MSGType*)pPayload.p)
    {
    case VOICE:
    {
        app_Voice_t*p = (app_Voice_t *)pPayload.p;
        if(pPayload.len == (sizeof(app_Voice_t) + p->len))
        {
            MP_ParseVoice(p, rssi);
        }
        break;
    }
    case ZC_CMD_DOWN:
    {
        app_CmdDown_t *p = (app_CmdDown_t *)pPayload.p;

        if(pPayload.len == sizeof(app_CmdDown_t)
                && MP_termNmbrCheck(&p->srcnbr)
                && MP_termNmbrCheck(&p->dstnbr))
        {
            MP_ParseCmd(p);
        }
        break;
    }
    case SMS:
    {
        app_SMS_t*p = (app_SMS_t *)pPayload.p;
        if(pPayload.len == (sizeof(app_SMS_t)+p->len)
                && MP_termNmbrCheck(&p->nmbr))
        {
            MP_ParseSms(p);
        }
        break;
    }
    case TIMAC_MP_SCAN:
    {
        app_Timac_Mp_Scan_t*p = (app_Timac_Mp_Scan_t *)pPayload.p;
        if(pPayload.len == sizeof(app_Timac_Mp_Scan_t))
        {
            MP_ParseScan(p, panid, rssi);
        }
        break;
    }
    case TIMAC_JOIN_NOTIFY:
    {
        app_Timac_JoinNwk_t*p = (app_Timac_JoinNwk_t *)pPayload.p;
        if(pPayload.len == sizeof(app_Timac_JoinNwk_t)
                && MP_termNmbrCheck(&p->srcnbr))
        {
            MP_ParseJoin(p, panid, rssi);
        }
        break;
    }
    case TIME_SSIND:
    {
        app_TimessInd_t *p = (app_TimessInd_t*)pPayload.p;
        if(pPayload.len == sizeof(app_TimessInd_t))
        {
            MP_ParseTime(p);
        }
        break;
    }

    case MP_POLL_ACK:
    {
        app_MP_Poll_Ack_t*p = (app_MP_Poll_Ack_t *)pPayload.p;

        if(pPayload.len == sizeof(app_MP_Poll_Ack_t)
                && panid == MP_DevInfo.CoordPanID
                && shortAddr == 0
                && MP_termNmbrCheck(&p->nbr))
        {
            MP_ParsePoll(p, rssi);
        }
        break;
    }
    case ZB_REJOIN_NOTIFY:
    {
        app_RejoinNwk_t*p = (app_RejoinNwk_t*)pPayload.p;

        if(pPayload.len == sizeof(app_RejoinNwk_t)
                && panid == MP_DevInfo.CoordPanID
                && shortAddr == 0
                && MP_termNmbrCheck(&p->nbr))
        {
            MP_ParseRejoin(p);
        }
        break;
    }
    case REPORT_STATION:
    {
        app_Report_t*p = (app_Report_t*)pPayload.p;

        if(pPayload.len == sizeof(app_Report_t)
                && panid == MP_DevInfo.CoordPanID
                && p->dstnbr== MP_DevInfo.nmbr)
        {
            MP_ParseVersion(p);
        }
        break;
    }
#ifdef MENU_CONTACTUPDATE_CTL
    case ADDR_LIST:
    {
        app_AddrList_t*p = (app_AddrList_t*)pPayload.p;

        if(pPayload.len == sizeof(app_AddrList_t)
                &&( p->dstAddr==MP_DevInfo.nmbr)
                && MP_termNmbrCheck(&p->nbr))
        {
            Contact_Node Node;
            app_AddrListRsp_t *app_AddrListRsp;
            uint8 flag,len;
            char data[64];
            if (p->item=='\0')
            {
                ismore_contact=false;
                flag=ZSuccess;
            }
            else
            {
                flag=menu_Contact_ReadContactNum(&len);
                if(flag!=ZSuccess)return flag;
                strncpy((char*)data,(char*)p->item,62);
                if(len<MAX_CONTACT_NUM)
                {
                    strncpy((char*)Node.num.Nmbr,(char*)p->item+11,NMBRDIGIT);
                    strncpy((char*)Node.name,p->item,11);
                    flag=menu_Contact_AddContact(&Node);
                    if(flag!=ZSuccess)return flag;
                    if(len<MAX_CONTACT_NUM-1)
                    {
                        strncpy((char*)Node.num.Nmbr,(char*)p->item+11+(11+20),NMBRDIGIT);
                        strncpy((char*)Node.name,p->item+(11+20),11);
                        flag=menu_Contact_AddContact(&Node);
                        if(flag!=ZSuccess)return flag;
                    }
                    app_AddrListRsp.ret=ZSuccess;
                }
                else
                {
                    app_AddrListRsp.ret=1;
                }

            }
            if(flag==ZSuccess)
            {
                app_AddrListRsp.msgtype=ADDR_LIST_ACK;
                app_AddrListRsp.srcnmbr=p->dstAddr;
                app_AddrListRsp.dstAddr=p->srcAddr;
                app_AddrListRsp.blk=p->blk;
                flag=MP_SendSignalToCoord((uint8 *)&app_AddrListRsp, sizeof(app_AddrListRsp_t), true);
                return flag;
            }
        }
        break;
    }
#endif
    default:
        break;
    }
    return ZSUCCESS;
}


void MP_ParseVoice(const app_Voice_t*pVoice, int8 rssi)
{
    /* if the voice is not mine */
    /* do not check the srcaddr*/
    if(MP_DevInfo.nmbr != pVoice->dstnbr || pVoice->len == 0 || pVoice->len > MAC_MAX_FRAME_SIZE - MAC_UTIL_ZIGBEEHDR_LEN)
    {
        return ;
    }

    /*save the peer voice timetick */
    MP_AudioInfo.peer_tick = osal_GetSystemClock();

    /*save coord rssi */
    MP_DevInfo.currentRssi = rssi;
    MP_DevInfo.hascoordlink = true;

//#ifdef PACKAGE_INFORMATION
    if(Menu_rf_debug==true)
    {
        uint16 diff = pVoice->blk - MP_AudioInfo.seqnum;
        if(diff == 0 || diff > 20)
        {
            diff = 1;
        }
        MP_packet_info.err_cnt += diff - 1;
        MP_packet_info.recv_cnt++;
    }
//#endif

    if((pVoice->srcnbr != MP_AudioInfo.peernmbr)  //source have changed
            || !(MP_AudioInfo.seqnum - pVoice->blk < 3))  // source not changed and new seqnum is older than last played
    {

        /* if source is changed for continues 5 times, changeed peer number */
        static uint8 src_change_cnt;
        if(pVoice->srcnbr == MP_AudioInfo.peernmbr)
        {
            src_change_cnt = 0;
        }
        else
        {
            if(src_change_cnt++ >=5)
            {
                src_change_cnt = 0;
                termNbr_t termNbr;
                if(!MP_AudioInfo.IsTalkWithGateWay)  //only change number display when doing inside call
                {
                    _itoa(pVoice->srcnbr, termNbr.Nmbr, 10);
                    MP_SetPeerNum(&termNbr);
                    Menu_RefreshNwkDisp();
                }
            }
        }

        MP_AudioInfo.seqnum = pVoice->blk;

        //uint8 num = 0;
        FillAudioBuffer((uint8 *)(pVoice + 1), pVoice->len);

        MP_RejoinTimes = 0;
        osal_start_timerEx(MP_TaskId, MP_PROBENWK_EVENT, MP_NWKTOLERANCE_TIME * 1000);

    }

    //Handle exception.
    if(IS_IDLE() || ON_WAKE())
        /*
        *hangup party push down hangup key, send CMD_UP_CLOSE signal and return to idle status,
        *but ARM don't receive CMD_UP_CLOSE signal, and relay voice pkt again,
        *so hangup party need to send CMD_UP_CLOSE signal again.
        */
    {
        MP_SendCmd(CMD_UP_CLOSE, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
    }
    else if(ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
        /*Calling party loses CMD_DOWN_ACCEPT signal, so it needs to start audio and switch to audio status
        *until receving voice pkt.
        */
    {
        MP_AudioInfo.seqnum = 0;//need reset frameblk.
        MP_StartTalk();
        Menu_handle_msg(MSG_DIALING_SUCCESS, NULL, 0);
    }

    /* update signal on screen */

    if(signal_update_count)signal_update_count--;
    else
    {
        signal_update_count=10;
        Menu_UpdateSignal(MP_Rssi2Level(rssi));
    }

    /*FIXME:To avoid ambe of the other side not start absolutely, so close myself.
    */
    MP_ResetAudioStatus(MP_STOP_AUDIO_TIMEOUT);

}

void MP_ParseCmd(const app_CmdDown_t*pCmd)
{

    /* check if the dst is me*/
    if(strcmp(pCmd->dstnbr.Nmbr, MP_DevInfo.termNbr.Nmbr) != 0)
    {
        return;
    }

    /* clear package informations */
//#ifdef PACKAGE_INFORMATION
    if(Menu_rf_debug==true)
    {
        MP_packet_info.err_cnt = 0;
        MP_packet_info.recv_cnt = 0;
    }
//#endif

    switch(pCmd->cmd)
    {
    case CMD_DOWN_ACCEPT:
    {
        if(strcmp(pCmd->srcnbr.Nmbr, MP_AudioInfo.peer_termnbr.Nmbr) == 0) /* command from peer */
        {
            if(ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
            {
                MP_StartTalk();
                Menu_handle_msg(MSG_DIALING_SUCCESS, NULL, 0);
                MP_ResetAudioStatus(MP_STOP_AUDIO_TIMEOUT);
                MP_SetPeerNum(&pCmd->srcnbr);
            }
            else if(ON_AUDIO())   /* filter redundant*/
            {
            }
            else // I am not calling u
            {
                MP_SendCmd(CMD_UP_CLOSE, &pCmd->srcnbr, 0);
            }
        }
        else
        {
            MP_SendCmd(CMD_UP_CLOSE, &pCmd->srcnbr, 0);
        }
        break;
    }
    case CMD_DOWN_FOUND:
    {
        if(strcmp(pCmd->srcnbr.Nmbr, MP_AudioInfo.peer_termnbr.Nmbr) == 0) /* command from peer */
        {
            if(ON_CALLING() || ON_CALLINGWAIT())   /* found peer successfully */
            {
                SET_ON_FOUND();
                Hal_RingStart(RING_RING, OPENFLAG_ASONCE);
                MP_SetPeerNum(&pCmd->srcnbr);
            }
            else if(ON_FOUND() || ON_AUDIO())       /* filter redundant*/
            {
            }
            else // I am not calling u
            {
                MP_SendCmd(CMD_UP_CLOSE, &pCmd->srcnbr, 0);
            }
        }
        else //  another device
        {
            MP_SendCmd(CMD_UP_CLOSE, &pCmd->srcnbr, 0);
        }

        break;
    }
    case CMD_DOWN_NOTFOUND:
    {
        if(strcmp(pCmd->srcnbr.Nmbr, MP_AudioInfo.peer_termnbr.Nmbr) == 0) /* command from peer */
        {
            if(ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
            {
                if(!Hal_IsVoiceBellPlaying())
                {
                    Hal_StartVoiceBell(VOICEBELL_OUTOFREACH);
                }
            }
        }

        break;
    }
    case CMD_DOWN_BUSY:
    {
        if(strcmp(pCmd->srcnbr.Nmbr, MP_AudioInfo.peer_termnbr.Nmbr) == 0) /* command from peer */
        {
            if(ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
            {
                if(!Hal_IsVoiceBellPlaying())
                {
                    Hal_StartVoiceBell(VOICEBELL_BUSY);
                }
            }
        }
        break;
    }
    case CMD_DOWN_CALL:
    {
        if(ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND() || ON_CALLED() || ON_AUDIO())
        {
            if(strcmp(pCmd->srcnbr.Nmbr, MP_AudioInfo.peer_termnbr.Nmbr) == 0) //audio started, it is peer downcall retrans
            {
                MP_SendCmd(CMD_UP_FOUND, &pCmd->srcnbr, 0);
                HalResetBackLightEvent();
            }
            else     //a new comer and refuse its calling.
            {
                MP_SendCmd(CMD_UP_BUSY, &pCmd->srcnbr, 0);
            }
        }
        else if(IS_IDLE() || ON_WAKE())      //Receive CMD_DOWN_CALL in the first time.
        {
            MP_SendCmd(CMD_UP_FOUND, &pCmd->srcnbr, 0);

            /* if called, stop sending signals */
            MP_StopSignalRetrys();

            /* set peer num and cmd seqnum */
            MP_SetPeerNum(&pCmd->srcnbr);

            /* 1.0 do not have cmd sequm */
            MP_AudioInfo.cmdseqnum = 0;

            HalResetBackLightEvent();

            Hal_RingStart(RING_ANY, OPENFLAG_ASBELL);

            Menu_handle_msg(MSG_INCOMING_CALL, pCmd->srcnbr.Nmbr, 0);
        }
        break;
    }
    case CMD_DOWN_CLOSE:
    {
        if(strcmp(pCmd->srcnbr.Nmbr, MP_AudioInfo.peer_termnbr.Nmbr) == 0) /* command from peer */
        {
            if(ON_CALLED())
            {
                Menu_handle_msg(MSG_MISSED_CALL, NULL, 0);
                Hal_RingStop();
                MP_ResetAudio();
                MP_StopSignalRetrys();
            }
            else if(ON_AUDIO())
            {
                Menu_handle_msg(MSG_VOICE_FINISH, NULL, 0);
                Hal_RingStop();
                MP_ResetAudio();
                MP_StopSignalRetrys();
            }
            else if(ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
            {
                Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
                Hal_RingStop();
                MP_ResetAudio();
                MP_StopSignalRetrys();
            }
        }
        break;
    }
    }

}

void MP_ParseSms(app_SMS_t* pSms)
{
    if(IS_IDLE() || ON_WAKE())   //only receive sms when system is IDLE
    {
        uint8 rtn;
        uint8 count;
        uint8 idx;
        sms_saved_t sms;
        Contact_Node c_node;


        count = Get_SMS_Quantity();
        if(count < MAX_SMS_NUM)
        {
            sms.head.isReaded=false;
            sms.head.len=pSms->len;
            sms.head.seqnum=pSms->blk;
            strcpy((char*)sms.head.nmbr.Nmbr,(char *)pSms->nmbr.Nmbr);
            if(ZSuccess == menu_Contact_SearchContactByNum(&c_node, &idx, pSms->nmbr.Nmbr))
            {
                sms.head.Contect_item_L= *((uint8*)contact_item_L+idx);
            }
            else
            {
                sms.head.Contect_item_L=LIST_ITEM_NULL;
            }
            strncpy((char*)sms.content,(char *)pSms->nmbr.Nmbr+NMBRDIGIT,pSms->len);


            if(menu_SMS_Search_Inbox(pSms)==ZFailure)
            {
                rtn =menu_SMS_Add(&sms,SMSTYPE_INBOX); //Save_New_SMS(pSms);
                if(rtn == ZSUCCESS)
                {
                    Menu_Set_SMS_Full_Ring_Flag(TRUE);
                    Hal_RingStart(RING_ANY, OPENFLAG_ASSMS_POW);
                    HalResetBackLightEvent();
                    Menu_handle_msg(MSG_SMS_INCOMING, NULL, 0);
                }
            }

            app_SMSRsp_t app_SMSRsp;
            app_SMSRsp.msgtype = SMSRsp;
            app_SMSRsp.srcnmbr = pSms->nmbr;
            app_SMSRsp.blk = pSms->blk;

            MP_SendSignalToCoord((uint8 *)&app_SMSRsp, sizeof(app_SMSRsp), true);
            return;

        }
        else
        {
            if(Menu_Get_SMS_Full_Ring_Flag())
            {
                Menu_Set_SMS_Full_Ring_Flag(FALSE);
                Hal_RingStart(RING_ANY, OPENFLAG_ASSMS_POW);
                HalResetBackLightEvent();
            }
            Menu_handle_msg(MSG_SMS_INCOMING, NULL, 0);
        }
    }

}

void MP_ParseJoin(app_Timac_JoinNwk_t*p, uint16 panid, int8 rssi)
{
    if(MP_NwkInfo.nwkState == NWK_DETAIL_JOINASSOCING || MP_NwkInfo.nwkState == NWK_DETAIL_CELLSWITCHING
            || MP_NwkInfo.nwkState == NWK_DETAIL_INIT)
    {
        if(p->joinnwktype == APP_TIMAC_JOINNWK_SUCCESS)
        {
            //first time or being lost doesn't need send leave
            if(MP_DevInfo.CoordPanID != 0xFFFF
                    && panid != MP_DevInfo.CoordPanID)
            {
                app_Timac_LeaveNwk_t app_mpLeaveNwk;

                app_mpLeaveNwk.msgtype = TIMAC_LEAVE_NOTIFY;
                app_mpLeaveNwk.seqnum = MP_seqnums.join_seqnum;  // in cellswitch, the seqnum is the same as join
                app_mpLeaveNwk.srcnbr =  MP_DevInfo.termNbr;
                MP_SendSignalToCoord((uint8 *)&app_mpLeaveNwk, sizeof(app_mpLeaveNwk), true);
            }

            MP_DevInfo.CoordPanID = panid;
            MP_NwkInfo.nwkState = NWK_DETAIL_ENDDEVICE;
            MP_DevInfo.currentRssi = rssi;
            MP_DevInfo.hascoordlink = true;

            /* stop other cell switchs */
            osal_stop_timerEx(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT);
            osal_clear_event(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT);
            MP_ClearCellInfo();

            MP_Demand_Version();

            /*stop join timeout alarm*/
            HAL_AlarmUnSet(MP_ALARM_JOIN);

            /* jump menu when searching nwk */
            Menu_RefreshNwkDisp();

            /* start a poll immederately*/
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_POLL_EVENT, 5);

            if(Hal_AllowSleep() && MP_AudioInfo.retrying_bitmap == 0)
            {
                osal_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, 150);
            }
            /* start probe network */
            MP_start_timerEx(MP_TaskId, MP_PROBENWK_EVENT, MP_NWKTOLERANCE_TIME * 1000);

        }
        else    //terminate cell swith, all things maintained
        {
            //try next immediately
            osal_stop_timerEx(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT);
            MP_set_event(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT);
        }
    }
}
void MP_ParseTime(app_TimessInd_t* p)
{
    Date_t date  = GetDate();
    Time_t time;

    // if(date.year == TIME_INIT_YEAR)
    {
        time.hour = p->hour;
        time.min = p->minute;
        time.sec = p->second;
        SetTime(time);

        date.year = p->year;
        date.mon = p->month;
        date.day = p->day;
        SetDate(date);
        Menu_UpdateTime();
    }
}
void MP_ParsePoll(app_MP_Poll_Ack_t *p, int8 rssi)
{
    MP_DevInfo.currentRssi = rssi;
    MP_DevInfo.hascoordlink = true;
    switch(p->flag)
    {
    case MP_POLL_FLAG_NONE:
    {
        /* if hal allowed, go to sleep at once, else idle until WORK INTERVAL ends*/
        if(Hal_AllowSleep())
        {
            MP_start_timerEx(MP_Function_TaskID,MP_FUNC_PERIODIC_SLEEP_EVENT,15);
        }
        break;
    }
    case MP_POLL_FLAG_START:
    {
        Menu_handle_msg(MSG_POLL_START, NULL, 0);
        break;
    }
    case MP_POLL_FLAG_END:
    {
        Menu_handle_msg(MSG_POLL_END, NULL, 0);
        break;
    }
    case MP_POLL_FLAG_REJOIN:
    {
        /* set nwk false, and join automately*/
        MP_NwkInfo.nwkState = NWK_DETAIL_INIT;
        MP_DevInfo.CoordPanID = 0xFFFF;
        MP_DevInfo.currentRssi = MP_MIN_RSSI;
        Menu_RefreshNwkDisp();
        break;
    }
    }

    uint8 signalLevel = MP_Rssi2Level(rssi);
    Menu_UpdateSignal(signalLevel);
//#ifdef  RSSI_INFORMATION
    if(Menu_rf_debug==true)
    {
        Menu_UpdateRSSI(rssi);
        Menu_UpdateLinkFlag(true);
    }
//#endif
    MP_RejoinTimes = 0;
    osal_start_timerEx(MP_TaskId, MP_PROBENWK_EVENT, MP_NWKTOLERANCE_TIME * 1000);

}

void MP_ParseRejoin(app_RejoinNwk_t *p)
{
    if(strcmp(p->nbr.Nmbr, MP_DevInfo.termNbr.Nmbr) == 0)
    {
        /* set nwk false, and join automately*/
        MP_NwkInfo.nwkState = NWK_DETAIL_INIT;
        MP_DevInfo.CoordPanID = 0xFFFF;
        MP_DevInfo.currentRssi = MP_MIN_RSSI;
        Menu_RefreshNwkDisp();
    }
}

void MP_ParseVersion(app_Report_t *p)
{
    uint8 len = 0;
    if(p->len<=VERSION_MAX_CHARS)
    {
        len = p->len;
    }
    else
    {
        len = VERSION_MAX_CHARS;
    }
    osal_memcpy(version, p->version, len);
    version[len]=='\0';
    Version_isGet=TRUE;

}

/*********************************************************************
* @fn
* @brief
* @return
*********************************************************************/
static  uint8  MP_ReadExtAddr(void)
{
    uint8 rs = ZFailure;
    //osal_nv_item_init( ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL );
    //osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &MP_DevInfo.ExitAddr);

    HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET, (uint8 *)&MP_DevInfo.ExitAddr, HAL_FLASH_IEEE_SIZE);

    HAL_ASSERT(MP_DevInfo.ExitAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_PHONE);
    HAL_ASSERT(MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL] <= 0x1A && MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL] >= 0x0B);

    if(Menu_rf_debug==TRUE)
    {
        rs = osal_nv_item_init(ZCD_NV_SET_CHANLIST, sizeof(uint8), NULL);
        if(rs == ZSUCCESS)
        {
            if(ZSuccess != osal_nv_read(ZCD_NV_SET_CHANLIST, 0, sizeof(uint8), &MP_DevInfo.Channel))
            {
                MP_DevInfo.Channel = MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
            }

            if(MP_DevInfo.Channel > 0x1A || MP_DevInfo.Channel < 0x0B)
            {
                MP_DevInfo.Channel = MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
            }
        }
        else
        {
            MP_DevInfo.Channel = MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
            if(rs == NV_ITEM_UNINIT)
            {
                osal_nv_write(ZCD_NV_SET_CHANLIST, 0, sizeof(uint8), &MP_DevInfo.Channel);
            }
        }

        rs = osal_nv_item_init(ZCD_NV_SET_PANID, sizeof(uint16), NULL);
        if(rs == ZSUCCESS)
        {
            if(ZSuccess != osal_nv_read(ZCD_NV_SET_PANID, 0, sizeof(uint16), &MP_DevInfo.DesireCoordPanID))
            {
                MP_DevInfo.DesireCoordPanID = 0xFFFF;
            }
        }
        else
        {
            MP_DevInfo.DesireCoordPanID = 0xFFFF;
            if(rs== NV_ITEM_UNINIT)
            {
                osal_nv_write(ZCD_NV_SET_PANID, 0, sizeof(uint16), &MP_DevInfo.DesireCoordPanID);
            }
        }

        /* phone Nmbr */
        MP_NumberInfo_t numberInfo;
        rs = osal_nv_item_init(ZCD_NV_SET_PHONENUM, sizeof(MP_NumberInfo_t), NULL);
        if(rs == NV_ITEM_UNINIT)
        {
            numberInfo.setted = FALSE;  // default unseted
            numberInfo.phone_number = 0xFFFF;
            numberInfo.checkxor = HI_UINT16(numberInfo.phone_number)  ^ LO_UINT16(numberInfo.phone_number);
            osal_nv_write(ZCD_NV_SET_PHONENUM,0, sizeof(numberInfo), &numberInfo);
        }

        rs = osal_nv_read(ZCD_NV_SET_PHONENUM,0, sizeof(MP_NumberInfo_t), &numberInfo);

        if(rs == ZSUCCESS
                && numberInfo.setted==TRUE
                && numberInfo.phone_number != 0
                && numberInfo.phone_number != 0xFFFF
                && numberInfo.checkxor == HI_UINT16(numberInfo.phone_number) ^ LO_UINT16(numberInfo.phone_number))  // if setted in flash
        {
            MP_DevInfo.nmbr = numberInfo.phone_number;
        }
        else
        {
            MP_DevInfo.nmbr =  BUILD_UINT16(MP_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE],
                                            MP_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);
        }
     }
     else
     {
            MP_DevInfo.DesireCoordPanID = 0xFFFF;
            MP_DevInfo.Channel = MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
            MP_DevInfo.nmbr =  BUILD_UINT16(MP_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE],
                                            MP_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);
     }
    _itoa(MP_DevInfo.nmbr, MP_DevInfo.termNbr.Nmbr, 10);
    return 0;
}


void MP_ResetFrameblk(void)
{
    MP_AudioInfo.seqnum = 0;
}

void MP_ResetAudioStatus(uint16 timeout)
{
    if(ZSUCCESS!=osal_start_timerEx(MP_TaskId, MP_STOP_AUDIO_EVENT, timeout))
    {
        SystemReset();
    }
    if(ZSUCCESS!=osal_start_timerEx(MP_TaskId, MP_SCAN_AUDIO_EVENT, timeout/2))
    {
        SystemReset();
    }
}

bool MP_ParseScan(app_Timac_Mp_Scan_t* p, uint16 panid, int8 rssi)
{
    if(p->scantype != APP_SCAN_TYPE_ACK)
    {
        return false;
    }
    /* save the coord rssi */
    if(MP_IsNwkOn() && panid == MP_DevInfo.CoordPanID)
    {
        MP_DevInfo.currentRssi = rssi ;
        MP_DevInfo.hascoordlink = true;
        return true;
    }

    if(rssi <  MP_DevInfo.currentRssi + MP_CELL_DIFFRSSI)
    {
        return false;
    }

    /* find idx */
    int8 idx;
    if((idx = MP_SearchCellInfo(panid, rssi)) >= 0)
    {
        MP_CellInfo.CellInfo[idx].matched = true;
        MP_CellInfo.CellInfo[idx].cnt++;
        MP_CellInfo.CellInfo[idx].rssi = rssi;
        MP_CellInfo.CellInfo[idx].panid = panid;
    }

    return true;
}

int8  MP_SearchCellInfo(uint16 panid , int8 rssi)
{
    /* find if already have */
    for(uint8 i = 0; i < MP_MAC_MAXSCAN_RESULTS; i++)
    {

        if(MP_CellInfo.CellInfo[i].panid == panid)
        {
            return (int8)i;
        }
    }

    /* find an empty */
    for(uint8 i = 0; i < MP_MAC_MAXSCAN_RESULTS; i++)
    {
        if(MP_CellInfo.CellInfo[i].cnt == 0)
        {
            return (int8)i;
        }
    }
    MP_SortCellInfo();
    if(rssi>MP_CellInfo.CellInfo[MP_MAC_MAXSCAN_RESULTS-1].rssi)
    {
        return (int8)(MP_MAC_MAXSCAN_RESULTS-1);
    }
    return -1;
}
