/**************************************************************************************************
Filename:       MP_MP_Fucntion.c
Revised:        $Date: 2011/04/14 18:03:57 $
Revision:       $Revision: 1.26 $

Description:   This is a Mine Application helper task for mobile phone, which will not register to ZDO and has
not an endpoint, not receive ZDO events and so on. It's a helper task for MP_MP and only capture keyboard events
and handle several none-timely events, i.e: lcd, menu-swtich, key and so on and will not receive OTA pkt.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "ZComdef.h"
#include "string.h"
#include "SleepUtil.h"
#include "WatchDogUtil.h"
#include "Mac_radio_defs.h"
#include "Hal_drivers.h"


#include "MenuLib_tree.h"
#include "MenuAdjustUtil.h"
#include "MenuChineseInputUtil.h"

#include "MobilePhone.h"
#include "MobilePhone_Global.h"
#include "MobilePhone_cfg.h"
#include "MobilePhone_Function.h"
#include "MobilePhone_MenuLib.h"
#include "MobilePhone_Dep.h"

#include "mac_pib.h"
#include "mac_radio_defs.h"
#include "mac_main.h"

#include "App_cfg.h"
#include "Delay.h"
#include "WatchDogUtil.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "OnBoard.h"

#include "hal_mcu.h"
#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "key.h"
#include "hal_led.h"
#include "hal_alarm.h"

#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#include "hal_audio.h"
#include "KeyAudioISR.h"
#include "lcd_serial.h"
#endif

#include "StringUtil.h"
#include "SleepUtil.h"
#include "MenuChineseInputUtil.h"
#include "numtrans.h"
#ifdef MENU_RF_DEBUG
#include "Menulib_Nv.h"
#endif
/*************************************************************************************************
*CONSTANTS
*/

/*************************************************************************************************
*MACROS
*/

/*********************************************************************
* typedefs
*/

/*********************************************************************
* GLOBAL VARIABLES
*/
uint8 MP_Function_TaskID;
/*********************************************************************
* LOCAL VARIABLES
*/

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void MP_RingAndPoweOff(void);
#ifdef MENU_RF_DEBUG
static uint16 MP_GetSettedPanID(void);
#endif
/*********************************************************************
* @fn      MP_Function_Init
*
* @brief   Initialization function for the MineApp none voice OSAL task.
*
* @param   task_id - the ID assigned by OSAL.
*
* @return  none
*/

void MP_Function_Init( uint8 task_id)
{
    MP_Function_TaskID = task_id;

    RegisterForKeys(MP_Function_TaskID);

    /* check vdd */
    if(MP_CheckVddLevel() < 0)
    {
        HalResetBackLightEvent();
        Menu_handle_msg(MSG_NO_POWER, NULL, 0);
        MP_LongDelay(500, 4);
        MP_RingAndPoweOff();
        return;
    }

    Menu_Init();

    /* read store param */
    StoreParam_t param = *(StoreParam_t *)MP_STOREPARAM_ADDR;
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
        /* play poweron anim and music */
        /* then start searching nwk */
        Menu_handle_msg(MSG_POWERON_ANIM, NULL, 0);
        Hal_RingStart(RING_POWERON, OPENFLAG_ASONCE);
        HalResetBackLightEvent();
    }

    param.abnormalRst = TRUE;
    *(StoreParam_t *)MP_STOREPARAM_ADDR = param;

    /* period do this  events */
    MP_start_timerEx(MP_Function_TaskID, MP_FUNC_UPDATE_EVENT, 10000);
    MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, 1000);
}

/*********************************************************************
* @fn      MP_ProcessEvent
*
* @brief   Mine Application none voice Task event processor.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events - Bit map of events to process.
*
* @return  none
*/

uint16 MP_Function_ProcessEvent(uint8 taskId, uint16 events)
{
    osal_event_hdr_t *MSGpkt;
    if (events & SYS_EVENT_MSG)
    {
        MSGpkt = (osal_event_hdr_t *)osal_msg_receive(MP_Function_TaskID);
        while ( MSGpkt )
        {
            switch ( MSGpkt->event )
            {
            case KEY_CHANGE:
                MP_Function_HandleKeys(((keyChange_t *)MSGpkt)->keys, ((keyChange_t *)MSGpkt)->state);
                break;
            }
            osal_msg_deallocate( (uint8 *)MSGpkt );
            MSGpkt = (osal_event_hdr_t *)osal_msg_receive( MP_Function_TaskID);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & MP_FUNC_RESET_EVENT)
    {
        if (ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
        {
            MP_ResetAudio();
            Hal_StartVoiceBell(VOICEBELL_NOBODY);
        }
        return events ^ MP_FUNC_RESET_EVENT;
    }

    if (events & MP_FUNC_UPDATE_EVENT)
    {
        /* period enable key board to avoid bug */
        KeyEnable();

        Menu_UpdateTime();

        /* Update battery*/
        int8 level = MP_CheckVddLevel();
        if(level < 0)
        {
            Menu_handle_msg(MSG_NO_POWER, NULL, 0);
            MP_LongDelay(500, 4);
            MP_RingAndPoweOff();
        }

        Menu_UpdateBattery((uint8)level);
        //Menu_handle_msg(MSG_REFRESH_SCREEN, NULL, 0);

        /* do updates every 10s */
        MP_start_timerEx(MP_Function_TaskID, MP_FUNC_UPDATE_EVENT, 10000);

        return events ^ MP_FUNC_UPDATE_EVENT;
    }

    if (events & MP_FUNC_MENULIB_EVENT)
    {
        Menu_ProcessMenuLibEvt();
        return events ^ MP_FUNC_MENULIB_EVENT;
    }
    if (events & MP_FUNC_CONTINUESPRESS_TEST_EVENT)
    {
        menu_ChineseContinuesPressTestEnd();
        return events ^ MP_FUNC_CONTINUESPRESS_TEST_EVENT;
    }

    /* energy scan prepare, if on_audio, wait for time shedule, else start Immediately*/
    if(events & MP_FUNC_SCANPREP_EVENT)
    {
        if(MP_IsNwkOn() && ON_AUDIO())  /* if onaudio, set the shedule enable bit and wait for time shedule */
        {
            MP_ScanInfo.isinshedule =  true;
        }
        else  /* start scan */
        {
            MP_set_event(MP_Function_TaskID, MP_FUNC_PERIODIC_SCAN_EVENT);
        }
        return events ^ MP_FUNC_SCANPREP_EVENT;
    }

    if (events & MP_FUNC_PERIODIC_SCAN_EVENT)
    {
        /* scan periodly */
        /* scan only current RSSI is not strong enough*/
        /*FIXME
        	a) -50 is a good value???
        */

        /* do scan when no network, or my rssi < -50 */
        if(MP_DevInfo.currentRssi < MP_CELL_THREHOLD1
                || !MP_IsNwkOn())
        {
            /* start scan */
            MP_UpdateCellInfo();
            MP_ScanInfo.isscaning = true;

            mp_Scan_t mp_Scan;
            mp_Scan.scantype = APP_SCAN_TYPE_REQ;
            mp_Scan.seqnum = MP_seqnums.scan_seqnum++;
            mp_Scan.armid = 0;

            //send ARM_ID request to all stations
            Hal_SendDataToAir((uint8 *)&mp_Scan, sizeof(mp_Scan), 0xFFFF, 0x0000, MP_SCAN, false, false);

            //scan for  50ms
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SCANEND_EVENT, 50);
        }
        else /* do not scan */
        {
            MP_ClearCellInfo();
            MP_ScanInfo.isscaning = false;
            MP_set_event(MP_Function_TaskID, MP_FUNC_POLL_EVENT);
        }

        return events ^ MP_FUNC_PERIODIC_SCAN_EVENT;
    }

    /* sort scan result and start join/switch */
    if (events & MP_FUNC_PERIODIC_SCANEND_EVENT)
    {
        MP_ScanInfo.isscaning = false;

        if(MP_IsCellInfoEmpty())
        {
            MP_set_event(MP_Function_TaskID, MP_FUNC_POLL_EVENT);
        }
        else //start join/switch
        {
            MP_SortCellInfo();
            MP_CellInfo.join_idx = 0;
            MP_set_event(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT);
        }
        return events ^ MP_FUNC_PERIODIC_SCANEND_EVENT;
    }

    if (events & MP_FUNC_POLL_EVENT)
    {
        if( MP_IsNwkOn() && !ON_AUDIO())
        {
            app_mpPoll_t app_mpPoll;
            app_mpPoll.hdr.srcaddr = MP_DevInfo.nmbr;
            app_mpPoll.hdr.dstaddr = APP_ARMSHORTADDR;
            app_mpPoll.polltype = APP_MPPOLL_TYPE_REQ;
            app_mpPoll.seqnum = MP_seqnums.poll_seqnum++;
            app_mpPoll.flag = 0;
            MP_SendSignalToCoord((uint8 *) &app_mpPoll, sizeof(app_mpPoll), MP_POLL, false);

            /* lost  poll_ack/scan by once, decrease the currentrssi by 15dB */
            if(MP_DevInfo.hascoordlink == false)
            {
                MP_DevInfo.currentRssi = MP_DevInfo.currentRssi > (MP_MIN_RSSI + 15) ? ( MP_DevInfo.currentRssi - 15) : MP_MIN_RSSI;
            }
            MP_DevInfo.hascoordlink = false;
        }
        return events ^ MP_FUNC_POLL_EVENT;
    }

    if (events & MP_FUNC_JOIN_SWITCH_EVENT)
    {
        uint8 count;

        /* join/switch condition */
        if(MP_IsNwkOn())
        {
            count = MP_DevInfo.currentRssi > MP_CELL_THREHOLD2 ? MP_CELL_COMPTIME : 1;
        }
        else
        {
            count = 1;
        }

        /* find next macth condition */
        while(MP_CellInfo.join_idx < MP_MAC_MAXSCAN_RESULTS && MP_CellInfo.CellInfo[MP_CellInfo.join_idx].cnt < count )
        {
            MP_CellInfo.join_idx++;
        }

        /* start to associate */
        if(MP_CellInfo.join_idx < MP_MAC_MAXSCAN_RESULTS
                && MP_CellInfo.CellInfo[MP_CellInfo.join_idx].panid != 0xFFFF)
        {
#ifdef MENU_RF_DEBUG
            /* if doing cell switch and the desire pan id is fixed, do NOT start cell switch */
            //     if( !((MP_NwkInfo.nwkState == NWK_DETAIL_CELLSWITCHING||MP_NwkInfo.nwkState == NWK_DETAIL_ENDDEVICE)&&MP_GetSettedPanID()!=0xFFFF)
            //       )
            {
#else
            if(1)
            {
#endif
                /* update nwk state */
                if(MP_NwkInfo.nwkState == NWK_DETAIL_INIT ||  MP_NwkInfo.nwkState == NWK_DETAIL_JOINASSOCING)
                {
                    MP_NwkInfo.nwkState = NWK_DETAIL_JOINASSOCING;
                }
                else
                {
                    MP_NwkInfo.nwkState = NWK_DETAIL_CELLSWITCHING;
                }

                /* join need longer wake time, extend the the next sleep time*/
                osal_unset_event(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT);
                MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, 350);

                HAL_AlarmSet (MP_ALARM_JOIN, 5000 );  /* join timeout 5s */

                app_mpJoinNwk_t  app_mpJoinNwk;
                app_mpJoinNwk.hdr.srcaddr = MP_DevInfo.nmbr;
                app_mpJoinNwk.hdr.dstaddr = APP_ARMSHORTADDR;
                app_mpJoinNwk.joinnwktype = APP_MP_JOINNWK_REQ;
                app_mpJoinNwk.armid = (MP_NwkInfo.nwkState == NWK_DETAIL_INIT ? APP_INVALIDARMADDR : MP_DevInfo.armid);
                app_mpJoinNwk.seqnum = MP_seqnums.join_seqnum++;
                Hal_SendDataToAir((uint8 *)&app_mpJoinNwk, sizeof(app_mpJoinNwk), MP_CellInfo.CellInfo[MP_CellInfo.join_idx].panid, 0x0, MP_JOIN_NOTIFY, true, true);

                //Timeout 300
                osal_start_timerEx(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT, 300);
            }

            MP_CellInfo.join_idx++;

        }
        else
        {
            /* join failed this round */
            osal_set_event(MP_Function_TaskID, MP_FUNC_POLL_EVENT);
        }
        return events ^ MP_FUNC_JOIN_SWITCH_EVENT;
    }
#ifdef MENU_OAD_UPDATE
    if(events & MP_FUNC_OADUPDATE_EVENT)
    {
        if(++ MP_OadUpdate.sendcnt < 2000)
        {
            if(MP_OadUpdate.panid!=0xFFFF && MP_OadUpdate.shortAddr!=0xFFFF
                    && MP_OadUpdate.msgtype>=TOF_CARD_OAD && MP_OadUpdate.msgtype<=TOF_COM_STATION_OAD)
            {
                uint8 channel;
                switch(MP_OadUpdate.msgtype)
                {
                case TOF_LOC_STATION_OAD:
                {
                    channel = 0x19;
                    break;
                }
                case TOF_LOCATOR_OAD:
                {
                    if(MP_OadUpdate.sendcnt % 5==0)
                    {
                        channel = 0x19;  // common channel
                    }
                    else if(MP_OadUpdate.sendcnt % 5==1)
                    {
                        channel = 0x18;  // tof channel 1
                    }
                    else if(MP_OadUpdate.sendcnt % 5==2)
                    {
                        channel = 0x17;  // tof channel 2
                    }
                    else if(MP_OadUpdate.sendcnt % 5==3)
                    {
                        channel = 0x16;  //tof channel 3
                    }
                    else
                    {
                        channel = 0x15;  // tof channel 4
                    }
                    break;
                }
                case TOF_COM_STATION_OAD:
                {
                    channel = 0x0E;
                    break;
                }
                default:
                {
                    channel = macPib.logicalChannel;
                    break;
                }
                }
                MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL,&channel);


                /* send tof data */
                rf_tof_oad_data_ts tof_oad_data;
                tof_oad_data.tof_head.protocoltype = APP_PROTOCOL_TYPE_CMD;
                tof_oad_data.tof_head.msgtype  = MP_OadUpdate.msgtype;

                uint16 len = sizeof(rf_tof_oad_data_ts) - sizeof(app_header_t);
                tof_oad_data.tof_head.len = BUILD_UINT16(HI_UINT16(len), LO_UINT16(len));
                tof_oad_data.u16Version = BUILD_UINT16(HI_UINT16(MP_OadUpdate.version), LO_UINT16(MP_OadUpdate.version));
                if(MP_OadUpdate.msgtype == TOF_CARD_OAD)
                {
                    tof_oad_data.u8DeviceType =DEVICE_TYPE_CARD;
                }
                else if(MP_OadUpdate.msgtype == TOF_LOCATOR_OAD)
                {
                    tof_oad_data.u8DeviceType =DEVICE_TYPE_LOCARTOR;
                }
                else
                {
                    tof_oad_data.u8DeviceType =DEVICE_TYPE_STATION;
                }
                macMcpsDataReq_t  *pData = MAC_McpsDataAlloc(sizeof(tof_oad_data), MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE);

                if(pData)
                {
                    pData->mac.srcAddrMode = SADDR_MODE_SHORT;
                    pData->mac.dstAddr.addrMode = SADDR_MODE_SHORT;
                    pData->mac.dstAddr.addr.shortAddr = MP_OadUpdate.shortAddr;
                    pData->mac.dstPanId = MP_OadUpdate.panid;
                    pData->mac.msduHandle = 0;
                    pData->mac.txOptions =  MAC_TXOPTION_ACK;
                    pData->sec.securityLevel = FALSE;

                    osal_memcpy ( pData->msdu.p, ( void * ) &tof_oad_data, sizeof ( tof_oad_data ) );
                    pData->msdu.len = sizeof ( tof_oad_data ) ;

                    MAC_McpsDataReq(pData);
                }

                /* update the menu */
                Menu_handle_msg(MSG_OAD_UPDATE, NULL, 0);

                /* start the next */
                MP_start_timerEx(MP_Function_TaskID, MP_FUNC_OADUPDATE_EVENT, 10);
            }
        }
        return events ^ MP_FUNC_OADUPDATE_EVENT;
    }
#endif
    if(events & MP_FUNC_PERIODIC_SLEEP_EVENT)
    {
        if((MP_NwkInfo.nwkState == NWK_DETAIL_INIT && Hal_AllowSleep() && MP_AudioInfo.retrying_bitmap == 0)
                || (MP_NwkInfo.nwkState == NWK_DETAIL_ENDDEVICE  && IS_IDLE() && Hal_AllowSleep() && MP_AudioInfo.retrying_bitmap == 0)
          )
        {
            bool mFalse = FALSE;
            bool rx_on_idle;
            bool bBackFromSleep = FALSE;

            if(MAC_PwrNextTimeout() == 0)
            {
                AudioIntoSleep();
                WaitKeySleep(5000);
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
                FeedWatchDog();
#endif
                MAC_MlmeGetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);
                MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &mFalse);
                if(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP) == MAC_SUCCESS)
                {
                    UtilSleep(CC2430_PM1, MP_WORK_INTERVAL);
                    bBackFromSleep = TRUE;
                    MAC_PwrOnReq();
                }
                MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);
            }

            if(bBackFromSleep)
            {
                MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, MP_WORK_TIMEOUT);
            }
            else
            {
                MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, MP_WORK_INTERVAL + MP_WORK_TIMEOUT);
            }

        }
        else
        {
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, MP_WORK_INTERVAL + MP_WORK_TIMEOUT);
        }

        //the cycle is about 3~3.3 s
        //MP_set_event(MP_Function_TaskID, MP_FUNC_PERIODIC_SCAN_EVENT);
        MP_start_timerEx(MP_Function_TaskID, MP_FUNC_SCANPREP_EVENT, 5);
#ifdef	RSSI_INFORMATION
        Menu_UpdateLinkFlag(false);
#endif
        return events ^ MP_FUNC_PERIODIC_SLEEP_EVENT;
    }

    return 0;
}

/*********************************************************************
* @fn      MP_Function_HandleKeys
*
* @brief   This function is used to handle key event.
*
* @param   keys - key.
*               shifts -
*
* @return  none
*/
void MP_Function_HandleKeys( uint16 keys, uint8 shifts)
{

#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
    FeedWatchDog();
#endif
    HalResetBackLightEvent();
    if(keys == HAL_KEY_CANCEL)
    {
        if( ((HalGetPadLockStat() == PADLOCK_UNLOCKED) ) //|| (!HalGetPadLockEnable())
                && MP_TestLongPress(HAL_KEY_POWER, MP_POWER_LONGPRESS_TIMEOUT))
        {
            app_mpLeaveNwk_t app_mpLeaveNwk;
            app_mpLeaveNwk.hdr.srcaddr = MP_DevInfo.nmbr;
            app_mpLeaveNwk.hdr.dstaddr = APP_ARMSHORTADDR;
            app_mpLeaveNwk.seqnum = MP_seqnums.join_seqnum++;

            /* should send leave when ring poweroff */
            MP_SendSignalToCoord((uint8 *)&app_mpLeaveNwk, sizeof(app_mpLeaveNwk), MP_LEAVE_NOTIFY, true);
            MP_RingAndPoweOff();
        }
        else     //if (MP_NwkState != DEV_INIT)
        {

            if ((ON_AUDIO()))
            {
                MP_EndTalk();
            }
            else if (ON_CALLED() || ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
            {
                MP_EndTalk();
                Hal_RingStop();
                Hal_EndVoiceBell();
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
    switch (keys)
    {
    case HAL_KEY_STAR:
        break;
    case HAL_KEY_CALL:
    {
        //Because Handle_Key(keys) will set the call status(calling or called), so it must execute first
        //when the Dial_Key is pressed. MP will send message to the opposite side after the call status is set.
        if (ON_CALLED())   //Called side.
        {
            MP_SendCmd(MP_UP_ACCEPT, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
            MP_AudioInfo.accept_cnt = 0;
            MP_AudioInfo.retrying_bitmap |= MP_ACCEPT_RETRY_BIT;
            MP_start_timerEx(MP_TaskId, MP_ACCEPT_RETRY_EVENT, 200);

            MP_StartTalk();
            MP_ResetAudioStatus(MP_STOP_AUDIO_TIMEOUT);//FIXME:To avoid the other side not start normally.
        }
        else if (ON_CALLING())     //The first time calling
        {
            MP_ResetAudio();

            /* if dial myself */
            app_termNbr_t terminalnum;
            num_str2term(&terminalnum, (char *)Menu_GetDialNumBuf());
            if (num_isequal(&terminalnum, &MP_DevInfo.termNbr))
            {
                Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
                return;
            }

            /* stop previous signal retrys */
            MP_StopSignalRetrys();

            /* set peer num and cmd seqnum */
            MP_SetPeerNum(&terminalnum);
            MP_AudioInfo.cmdseqnum = MP_seqnums.dialup_seqnum++;

            /* start new dialup */
            MP_SendCmd(MP_UP_DIALUP, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
            MP_AudioInfo.retrying_bitmap |= MP_DIALUP_RETRY_BIT;
            MP_start_timerEx(MP_TaskId, MP_DIALUP_RETRY_EVENT, MP_SIGNAL_RETRY_TIME);

            /* set timeout */
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_RESET_EVENT, 30000);
            MP_ResetAudioStatus(40000);
        }
        break;
    }
    case HAL_KEY_SELECT:
    {
        if(ON_SM_SENDING())
        {
            uint8 rtn;
            uint8 *message = Menu_Get_SMS_Data();
            uint8 datasize = Menu_Get_SMS_Len();
            uint8 *sms = osal_mem_alloc(sizeof(app_mpSMS_t) + datasize);
            if (sms)
            {
                app_mpSMS_t *app_SMS = (app_mpSMS_t *)sms;
                app_SMS->srcnbr =  MP_DevInfo.termNbr;
                app_SMS->smstype = APP_MP_SMSTYPE_CONTENT;

                strcpy((char *)app_SMS->dstnbr.nbr, (const char *) Menu_GetNumBuf());
                app_SMS->len = datasize;
                osal_memcpy((void *)(app_SMS + 1), message, app_SMS->len);
                rtn = MP_SendSignalToCoord(sms, sizeof(app_mpSMS_t) + app_SMS->len, MP_SMS, true);
                osal_mem_free(sms);
            }
            if(rtn == MAC_SUCCESS)
            {
                Menu_handle_msg(MSG_SMS_SUCCESS, NULL, 0);
            }
            else
            {
                Menu_handle_msg(MSG_SMS_FAILED, NULL, 0);
            }
        }
        break;
    }
    default:
        break;
    }
    //HalResetBackLightEvent();
}


void MP_StartMenuLibEvt (uint16 timeout)
{
    MP_start_timerEx(MP_Function_TaskID, MP_FUNC_MENULIB_EVENT, timeout);
}

void MP_StopMenuLibEvt (void)
{
    osal_stop_timerEx(MP_Function_TaskID, MP_FUNC_MENULIB_EVENT);
    osal_unset_event(MP_Function_TaskID, MP_FUNC_MENULIB_EVENT);
}

void MP_RingAndPoweOff(void)
{
    Menu_handle_msg(MSG_POWEROFF_ANIM, NULL, 0);
    Hal_RingStart(RING_POWEROFF, OPENFLAG_ASONCE);
}

#ifdef MENU_RF_DEBUG
uint16 MP_GetSettedPanID(void)
{
    uint8 rs;
    uint16 panid;
    rs = osal_nv_read( ZCD_NV_SET_PANID, 0, sizeof(uint16), &panid);
    if(rs != ZSUCCESS)
    {
        panid = 0xFFFF;
    }
    return panid;
}
#endif



