/**************************************************************************************************
Filename:       MobilePhone.c
Revised:        $Date: 2011/02/23 19:09:31 $
Revision:       $Revision: 1.24 $

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
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_lcd.h"
#include "hal_audio.h"
#include "hal_assert.h"
#include "hal_alarm.h"
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
#include "stringUtil.h"
#include "WatchDogUtil.h"
#include "numtrans.h"

/* Application */
#include "MobilePhone.h"
#include "MobilePhone_Function.h"
#include "MobilePhone_global.h"
#include "MobilePhone_Menulib.h"
#include "Menulib_Nv.h"
/**************************************************************************************************
*                                           Constant
**************************************************************************************************/

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
/**************************************************************************************************
*                                         Typedefs
**************************************************************************************************/

#ifdef PACKAGE_INFORMATION
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

#ifdef PACKAGE_INFORMATION
static MP_packet_info_t MP_packet_info;
#endif

static uint8   MP_RejoinTimes;

//8*2+1
static char MP_numstr[21];

uint8   MP_VoiceBuf[VOICE_IDX_THRESHOLD *VOICE_PER_RAW_DATA_LEN];

/*************************************************************************************************
*MACROS
*/

/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
static void MP_DeviceStartup(void);
static  uint8  MP_ReadExtAddr(void );
static byte MP_ParseAppFrame(const sData_t data, uint16 panid, uint16 shortAddr , uint8 LinkQuality);
static void MP_ParseVoice(const app_mpVoice_t *pVoice, uint8 LinkQuality);
static void MP_ParseCmd(const app_mpCmd_t *pCmd);
static void MP_ParseSms( app_mpSMS_t *pSms);
static void MP_ParseJoin(app_mpJoinNwk_t *p, uint16 panid, uint8 LinkQuality);
static void MP_ParseTime(app_mpTime_t *p);
static void MP_ParsePoll(app_mpPoll_t *p, uint8 LinkQuality);
static bool MP_CellSwitchCondition(uint16 panid, int8 rssi);
static int8  MP_SearchCellInfo(uint16 panid);
static bool  MP_SilenceSignCheck(uint16 sign, uint8 datasize);
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
    MP_DevInfo.armid = APP_INVALIDARMADDR;
    MP_DevInfo.currentRssi = MP_MIN_RSSI;
    MP_DevInfo.DesireCoordPanID = 0xFFFF;

    MP_AudioInfo.peernmbr = 0;
    MP_AudioInfo.seqnum = 0;
    MP_AudioInfo.cmdseqnum = 0;

    MP_NwkInfo.nwkState = NWK_DETAIL_INIT;

    MP_ReadExtAddr();
    MP_DeviceStartup();

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    StartWatchDog(DOGTIMER_INTERVAL_1S);
    osal_set_event(MP_TaskId, MP_FEEDDOG_EVENT);
#endif

    /* Audio */
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
    HalAudioInit();
#endif


#ifdef PACKAGE_INFORMATION
    MP_start_timerEx(MP_TaskId, MP_PACKAGE_INFORM_EVENT, 1000);
#endif

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
    if (events & MP_FEEDDOG_EVENT)
    {
        if (NO_TIMER_AVAIL == osal_start_timerEx(MP_TaskId, MP_FEEDDOG_EVENT, 300))
        {
            SystemReset();
        }
        FeedWatchDog();
        return events ^ MP_FEEDDOG_EVENT;
    }
#endif

    if (events & SYS_EVENT_MSG)
    {
        uint8 *pMsg;
        while ((pMsg = osal_msg_receive(MP_TaskId)) != NULL)
        {
            macCbackEvent_t *pData = (macCbackEvent_t *)pMsg;
            switch ( *pMsg )
            {
            case MAC_MLME_COMM_STATUS_IND:
                break;

            case MAC_MCPS_DATA_CNF:
                pData = (macCbackEvent_t *) pMsg;
                mac_msg_deallocate((uint8 **)&pData->dataCnf.pDataReq);
                break;

            case MAC_MCPS_DATA_IND:
            {
                pData = (macCbackEvent_t *)pMsg;
                MP_ParseAppFrame(pData->dataInd.msdu, pData->dataInd.mac.srcPanId, pData->dataInd.mac.srcAddr.addr.shortAddr, pData->dataInd.mac.mpduLinkQuality);

                break;
            }
            }

            /* Deallocate */
            //MAC_MCPS_DATA_CNF is handled in hal_drivers
            mac_msg_deallocate((uint8 **)&pMsg);
        }

        return events ^ SYS_EVENT_MSG;
    }
    /* resend dialup command */
    if (events & MP_DIALUP_RETRY_EVENT)
    {
        /* continues to send dial up until found, accepted, or start voice  */
        if(MP_AudioInfo.peernmbr != MP_SHORT_INVALIDNMMBR
                &&  (ON_CALLING() || ON_CALLINGWAIT()))
        {
            MP_AudioInfo.retrying_bitmap |= MP_DIALUP_RETRY_BIT;
            MP_SendCmd(MP_UP_DIALUP, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
            MP_start_timerEx(MP_TaskId, MP_DIALUP_RETRY_EVENT, MP_SIGNAL_RETRY_TIME);
        }
        else
        {
            MP_AudioInfo.retrying_bitmap &= ~MP_DIALUP_RETRY_BIT;
        }
        return (events ^ MP_DIALUP_RETRY_EVENT);
    }

    /* resend accept command */
    if (events & MP_ACCEPT_RETRY_EVENT)
    {
        if(++MP_AudioInfo.accept_cnt < 3 && MP_AudioInfo.peernmbr != MP_SHORT_INVALIDNMMBR)
        {
            MP_SendCmd(MP_UP_ACCEPT, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
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
    if (events & MP_CLOSE_RETRY_EVENT)
    {
        if(++MP_AudioInfo.close_cnt < 3 && MP_AudioInfo.peernmbr != MP_SHORT_INVALIDNMMBR)
        {
            MP_SendCmd(MP_UP_CLOSE, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
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

    if (events & MP_PROBENWK_EVENT)
    {
        if ( ++MP_RejoinTimes > MP_REJOINTIMES)
        {
            MP_RejoinTimes = 0;

            MP_NwkInfo.nwkState = NWK_DETAIL_INIT;
            MP_DevInfo.CoordPanID = 0xFFFF;
            MP_DevInfo.armid = APP_INVALIDARMADDR;
            MP_DevInfo.currentRssi = MP_MIN_RSSI;

            if(ON_AUDIO()) /* if on audio, jump to searching nwk */
            {
                HalAudioClose();
                Menu_handle_msg(MSG_INIT_NWK, NULL, 0);
                HAL_AlarmSet ( MP_ALARM_INITNWK, MP_INIT_NWK_TIMEOUT );
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

    if (events & MP_STOP_AUDIO_EVENT)
    {
        if (ON_AUDIO())
        {
            MP_SetPeerNum(NULL);
            MP_ResetAudio();
            Menu_handle_msg(MSG_INIT_MAIN, 0, 0);
        }
        return events ^ MP_STOP_AUDIO_EVENT;

    }
#ifdef PACKAGE_INFORMATION
    if(events & MP_PACKAGE_INFORM_EVENT)
    {
        Menu_UpdatePackage(MP_packet_info.recv_cnt, MP_packet_info.err_cnt);
        if (NO_TIMER_AVAIL == osal_start_timerEx(MP_TaskId, MP_PACKAGE_INFORM_EVENT, 1000))
        {
            SystemReset();
        }
        return events ^ MP_PACKAGE_INFORM_EVENT;
    }
#endif
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

    switch (pData->hdr.event)
    {
    case MAC_MLME_BEACON_NOTIFY_IND:

        len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
               MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
        if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            /* Copy data over and pass them up */
            osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
            pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *) ((uint8 *) pMsg + sizeof(macMlmeBeaconNotifyInd_t));
            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc, pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));
            pMsg->beaconNotifyInd.pSdu = (uint8 *) (pMsg->beaconNotifyInd.pPanDesc + 1);
            osal_memcpy(pMsg->beaconNotifyInd.pSdu, pData->beaconNotifyInd.pSdu, pData->beaconNotifyInd.sduLength);
        }
        break;

    case MAC_MCPS_DATA_IND:
        pMsg = pData;
        break;

    default:
        if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            osal_memcpy(pMsg, pData, len);
        }
        break;
    }

    if (pMsg != NULL)
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
    MAC_MlmeSetReq ( MAC_MAX_FRAME_RETRIES, &maxFrameRetries );

    uint8 maxCsmaBackoff  = 5;
    MAC_MlmeSetReq ( MAC_MAX_CSMA_BACKOFFS, &maxCsmaBackoff );

    uint8 minBe = 4;
    MAC_MlmeSetReq ( MAC_MIN_BE, &minBe);

    uint8 maxBe = 6;
    MAC_MlmeSetReq ( MAC_MAX_BE, &maxBe );

    /* set associated */
    bool associated = true;
    MAC_MlmeSetReq ( MAC_ASSOCIATED_PAN_COORD, &associated );

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
byte MP_ParseAppFrame(const sData_t data, uint16 panid, uint16 shortAddr, uint8 LinkQuality)
{
    app_header_t *pHeader  = (app_header_t * )(data.p);
    uint8 *pPayload = (uint8 *) (pHeader + 1);

    if( pHeader->protocoltype != APP_PROTOCOL_TYPE_MOBILE  || data.len < sizeof(app_header_t) + pHeader->len)
    {
        return FAILURE;
    }

    switch (pHeader->msgtype)
    {
    case MP_VOICE:
    {
        app_mpVoice_t *p = (app_mpVoice_t *)pPayload;
        if(data.len >= sizeof(app_header_t) + sizeof(app_mpVoice_t) + p->len)
        {
            MP_ParseVoice(p, LinkQuality);
        }
        break;
    }
    case MP_CMD_DOWN:
    {
        if(data.len >= sizeof(app_header_t) + sizeof(app_mpCmd_t) )
        {
            MP_ParseCmd((app_mpCmd_t *)pPayload);
        }
        break;
    }
    case MP_SMS:
    {
        app_mpSMS_t *p = (app_mpSMS_t *)pPayload;
        if(data.len >= sizeof(app_header_t) + sizeof(app_mpSMS_t) + p->len
                &&   p->smstype == APP_MP_SMSTYPE_CONTENT &&  num_isequal(&p->dstnbr, &MP_DevInfo.termNbr))
        {
            MP_ParseSms(p);
        }
        break;
    }
    case MP_SCAN:
    {
        mp_Scan_t *p = (mp_Scan_t *)pPayload;
        if(data.len >= sizeof(app_header_t) + sizeof(mp_Scan_t)
                &&  MP_ScanInfo.isscaning && p->scantype == APP_SCAN_TYPE_ACK && shortAddr == 0)
        {
            int8 rssi = CONV_LQI_TO_RSSI(LinkQuality);
            MP_CellSwitchCondition(panid, rssi);
            //Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
        }
        break;
    }
    case MP_JOIN_NOTIFY:
    {
        app_mpJoinNwk_t *p = (app_mpJoinNwk_t *)pPayload;
        if(data.len >=  sizeof(app_header_t) + sizeof(app_mpJoinNwk_t)
                && p->hdr.dstaddr == MP_DevInfo.nmbr)
        {
            MP_ParseJoin(p, panid, LinkQuality);
        }
        break;
    }
    case MP_TIME:
    {
        Date_t CurrentDate = GetDate();
        app_mpTime_t *p = (app_mpTime_t * )pPayload;
        if(data.len >=  sizeof(app_header_t) + sizeof(app_mpTime_t)
                && CurrentDate.year == TIME_INIT_YEAR)
        {
            MP_ParseTime(p);
        }
        break;
    }

    case MP_POLL:
    {
        app_mpPoll_t *p = (app_mpPoll_t *)pPayload;

        if(data.len >=  sizeof(app_header_t) + sizeof(app_mpPoll_t)
                && p->polltype == APP_MPPOLL_TYPE_ACK
                && panid == MP_DevInfo.CoordPanID && shortAddr == 0
                && p->hdr.dstaddr == MP_DevInfo.nmbr)
        {
            MP_ParsePoll(p, LinkQuality);
        }
        break;
    }
    default:
        break;
    }
    return ZSUCCESS;
}

void MP_ParseVoice(const app_mpVoice_t *pVoice, uint8 LinkQuality)
{
    /* if the voice is not mine or the peer is not my peer  */
    if(MP_DevInfo.nmbr != pVoice->hdr.dstaddr || pVoice->hdr.srcaddr != MP_AudioInfo.peernmbr)
    {
        return ;
    }

    /*save the peer voice timetick */
    MP_AudioInfo.peer_tick = osal_GetSystemClock();

    /*save coord rssi */
    MP_DevInfo.currentRssi = CONV_LQI_TO_RSSI(LinkQuality);
    MP_DevInfo.hascoordlink = true;

#ifdef PACKAGE_INFORMATION
    uint16 diff = pVoice->seqnum - MP_AudioInfo.seqnum;
    if(diff == 0 || diff > 20)
    {
        diff = 1;
    }
    MP_packet_info.err_cnt += diff - 1;
    MP_packet_info.recv_cnt++;
#endif

    //filter duplicated frames.
    if (pVoice->seqnum > MP_AudioInfo.seqnum
            || (pVoice->seqnum != MP_AudioInfo.seqnum &&  MP_AudioInfo.seqnum > 245 && pVoice->seqnum < 10))
    {
        if(MP_SilenceSignCheck(pVoice->silencemap, pVoice->len) == false)
        {
            return ;
        }

        MP_AudioInfo.seqnum = pVoice->seqnum;

        uint8 num = 0;
        uint8 *pvoicedata = (uint8 *)(pVoice + 1);

        for ( uint8 i = 0; i < VOICE_IDX_THRESHOLD; i++ )
        {
            if ( (pVoice->silencemap >> i) & 0x01 == 1)
            {
                memcpy(MP_VoiceBuf + i * VOICE_PER_RAW_DATA_LEN, pvoicedata + num * VOICE_PER_RAW_DATA_LEN, VOICE_PER_RAW_DATA_LEN);
                num++;
            }
            else
            {

                memcpy(MP_VoiceBuf + i * VOICE_PER_RAW_DATA_LEN, SilenceFrame, VOICE_PER_RAW_DATA_LEN);

            }
        }
    }

    FillAudioBufferFlush ( MP_VoiceBuf, VOICE_IDX_THRESHOLD * VOICE_PER_RAW_DATA_LEN );

    MP_RejoinTimes = 0;
    osal_start_timerEx(MP_TaskId, MP_PROBENWK_EVENT, MP_NWKTOLERANCE_TIME * 1000);


    //Handle exception.
    if (IS_IDLE() || ON_WAKE())
        /*
        *hangup party push down hangup key, send CMD_UP_CLOSE signal and return to idle status,
        *but ARM don't receive CMD_UP_CLOSE signal, and relay voice pkt again,
        *so hangup party need to send CMD_UP_CLOSE signal again.
        */
    {
        MP_SendCmd(MP_UP_CLOSE, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
    }
    else if (ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
        /*Calling party loses CMD_DOWN_ACCEPT signal, so it needs to start audio and switch to audio status
        *until receving voice pkt.
        */
    {
        MP_AudioInfo.seqnum = 0;//need reset frameblk.
        MP_StartTalk();
        Menu_handle_msg(MSG_DIALING_SUCCESS, NULL, 0);
    }

    /*FIXME:To avoid ambe of the other side not start absolutely, so close myself.
    */
    MP_ResetAudioStatus(MP_STOP_AUDIO_TIMEOUT);

}

void MP_ParseCmd(const app_mpCmd_t *pCmd)
{
    /* clear package informations */
#ifdef PACKAGE_INFORMATION
    MP_packet_info.err_cnt = 0;
    MP_packet_info.recv_cnt = 0;
#endif

    /* dst num must be me */
    if (!num_isequal(&pCmd->dstnbr,  &MP_DevInfo.termNbr))
    {
        return ;
    }

    switch(pCmd->cmdtype)
    {
    case MP_DOWN_ACCEPT:
    {
        if(num_isequal(&MP_AudioInfo.peer_termnbr, &pCmd->srcnbr))  /* command from peer */
        {
            if (ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
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
                MP_SendCmd(MP_UP_CLOSE, &pCmd->srcnbr, pCmd->seqnum);
            }
        }
        else
        {
            MP_SendCmd(MP_UP_CLOSE, &pCmd->srcnbr, pCmd->seqnum);
        }
        break;
    }
    case MP_DOWN_FOUND:
    {
        if(num_isequal(&MP_AudioInfo.peer_termnbr, &pCmd->srcnbr))  /* command from peer */
        {
            if (ON_CALLING() || ON_CALLINGWAIT())  /* found peer successfully */
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
                MP_SendCmd(MP_UP_CLOSE, &pCmd->srcnbr, pCmd->seqnum);
            }
        }
        else //  another device
        {
            MP_SendCmd(MP_UP_CLOSE, &pCmd->srcnbr, pCmd->seqnum);
        }

        break;
    }
    case MP_DOWN_NOTFOUND:
    {
        if(num_isequal(&MP_AudioInfo.peer_termnbr, &pCmd->srcnbr))  /* command from peer */
        {
            if (ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
            {
                if(!Hal_IsVoiceBellPlaying())
                {
                    Hal_StartVoiceBell(VOICEBELL_OUTOFREACH);
                }
            }
        }

        break;
    }
    case MP_DOWN_BUSY:
    {
        if(num_isequal(&MP_AudioInfo.peer_termnbr, &pCmd->srcnbr))  /* command from peer */
        {
            if (ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
            {
                if(!Hal_IsVoiceBellPlaying())
                {
                    Hal_StartVoiceBell(VOICEBELL_BUSY);
                }
            }
        }
        break;
    }
    case MP_DOWN_CALL:
    {
        if (ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND() || ON_CALLED() || ON_AUDIO())
        {
            if(num_isequal(&pCmd->srcnbr, &MP_AudioInfo.peer_termnbr)) //audio started, it is peer downcall retrans
            {
                MP_SendCmd(MP_UP_FOUND, &pCmd->srcnbr, pCmd->seqnum);
                HalResetBackLightEvent();
            }
            else     //a new comer and refuse its calling.
            {
                MP_SendCmd(MP_UP_BUSY, &pCmd->srcnbr, pCmd->seqnum);
            }
        }
        else if (IS_IDLE() || ON_WAKE())     //Receive CMD_DOWN_CALL in the first time.
        {
            MP_SendCmd(MP_UP_FOUND, &pCmd->srcnbr, pCmd->seqnum);

            /* if called, stop sending signals */
            MP_StopSignalRetrys();

            /* set peer num and cmd seqnum */
            MP_SetPeerNum(&pCmd->srcnbr);
            MP_AudioInfo.cmdseqnum = pCmd->seqnum;

            HalResetBackLightEvent();

            Hal_RingStart(RING_ANY, OPENFLAG_ASBELL);

            num_term2str(MP_numstr, &pCmd->srcnbr);
            Menu_handle_msg(MSG_INCOMING_CALL, MP_numstr, 0);
        }
        break;
    }
    case MP_DOWN_CLOSE:
    {
        if(num_isequal(&MP_AudioInfo.peer_termnbr, &pCmd->srcnbr))  /* command from peer */
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
                Hal_RingStop ();
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
void MP_ParseSms( app_mpSMS_t *pSms)
{
    if(IS_IDLE() || ON_WAKE())   //only receive sms when system is IDLE
    {
        uint8 rtn;
        uint8 count;
        count = Get_SMS_Quantity();
        if(count < MAX_SMS_NUM)
        {
            Menu_Set_SMS_Full_Ring_Flag(TRUE);
            Hal_RingStart(RING_ANY, OPENFLAG_ASSMS);
            HalResetBackLightEvent();
            Menu_handle_msg(MSG_SMS_INCOMING, NULL, 0);

            rtn = Save_New_SMS(pSms);
            if(rtn == ZSUCCESS)
            {
                app_mpSMS_t app_mpSMS;
                app_mpSMS.srcnbr = pSms->dstnbr;
                app_mpSMS.dstnbr = pSms->srcnbr;
                app_mpSMS.smstype = APP_MP_SMSTYPE_ACK;
                app_mpSMS.seqnum = pSms->seqnum;
                app_mpSMS.len = 0;
                MP_SendSignalToCoord((uint8 *)&app_mpSMS, sizeof(app_mpSMS), MP_SMS, true);
                return;
            }
        }
        else
        {
            if(Menu_Get_SMS_Full_Ring_Flag())
            {
                Menu_Set_SMS_Full_Ring_Flag(FALSE);
                Hal_RingStart(RING_ANY, OPENFLAG_ASSMS);
                HalResetBackLightEvent();
            }
            Menu_handle_msg(MSG_SMS_INCOMING, NULL, 0);
        }
    }

}

void MP_ParseJoin(app_mpJoinNwk_t *p, uint16 panid, uint8 LinkQuality)
{
    if(MP_NwkInfo.nwkState == NWK_DETAIL_JOINASSOCING || MP_NwkInfo.nwkState == NWK_DETAIL_CELLSWITCHING
            || MP_NwkInfo.nwkState == NWK_DETAIL_INIT)
    {
        if(p->joinnwktype == APP_MP_JOINNWK_SUCCESS)
        {
            //first time or being lost doesn't need send leave
            if( MP_DevInfo.CoordPanID != 0xFFFF
                    && panid != MP_DevInfo.CoordPanID)
            {
                app_mpLeaveNwk_t app_mpLeaveNwk;
                app_mpLeaveNwk.hdr.srcaddr = MP_DevInfo.nmbr;
                app_mpLeaveNwk.hdr.dstaddr = APP_ARMSHORTADDR;
                app_mpLeaveNwk.seqnum = MP_seqnums.join_seqnum;  // in cellswitch, the seqnum is the same as join
                MP_SendSignalToCoord((uint8 *)&app_mpLeaveNwk, sizeof(app_mpLeaveNwk), MP_LEAVE_NOTIFY, true);
            }

            MP_DevInfo.armid = p->armid;
            MP_DevInfo.CoordPanID = panid;
            MP_NwkInfo.nwkState = NWK_DETAIL_ENDDEVICE;
            MP_DevInfo.currentRssi = CONV_LQI_TO_RSSI(LinkQuality);
            MP_DevInfo.hascoordlink = true;

            /* stop other cell switchs */
            osal_stop_timerEx(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT);
            osal_unset_event(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT);
            MP_ClearCellInfo();

            /*stop join timeout alarm*/
            HAL_AlarmUnSet(MP_ALARM_JOIN);

            /* jump menu when searching nwk */
            Menu_RefreshNwkDisp();

            /* start a poll immederately*/
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_POLL_EVENT, 5);

            /* start probe network */
            MP_start_timerEx(MP_TaskId, MP_PROBENWK_EVENT, MP_NWKTOLERANCE_TIME * 1000);

        }
        else if(p->joinnwktype == APP_MP_JOINNWK_DENIED)     //terminate cell swith, all things maintained
        {
            //try next immediately
            osal_stop_timerEx(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT);
            osal_set_event(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT);
        }
    }
}
void MP_ParseTime(app_mpTime_t *p)
{
    Date_t date;
    Time_t time;
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
void MP_ParsePoll(app_mpPoll_t *p, uint8 LinkQuality)
{
    MP_DevInfo.currentRssi = CONV_LQI_TO_RSSI(LinkQuality);
    MP_DevInfo.hascoordlink = true;
    switch(p->flag)
    {
    case APP_MPPOLL_FLAG_NONE:
    {
        /* if hal allowed, go to sleep at once, else idle until WORK INTERVAL ends*/
        if(Hal_AllowSleep())
        {
            //MP_start_timerEx(MP_Function_TaskID,MP_FUNC_PERIODIC_SLEEP_EVENT,15);
        }
        break;
    }
    case APP_MPPOLL_FLAG_START:
    {
        Menu_handle_msg(MSG_POLL_START, NULL, 0);
        break;
    }
    case APP_MPPOLL_FLAG_END:
    {
        Menu_handle_msg(MSG_POLL_END, NULL, 0);
        break;
    }
    case APP_MPPOLL_FLAG_REJOIN:
    {
        /* set nwk false, and join automately*/
        MP_NwkInfo.nwkState = NWK_DETAIL_INIT;
        MP_DevInfo.CoordPanID = 0xFFFF;
        MP_DevInfo.armid = APP_INVALIDARMADDR;
        MP_DevInfo.currentRssi = MP_MIN_RSSI;
        Menu_RefreshNwkDisp();
        break;
    }
    }


    uint8 signalLevel = MP_LQI2Level(LinkQuality);
    Menu_UpdateSignal(signalLevel);
#ifdef	RSSI_INFORMATION
    Menu_UpdateRSSI(LinkQuality);
    Menu_UpdateLinkFlag(true);
#endif
    MP_RejoinTimes = 0;
    osal_start_timerEx(MP_TaskId, MP_PROBENWK_EVENT, MP_NWKTOLERANCE_TIME * 1000);

}
/*********************************************************************
* @fn
* @brief
* @return
*********************************************************************/
static  uint8  MP_ReadExtAddr(void )
{
    osal_nv_item_init( ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL );
    osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &MP_DevInfo.ExitAddr);

    HAL_ASSERT(MP_DevInfo.ExitAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_PHONE);
    HAL_ASSERT(MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL] <= 0x1A && MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL] >= 0x0B);

    /* Channel */
#ifdef MENU_RF_DEBUG
    uint8 rs = osal_nv_item_init( ZCD_NV_SET_CHANLIST, sizeof(uint8), NULL);
    if(rs == NV_ITEM_UNINIT)
    {
        MP_DevInfo.Channel = MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
        osal_nv_write( ZCD_NV_SET_CHANLIST, 0, sizeof(uint8), &MP_DevInfo.Channel);
    }
    else if(rs == ZSUCCESS)
    {
        if(ZSuccess != osal_nv_read( ZCD_NV_SET_CHANLIST, 0, sizeof(uint8), &MP_DevInfo.Channel))
        {
            MP_DevInfo.Channel = MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
        }
    }
    else
    {
        MP_DevInfo.Channel = MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
    }
#else
    MP_DevInfo.Channel = MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
#endif

    /* Coord pan ID */
#ifdef MENU_RF_DEBUG
    rs = osal_nv_item_init( ZCD_NV_SET_PANID, sizeof(uint16), NULL);

    if(rs == NV_ITEM_UNINIT)
    {
        MP_DevInfo.DesireCoordPanID = 0xFFFF;
        osal_nv_write( ZCD_NV_SET_PANID, 0, sizeof(uint16), &MP_DevInfo.DesireCoordPanID);
    }
    else if(rs == ZSUCCESS)
    {
        if(ZSuccess != osal_nv_read( ZCD_NV_SET_PANID, 0, sizeof(uint16), &MP_DevInfo.DesireCoordPanID))
        {
            MP_DevInfo.DesireCoordPanID = 0xFFFF;
        }
    }
    else
    {
        MP_DevInfo.DesireCoordPanID = 0xFFFF;
    }

#else
    MP_DevInfo.DesireCoordPanID = 0xFFFF;
#endif

    /* Nmbr */
    MP_DevInfo.nmbr = BUILD_UINT16(MP_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE],
                                   MP_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);

    _itoa(MP_DevInfo.nmbr, MP_numstr, 10);
    num_str2term(&MP_DevInfo.termNbr, MP_numstr);
    return 0;
}


void MP_ResetFrameblk(void)
{
    MP_AudioInfo.seqnum = 0;
}

void MP_ResetAudioStatus(uint16 timeout)
{
    osal_start_timerEx(MP_TaskId, MP_STOP_AUDIO_EVENT, timeout);
}

bool MP_CellSwitchCondition(uint16 panid, int8 rssi)
{
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
    if((idx = MP_SearchCellInfo(panid)) >= 0)
    {
        MP_CellInfo.CellInfo[idx].matched = true;
        MP_CellInfo.CellInfo[idx].cnt++;
        MP_CellInfo.CellInfo[idx].rssi = rssi;
        MP_CellInfo.CellInfo[idx].panid = panid;
    }

    return true;
}

int8  MP_SearchCellInfo(uint16 panid)
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
    return -1;
}

bool  MP_SilenceSignCheck(uint16 sign, uint8 datasize)
{
    uint8 validbits = 0;
    for(uint8 i = 0; i < VOICE_IDX_THRESHOLD; i++)
    {
        if(sign >> i & 0x01)
        {
            validbits++;
        }
    }
    return (datasize == VOICE_PER_RAW_DATA_LEN * validbits) ? true : false;
}

