/**************************************************************************************************
Filename:       MP_MP_Fucntion.c
Revised:        $Date: 2011/08/05 18:52:03 $
Revision:       $Revision: 1.16 $

Description:   This is a Mine Application helper task for mobile phone, which will not register to ZDO and has
not an endpoint, not receive ZDO events and so on. It's a helper task for MP_MP and only capture keyboard events
and handle several none-timely events, i.e: lcd, menu-swtich, key and so on and will not receive OTA pkt.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "ZComdef.h"
#include "string.h"
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
#include "OSAL_Clock.h"
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
#include "MenuChineseInputUtil.h"
#include "numtrans.h"
#include "Hal_sleep.h"

#include "OSAL_Nv.h"


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
uint16 timeSyncCnt = 0;
#ifdef CFG_STATION_SIMULATE
uint16 V2SimulateStationID = 0;
#endif
#ifdef CFG_TEST_WIRELESS
uint16 testDevID = 0;
uint16 recCount = 0;
uint16 recErrCount = 0;  //seqnum error
uint16 dstRecCount = 0;  //被测试设备接收到的测试帧数
uint16 sentSeq = 1;
uint8  handleSeq = 1;
#endif
extern  bool  Rssi_information;
extern  bool IsReadFull;
pointphone  Phone;

int keycnt=0;


/*********************************************************************
* LOCAL VARIABLES
*/

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void MP_RingAndPoweOff(void);
static void MP_Function_HandleKeys( uint16 keys, uint8 shifts);
/*********************************************************************
* @fn      MP_Function_Init
*
* @brief   Initialization function for the MineApp none voice OSAL task.
*
* @param   task_id - the ID assigned by OSAL.
*
* @return  none
*/

void MP_Function_Init(uint8 task_id)
{
    MP_Function_TaskID = task_id;
    RegisterForKeys(MP_Function_TaskID);

	Phone=osal_mem_alloc(sizeof(phone)+1);
	Phone->audiostatus=OFF;
	Phone->callstatus=NOTCALLED;
	Phone->whichphone=CARDphone;
	Phone->huangupstatus = WAITOFF;
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    StartWatchDog(DOGTIMER_INTERVAL_1S);
#endif
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
    if(events & SYS_EVENT_MSG)
    {
        MSGpkt = (osal_event_hdr_t *)osal_msg_receive(MP_Function_TaskID);
        while(MSGpkt)
        {
            switch(MSGpkt->event)
            {
            case KEY_CHANGE:
                MP_Function_HandleKeys(((keyChange_t *)MSGpkt)->keys, ((keyChange_t *)MSGpkt)->state);
                break;
            }
            osal_msg_deallocate((uint8 *)MSGpkt);
            MSGpkt = (osal_event_hdr_t *)osal_msg_receive(MP_Function_TaskID);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & MP_FUNC_RESET_EVENT)
    {
        if(ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
        {
            MP_SendCmd(MP_UP_CLOSE, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum++);
            Phone->audiostatus=OFF;
	        Phone->callstatus=NOTCALLED;

            MP_ResetAudio();
            MP_StopSignalRetrys();
            Hal_StartVoiceBell(VOICEBELL_NOBODY);
        }
        return events ^ MP_FUNC_RESET_EVENT;
    }

#ifdef CFG_STATION_SIMULATE
    if(events & MP_FUNC_SIMULATESTATION_EVENT)
    {
        if(V2SimulateStationID)
        {
            /* Setup shortAddr */
            uint16 ShortAddr =  0x0000;
            MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &ShortAddr);

            /* Setup PAN ID */
            uint16 MobilePanID = V2SimulateStationID;
            MAC_MlmeSetReq(MAC_PAN_ID, &MobilePanID);
            RfTofWrapper_tu RfTofData;
            RfTofData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
            RfTofData.tof_head.msgtype = 0x08;//TOF_STATION_AVAILABLE

            RfTofData.rf_tof_station_signal.u8AvailableMs = 0;    // card will send request in random (0 ~ u8Val) ms
            RfTofData.rf_tof_station_signal.u8StationStatus = 0;
            RfTofData.rf_tof_station_signal.u16CurSlot = 2;
            RfTofData.rf_tof_station_signal.u8RunMs = 0;

            RfTofData.rf_tof_station_signal.u8LocIdle = 0xFF;
            RfTofData.tof_head.len = 6;    //this is not a 4-byte struct !!!!!!!!!!!

            macMcpsDataReq_t  *pData = MAC_McpsDataAlloc(sizeof(RfTofWrapper_tu) + 4, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE);

            /* fill pData */
            if(pData)
            {
                pData->mac.srcAddrMode = SADDR_MODE_SHORT;
                pData->mac.dstAddr.addrMode = SADDR_MODE_SHORT;
                pData->mac.dstAddr.addr.shortAddr = 0xFFFF;
                pData->mac.dstPanId = 0xFFFF;
                pData->mac.msduHandle = 0;  //use heapIdx as machandle
                pData->mac.txOptions =  0 ? MAC_TXOPTION_ACK : 0; //always  need mac ack
                pData->sec.securityLevel = FALSE;

                /* app_header */
                osal_memcpy(pData->msdu.p, (void *) &RfTofData, sizeof(RfTofWrapper_tu));
                pData->msdu.len = sizeof(RfTofWrapper_tu);

                MAC_McpsDataReq(pData);
            }
        }
        else
        {
            /* Setup shortAddr */
            uint16 ShortAddr =  MP_DevInfo.nmbr;
            MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &ShortAddr);

            /* Setup PAN ID */
            uint16 MobilePanID = MOBILEPHONE_NWK_ADDR;
            MAC_MlmeSetReq(MAC_PAN_ID, &MobilePanID);
        }
        MP_start_timerEx(MP_Function_TaskID, MP_FUNC_SIMULATESTATION_EVENT, 20);
        return events ^ MP_FUNC_SIMULATESTATION_EVENT;
    }
#endif


#ifdef CFG_TEST_WIRELESS
    if(events & MP_FUNC_TESTWIRELESS_EVENT)
    {
        if(testDevID)
        {
            static uint8 u8Count = 0;

            if(u8Count++%20)
            {
                /* Setup PAN ID */
                RfTofWrapper_tu RfTofData;
                RfTofData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
                RfTofData.tof_head.msgtype = 0xf8;//TOF_STATION_AVAILABLE

                RfTofData.rf_tof_station_signal.u8AvailableMs = MP_NwkInfo.nwkState;    // card will send request in random (0 ~ u8Val) ms
                RfTofData.rf_tof_station_signal.u8StationStatus = MP_AudioInfo.retrying_bitmap;
                RfTofData.rf_tof_station_signal.u16CurSlot = sentSeq;
                RfTofData.rf_tof_station_signal.u8RunMs = 0;

                RfTofData.rf_tof_station_signal.u8LocIdle = 0xFF;
                RfTofData.tof_head.len = 6;    //this is not a 4-byte struct !!!!!!!!!!!


                macMcpsDataReq_t  *pData = MAC_McpsDataAlloc(sizeof(RfTofWrapper_tu) + 4, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE);

                /* fill pData */
                if(pData)
                {
                    pData->mac.srcAddrMode = SADDR_MODE_SHORT;
                    pData->mac.dstAddr.addrMode = SADDR_MODE_SHORT;
                    pData->mac.dstAddr.addr.shortAddr = 0x000F;
                    pData->mac.dstPanId = testDevID;
                    pData->mac.msduHandle = handleSeq;  //use heapIdx as machandle
                    pData->mac.txOptions =  0 ? MAC_TXOPTION_ACK : 0; //always  need mac ack
                    pData->sec.securityLevel = FALSE;

                    /* app_header */
                    osal_memcpy(pData->msdu.p, (void *) &RfTofData, sizeof(RfTofWrapper_tu));
                    pData->msdu.len = sizeof(RfTofWrapper_tu);

                    MAC_McpsDataReq(pData);
                }
             }
         #if 1
             else
             {
                    uint8 str[8]={0};
                    uint8 offset = 0;
                    LCD_Line_Clear(2);
                    LCD_Line_Clear(3);

                    LCD_Str_Print("s:", 0, 2, true);
                    _itoa(sentSeq, (char *)str, 10);
                    offset = osal_strlen("s:");
                    LCD_Str_Print((char *)str,offset, 2, true);
                    offset += osal_strlen((char *)str);

                    if(offset < 8)
                    {
                        offset = 8;
                    }
                    LCD_Str_Print("dr:", offset, 2, true);
                    offset += osal_strlen("dr:");
                    _itoa(dstRecCount, (char *)str, 10);
                    LCD_Str_Print((char *)str, offset, 2, true);


                    offset = 0;
                    LCD_Str_Print("r:", 0, 3, true);
                    offset = osal_strlen("r:");
                    _itoa(recCount, (char *)str, 10);
                    LCD_Str_Print((char *)str,offset, 3, true);
                    offset += osal_strlen((char *)str);
                    if(offset < 8)
                    {
                        offset = 8;
                    }
                    LCD_Str_Print("e:", offset, 3, true);
                    offset += osal_strlen("e:");
                    _itoa(recErrCount, (char *)str, 10);
                    LCD_Str_Print((char *)str, offset, 3, true);
             }
        #endif
        }
        else
        {
            sentSeq = 0;
            recCount = 0;
            recErrCount = 0;
            dstRecCount = 0;
            sentSeq = 0;
        }
        MP_start_timerEx(MP_Function_TaskID, MP_FUNC_TESTWIRELESS_EVENT, 100);
        return events ^ MP_FUNC_TESTWIRELESS_EVENT;
    }
#endif

    if(events & MP_FUNC_UPDATE_EVENT)
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

    if(events & MP_FUNC_MENULIB_EVENT)
    {
        Menu_ProcessMenuLibEvt();
        return events ^ MP_FUNC_MENULIB_EVENT;
    }
    if(events & MP_FUNC_CONTINUESPRESS_TEST_EVENT)
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

            /* if shedule failed, start scan after 120ms for timeout */
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SCAN_EVENT, 120);
        }
        else  /* start scan */
        {
            MP_set_event(MP_Function_TaskID, MP_FUNC_PERIODIC_SCAN_EVENT);
        }
        return events ^ MP_FUNC_SCANPREP_EVENT;
    }

    if(events & MP_FUNC_PERIODIC_SCAN_EVENT)
    {
        /* scan periodly */
        /* scan only current RSSI is not strong enough*/
        /*FIXME
            a) -50 is a good value???
        */

        /* do scan when no network, or my rssi < -35 */
        /* always scan when on audio, to assure for sellswitch*/
        if(MP_DevInfo.currentRssi < MP_CELL_THREHOLD1
                || !MP_IsNwkOn())//|| ON_AUDIO())
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
    if(events & MP_FUNC_PERIODIC_SCANEND_EVENT)
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

    if(events & MP_FUNC_POLL_EVENT)
    {
        if(MP_IsNwkOn() && !ON_AUDIO())
        {
            app_mpPoll_t app_mpPoll;
            app_mpPoll.hdr.srcaddr = MP_DevInfo.nmbr;
            app_mpPoll.hdr.dstaddr = APP_ARMSHORTADDR;
            app_mpPoll.polltype = APP_MPPOLL_TYPE_REQ;
            app_mpPoll.seqnum = MP_seqnums.poll_seqnum++;
            //app_mpPoll.flag = 0;


            /* sync time every half hour  and power on */
            if((timeSyncCnt==0)||( timeSyncCnt++ == (30*60/3)))
            {
                app_mpPoll.flag = APP_MPPOLL_FLAG_REQTIME;
                timeSyncCnt = 1;
            }
            else
            {
                app_mpPoll.flag = 0;
            }

            MP_SendSignalToCoord((uint8 *) &app_mpPoll, sizeof(app_mpPoll), MP_POLL, false);
        }

        /* lost  poll_ack/scan by once, decrease the currentrssi by 25dB */
        if(MP_DevInfo.hascoordlink == false)
        {
            if(MP_DevInfo.currentRssi > (MP_MIN_RSSI +40))
            {
                MP_DevInfo.currentRssi = MP_DevInfo.currentRssi > (MP_MIN_RSSI + 25) ? (MP_DevInfo.currentRssi - 25) : (MP_MIN_RSSI);
            }
        }
        MP_DevInfo.hascoordlink = false;
        return events ^ MP_FUNC_POLL_EVENT;
    }

    if(events & MP_FUNC_JOIN_SWITCH_EVENT)
    {
        uint8 count;

        /* join/switch condition */
        if(MP_IsNwkOn())
        {
            count = MP_DevInfo.currentRssi > MP_CELL_THREHOLD2 ? MP_CELL_COMPTIME : 1;

            while(MP_CellInfo.join_idx < MP_MAC_MAXSCAN_RESULTS && MP_CellInfo.CellInfo[MP_CellInfo.join_idx].cnt < count)
            {
                MP_CellInfo.join_idx++;
            }
        }
        else
        {
            count = 1;
            if(0xFFFF== MP_DevInfo.DesireCoordPanID)
            {
                while( MP_CellInfo.join_idx < MP_MAC_MAXSCAN_RESULTS && MP_CellInfo.CellInfo[MP_CellInfo.join_idx].cnt < count)
                {
                    MP_CellInfo.join_idx++;
                }
            }
            else
            {
                while(MP_CellInfo.join_idx < MP_MAC_MAXSCAN_RESULTS &&
                        ( MP_DevInfo.DesireCoordPanID!= MP_CellInfo.CellInfo[MP_CellInfo.join_idx].panid))
                {
                    MP_CellInfo.join_idx++;
                }
            }
        }

        /* start to associate */
        if(MP_CellInfo.join_idx < MP_MAC_MAXSCAN_RESULTS
                && MP_CellInfo.CellInfo[MP_CellInfo.join_idx].panid != 0xFFFF)
        {
            /* update nwk state */
            if(MP_NwkInfo.nwkState == NWK_DETAIL_INIT ||  MP_NwkInfo.nwkState == NWK_DETAIL_JOINASSOCING)
            {
                MP_NwkInfo.nwkState = NWK_DETAIL_JOINASSOCING;
            }
            else
            {
                MP_NwkInfo.nwkState = NWK_DETAIL_CELLSWITCHING;
            }
            HAL_AlarmSet(MP_ALARM_JOIN, 5000);    /* join timeout 5s */

            app_mpJoinNwk_t  app_mpJoinNwk;
            app_mpJoinNwk.hdr.srcaddr = MP_DevInfo.nmbr;
            app_mpJoinNwk.hdr.dstaddr = APP_ARMSHORTADDR;
            app_mpJoinNwk.joinnwktype = APP_MP_JOINNWK_REQ;
            app_mpJoinNwk.armid = (MP_NwkInfo.nwkState == NWK_DETAIL_INIT ? APP_INVALIDARMADDR : MP_DevInfo.armid);
            app_mpJoinNwk.seqnum = MP_seqnums.join_seqnum++;
            Hal_SendDataToAir((uint8 *)&app_mpJoinNwk, sizeof(app_mpJoinNwk), MP_CellInfo.CellInfo[MP_CellInfo.join_idx].panid, 0x0, MP_JOIN_NOTIFY, true, true);

            //Timeout 60ms
            if(ON_AUDIO())
            {
                osal_start_timerEx(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT, 300);
            }
            else
            {
                osal_start_timerEx(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT, 150);
            }
            MP_CellInfo.join_idx++;
        }
        else
        {
            /* join failed this round */
            MP_set_event(MP_Function_TaskID, MP_FUNC_POLL_EVENT);
            if(Hal_AllowSleep() && MP_AudioInfo.retrying_bitmap == 0
                    && (MP_NwkInfo.nwkState == NWK_DETAIL_JOINASSOCING || MP_NwkInfo.nwkState == NWK_DETAIL_CELLSWITCHING))
            {
                osal_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, 100);
            }
            if(MP_NwkInfo.nwkState == NWK_DETAIL_JOINASSOCING)
            {
                MP_NwkInfo.nwkState = NWK_DETAIL_INIT;
            }
            else if(MP_NwkInfo.nwkState == NWK_DETAIL_CELLSWITCHING)
            {
                MP_NwkInfo.nwkState= NWK_DETAIL_ENDDEVICE;
            }
        }
        return events ^ MP_FUNC_JOIN_SWITCH_EVENT;
    }

    if(events & MP_FUNC_PERIODIC_SLEEP_EVENT)
    {
        if(((MP_NwkInfo.nwkState == NWK_DETAIL_INIT && Hal_AllowSleep() && MP_AudioInfo.retrying_bitmap == 0 &&(Phone->whichphone!=INTERphone))
                || (MP_NwkInfo.nwkState == NWK_DETAIL_ENDDEVICE  && IS_IDLE() && Hal_AllowSleep() && MP_AudioInfo.retrying_bitmap == 0 &&(Phone->whichphone!=INTERphone))))
        //if(1)
        {
            bool mFalse = FALSE;
            bool mTure = TRUE;
            bool bBackFromSleep = FALSE;
            static uint8 sleepCount = 0;

            AudioIntoSleep();
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
            FeedWatchDog();
#endif
            MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &mFalse);
            if(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP) == MAC_SUCCESS)
            {
                //halSleep(MP_WORK_INTERVAL);
                //P0_6 = 1;
                halSleep(2000);
                bBackFromSleep = TRUE;

                //P0_6 = 0;
                sleepCount = 0;
            }
            /* halSleep will open mac */
            if(bBackFromSleep)
            {
                osalTimeUpdate();
            }
            else
            {
                MAC_PwrOnReq();
            }
            MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &mTure);

            if(bBackFromSleep)
            {
                MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, 20);
                //MP_SendCmd(MP_UP_DIALUP, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum++);
            }
            else
            {
                MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, 50);
                if( ++sleepCount < 60)
                {
                    return events ^ MP_FUNC_PERIODIC_SLEEP_EVENT;
                }
                sleepCount = 0;
            }
        }
        else
        {
            //MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, MP_WORK_INTERVAL + MP_WORK_TIMEOUT);
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, 100);
        }
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
void MP_Function_HandleKeys(uint16 keys, uint8 shifts)
{

#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
    FeedWatchDog();
#endif

    switch(keys)
    {
    case HAL_KEY_STAR:
        break;
    case  HAL_KEY_BACKSPACE:
    {

      Phone->whichphone = INTERphone;
      if(Phone->audiostatus==OFF)
      {
        Phone->audiostatus=ON;
        Phone->panid=0xfff4;
        MP_DevInfo.CoordPanID=Phone->panid;
        HalAudioOpen();
      }
      else
      {
        Phone->audiostatus=OFF;
      }
    }
    break;
    case  HAL_KEY_SELECT:
    {
      if(Phone->callstatus == CALLED)break;
      Phone->whichphone = CARDphone;
      /*if((Phone->whichphone == CARDphone) && (Phone->audiostatus == ON))
      {
		Phone->audiostatus = OFF;
		Phone->huangupstatus = WAITON;
		if((ON_AUDIO()))
		{
		    MP_EndTalk();
		}
		else if(ON_CALLED() || ON_CALLING() || ON_CALLINGWAIT() || ON_FOUND())
		{
		    MP_EndTalk();
		    Hal_RingStop();
		    Hal_EndVoiceBell();
		}
		osal_stop_timerEx(MP_TaskId, MP_DIALUP_RETRY_EVENT);
		osal_stop_timerEx(MP_TaskId, MP_STOP_AUDIO_EVENT);
		osal_stop_timerEx(MP_Function_TaskID, MP_FUNC_RESET_EVENT);
		MP_start_timerEx(MP_TaskId, MP_HANG_UP_TIMEOUT_EVENT, MP_HANG_UP_WAIT_TIME);
		//The  CMD_UP_CLOSE message must be send to caller  before hanle_key()  when the called MP
		//cancel the incmoming. Because hanle_key() will clear the call status.
		//KeyIntoSleep();
      }*/
      if((Phone->whichphone == CARDphone) && (Phone->audiostatus == OFF))
      {
        //if(MP_AudioInfo.retrying_bitmap&0x4)
        //{
          //break;
        //}

		SET_ON_CALLING();      //The first time calling
		MP_ResetAudio();
		/* stop previous signal retrys */
		MP_StopSignalRetrys();
		app_termNbr_t termNbr;
		char savenbr[20];
        Phone->audiostatus = ON;
		/* set peer num and cmd seqnum */
		_itoa(9999, savenbr, 10);
		num_str2term(&termNbr,savenbr);
		MP_SetPeerNum(&termNbr);

        //MP_AudioInfo.cmdseqnum = 4999;
		//MP_AudioInfo.cmdseqnum = MP_seqnums.dialup_seqnum++;
		/* start new dialup */
		//MP_SendCmd(MP_UP_DIALUP, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum++);
        MP_SendCmd(MP_UP_DIALUP, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum++);
		MP_AudioInfo.retrying_bitmap |= MP_DIALUP_RETRY_BIT;
		MP_start_timerEx(MP_TaskId, MP_DIALUP_RETRY_EVENT, MP_SIGNAL_RETRY_TIME);

		/* set timeout */
		MP_start_timerEx(MP_Function_TaskID, MP_FUNC_RESET_EVENT, 30000);
		MP_ResetAudioStatus(40000);
      }

        break;
    }
#if 0
    case HAL_KEY_SELECT:
    {

/*
*/
        if(ON_SM_SENDING())
        {
            uint8 rtn;
            uint8 *message = Menu_Get_SMS_Data();
            uint8 datasize = Menu_Get_SMS_Len();
            uint8 *sms = osal_mem_alloc(sizeof(app_mpSMS_t) + datasize);
            uint16 seqnum;
            osal_nv_read(MP_SMS_SEQNUM,0, sizeof(uint16), &seqnum);
            if(sms)
            {
                app_mpSMS_t *app_SMS = (app_mpSMS_t *)sms;
                app_SMS->srcnbr =  MP_DevInfo.termNbr;
                app_SMS->smstype = APP_MP_SMSTYPE_CONTENT;

                //unsigned int  num_str2term(app_termNbr_t *pterm, char* s)
                num_str2term((app_termNbr_t *)app_SMS->dstnbr.nbr,(char *)Menu_GetNumBuf());
                //strcpy((char *)app_SMS->dstnbr.nbr, (const char *) Menu_GetNumBuf());
                app_SMS->len = datasize;
                app_SMS->seqnum=seqnum++;
                osal_nv_write(MP_SMS_SEQNUM,0, sizeof(uint16), &seqnum);
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
#endif
    default:
        break;
    }
    //HalResetBackLightEvent();
}

void MP_StartMenuLibEvt(uint16 timeout)
{
    MP_start_timerEx(MP_Function_TaskID, MP_FUNC_MENULIB_EVENT, timeout);
}

void MP_StopMenuLibEvt(void)
{
    osal_stop_timerEx(MP_Function_TaskID, MP_FUNC_MENULIB_EVENT);
    osal_clear_event(MP_Function_TaskID, MP_FUNC_MENULIB_EVENT);
}

void MP_RingAndPoweOff(void)
{
    Hal_RingStart(RING_POWEROFF, OPENFLAG_ASSMS_POW);
}



