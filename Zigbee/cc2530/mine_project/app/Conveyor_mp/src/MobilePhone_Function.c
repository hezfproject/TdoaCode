/**************************************************************************************************
Filename:       MP_MP_Fucntion.c
Revised:        $Date: 2011/08/12 01:32:36 $
Revision:       $Revision: 1.2 $

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
#include "TimeUtil.h"

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

//#include "StringUtil.h"
#include "MenuChineseInputUtil.h"
#include "Hal_sleep.h"

#include "OSAL_Nv.h"
#include "StringUtil.h"
#include "Conveyor.h"
#include "crc.h"

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
extern  bool  Rssi_information;

/*********************************************************************
* LOCAL VARIABLES
*/

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void MP_RingAndPoweOff(void);
static void MP_Function_HandleKeys(uint16 keys, uint8 shifts);
static uint8 Conveyor_sent(uint16 num, uint16  seqnum);
#define  CONVEYOR_RESENT_COUNT              (30-1)
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


    if(ResetReason() == RESET_FLAG_WATCHDOG && param.abnormalRst == TRUE)
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
        //Menu_handle_msg(MSG_POWERON_ANIM, NULL, 0);
        Hal_RingStart(RING_POWERON, OPENFLAG_ASSMS_POW);
        HalResetBackLightEvent();
    }

    param.abnormalRst = TRUE;
    *(StoreParam_t *)MP_STOREPARAM_ADDR = param;

    /* period do this  events */
    MP_start_timerEx(MP_Function_TaskID, MP_FUNC_UPDATE_EVENT, 100);
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
            MP_ResetAudio();
            Hal_StartVoiceBell(VOICEBELL_NOBODY);
        }
        return events ^ MP_FUNC_RESET_EVENT;
    }
    
    if(events & MP_FUNC_CONVEYOR_RESENT_EVENT)
    {
        static uint8  resent_count;        
        if(Conveyor_Get_isresent()&&(resent_count++<CONVEYOR_RESENT_COUNT))
            {
                Conveyor_sent(0x00,0x00);                     
            }
            else
            {
                if(resent_count>=(CONVEYOR_RESENT_COUNT+1))
                {
                        Menu_Set_SMS_Full_Ring_Flag(TRUE);
                        Hal_RingStart(RING_ANY, OPENFLAG_ASSMS_POW);
                        HalResetBackLightEvent();
                        Menu_handle_msg(MSG_CONVEYOR_FAILED, NULL, 0);
                        Conveyor_Set_resent(0x00);      
                        resent_count=0;
                }                
           }
        return events ^ MP_FUNC_CONVEYOR_RESENT_EVENT;
    }

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
                || !MP_IsNwkOn())
        {
            /* start scan */
            MP_UpdateCellInfo();
            MP_ScanInfo.isscaning = true;


            app_Timac_Mp_Scan_t  mp_Scan;
            mp_Scan.msgtype = TIMAC_MP_SCAN;
            mp_Scan.scantype = APP_SCAN_TYPE_REQ;
            mp_Scan.seqnum = MP_seqnums.scan_seqnum++;

            //send ARM_ID request to all stations
            Hal_SendDataToAir((uint8 *)&mp_Scan, sizeof(mp_Scan), 0xFFFF, 0x0000, false, false);

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
            app_MP_Poll_Req_t app_mpPoll;
            app_mpPoll.msgtype = MP_POLL_REQ;
            app_mpPoll.nbr = MP_DevInfo.termNbr;

            static uint16 timeSyncCnt;

            /* sync time every half hour */
            if( timeSyncCnt++ == (30*60/3))
            {
                app_mpPoll.flag = POLL_FLAG_REQTIME;
                timeSyncCnt = 0;
            }
            else
            {
                app_mpPoll.flag = 0;
            }
            MP_SendSignalToCoord((uint8 *) &app_mpPoll, sizeof(app_mpPoll), false);
        }
        /* lost  poll_ack/scan by once, decrease the currentrssi by 25dB */
        if(MP_DevInfo.hascoordlink == false)
        {
            MP_DevInfo.currentRssi = MP_DevInfo.currentRssi > (MP_MIN_RSSI + 25) ? (MP_DevInfo.currentRssi - 25) : MP_MIN_RSSI;
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


            app_Timac_JoinNwk_t  app_mpJoinNwk;
            app_mpJoinNwk.msgtype = TIMAC_JOIN_NOTIFY;
            app_mpJoinNwk.joinnwktype = APP_TIMAC_JOINNWK_REQ;
            app_mpJoinNwk.seqnum = MP_seqnums.join_seqnum++;
            app_mpJoinNwk.panid = (MP_NwkInfo.nwkState == NWK_DETAIL_INIT ? INVALIDNMMBR : MP_DevInfo.CoordPanID);
            app_mpJoinNwk.srcnbr = MP_DevInfo.termNbr;
            if(MP_NwkInfo.nwkState == NWK_DETAIL_INIT)
            {
                app_mpJoinNwk.status = APP_TIMAC_STATUS_IDLE;
            }
            else if(IS_IDLE())
            {
                app_mpJoinNwk.status = APP_TIMAC_STATUS_IDLE;
            }
            else if(ON_AUDIO())
            {
                app_mpJoinNwk.status = APP_TIMAC_STATUS_ONVOICE;
            }
            else
            {
                app_mpJoinNwk.status = APP_TIMAC_STATUS_ONCMD;
            }

            Hal_SendDataToAir((uint8 *)&app_mpJoinNwk, sizeof(app_mpJoinNwk), MP_CellInfo.CellInfo[MP_CellInfo.join_idx].panid, 0x0, true, true);

            //Timeout 60ms
            osal_start_timerEx(MP_Function_TaskID, MP_FUNC_JOIN_SWITCH_EVENT, 300);
            MP_CellInfo.join_idx++;
        }
        else
        {
            /* join failed this round */
            MP_set_event(MP_Function_TaskID, MP_FUNC_POLL_EVENT);
        }
        return events ^ MP_FUNC_JOIN_SWITCH_EVENT;
    }

    if(events & MP_FUNC_PERIODIC_SLEEP_EVENT)
    {
    if(!Conveyor_Get_isresent())
        {
              if((MP_NwkInfo.nwkState == NWK_DETAIL_INIT && Hal_AllowSleep() && MP_AudioInfo.retrying_bitmap == 0)
                      || (MP_NwkInfo.nwkState == NWK_DETAIL_ENDDEVICE  && IS_IDLE() && Hal_AllowSleep() && MP_AudioInfo.retrying_bitmap == 0)
                )
              {
                  bool mFalse = FALSE;
                  bool mTure = TRUE;
                  bool bBackFromSleep = FALSE;
      
                  AudioIntoSleep();
                  WaitKeySleep(5000);
      #if(defined WATCHDOG) && (WATCHDOG==TRUE)
                  FeedWatchDog();
      #endif
                  MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &mFalse);
                  if(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP) == MAC_SUCCESS)
                  {
                      halSleep(MP_WORK_INTERVAL * 1000);
                      bBackFromSleep = TRUE;
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
                      MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, MP_WORK_TIMEOUT);
                  }
                  else
                  {
                      MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, MP_WORK_INTERVAL * 1000 + MP_WORK_TIMEOUT);
                  }
      
              }
              else
              {
                  MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT, MP_WORK_INTERVAL * 1000 + MP_WORK_TIMEOUT);
              }
        }
        else
        {
           MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SLEEP_EVENT,  1 * 1000+20);
           MP_start_timerEx(MP_Function_TaskID, MP_FUNC_CONVEYOR_RESENT_EVENT, 1 * 1000);
        }
        //the cycle is about 3~3.3 s
        //MP_set_event(MP_Function_TaskID, MP_FUNC_PERIODIC_SCAN_EVENT);
        MP_start_timerEx(MP_Function_TaskID, MP_FUNC_SCANPREP_EVENT, 5);
//#ifdef  RSSI_INFORMATION
        if(Rssi_information==true)
        {
            Menu_UpdateLinkFlag(false);
        }
//#endif
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
    HalResetBackLightEvent();
    if(keys == HAL_KEY_CANCEL)
    {
        if(((HalGetPadLockStat() == PADLOCK_UNLOCKED))   //|| (!HalGetPadLockEnable())
                && MP_TestLongPress(HAL_KEY_POWER, MP_POWER_LONGPRESS_TIMEOUT))
        {

            app_Timac_LeaveNwk_t app_mpLeaveNwk;
            app_mpLeaveNwk.msgtype = TIMAC_LEAVE_NOTIFY;
            app_mpLeaveNwk.seqnum = MP_seqnums.join_seqnum++;
            app_mpLeaveNwk.srcnbr = MP_DevInfo.termNbr;

            /* should send leave when ring poweroff */
            MP_SendSignalToCoord((uint8 *)&app_mpLeaveNwk, sizeof(app_mpLeaveNwk), true);
            MP_RingAndPoweOff();
        }
        else     //if (MP_NwkState != DEV_INIT)
        {

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
            //The  CMD_UP_CLOSE message must be send to caller  before hanle_key()  when the called MP
            //cancel the incmoming. Because hanle_key() will clear the call status.
            Menu_handle_key(keys, KEY_SHORT_PRESS);
            KeyIntoSleep();
        }
        return;
    }
    else if(keys == HAL_KEY_POUND)
    {
        if(((HalGetPadLockStat() == PADLOCK_UNLOCKED))   //|| (!HalGetPadLockEnable())
                && MP_TestLongPress(HAL_KEY_POUND, MP_POWER_LONGPRESS_TIMEOUT))
        {
#ifdef ONLY_NEED_STOP                   
	    menu_JumptoMenu(MENU_ID_CONVEYOR_STOP);
#else   
            menu_JumptoMenu(MENU_ID_CONVEYOR_HANDLE);
#endif
        }
        else    
        {
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
        if(ON_CALLED())    //Called side.
        {
            MP_SendCmd(CMD_UP_ACCEPT, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
            MP_AudioInfo.accept_cnt = 0;
            MP_AudioInfo.retrying_bitmap |= MP_ACCEPT_RETRY_BIT;
            MP_start_timerEx(MP_TaskId, MP_ACCEPT_RETRY_EVENT, 200);

            MP_StartTalk();
            MP_ResetAudioStatus(MP_STOP_AUDIO_TIMEOUT);//FIXME:To avoid the other side not start normally.
        }
        else if(ON_CALLING())      //The first time calling
        {
            MP_ResetAudio();

            /* if dial myself */
            termNbr_t terminalnum;
            char* pDial = (char *)Menu_GetDialNumBuf();
            strcpy(terminalnum.Nmbr, pDial);

            if(strcmp(terminalnum.Nmbr, MP_DevInfo.termNbr.Nmbr) == 0)
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
            MP_SendCmd(CMD_UP_DIALUP, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum);
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
            uint8 rtn = FAILURE;
            uint8 *message = Menu_Get_SMS_Data();
            uint8 datasize = Menu_Get_SMS_Len();
            uint16 seqnum;
            osal_nv_read(MP_SMS_SEQNUM,0, sizeof(uint16), &seqnum);
            if(message!=NULL && datasize <= APP_SMS_MAX_LEN)
            {

                uint8 *sms = osal_mem_alloc(sizeof(app_SMS_t) + datasize);
                if(sms)
                {
                    app_SMS_t *app_SMS = (app_SMS_t *)sms;
                    app_SMS->msgtype = SMS;
                    strcpy((char *)app_SMS->nmbr.Nmbr , (const char *) Menu_GetNumBuf());
                    app_SMS->len = datasize;
                    app_SMS->blk=seqnum++;
                    osal_nv_write(MP_SMS_SEQNUM,0, sizeof(uint16), &seqnum);
                    osal_memcpy((void *)(app_SMS + 1), message, app_SMS->len);

                    rtn = MP_SendSignalToCoord(sms, sizeof(app_SMS_t) + app_SMS->len, true);
                    osal_mem_free(sms);
                }
                if(rtn == SUCCESS)
                {
                    Menu_handle_msg(MSG_SMS_SUCCESS, NULL, 0);
                }
                else
                {
                    Menu_handle_msg(MSG_SMS_FAILED, NULL, 0);
                }
            }
        }
	else if(ON_CONVEYOR_SENDING())
	{
	        uint8  rtn;
                uint16 seqnum=0;
                
	        Conveyor_Set_resent(0xff);
            
                osal_nv_read(MP_SMS_SEQNUM,0, sizeof(uint16), &seqnum);
                seqnum++;
                osal_nv_write(MP_SMS_SEQNUM,0, sizeof(uint16), &seqnum);
              
	        rtn=Conveyor_sent(LO_UINT16(atoul(num_buf.p)),seqnum);     
                if(rtn == SUCCESS)
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

uint8 Conveyor_sent(uint16 num, uint16  seqnum)
{
            uint8 rtn = FAILURE;
            static uint16 save_seqnum;
            static uint16  save_num;
            
            if(num)save_num=num;          
            if(seqnum)save_seqnum=seqnum;
            
            uint8 *conveyor_s = osal_mem_alloc(sizeof(Conveyor_sms_t) + 1);
            if(conveyor_s)
                {
                    app_Conveyor_cmd_t *app_Conveyor_cmd = (app_Conveyor_cmd_t *)conveyor_s;
                    app_Conveyor_cmd->msgtype = CONVEYOR_CMD;
		    app_Conveyor_cmd->num=save_num;//LO_UINT16(atoul(num_buf.p))	;		
                    app_Conveyor_cmd->blk=save_seqnum;
		    app_Conveyor_cmd->reserve=0; 	
		    app_Conveyor_cmd->cmdtype=sent_conveyortype;		      
		    app_Conveyor_cmd->crc=CRC16(conveyor_s,sizeof(app_Conveyor_cmd_t)-sizeof(unsigned short),0xFFFF);	  ;

                    rtn = MP_SendSignalToCoord(conveyor_s, sizeof(app_Conveyor_cmd_t) , true);
                    osal_mem_free(conveyor_s);
                    
              }        
            return rtn;
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
    Menu_handle_msg(MSG_POWEROFF_ANIM, NULL, 0);
    Hal_RingStart(RING_POWEROFF, OPENFLAG_ASSMS_POW);
}



