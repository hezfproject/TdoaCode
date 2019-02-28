/**************************************************************************************************
Filename:       MineApp_MP_Fucntion.c
Revised:        $Date: 2011/07/22 17:38:35 $
Revision:       $Revision: 1.83 $

Description:   This is a Mine Application helper task for mobile phone, which will not register to ZDO and has
not an endpoint, not receive ZDO events and so on. It's a helper task for MineApp_MP and only capture keyboard events
and handle several none-timely events, i.e: lcd, menu-swtich, key and so on and will not receive OTA pkt.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "MineApp_MP_Function.h"
#include "MineApp_MP.h"
#include "string.h"
#ifndef   NEW_MENU_LIB
#include "MineApp_MP_Menu.h"
#else
#include "MineApp_MenuLib.h"
//#include "MenuLib_global.h"
#include "MenuLib_tree.h"
#include "MenuAdjustUtil.h"
#include "MenuChineseInputUtil.h"
#include "AppProtocolWrapper.h"
#include "SleepUtil.h"
#include "WatchDogUtil.h"
#include "Mac_radio_defs.h"
#include "Hal_drivers.h"
#endif

#include "MPDep.h"
#include "MineApp_Global.h"
#include "MineApp_Local.h"

#include "mac_pib.h"
#include "mac_radio_defs.h"

#include "App_cfg.h"
#include "MacUtil.h"
#include "Delay.h"
#include "WatchDogUtil.h"

#if !defined( NONWK )
#include "ZDApp.h"
#include "ZDObject.h"
#endif
#include "ZDProfile.h"

#include "OSAL.h"
#include "OnBoard.h"

#include "hal_mcu.h"
#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#include "hal_audio.h"
#include "KeyAudioISR.h"
#include "lcd_interface.h"
#include "ZGlobals.h"
#endif
#include "StringUtil.h"
#include "MenuChineseInputUtil.h"
/*************************************************************************************************
*CONSTANTS
*/

/*************************************************************************************************
*MACROS
*/
#ifndef MINEAPP_SHUTDWONTIMES
#define MINEAPP_SHUTDWONTIMES 2
#endif

/*********************************************************************
*  typedefs
*/
typedef struct
{
    uint16 newpanid;
    uint8  channel;
} mineapp_cell2info_t;
/*********************************************************************
* LOCAL VARIABLES
*/
uint8 MineApp_Function_TaskID;
static uint8 MineApp_RetryShutDownTimes = 0;
static mineapp_cell2info_t MineApp_Cell2info;

/* blast_seqnum */
static uint16 MineApp_seqnum = 0;
/*************************************************************************************************
*MACROS
*/
#define IsNumberEqual(termNbr_t1, termNbr_t2) (osal_strcmp(termNbr_t1, termNbr_t2) == 0)

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void MineApp_Function_HandleKeys(uint16 keys, byte shifts);
static uint8 MineApp_Function_LeaveNWK(uint8 notify);
static void MineApp_PowerOff(void);
static void MineApp_RingAndPoweOff(void);
static networkDesc_t  *MineApp_getBestScanResult(byte ResultCount, networkDesc_t *NetworkList);
static void MineAPP_NeighborComp(networkDesc_t *pbestpan);
static void MineApp_StartBlast(void);
/*********************************************************************
* @fn      MineApp_Function_Init
*
* @brief   Initialization function for the MineApp none voice OSAL task.
*
* @param   task_id - the ID assigned by OSAL.
*
* @return  none
*/

void MineApp_Function_Init(uint8 task_id)
{
    MineApp_Function_TaskID = task_id;
    //afRegister((endPointDesc_t *)&MineApp_Function_epDesc);
    RegisterForKeys(MineApp_Function_TaskID);

    //The dstAddr of mobile phone is fixed and equal to ZC's nwkaddress
    //and endpoint is fixed, too.
    dstAddr.endPoint = MINEAPP_ENDPOINT;
    dstAddr.addrMode = afAddr16Bit;
    dstAddr.addr.shortAddr = 0;

    MP_SetPowerOn(TRUE);

    if(!MP_vdd_check())
    {

        HalResetBackLightEvent();
#ifndef NEW_MENU_LIB
        Display_Msg_Menu(NO_POWER_MSG, NULL);
#else
        Menu_handle_msg(MSG_NO_POWER, NULL, 0);
#endif
        MP_LongDelay(500, 4);
        //MineApp_PowerOff();
        //MP_PowerOFF();
        MineApp_RingAndPoweOff();
        return;

    }
    StoreParam_t param = *(StoreParam_t *)MINEAPP_PARAM_ADDR;
    if(GetResetFlag() == RESET_FLAG_WATCHDOG && param.abnormalRst == TRUE)
    {
        Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
        if(param.backLightOn == true)
        {
            HalResetBackLightEvent();
        }
    }
    else
    {
        /* play poweron music */
        HalRingOpen(RING_POWERON, OPENFLAG_ASSMS);
        osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 60);
        osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_POWERON_EVENT, 15);

        Menu_handle_msg(MSG_POWERON_ANIM, NULL, 0);
        HalResetBackLightEvent();
    }

    param.abnormalRst = TRUE;
    *(StoreParam_t *)MINEAPP_PARAM_ADDR = param;

#ifdef MP_VERSION_1_1
    BACKLIGHT_OPEN();
    InitialLcd();
#endif

#ifndef NEW_MENU_LIB
    Init_Menu_NV();
    Update_Signal_Battery(BATTERY_STRENTH, HalAdcMeasureVdd());
    Display_Msg_Menu(INIT_NWK_MSG, NULL);
#else
    Menu_Init();
    Menu_UpdateBattery(HalAdcMeasureVdd());

    osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_PERIOD_SLEEP_EVENT, 100);
#endif
    osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_UPDATE_EVENT, 10000);
    //osal_set_event(MineApp_Function_TaskID, MINEAPP_UPDATE_EVENT);

}

/*********************************************************************
* @fn      MineApp_ProcessEvent
*
* @brief   Mine Application none voice Task event processor.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events - Bit map of events to process.
*
* @return  none
*/
uint16 MineApp_Function_ProcessEvent(uint8 task_id, uint16 events)
{
    afIncomingMSGPacket_t *MSGpkt;
    if(events & SYS_EVENT_MSG)
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(MineApp_Function_TaskID);
        while(MSGpkt)
        {
            switch(MSGpkt->hdr.event)
            {
            case KEY_CHANGE:
                MineApp_Function_HandleKeys(((keyChange_t *)MSGpkt)->keys, ((keyChange_t *)MSGpkt)->state);
                break;
            }
            osal_msg_deallocate((uint8 *)MSGpkt);
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(MineApp_Function_TaskID);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    /* energy scan prepare, if on_audio, wait for time shedule, else start Immediately*/
    if(events & MINEAPP_ENERGYSCAN_PREP_EVENT)
    {
        if(Menu_GetNwkStat()==NWK_STAT_NORMAL && ON_AUDIO())
        {
            AppEnScan_info.scanen =  true;
        }
        else
        {
            MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_ENERGYSCAN_EVENT, 2);
        }
        return events ^ MINEAPP_ENERGYSCAN_PREP_EVENT;
    }

    /* do energy scan */
    if(events & MINEAPP_ENERGYSCAN_EVENT)
    {
        /* start once scan */
        ZDO_SetOnceScan(true);
        //NLME_NetworkDiscoveryRequest(zgDefaultChannelList, BEACON_ORDER_30_MSEC);

        NLME_ScanFields_t  fields;
        fields.channels = zgDefaultChannelList;
        fields.duration = BEACON_ORDER_30_MSEC;
        NLME_NwkDiscReq2(&fields);

        return events ^ MINEAPP_ENERGYSCAN_EVENT;
    }

    if(events & MINEAPP_BLAST_EVENT)
    {
        MineApp_StartBlast();
        MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_POLL_EVENT, 10);
        return events ^ MINEAPP_BLAST_EVENT;
    }

    if(events & MINEAPP_POLL_EVENT)
    {
        if(!ON_AUDIO())
        {
            AppData.app_MP_Poll_Req.msgtype = MP_POLL_REQ;
            AppData.app_MP_Poll_Req.nbr = MineApp_NodeAttr.Node_nmbr;
            AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
                           MINEAPP_CLUSTERID, sizeof(app_MP_Poll_Req_t), (uint8 *)&AppData,
                           &MineApp_TransID, INIT_OPN, 0x01);
        }
        return events ^ MINEAPP_POLL_EVENT;
    }

    if(events & MINEAPP_RESET_COMM_EVENT)
    {
        if(ON_CALLING() || ON_CALLINGWAIT())
        {
            //Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);    //jump to main when ring is off
            HalRingOpen(RING_BUSY, OPENFLAG_NONE);
            osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 15);
        }
        return events ^ MINEAPP_RESET_COMM_EVENT;
    }

    if(events & MINEAPP_POWERON_EVENT)
    {
        //MP_PowerOFF();
        if(!HalRingIsPlaying())
        {
            Menu_handle_msg(MSG_INIT_NWK, NULL, 0);
        }
        else
        {
            if(NO_TIMER_AVAIL == osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_POWERON_EVENT, 200))
                SystemReset();
        }
        return events ^ MINEAPP_POWERON_EVENT;
    }

    if(events & MINEAPP_POWEROFF_EVENT)
    {
        //MP_PowerOFF();
        if(!HalRingIsPlaying())
        {
            MineApp_PowerOff();
        }
        else
        {
            if(NO_TIMER_AVAIL == osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_POWEROFF_EVENT, 200))
                SystemReset();
        }
        return events ^ MINEAPP_POWEROFF_EVENT;
    }
    if(events & MINEAPP_CELL2_LEAVE_EVENT)
    {
        zAddrType_t addr;
        addr.addrMode = afAddr16Bit;
        addr.addr.shortAddr = NLME_GetShortAddr();

        ZDP_MgmtLeaveReq(&addr, NLME_GetExtAddr(), false);
        //ZDApp_StopStartUp();
        //NLME_ResetRequest();
        osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_CELL2_JOIN_EVENT, 10);

        return events ^ MINEAPP_CELL2_LEAVE_EVENT;
    }
    if(events & MINEAPP_CELL2_JOIN_EVENT)
    {
        /* set config PANID as scaned panid */
        MineApp_NwkState = DEV_INIT;
        Menu_SetNwkStat(NWK_STAT_SWITCH);
        //Menu_UpdateNwkLogo();
        MineApp_StartSearchNWK2(MineApp_Cell2info.newpanid);

        /* switch timeout for 6s*/
        if(ZSuccess!=osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_CELL_TIMEOUT_EVENT, 6000))
        {
            SystemReset();
        }
        return events ^ MINEAPP_CELL2_JOIN_EVENT;
    }
    if(events & MINEAPP_CELL_TIMEOUT_EVENT)
    {
        uint8 nwk_stat = Menu_GetNwkStat();
        if(nwk_stat == NWK_STAT_SWITCH)
        {
            Menu_SetNwkStat(NWK_STAT_INIT);
        }
        return events ^ MINEAPP_CELL_TIMEOUT_EVENT;
    }

    if(events & MINEAPP_UPDATE_EVENT)
    {
        /*update time*/
        //SyncTime();
        //Update_Time();

        /* Update battery*/
        uint8 battery = HalAdcMeasureVdd();

        /* update key status */
        KeyEnable();

#ifndef NEW_MENU_LIB
        Update_Signal_Battery(BATTERY_STRENTH, battery);
#else
        Menu_UpdateBattery(battery);
        Menu_UpdateTime();
#endif
        //if(battery == 0)
        {
            if(!MP_vdd_check())
            {
#ifndef NEW_MENU_LIB
                Display_Msg_Menu(NO_POWER_MSG, NULL);
#else
                Menu_handle_msg(MSG_NO_POWER, NULL, 0);
#endif
                MP_LongDelay(500, 4);
                uint8 LeaveStatus = ZFailure;
                do
                {
                    ++MineApp_RetryShutDownTimes;
                    LeaveStatus = MineApp_Function_LeaveNWK(0);
                }
                while(MineApp_RetryShutDownTimes < MINEAPP_SHUTDWONTIMES
                        && LeaveStatus != ZSuccess);
                //osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_POWEROFF_EVENT, 200);
                MineApp_RingAndPoweOff();

            }
        }

        if(NO_TIMER_AVAIL == osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_UPDATE_EVENT, 10000))
            SystemReset();
        return events ^ MINEAPP_UPDATE_EVENT;
    }

#ifdef NEW_MENU_LIB
    if(events & MINEAPP_PERIOD_SLEEP_EVENT)
    {
        bool sleeped = false;
        static uint8 poweroffcnt;
        bool mFalse = FALSE;
        bool rx_on_idle;
        if((Menu_GetNwkStat()==NWK_STAT_NORMAL && IS_IDLE()) || Menu_GetNwkStat()!=NWK_STAT_NORMAL)
        {
            if(Hal_AllowSleep())  // Idle and hardware not active
            {
                //-----------------------
                MAC_MlmeGetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);
                MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &mFalse);

                AudioIntoSleep();
                WaitKeySleep(5000);
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
                FeedWatchDog();
#endif

                if(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP) == MAC_SUCCESS)
                {
                    UtilSleep(CC2430_PM1, MP_WORK_INTERVAL * 1000);
                    sleeped = true;
                }
                MAC_PwrOnReq();
                MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);

            }

#ifdef  RSSI_INFORMATION
            Menu_UpdateLinkFlag(false);
#endif
        }

        if(sleeped == true)
        {
            poweroffcnt = 0;
            MineApp_set_event(MineApp_Function_TaskID, MINEAPP_ENERGYSCAN_PREP_EVENT);
            MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_PERIOD_SLEEP_EVENT, MP_WORK_TIMEOUT);
        }
        else
        {
            if(poweroffcnt++ >= 5)   // try time > 5
            {
                poweroffcnt = 0;
                MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_ENERGYSCAN_PREP_EVENT, MP_WORK_INTERVAL * 1000 - 100);
                MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_PERIOD_SLEEP_EVENT, MP_WORK_INTERVAL * 1000 - 100 + MP_WORK_TIMEOUT);
            }
            else   // try sleep again after 20ms
            {
                MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_PERIOD_SLEEP_EVENT, 20);
            }
        }

        return events ^ MINEAPP_PERIOD_SLEEP_EVENT;
    }
#endif

#ifdef NEW_MENU_LIB
    if(events & MINEAPP_MENULIB_EVENT)
    {
        Menu_ProcessMenuLibEvt();
        return events ^ MINEAPP_MENULIB_EVENT;
    }
#endif
    if(events & MINEAPP_CONTINUESPRESS_TEST_EVENT)
    {
        menu_ChineseContinuesPressTestEnd();
        return events ^ MINEAPP_CONTINUESPRESS_TEST_EVENT;
    }
    return 0;
}

/*********************************************************************
* @fn      MineApp_Function_HandleKeys
*
* @brief   This function is used to handle key event.
*
* @param   keys - key.
*               shifts -
*
* @return  none
*/
void MineApp_Function_HandleKeys(uint16 keys, byte shifts)
{

#ifndef NEW_MENU_LIB
#else
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
    FeedWatchDog();
#endif
    HalResetBackLightEvent();
    if(keys == HAL_KEY_CANCEL)
    {
        if(((HalGetPadLockStat() == PADLOCK_UNLOCKED))   //|| (!HalGetPadLockEnable())
                && MP_TestLongPress(HAL_KEY_POWER, MINEAPP_POWER_LONGPRESS_TIMEOUT))
        {
            uint8 LeaveStatus = ZFailure;
            do
            {
                ++MineApp_RetryShutDownTimes;
                LeaveStatus = MineApp_Function_LeaveNWK(0);
            }
            while(MineApp_RetryShutDownTimes < MINEAPP_SHUTDWONTIMES
                    && LeaveStatus != ZSuccess);
            //osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_POWEROFF_EVENT, 200);
            MineApp_RingAndPoweOff();
        }
        else //if (MineApp_NwkState != DEV_INIT)
        {

            if((ON_AUDIO()))
            {
                MineApp_EndTalk();
            }
            else if(ON_CALLED() || ON_CALLING() || ON_CALLINGWAIT())
            {
                MineApp_EndTalk();
                osal_stop_timerEx(Hal_TaskID, MINEAPP_RING_EVENT);
                osal_unset_event(Hal_TaskID, MINEAPP_RING_EVENT);
            }
            //The  CMD_UP_CLOSE message must be send to caller  before hanle_key()  when the called MP
            //cancel the incmoming. Because hanle_key() will clear the call status.
            Menu_handle_key(keys, KEY_SHORT_PRESS);
            KeyIntoSleep();
        }
        return;
    }

    Menu_handle_key(keys, KEY_SHORT_PRESS);
    KeyIntoSleep();
    switch(keys)
    {
    case HAL_KEY_STAR:
        break;
    case HAL_KEY_CALL:
    {
        //Because Handle_Key(keys) will set the call status(calling or called), so it must execute first
        //when the Dial_Key is pressed. MP will send message to the opposite side after the call status is set.
        if(ON_CALLED())  //Called side.
        {

            AppData.app_Cmdup.msgtype = ZB_CMD_UP;
            AppData.app_Cmdup.cmd = CMD_UP_ACCEPT;
            AppData.app_Cmdup.srcnbr = MineApp_NodeAttr.Node_nmbr;
            strcpy((char *)(AppData.app_Cmdup.dstnbr.Nmbr), (const char *)Menu_GetDialNumBuf());
            MineApp_SignalLengh = sizeof(app_CmdUp_t);
            MineApp_SendSignal(MineApp_SignalLengh);
            //AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
            //  MINEAPP_CLUSTERID, sizeof(app_CmdUp_t), (uint8 *)&AppData,
            //  &MineApp_TransID, INIT_OPN|AF_ACK_REQUEST, 0x01);

            //osal_start_timerEx(Hal_TaskID, MINEAPP_AUDIO_START_EVT, 60);  //need to move away and wait until recving ack.
            MineApp_StartTalk();
            MineApp_ResetAudioStatus(MINEAPP_STOP_AUDIO_TIMEOUT);//FIXME:To avoid the other side not start normally.
        }
        else if(ON_CALLING()) //Calling side.
        {
            termNbr_t terminalnum;
            //Get_Num_From_Menu((uint8 *)terminalnum.Nmbr);
            strcpy((char *) terminalnum.Nmbr, (const char *)Menu_GetDialNumBuf());
            MineApp_ResetAudio();
            if(IsNumberEqual(terminalnum.Nmbr, MineApp_NodeAttr.Node_nmbr.Nmbr))
            {
#ifndef NEW_MENU_LIB
                Display_Msg_Menu(INIT_MAIN_MSG, NULL);
#else
                Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
#endif
                return;
            }

            /* start a dial */
            AppDialUpData.msgtype = ZB_CMD_UP;
            AppDialUpData.cmd = CMD_UP_DIALUP;
            AppDialUpData.srcnbr = MineApp_NodeAttr.Node_nmbr;
            AppDialUpData.dstnbr = terminalnum;
            AppDialUpFound = false;
            MineApp_NodeAttr.peernmbr = INVALIDNMMBR; //Update to invalidnmbr until recv the corresponding signal from called party.
            osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_RESET_COMM_EVENT, 32000);

            MineApp_set_event(MineApp_TaskID, MINEAPP_DIALUP_RETRY_EVENT);

            MineApp_ResetAudioStatus(40000);	
	        MineApp_SetPeerNumber(&terminalnum);

        }
        break;
    }
    case HAL_KEY_SELECT:
    {
        if(ON_SM_SENDING())
        {
            //uint16 sms_id = 0;
            uint8 rtn = afStatus_FAILED;
            uint8 *message = Menu_Get_SMS_Data();
            //uint8* message = menu_ChineseOutput();
            uint8 datasize = Menu_Get_SMS_Len();
            uint8 *sms = osal_mem_alloc(sizeof(app_SMS_t) + datasize);
            if(sms)
            {
                app_SMS_t *app_SMS = (app_SMS_t *)sms;
                app_SMS->msgtype = SMS;
                //Get_Num_From_Menu((uint8 *)(app_SMS->nmbr.Nmbr));
                strcpy((char *)app_SMS->nmbr.Nmbr , (const char *) Menu_GetNumBuf());
                app_SMS->len = datasize;
                osal_memcpy((void *)(app_SMS + 1), message, app_SMS->len);
                rtn = AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
                                     MINEAPP_CLUSTERID, sizeof(app_SMS_t) + app_SMS->len, sms,
                                     &MineApp_TransID, INIT_OPN | AF_ACK_REQUEST, 0x01);
                osal_mem_free(sms);
            }
            if(rtn == afStatus_SUCCESS)
                Menu_handle_msg(MSG_SMS_SUCCESS, NULL, 0);
            else if(rtn == afStatus_FAILED)
                Menu_handle_msg(MSG_SMS_FAILED, NULL, 0);
        }
        break;
    }
    default:
        break;
    }
    //HalResetBackLightEvent();

#endif
}


uint8 MineApp_Function_LeaveNWK(uint8 notify)
{
    AppData.app_LeaveNwk.msgtype = ZB_LEAVE_NOTIFY;
    AppData.app_LeaveNwk.srcnbr = MineApp_NodeAttr.Node_nmbr;
    sAddrExtCpy(AppData.app_LeaveNwk.macAddr, NLME_GetExtAddr());
    afAddrType_t dstaddr;
    dstaddr.endPoint = MINEAPP_ENDPOINT;
    dstaddr.addrMode = afAddr16Bit;
    dstaddr.addr.shortAddr = NLME_GetCoordShortAddr();
    uint8 rval = AF_DataRequest(&dstaddr, (endPointDesc_t *)&MineApp_epDesc,
                                MINEAPP_CLUSTERID, sizeof(app_LeaveNwk_t), (byte *)&AppData,
                                &MineApp_TransID, INIT_OPN | AF_ACK_REQUEST, 0x01);
    return rval;
}

void MineApp_StartMenuLibEvt(uint16 timeout)
{
    osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, timeout);
}

void MineApp_StopMenuLibEvt(void)
{
    osal_stop_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT);
    osal_unset_event(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT);
}

void MineApp_PowerOff(void)
{
    /* set normal reset flag */
    StoreParam_t param = *(StoreParam_t *)MINEAPP_PARAM_ADDR;
    param.abnormalRst = FALSE;
    *(StoreParam_t *)MINEAPP_PARAM_ADDR = param;
    MP_PowerOFF();
}
void MineApp_RingAndPoweOff(void)
{
    HalRingOpen(RING_POWEROFF, OPENFLAG_ASSMS);
    osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 50);

    Menu_handle_msg(MSG_POWEROFF_ANIM, NULL, 0);
    if(NO_TIMER_AVAIL == osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_POWEROFF_EVENT, 100))
        SystemReset();

}


void  MineApp_NetworkDiscoveryConfirmCB(uint8 ResultCount, networkDesc_t *NetworkList)
{
    ZDO_SetOnceScan(false);
    NLME_NwkDiscTerm();

    if(Menu_GetNwkStat()==NWK_STAT_NORMAL)  // if have nwk, scan for cell switch
    {
        if(ResultCount == 0) // scan fail
        {
            /* start poll */
            MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_BLAST_EVENT, 5);
            //MineApp_set_event(MineApp_Function_TaskID, MINEAPP_POLL_EVENT);
        }
        else
        {
            networkDesc_t *pNwkDesc;
            pNwkDesc =  MineApp_getBestScanResult(ResultCount, NetworkList);
            MineAPP_NeighborComp(pNwkDesc);
        }
    }
    else        // if do not have nwk, scan for join
    {
        MineApp_SearchNWKStopped();

        if(ResultCount == 0)  // no scan result, sleep
        {
            MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_PERIOD_SLEEP_EVENT, 5);

        }
        else  // have scan result, join for 6s
        {
            /* use zdo_startDevice to start */
            //MineApp_StartSearchNWK();

            /* use cellswitch as join new nwk */
#ifdef MENU_RF_DEBUG
            MineApp_Cell2info.newpanid = zgConfigPANID;
#else
            MineApp_Cell2info.newpanid = 0xFFFF;
#endif
            MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_CELL2_LEAVE_EVENT, 5);

            /* use NLME_JoinRequest to start, have some bugs */
            //networkDesc_t *pNwkDesc;
            //pNwkDesc =  MineApp_getBestScanResult(ResultCount, NetworkList);
            //NLME_JoinRequest(pNwkDesc->extendedPANID,  pNwkDesc->panId, pNwkDesc->logicalChannel, CAPINFO_ALLOC_ADDR);//|CAPINFO_ALTPANCOORD|CAPINFO_DEVICETYPE_RFD|CAPINFO_POWER_AC|CAPINFO_RCVR_ON_IDLE);

            MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_PERIOD_SLEEP_EVENT, 6000);
        }
    }

    return;
}
networkDesc_t  *MineApp_getBestScanResult(byte ResultCount, networkDesc_t *NetworkList)
{
    if(ResultCount > 0)
    {
        uint8 bestlqi  = 0;
        networkDesc_t *pbestPan = NULL;
        networkDesc_t *pPan = NetworkList;

        while(pPan != NULL)
        {
            if(NUMBER_IS_SUBSTATION(pPan->panId) && pPan->chosenRouterLinkQuality > bestlqi)
            {
                bestlqi =  pPan->chosenRouterLinkQuality;
                pbestPan = pPan;
            }
            pPan = pPan->nextDesc;
        }
        return pbestPan;
    }
    else
    {
        return NULL;
    }
}

void MineAPP_NeighborComp(networkDesc_t *pbestpan)
{
    static uint8 MineApp_gChecktime;

    /* if no scan result,  start poll */
    if(pbestpan == NULL || pbestpan->panId == macPib.panId || !NUMBER_IS_SUBSTATION(pbestpan->panId))
    {
        goto ret;
    }

    static int8 rssi;
    rssi = CONV_LQI_TO_RSSI(pbestpan->chosenRouterLinkQuality);

    if(MineApp_NodeAttr.coordrssi < -45 && rssi > (MineApp_NodeAttr.coordrssi + 6))
        //if( rssi > (MineApp_NodeAttr.coordrssi + 6))   // test condition
    {
        MineApp_gChecktime++;

        /* if on audio, donot use the doule check */
        if(MineApp_gChecktime >= 2 || (Menu_GetNwkStat()==NWK_STAT_NORMAL && ON_AUDIO()))    //matched the cell2 condition
        {
            MineApp_gChecktime = 0;
            if(IS_IDLE())  // if idle, tranfer directly
            {
                MineApp_Cell2info.newpanid = pbestpan->panId;
                MineApp_Cell2info.channel = pbestpan->logicalChannel;

                //for cell switch,  join for 6s
                MineApp_Function_LeaveNWK(0);
                MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_CELL2_LEAVE_EVENT, 5);
                MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_PERIOD_SLEEP_EVENT, 6000);
            }
            else  //notify arm to cellswitch
            {
                app_switchreq_t app_switchreq;
                app_switchreq.msgtype = MP_SWITCH_REQ;
                app_switchreq.srcnbr = MineApp_NodeAttr.Node_nmbr;
                app_switchreq.newpan = pbestpan->panId;
                AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
                               MINEAPP_CLUSTERID, sizeof(app_switchreq), (uint8 *)&app_switchreq,
                               &MineApp_TransID, INIT_OPN, 0x01);

                MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_BLAST_EVENT, 10);
            }
        }
        else  /* scan double check  */
        {
            MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_ENERGYSCAN_EVENT, 5);
        }
    }
    else
    {
        goto ret;
    }
    return;

ret:
    MineApp_gChecktime = 0;
    MineApp_start_timerEx(MineApp_Function_TaskID, MINEAPP_BLAST_EVENT, 5);
    //MineApp_set_event(MineApp_Function_TaskID, MINEAPP_POLL_EVENT);
}

void MineApp_StartBlast(void)
{
    if(Menu_GetNwkStat()==NWK_STAT_NORMAL)
    {
        AppData.app_MPssReq.msgtype = MP_SSREQ;
        AppData.app_MPssReq.reqtype = SSREQ_OUT;
        AppData.app_MPssReq.NWK_ADDR = _NIB.nwkPanId;
        AppData.app_MPssReq.seqnum = MineApp_seqnum++;
        osal_memcpy(AppData.app_MPssReq.nmbr.Nmbr,
                    MineApp_NodeAttr.Node_nmbr.Nmbr, sizeof(termNbr_t));

        MacParam_t param;
        param.cluster_id = MINEAPP_CLUSTERID;
        param.panID = 0xFFFF;
        param.radius = 1;
        MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&AppData, sizeof(app_MPssReq_t),
                                     MAC_UTIL_BROADCAST, MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR, NULL);
    }
}
