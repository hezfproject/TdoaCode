/**************************************************************************************************
Filename:       MineApp_MP.c
Revised:        $Date: 2011/07/22 17:38:35 $
Revision:       $Revision: 1.143 $

Description:    Mine Application of mobile phone.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/

#include "MineApp.h"
#include "mac_pib.h"
#include "MineApp_MP_Menu.h"
#include "string.h"
#ifdef NEW_MENU_LIB
#include "MineApp_MenuLib.h"
#include "MineApp_MP_Function.h"
//#include "MenuLib_global.h"
#endif
#include "MineApp_Global.h"
#include "MineApp_Local.h"

#include "App_cfg.h"
#include "MacUtil.h"
#include "Delay.h"
#include "WatchDogUtil.h"
#include "TimeUtil.h"

#if !defined( NONWK )
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#endif
#include "ZGlobals.h"

#include "OSAL.h"

#include "OnBoard.h"

#include "hal_mcu.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_drivers.h"

#include "hal_audio.h"
#include "KeyAudioISR.h"
#include "lcd_interface.h"
#include "StringUtil.h"

/*************************************************************************************************
*CONSTANTS
*/
/*************************************************************************************************
*MACROS
*/
//status for AF_DATA_CONFIRM_CMD to identify leave nwk state.
#define CONFIRM_NORMAL        0
#define CONFIRM_NWKTRANSFER   1

//MUST be times of BLAST_SIGNALSTRENGTH_PERIOD
#ifndef TOLERANCE_TIME
#define TOLERANCE_TIME (BLAST_SIGNALSTRENGTH_PERIOD*4)
#endif

#ifndef MINEAPP_REJOINTIMES
#define MINEAPP_REJOINTIMES (TIMEOUT_REJOIN_NWK/(TOLERANCE_TIME/1000))
#endif

#ifndef MINEAPP_RETRY_TIMES
#define MINEAPP_RETRY_TIMES 1
#endif

/*********************************************************************
* TYPEDEFS
*/

/*********************************************************************
* GLOBAL VARIABLES
*/
uint8 MineApp_TaskID;

/*********************************************************************
* LOCAL VARIABLES
*/
static __code const uint8 LQI_table[] =
{
    28,     // RSSI: -80
    53,     // RSSI: -70
    104,   // RSSI: -50
    154,    // RSSI: -30
};

static NodeAddr nodeAddr;
static uint16 MineApp_RejoinTimes = 0;
static uint8 MineApp_LeaveNWKNotify = CONFIRM_NORMAL;

#ifdef PACKAGE_INFORMATION
typedef struct
{
    uint16 err_cnt;
    uint16 recv_cnt;
    uint16 send_cnt;
} packet_info_t;
static packet_info_t packet_info;

#endif

/*************************************************************************************************
*MACROS
*/
#define IsNumberEqual(termNbr_t1, termNbr_t2) (osal_strcmp(termNbr_t1, termNbr_t2) == 0)

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void MineApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg);
static void MineApp_ProcessMSGCB(const afIncomingMSGPacket_t *MSGpkt);
static byte MineApp_ParseAppFrame(const afIncomingMSGPacket_t *MSGpkt);
static void MineApp_JoinNeighborPAN(void);
static uint8 MineApp_LQI2Level(byte LQI);
static void MineApp_CellSwitch(void);
static void MineApp_ReceivedRssiInd(uint8 LQI);
//static void MineApp_ProcessConfirmMSG(const afDataConfirm_t* afDataConfirm);
static void MineApp_VoiceEnScanShedule(void);
/*********************************************************************
* FUNCTIONS
*********************************************************************/

/*********************************************************************
* @fn      MineApp_Init
*
* @brief   Initialization function for the MineApp MP OSAL task.
*
* @param   task_id - the ID assigned by OSAL.
*
* @return  none
*/
void MineApp_Init( uint8 task_id)
{
    MineApp_TaskID = task_id;
    MineApp_NwkState = DEV_INIT;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    StartWatchDog(DOGTIMER_INTERVAL_1S);
    osal_set_event(MineApp_TaskID, MINEAPP_FEEDDOG_EVENT);
#endif


    uint16 Nmbr = BUILD_UINT16(macPib.extendedAddress.addr.extAddr[EXT_MACADDR_DEVID_LBYTE],
                               macPib.extendedAddress.addr.extAddr[EXT_MACADDR_DEVID_HBYTE]);
    _itoa(Nmbr, (uint8 *)(MineApp_NodeAttr.Node_nmbr.Nmbr), 10);
    MineApp_NodeAttr.nmbr = Nmbr;

    MineApp_NodeAttr.peernmbr = INVALIDNMMBR;
    memset(MineApp_NodeAttr.peer_termnmbr.Nmbr, 0, NMBRDIGIT*sizeof(char));
    MineApp_NodeAttr.IsTalkWithGateWay = FALSE;

    afRegister((endPointDesc_t *)&MineApp_epDesc);

#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
    HalAudioStart();
    RegisterForAudio(MineApp_TaskID);
#endif
    osal_start_timerEx(MineApp_TaskID, MINEAPP_INITNWK_TIMEOUT_EVENT, INIT_NWK_TIMEOUT);

#ifdef PACKAGE_INFORMATION
    if (NO_TIMER_AVAIL == osal_start_timerEx(MineApp_TaskID, MINEAPP_PACKAGE_INFORM_EVENT, 1000))
        SystemReset();
#endif
}

/*********************************************************************
* @fn      MineApp_ProcessEvent
*
* @brief   Mine Application Task event processor.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events - Bit map of events to process.
*
* @return  none
*/
uint16 MineApp_ProcessEvent( uint8 task_id, uint16 events )
{
    afIncomingMSGPacket_t *MSGpkt;
#ifdef TIME_TEST
    uint32 times = 0;
#endif
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
    FeedWatchDog();
#endif
    if (events & SYS_EVENT_MSG)
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(MineApp_TaskID);
        while ( MSGpkt )
        {
            switch ( MSGpkt->hdr.event )
            {
            case ZDO_STATE_CHANGE:
            {
                byte *tempPtr = (byte *)&AppData;

                MineApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                MineApp_SearchNWKStopped();
                zgConfigPANID = 0xFFFF;

                /* stop cell switch timeout */
                osal_stop_timerEx(MineApp_Function_TaskID, MINEAPP_CELL_TIMEOUT_EVENT);
                osal_unset_event(MineApp_Function_TaskID, MINEAPP_CELL_TIMEOUT_EVENT);

                //Notify ARM a new node is coming.
                if (MineApp_NwkState != DEV_INIT && !ON_AUDIO())
                {
                    *tempPtr++ = ZB_JOIN_NOTIFY;
                    osal_cpyExtAddr(tempPtr, NLME_GetExtAddr());
                    tempPtr += 8;
                    *tempPtr++ = _NIB.CapabilityInfo;
                    osal_memcpy(tempPtr, MineApp_NodeAttr.Node_nmbr.Nmbr, sizeof(termNbr_t));
                    //send a Join NWK pkt to ARM through ZC.
                    AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
                                   MINEAPP_CLUSTERID, sizeof(app_JoinNwk_t), (byte *)&AppData,
                                   &MineApp_TransID, INIT_OPN | AF_ACK_REQUEST, 0x01);

                    //record current PAN and address in the PAN.
                    nodeAddr.PANInfo.PanId = _NIB.nwkPanId;
                    osal_cpyExtAddr(nodeAddr.PANInfo.extendedPANID, _NIB.extendedPANID);
                    osal_cpyExtAddr(nodeAddr.PANInfo.extendedCoordAddr, _NIB.nwkCoordExtAddress);
                    nodeAddr.PANInfo.Channel = _NIB.nwkLogicalChannel;
                    nodeAddr.PANInfo.CoordAddr = _NIB.nwkCoordAddress;
                    nodeAddr.shortAddr = NLME_GetShortAddr();

                    MacUtil_t MacUtil;
                    MacUtil.panID = _NIB.nwkPanId;
                    MacUtil.dst_endpoint = MINEAPP_ENDPOINT;
                    MacUtil.src_endpoint = 0x20;
                    MacUtil.profile_id = 0x0100;//APS_PROFID
                    MacUtil.cluster_id = MINEAPP_CLUSTERID;
                    MacUtil.NodeType = ZDO_Config_Node_Descriptor.LogicalType;
                    MAC_UTIL_INIT(&MacUtil);

#ifndef NEW_MENU_LIB
                    Display_Msg_Menu(INIT_MAIN_MSG, NULL);
#else

                    Menu_SetNwkStat(NWK_STAT_NORMAL);
                    osal_stop_timerEx(MineApp_TaskID, MINEAPP_INITNWK_TIMEOUT_EVENT);
                    osal_unset_event(MineApp_TaskID, MINEAPP_INITNWK_TIMEOUT_EVENT);

                    Menu_SearchNwkFinish();
#endif
                    /* after search nwk finished, go to first sleep */
                    MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_PERIOD_SLEEP_EVENT, 300);
                }
                else
                {
                    Menu_SetNwkStat(NWK_STAT_INIT);
                    if(ON_AUDIO())
                    {
                        HalAudioClose();
                        Menu_handle_msg(MSG_INIT_NWK, NULL, 0);
                        osal_start_timerEx(MineApp_TaskID, MINEAPP_INITNWK_TIMEOUT_EVENT, INIT_NWK_TIMEOUT);
                    }
                    else/* Update the loggo to NO NETWORK when on Main */
                    {
                        Menu_UpdateNwkLogo();
                    }
                }


                break;
            }
            case AF_DATA_CONFIRM_CMD:
            {
                //MineApp_ProcessConfirmMSG((afDataConfirm_t *)MSGpkt);
                break;
            }
            case AF_INCOMING_MSG_CMD:
            {
#ifdef TIME_TEST
                times = 0;
                RESET_T1CNT();
#endif
                MineApp_ProcessMSGCB(MSGpkt);
                break;
            }
            case ZDO_CB_MSG:
            {
                MineApp_ProcessZDOMsgs((zdoIncomingMsg_t *)MSGpkt);
                break;
            }
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
            case APP_AUDIO:
            {
                AppPacket_t *pAudio = (AppPacket_t *)MSGpkt;
                //Add srcnbr and dstnbr to voice pkt.
                pAudio->data.app_Voice.srcnbr = MineApp_NodeAttr.nmbr;
                pAudio->data.app_Voice.dstnbr = MineApp_NodeAttr.peernmbr;
                if (MineApp_LeaveNWKNotify != CONFIRM_NWKTRANSFER
                        && pAudio->data.app_Voice.dstnbr != INVALIDNMMBR)
                {
                    /*
                    MAC_UTIL_BuildandSendData((uint8 *)pAudio + sizeof(spi_event_hdr_t),
                    pAudio->data.app_Voice.len + sizeof(app_Voice_t),
                    MAC_UTIL_UNICAST, dstAddr.addr.shortAddr,
                    NULL);
                    */
                    AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
                                   MINEAPP_CLUSTERID, pAudio->data.app_Voice.len + sizeof(app_Voice_t),  (uint8 *)pAudio + sizeof(spi_event_hdr_t),
                                   &MineApp_TransID, INIT_OPN, 0x01);

                }

                MineApp_VoiceEnScanShedule();

                break;
            }
#endif
            default:
                break;
            }
            osal_msg_deallocate( (uint8 *)MSGpkt );
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( MineApp_TaskID );
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & MINEAPP_DIALUP_RETRY_EVENT)
    {
        /* continues to send dial up until the state is calling */
        if((ON_CALLING() || ON_CALLINGWAIT())
                && !AppDialUpFound)
        {
            AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
                           MINEAPP_CLUSTERID, sizeof(AppDialUpData), (uint8 *)&AppDialUpData,
                           &MineApp_TransID, INIT_OPN | AF_ACK_REQUEST, 0x01);
            MineApp_start_timerEx(MineApp_TaskID, MINEAPP_DIALUP_RETRY_EVENT, 3000);
        }
        return (events ^ MINEAPP_DIALUP_RETRY_EVENT);
    }

    if (events & MINEAPP_PROBENWK_EVENT)
    {

        if ( ++MineApp_RejoinTimes > MINEAPP_REJOINTIMES)
        {
            MineApp_RejoinTimes = 0;

            HalAudioClose();
#ifndef NEW_MENU_LIB
            Display_Msg_Menu(INIT_NWK_MSG, NULL);
#else
            MineApp_NwkState = DEV_INIT;
            Menu_SetNwkStat(NWK_STAT_INIT);
            if(ON_AUDIO())
            {
                Menu_handle_msg(MSG_INIT_NWK, NULL, 0);
                osal_start_timerEx(MineApp_TaskID, MINEAPP_INITNWK_TIMEOUT_EVENT, INIT_NWK_TIMEOUT);
            }
            else	/* Update the loggo to NO NETWORK when on Main */
            {
                Menu_UpdateNwkLogo();
            }

#endif
            MineApp_StartSearchNWK();
        }
        else
        {
#ifndef NEW_MENU_LIB
            Update_Signal_Battery(SIGNAL_STRENTH, 0);
#else
            if(MineApp_RejoinTimes > 1)
            {
                Menu_UpdateSignal(0);
            }
#endif
            if (NO_TIMER_AVAIL == osal_start_timerEx(MineApp_TaskID, MINEAPP_PROBENWK_EVENT, TOLERANCE_TIME))
                SystemReset();
        }
        return events ^ MINEAPP_PROBENWK_EVENT;
    }


#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    if (events & MINEAPP_FEEDDOG_EVENT)
    {
        if (NO_TIMER_AVAIL == osal_start_timerEx(MineApp_TaskID, MINEAPP_FEEDDOG_EVENT, 500))
            SystemReset();
        FeedWatchDog();
        return events ^ MINEAPP_FEEDDOG_EVENT;
    }
#endif

#ifdef NEW_MENU_LIB
    if(events & MINEAPP_WAKE_TIMEOUT_EVT)
    {
        Menu_handle_msg(MSG_POLL_END, NULL, 0);
        return events ^ MINEAPP_WAKE_TIMEOUT_EVT;
    }
    if (events & MINEAPP_INITNWK_TIMEOUT_EVENT)
    {
        Menu_SearchNwkFinish();
        return events ^ MINEAPP_INITNWK_TIMEOUT_EVENT;
    }
#endif
    if (events & MINEAPP_STOP_AUDIO_EVENT)
    {
        if (ON_AUDIO())
        {
            //MineApp_ResetAudio();
            MineApp_EndTalk();      //send close to peer
            Menu_handle_msg(MSG_INIT_MAIN, 0, 0);
        }
        return events ^ MINEAPP_STOP_AUDIO_EVENT;
    }
#ifdef MENU_RF_DEBUG
    if (events & MINEAPP_RESTART_EVENT)
    {
        EA = 0;
        STARTWATCHDOG(DOGTIMER_INTERVAL_2MS);
        while(1);
        //return events ^ MINEAPP_RESTART_EVENT;
    }
#endif
    if (events & MINE_JOINNOTIFY_EVENT)
    {
        MineApp_LeaveNWKNotify = CONFIRM_NORMAL;
        // join neighobor PAN and notify ARM
        MAC_UTIL_SetPANID(nodeAddr.PANInfo.PanId);
        byte *tempPtr = (byte *)&AppData;
        *tempPtr++ = ZB_JOIN_NOTIFY;
        osal_cpyExtAddr(tempPtr, NLME_GetExtAddr());
        tempPtr += 8;
        *tempPtr++ = _NIB.CapabilityInfo;
        osal_memcpy(tempPtr, MineApp_NodeAttr.Node_nmbr.Nmbr, sizeof(termNbr_t));
        /*
        AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
        MINEAPP_CLUSTERID, sizeof(app_JoinNwk_t), (byte *)&AppData,
        &MineApp_TransID, INIT_OPN|AF_ACK_REQUEST, 0x01);
        */
        MAC_UTIL_BuildandSendData((uint8 *)&AppData, sizeof(app_JoinNwk_t), MAC_UTIL_UNICAST, dstAddr.addr.shortAddr, NULL);

#ifdef CELLSWITCH_DEBUG
        if(ON_AUDIO())
        {
            Menu_handle_msg(MSG_REFRESH_SCREEN, NULL, 0);
        }
#endif
        MineApp_NwkState = DEV_END_DEVICE;

        //MineApp_StartBlast();

        // notify the cell switch is finished
        Hal_SetCellSwitchStat(false);

        return events ^ MINE_JOINNOTIFY_EVENT;
    }
    if(events & MINE_JOINNEIGHTBOR_EVENT)
    {
        MineApp_JoinNeighborPAN();
        if(NO_TIMER_AVAIL == osal_start_timerEx(MineApp_TaskID, MINE_JOINNOTIFY_EVENT, 50))
            SystemReset();
        return events ^ MINE_JOINNEIGHTBOR_EVENT;
    }
#ifdef PACKAGE_INFORMATION
    if(events & MINEAPP_PACKAGE_INFORM_EVENT)
    {
        //uint16 framecnt, frameSilencecnt;
        //HalAudioGetFrameInfo(&framecnt, &frameSilencecnt);
        Menu_UpdatePackage(packet_info.recv_cnt, packet_info.err_cnt);
        if (NO_TIMER_AVAIL == osal_start_timerEx(MineApp_TaskID, MINEAPP_PACKAGE_INFORM_EVENT, 1000))
            SystemReset();
        return events ^ MINEAPP_PACKAGE_INFORM_EVENT;
    }
#endif
#ifdef MINE_TEST
    events = MineAppTest_HandleMSG(events);
#endif
    return 0;
}

/*********************************************************************
* @fn      MineApp_ProcessMSGCB
*
* @brief   This function processes OTA incoming message.
*
* @param   MSGpkt - The incoming msg.
*
* @return  none
*/
void MineApp_ProcessMSGCB(const afIncomingMSGPacket_t *MSGpkt)
{
    switch ( MSGpkt->clusterId )
    {
    case MINEAPP_CLUSTERID:
    {
#ifdef MINE_TEST
        MineAppTest_MessageMSGCB(MSGpkt);
#else
        //HalLedSet(HAL_LED_1, HAL_LED_MODE_TOGGLE);
        MineApp_ParseAppFrame(MSGpkt);
#endif

        break;
    }
    default:
        break;
    }
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
uint16 frameblk;

byte MineApp_ParseAppFrame(const afIncomingMSGPacket_t *MSGpkt)
{
    APPWrapper_t *AppPkt = (APPWrapper_t * )(MSGpkt->cmd.Data);
    byte appflag = AppPkt->app_flag;
    switch (appflag)
    {
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
    case VOICE:
    {
        if (MineApp_NodeAttr.nmbr != AppPkt->app_Voice.dstnbr)
            break;

        /*save the peer voice timetick */
        AppEnScan_info.peer_tick = osal_GetSystemClock();

        /*save coord rssi */
        MineApp_NodeAttr.coordrssi = CONV_LQI_TO_RSSI( MSGpkt->LinkQuality);

        /* if source is changed for continues 5 times, changeed peer number */
        static uint8 src_change_cnt;
        if(AppPkt->app_Voice.srcnbr == MineApp_NodeAttr.peernmbr)
        {
            src_change_cnt = 0;
        }
        else
        {
            if(src_change_cnt++ >=5)
            {
                src_change_cnt = 0;
                MineApp_NodeAttr.peernmbr = AppPkt->app_Voice.srcnbr;
                if(!MineApp_NodeAttr.IsTalkWithGateWay)  //only change number display when doing inside call
                {
                    _itoa(MineApp_NodeAttr.peernmbr, MineApp_NodeAttr.peer_termnmbr.Nmbr, 10);
		     		MineApp_SetPeerNumber(&MineApp_NodeAttr.peer_termnmbr);
                    Menu_handle_msg(MSG_REFRESH_SCREEN, NULL, 0);
                }
            }
        }

        /* update package information */
#ifdef PACKAGE_INFORMATION
        uint16 diff = AppPkt->app_Voice.blk - frameblk;
        if(diff == 0 || diff > 20)
            diff = 1;

        packet_info.err_cnt += diff - 1;
        packet_info.recv_cnt++;
#endif

        //filter duplicated frames.
        if (frameblk < AppPkt->app_Voice.blk
                || AppPkt->app_Voice.blk == 0
                || (AppPkt->app_Voice.blk > 0 && AppPkt->app_Voice.blk <= 5))
        {
            frameblk = AppPkt->app_Voice.blk;
            uint8 *voicedata = (uint8 *)AppPkt + sizeof(app_Voice_t); //skip: msgtype + len + blk.
            FillAudioBuffer(voicedata, AppPkt->app_Voice.len);
        }

        MineApp_RejoinTimes = 0;
        osal_start_timerEx(MineApp_TaskID, MINEAPP_PROBENWK_EVENT, TOLERANCE_TIME);

        //Handle exception.
        if (IS_IDLE() || ON_WAKE())
            /*
            *hangup party push down hangup key, send CMD_UP_CLOSE signal and return to idle status,
            *but ARM don't receive CMD_UP_CLOSE signal, and relay voice pkt again,
            *so hangup party need to send CMD_UP_CLOSE signal again.
            */
        {
            AppData.app_Cmdup.msgtype = ZB_CMD_UP;
            AppData.app_Cmdup.cmd = CMD_UP_CLOSE;
            AppData.app_Cmdup.srcnbr = MineApp_NodeAttr.Node_nmbr;
            MineApp_SignalLengh = sizeof(app_CmdUp_t);
            MineApp_SendSignal(MineApp_SignalLengh);
        }
        else if (ON_CALLING() || ON_CALLINGWAIT())
            /*Calling party loses CMD_DOWN_ACCEPT signal, so it needs to start audio and switch to audio status
            *until receving voice pkt.
            */
        {
            frameblk = 0;//need reset frameblk.
            //osal_start_timerEx(Hal_TaskID, MINEAPP_AUDIO_START_EVT, 60);
            MineApp_StartTalk();
#ifndef NEW_MENU_LIB
            Display_Msg_Menu(DIALING_SUCCESS_MSG, NULL);
#else
            Menu_handle_msg(MSG_DIALING_SUCCESS, NULL, 0);
#endif
            MineApp_NodeAttr.peernmbr = AppPkt->app_Voice.srcnbr;
        }

        /*FIXME:To avoid ambe of the other side not start absolutely, so close myself.
        */
        MineApp_ResetAudioStatus(MINEAPP_STOP_AUDIO_TIMEOUT);
        break;
    }
    case ZC_CMD_DOWN:
    {
        /* clear package informations */
#ifdef PACKAGE_INFORMATION
        packet_info.err_cnt = 0;
        packet_info.recv_cnt = 0;
#endif

        uint8 AckTag = AppPkt->app_CmdDown.cmd;
        if (!IsNumberEqual(AppPkt->app_CmdDown.dstnbr.Nmbr, MineApp_NodeAttr.Node_nmbr.Nmbr))
            break;
        if (AckTag == CMD_DOWN_ACCEPT)
        {
            if (ON_CALLING() || ON_CALLINGWAIT())
            {
                AppDialUpFound = true;
                MineApp_SetPeerNumber(&AppPkt->app_CmdDown.srcnbr);
                MineApp_StartTalk();
#ifndef NEW_MENU_LIB
                Display_Msg_Menu(DIALING_SUCCESS_MSG, NULL);
#else
                Menu_handle_msg(MSG_DIALING_SUCCESS, NULL, 0);
#endif
                MineApp_ResetAudioStatus(MINEAPP_STOP_AUDIO_TIMEOUT);
            }
        }
        else if (AckTag == CMD_DOWN_FOUND)
        {
            if (ON_CALLING() || ON_CALLINGWAIT())
            {
                AppDialUpFound = true;
                if(!HalRingIsPlaying()) //if the ring is already ringing
                {
                    HalRingOpen(RING_RING, OPENFLAG_NONE);
                }
                osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 15);
                MineApp_ResetAudioStatus(35000);
                MineApp_SetPeerNumber(&AppPkt->app_CmdDown.srcnbr);
            }
        }
        else if (AckTag == CMD_DOWN_CLOSE)
        {
#ifdef AUDIO_SERIAL
            if(ON_CALLED())
            {
#ifndef NEW_MENU_LIB
                Display_Msg_Menu(MISSED_CALL_MSG, NULL);
#else
                Menu_handle_msg(MSG_MISSED_CALL, NULL, 0);
#endif
                osal_stop_timerEx(Hal_TaskID, MINEAPP_RING_EVENT);
                osal_unset_event(Hal_TaskID, MINEAPP_RING_EVENT);
                MineApp_ResetAudio();
            }
            else if(ON_AUDIO())
            {
#ifndef NEW_MENU_LIB
                Display_Msg_Menu(INIT_MAIN_MSG, NULL);
#else
                Menu_handle_msg(MSG_VOICE_FINISH, NULL, 0);
#endif
                osal_stop_timerEx(Hal_TaskID, MINEAPP_RING_EVENT);
                osal_unset_event(Hal_TaskID, MINEAPP_RING_EVENT);
                MineApp_ResetAudio();
            }
            else if(ON_CALLING() || ON_CALLINGWAIT())
            {
#ifndef NEW_MENU_LIB
                Display_Msg_Menu(INIT_MAIN_MSG, NULL);
#else
                Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
#endif
                osal_stop_timerEx(Hal_TaskID, MINEAPP_RING_EVENT);
                osal_unset_event(Hal_TaskID, MINEAPP_RING_EVENT);
                MineApp_ResetAudio();
            }
#else
            HalResetNumber();
            StartLCD();
#endif
        }
        else if (AckTag == CMD_DOWN_NOTFOUND || AckTag == CMD_DOWN_BUSY)
        {
            if (ON_CALLING() || ON_CALLINGWAIT())
            {
                HalRingOpen(RING_BUSY, OPENFLAG_NONE);
                osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 15);
                MineApp_NodeAttr.peernmbr = INVALIDNMMBR;
                memset(MineApp_NodeAttr.peer_termnmbr.Nmbr, 0, NMBRDIGIT*sizeof(char));
                MineApp_NodeAttr.IsTalkWithGateWay = FALSE;
            }
        }
        else if (AckTag == CMD_DOWN_CALL)
        {
            if (ON_CALLED())
            {
                //termNbr_t terminalnum;
                //Get_Num_From_Menu((uint8 *)terminalnum.Nmbr);
                if (IsNumberEqual(AppPkt->app_CmdDown.srcnbr.Nmbr, Menu_GetDialNumBuf()))//lost signal and resend again.
                {
                    AppData.app_Cmdup.msgtype = ZB_CMD_UP;
                    AppData.app_Cmdup.cmd = CMD_UP_FOUND;
                    AppData.app_Cmdup.srcnbr = MineApp_NodeAttr.Node_nmbr;
                    AppData.app_Cmdup.dstnbr = AppPkt->app_CmdDown.srcnbr;
                    MineApp_SignalLengh = sizeof(app_CmdUp_t);

                    MineApp_SetPeerNumber(&AppPkt->app_CmdDown.srcnbr);
                    MineApp_SendSignal(MineApp_SignalLengh);
                    HalResetBackLightEvent();
                }
                else//a new comer and refuse its calling.
                {
                    AppData.app_Cmdup.msgtype = ZB_CMD_UP;
                    AppData.app_Cmdup.cmd = CMD_UP_BUSY;
                    AppData.app_Cmdup.srcnbr = MineApp_NodeAttr.Node_nmbr;
                    AppData.app_Cmdup.dstnbr = AppPkt->app_CmdDown.srcnbr;
                    MineApp_SignalLengh = sizeof(app_CmdUp_t);
                    MineApp_SendSignal(MineApp_SignalLengh);
                }
            }
            else if (ON_AUDIO())
            {
                //termNbr_t terminalnum;
                //Get_Num_From_Menu((uint8 *)terminalnum.Nmbr);
                if (!IsNumberEqual(AppPkt->app_CmdDown.srcnbr.Nmbr, Menu_GetDialNumBuf()))//Filter the same dialup pkt.
                {
                    AppData.app_Cmdup.msgtype = ZB_CMD_UP;
                    AppData.app_Cmdup.cmd = CMD_UP_BUSY;
                    AppData.app_Cmdup.srcnbr = MineApp_NodeAttr.Node_nmbr;
                    AppData.app_Cmdup.dstnbr = AppPkt->app_CmdDown.srcnbr;
                    MineApp_SignalLengh = sizeof(app_CmdUp_t);
                    MineApp_SendSignal(MineApp_SignalLengh);
                }
            }
            else	if (ON_CALLING() || ON_CALLINGWAIT() || ON_SM_SENDING())
            {
                AppData.app_Cmdup.msgtype = ZB_CMD_UP;
                AppData.app_Cmdup.cmd = CMD_UP_BUSY;
                AppData.app_Cmdup.srcnbr = MineApp_NodeAttr.Node_nmbr;
                AppData.app_Cmdup.dstnbr = AppPkt->app_CmdDown.srcnbr;
                MineApp_SignalLengh = sizeof(app_CmdUp_t);
                MineApp_SendSignal(MineApp_SignalLengh);
            }
            else if (IS_IDLE() || ON_WAKE()) //Receive CMD_DOWN_CALL in the first time.
            {
                AppData.app_Cmdup.msgtype = ZB_CMD_UP;
                AppData.app_Cmdup.cmd = CMD_UP_FOUND;
                AppData.app_Cmdup.srcnbr = MineApp_NodeAttr.Node_nmbr;
                AppData.app_Cmdup.dstnbr = AppPkt->app_CmdDown.srcnbr;
                MineApp_SetPeerNumber(&AppPkt->app_CmdDown.srcnbr);

                MineApp_SignalLengh = sizeof(app_CmdUp_t);
                MineApp_SendSignal(MineApp_SignalLengh);
                HalResetBackLightEvent();
                HalRingOpen(RING_ANY, OPENFLAG_ASBELL);
                osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 60);
#ifndef NEW_MENU_LIB
                Display_Msg_Menu(INCOMING_CALL_MSG, (uint8 *)(AppPkt->app_CmdDown.srcnbr.Nmbr));
#else
                Menu_handle_msg(MSG_INCOMING_CALL, (uint8 *)(AppPkt->app_CmdDown.srcnbr.Nmbr), 0);
#endif
            }
        }
        break;
    }
#ifdef NEW_MENU_LIB
    case SMS:
    {
        if(IS_IDLE() || ON_WAKE())  //only receive sms when system is IDLE
        {
            uint8 rtn;
            uint8 count;


            count = Get_SMS_Quantity();

            if(count < MAX_SMS_NUM)
            {
                Menu_Set_SMS_Full_Ring_Flag(TRUE);
                HalRingOpen(RING_ANY, OPENFLAG_ASSMS);
                osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 10);
                HalResetBackLightEvent();
                Menu_handle_msg(MSG_SMS_INCOMING, NULL, 0);
                rtn = Save_New_SMS(AppPkt);
                if(rtn == ZSUCCESS)
                {
                    uint8 len = osal_strlen((char *)AppPkt->app_SMS.nmbr.Nmbr);

                    osal_memcpy(AppData.app_SMSRsp.srcnmbr.Nmbr, AppPkt->app_SMS.nmbr.Nmbr, len);
                    AppData.app_SMSRsp.srcnmbr.Nmbr[len] = '\0';
                    AppData.app_SMSRsp.msgtype = SMSRsp;
                    AppData.app_SMSRsp.blk = AppPkt->app_SMS.blk;

                    rtn = AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
                                         MINEAPP_CLUSTERID, sizeof(app_SMSRsp_t), (uint8 *)&AppData,
                                         &MineApp_TransID, INIT_OPN | AF_ACK_REQUEST, 0x01);

                    break;
                }
            }
            else
            {
                if(Menu_Get_SMS_Full_Ring_Flag())
                {
                    Menu_Set_SMS_Full_Ring_Flag(FALSE);
                    HalRingOpen(RING_ANY, OPENFLAG_ASSMS);
                    osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 10);
                    HalResetBackLightEvent();
                }
                Menu_handle_msg(MSG_SMS_INCOMING, NULL, 0);

            }
        }
        break;
    }
#endif
    case DEPUTYJOIN_RSP:
    {
        uint8 Rspflag = AppPkt->app_DeputyJoinrsp.rsptype;
        if (Rspflag == PERMIT_JOIN &&
                IsNumberEqual(AppPkt->app_DeputyJoinrsp.deputyNmbr.Nmbr, MineApp_NodeAttr.Node_nmbr.Nmbr))
        {
            //record pre-allocated nwkaddress in the neighbor PAN.
            nodeAddr = AppPkt->app_DeputyJoinrsp.nodeAddr;

            MineApp_CellSwitch();
            //osal_start_timerEx(MineApp_TaskID, MINEAPP_JOINNEIGHBOR_EVENT, 100);
        }
        break;
    }

    case SIGNAL_STRENGTH:
    {
        // MineApp_ReceivedRssiInd(MSGpkt->LinkQuality);

        break;
    }

    case TIME_SSIND:
    {
#ifdef 	MENU_TIMEUPDATE_CTL
        if(GetTimeAutoUpdate())
        {
            Date_t date;
            Time_t time;
            time.hour = AppPkt->app_TimessInd.hour;
            time.min = AppPkt->app_TimessInd.minute;
            time.sec = AppPkt->app_TimessInd.second;
            SetTime(time);

            date.year = AppPkt->app_TimessInd.year;
            date.mon = AppPkt->app_TimessInd.month;
            date.day = AppPkt->app_TimessInd.day;
            SetDate(date);
            Menu_UpdateTime();
        }
#else
        Date_t CurrentDate = GetDate();
        if(CurrentDate.year == TIME_INIT_YEAR)
        {
            Date_t date;
            Time_t time;
            time.hour = AppPkt->app_TimessInd.hour;
            time.min = AppPkt->app_TimessInd.minute;
            time.sec = AppPkt->app_TimessInd.second;
            SetTime(time);

            date.year = AppPkt->app_TimessInd.year;
            date.mon = AppPkt->app_TimessInd.month;
            date.day = AppPkt->app_TimessInd.day;
            SetDate(date);
            Menu_UpdateTime();
        }
#endif
        break;
    }
    case ZB_REJOIN_NOTIFY:
    {
        if (MineApp_LeaveNWKNotify != CONFIRM_NWKTRANSFER
                && IsNumberEqual(AppPkt->app_RejoinNwk.nbr.Nmbr, MineApp_NodeAttr.Node_nmbr.Nmbr))
        {
            byte *tempPtr = (byte *)&AppData;
            *tempPtr++ = ZB_JOIN_NOTIFY;
            osal_cpyExtAddr(tempPtr, NLME_GetExtAddr());
            tempPtr += 8;
            *tempPtr++ = _NIB.CapabilityInfo;
            osal_memcpy(tempPtr, MineApp_NodeAttr.Node_nmbr.Nmbr, sizeof(termNbr_t));

            //send a Join NWK pkt to ARM through ZC.
            AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
                           MINEAPP_CLUSTERID, sizeof(app_JoinNwk_t), (byte *)&AppData,
                           &MineApp_TransID, INIT_OPN/*|AF_ACK_REQUEST*/, 0x01);
        }
        break;
    }
#ifdef NEW_MENU_LIB
    case MP_POLL_ACK:
    {
        if (!IsNumberEqual(AppPkt->app_MP_Poll_Ack.nbr.Nmbr, MineApp_NodeAttr.Node_nmbr.Nmbr))
            break;

        MineApp_ReceivedRssiInd(MSGpkt->LinkQuality);

        switch(AppPkt->app_MP_Poll_Ack.flag)
        {
        case MP_POLL_FLAG_NONE:
        {
            /* if hal allowed, go to sleep at once, else idle until WORK INTERVAL ends*/
            MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_PERIOD_SLEEP_EVENT, 2);
            break;
        }
        case MP_POLL_FLAG_START:
        {
            Menu_handle_msg(MSG_POLL_START, NULL, 0);
            osal_start_timerEx(MineApp_TaskID, MINEAPP_WAKE_TIMEOUT_EVT, 1000);
            break;
        }
        case MP_POLL_FLAG_END:
        {
            Menu_handle_msg(MSG_POLL_END, NULL, 0);
            break;
        }
        }
        break;
    }
#endif
#endif
    case CLEARNWK:
    {
        if (sAddrExtCmp(AppPkt->app_ClearNWK.macAddr, aExtendedAddress))
        {
            ZDApp_StartUpFromApp(0);
        }
        break;
    }

    default:
        break;
    }
    return ZSUCCESS;
}

void MineApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg)
{
}

/*********************************************************************
* @fn      MineApp_JoinNeighborPAN
*
* @brief   This function help to fake a nwk-join procedure instead of start a join cycle.
*
* @param  none.
*
* @return  none
*/
void MineApp_JoinNeighborPAN(void)
{
    //NLME_ResetRequest();
    byte CapabilityInfo = CAPINFO_DEVICETYPE_RFD
#if ( RFD_RCVC_ALWAYS_ON == TRUE)
                          | CAPINFO_RCVR_ON_IDLE
#endif
                          ;
    bool rxonidle = false;
    uint8 maxFrameRetries = 3;
    _NIB.nwkDevAddress = nodeAddr.shortAddr;
    _NIB.nwkLogicalChannel = nodeAddr.PANInfo.Channel;
    _NIB.nwkCoordAddress = nodeAddr.PANInfo.CoordAddr;
    osal_cpyExtAddr(_NIB.nwkCoordExtAddress, nodeAddr.PANInfo.extendedCoordAddr);
    _NIB.nwkPanId = nodeAddr.PANInfo.PanId;
    _NIB.nwkState = NWK_ENDDEVICE;
    _NIB.beaconOrder = DEF_BEACON_ORDER;
    _NIB.superFrameOrder = DEF_BEACON_ORDER;
    osal_cpyExtAddr(_NIB.extendedPANID, nodeAddr.PANInfo.extendedPANID);

    MAC_MlmeResetReq(TRUE);
    MAC_InitDevice();
    MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);
    MAC_MlmeSetReq(MAC_COORD_EXTENDED_ADDRESS, (void *)nodeAddr.PANInfo.extendedCoordAddr);
    MAC_MlmeSetReq(MAC_COORD_SHORT_ADDRESS, (void *)&nodeAddr.PANInfo.CoordAddr);
    MAC_MlmeSetReq(MAC_PAN_ID, (void *)&nodeAddr.PANInfo.PanId);
    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, (void *)&nodeAddr.shortAddr);
    MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, (void *)&nodeAddr.PANInfo.Channel);
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, (void *)aExtendedAddress);
    if (CapabilityInfo & CAPINFO_RCVR_ON_IDLE)
    {
        // The receiver is on, turn network layer polling off.
        rxonidle = true;
        MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, (void *)&rxonidle);
        NLME_SetPollRate( 0 );
        NLME_SetQueuedPollRate( 0 );
        NLME_SetResponseRate( 0 );
    }
}

uint8 MineApp_LQI2Level(byte LQI)
{
    uint8 len = sizeof(LQI_table) / sizeof(LQI_table[0]);
    uint8 i;

    for(i = 0; i < len; i++)
    {
        if(LQI < LQI_table[i])
        {
            return i;
        }
    }
    return len;
}

void MineApp_CellSwitch()
{
    // set doing cell switch flag
    Hal_SetCellSwitchStat(true);

    //Leave current zigbee network,
    MineApp_LeaveNWKNotify = CONFIRM_NWKTRANSFER;
    AppData.app_LeaveNwk.msgtype = ZB_LEAVE_NOTIFY;
    AppData.app_LeaveNwk.srcnbr = MineApp_NodeAttr.Node_nmbr;
    sAddrExtCpy(AppData.app_LeaveNwk.macAddr, NLME_GetExtAddr());

    MAC_UTIL_BuildandSendData((uint8 *)&AppData, sizeof(app_LeaveNwk_t), MAC_UTIL_UNICAST, dstAddr.addr.shortAddr, NULL);

    if(NO_TIMER_AVAIL == osal_start_timerEx(MineApp_TaskID, MINE_JOINNEIGHTBOR_EVENT, 50))
        SystemReset();
}

void MineApp_ResetFrameblk(void)
{
    frameblk = 0;
}

void MineApp_ResetAudioStatus(uint16 timeout)
{
    osal_start_timerEx(MineApp_TaskID, MINEAPP_STOP_AUDIO_EVENT, timeout);
}


void MineApp_ReceivedRssiInd(uint8 LQI)
{
    MineApp_NodeAttr.coordrssi = CONV_LQI_TO_RSSI( LQI);

    uint8 signalLevel = MineApp_LQI2Level(LQI);
#ifndef NEW_MENU_LIB
    Update_Signal_Battery(SIGNAL_STRENTH, signalLevel);
#else
    Menu_UpdateSignal(signalLevel);
#endif
#ifdef	RSSI_INFORMATION
    Menu_UpdateRSSI(LQI);
    Menu_UpdateLinkFlag(true);
#endif

    MineApp_RejoinTimes = 0;
    osal_start_timerEx(MineApp_TaskID, MINEAPP_PROBENWK_EVENT, TOLERANCE_TIME);

}
void MineApp_VoiceEnScanShedule(void)
{
#define VOICE_TIMEINTERVAL	 (20*AUDIO_IDX_THRESHOLD)

    if(AppEnScan_info.scanen)
    {
        uint32 tick = osal_GetSystemClock();
        uint32 diff = tick - AppEnScan_info.peer_tick;

        if(tick < AppEnScan_info.peer_tick || diff > VOICE_TIMEINTERVAL)
        {
            /* may be peer voice losted, fail */
            return;
        }
        else
        {
            if(diff < VOICE_TIMEINTERVAL / 2)
            {
                uint16 time = 30;
                MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_ENERGYSCAN_EVENT, time);
                AppEnScan_info.scanen = false;
            }
            else
            {
                uint16 time = VOICE_TIMEINTERVAL - diff + 40;
                MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_ENERGYSCAN_EVENT, time);
                AppEnScan_info.scanen = false;
            }
        }
    }
}

void MineApp_SetPeerNumber(termNbr_t* pnmbr)
{
    MineApp_NodeAttr.peer_termnmbr = *pnmbr;

    if (osal_strlen(pnmbr->Nmbr) == NMBRSIZE)
    {
        MineApp_NodeAttr.peernmbr = MineaApp_atoul((uint8 *)(pnmbr->Nmbr));
        if( (MineApp_NodeAttr.peernmbr >=GATEWAYMIN && MineApp_NodeAttr.peernmbr <= GATEWAYMAX)
                || (pnmbr->Nmbr[0] == GATEWAYDELIMITER))
        {
            AudioSetInputGain(INGAIN_PHONE2GATEWAY);
            MineApp_NodeAttr.IsTalkWithGateWay = TRUE;
        }
        else
        {
            AudioSetInputGain(INGAIN_PHONE2PHONE);
            MineApp_NodeAttr.IsTalkWithGateWay = FALSE;
        }
    }
    else
    {
        MineApp_NodeAttr.peernmbr = GATEWAYNMBR;
        AudioSetInputGain(INGAIN_PHONE2GATEWAY);
        MineApp_NodeAttr.IsTalkWithGateWay = TRUE;
    }
}
