/**************************************************************************************************
Filename:       MP_global.c
Revised:        $Date: 2011/08/24 18:07:25 $
Revision:       $Revision: 1.8 $

Description:  User definable common Parameters.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "iocc2530.h"
#include "App_cfg.h"
#include "key.h"
#include "ZComdef.h"
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#include "hal_audio.h"
#include "KeyAudioISR.h"
#include "lcd_serial.h"
#endif
#include "numtrans.h"
#include "hal_drivers.h"
#include "mac_pib.h"
#include "OnBoard.h"
#include "StringUtil.h"

/* app */
#include "MobilePhone.h"
#include "MobilePhone_Function.h"
#include "MobilePhone_global.h"

/*********************************************************************
* typedefs
*/
/*********************************************************************
* LOCAL VARIABLES
*/
/* common device info */
MP_DevInfo_t   MP_DevInfo;
MP_AudioInfo_t MP_AudioInfo;
MP_NwkInfo_t   MP_NwkInfo;
MP_ScanInfo_t  MP_ScanInfo;
MP_CellCfg_info_t MP_CellInfo;
MP_SeqNums_t MP_seqnums;
/*********************************************************************
* LOCAL VARIABLES
*/

/*identify current MP status: idle/calling/called/talking*/
static uint8 nWorkStatus;
static  const int8 RSSI_table[] =
{
    -80,     // RSSI: -80
    -70,     // RSSI: -70
    -50,   // RSSI: -50
    -30,    // RSSI: -30
};
/*********************************************************************
* LOCAL FUNCTIONS
*/
/*********************************************************************
* FUNCTIONS
*/
bool MP_JudgeStatus(uint8 WorkStatus)
{
    return (nWorkStatus == WorkStatus);
}

void MP_SetStatus(uint8 WorkStatus)
{
    nWorkStatus = WorkStatus;
}

void MP_ResetAudio(void)
{
	uint16 lastcmdseqnum;
    lastcmdseqnum = MP_AudioInfo.cmdseqnum;
    MP_AudioInfo.peernmbr = APP_INVALIDARMADDR;
    memset(MP_AudioInfo.peer_termnbr.nbr, 0xFF, NMBRDIGIT);
    MP_AudioInfo.IsTalkWithGateWay = FALSE;

    MP_AudioInfo.cmdseqnum = lastcmdseqnum;
    AudioSetInputGain(INGAIN_PHONE2PHONE);
    MP_ResetFrameblk();
    HalRingClose();
    HalAudioClose();
    HalResetBackLightEvent();
    FlushAudioBuffer();
}

void MP_StartTalk(void)
{
    HalResetBackLightEvent();
    shake_ctrl(false);
    Hal_RingStop();
    HalAudioOpen();
}

void MP_EndTalk(void)
{

   if(Phone->whichphone==CARDphone)
   	MP_SendCmd(MP_UP_CLOSE, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum++);



    /* stop sending dialup and accept */
    MP_StopSignalRetrys();

    /* signal retry */
    MP_AudioInfo.close_cnt = 0;
    MP_AudioInfo.retrying_bitmap |= MP_CLOSE_RETRY_BIT;
    MP_start_timerEx(MP_TaskId,MP_CLOSE_RETRY_EVENT, 100);
}

uint8 MP_Rssi2Level(int8 rssi)
{
    uint8 len = sizeof(RSSI_table) / sizeof(RSSI_table[0]);
    uint8 i;

    for(i = 0; i < len; i++)
    {
        if(rssi < RSSI_table[i])
        {
            return i;
        }
    }
    return len;
}
//uint16 debugnum[100];
//uint16 debugnum2[100];
//int testi=0;
//int testj=0;
uint8 MP_SendCmd(uint8  cmdtype, const app_termNbr_t *dstnmbr, uint16 seqnum)
{
    app_mpCmd_t	app_mpCmd;
    app_mpCmd.srcnbr = MP_DevInfo.termNbr;
    app_mpCmd.dstnbr = *dstnmbr;
    app_mpCmd.cmdtype = cmdtype;
    app_mpCmd.reserved = 0;
    app_mpCmd.seqnum = seqnum;
    if(cmdtype == MP_UP_CLOSE)
    {
//      debugnum[testi++] = *(uint16 *)app_mpCmd.dstnbr.nbr;
//      debugnum2[testj++] = seqnum;
    }
    return MP_SendSignalToAllCoord((uint8 *)&app_mpCmd, sizeof(app_mpCmd), MP_CMD_UP, true);
}


uint8 MP_SendSignalToCoord(const uint8 *p, uint8 len, uint8 msgtype, bool retrans )
{
    return Hal_SendDataToAir(p,  len, MP_DevInfo.CoordPanID, 0xffff, msgtype, true, retrans);
}

uint8 MP_SendSignalToAllCoord(const uint8 *p, uint8 len, uint8 msgtype, bool retrans )
{
    return Hal_SendDataToAir(p,  len, 0xFFFF, 0x0000, msgtype, false, false);
}

uint8 MP_SendTunnelToAllCoord(const uint8 *p, uint8 len, uint8 msgtype, bool retrans )
{
    return Hal_SendDataTuunel(p,  len, 0xFFFF, 0x0000, msgtype, false, false);
}

uint8 MP_SendSignalToAllDev(const uint8 *p, uint8 len, uint8 msgtype, bool retrans )
{
    return Hal_SendDataToAir(p,  len, 0xFFFF, 0xFFFF, msgtype, false, false);
}

void MP_start_timerEx( uint8 taskID, uint16 event_id, uint16 timeout_value )
{
    if(ZSuccess != osal_start_timerEx( taskID,  event_id,  timeout_value))
    {
        SystemReset();
    }
}
void MP_set_event(uint8 task_id, uint16 event_flag)
{
    //if(ZSuccess != osal_set_event( task_id,  event_flag))
    if(ZSuccess != osal_start_timerEx(task_id,  event_flag, 1))
    {
        SystemReset();
    }
}
bool MP_IsNwkOn(void)
{

    if(MP_NwkInfo.nwkState == NWK_DETAIL_ENDDEVICE || MP_NwkInfo.nwkState == NWK_DETAIL_CELLSWITCHING)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void MP_UpdateCellInfo(void)
{
    MP_CellInfo.join_idx = 0;
    uint8 i;

    for(i = 0; i < MP_MAC_MAXSCAN_RESULTS; i++)
    {
        if(MP_CellInfo.CellInfo[i].matched)  /* mark off matched flag for next scan */
        {
            MP_CellInfo.CellInfo[i].matched = false;
        }
        else		/* do not scaned the last time */
        {
            MP_CellInfo.CellInfo[i].cnt = 0;
            MP_CellInfo.CellInfo[i].panid = APP_INVALIDARMADDR;
            MP_CellInfo.CellInfo[i].rssi  = MP_MIN_RSSI;
        }
    }
}

void MP_ClearCellInfo(void)
{
    osal_memset(&MP_CellInfo, 0, sizeof(MP_CellCfg_info_t) );

    uint8 i;
    for(i = 0; i < MP_MAC_MAXSCAN_RESULTS; i++)
    {
        MP_CellInfo.CellInfo[i].panid = APP_INVALIDARMADDR;
        MP_CellInfo.CellInfo[i].rssi  = MP_MIN_RSSI;
        MP_CellInfo.CellInfo[i].cnt = 0;
    }
}

bool MP_IsCellInfoEmpty(void)
{
    bool flag = true;

    for(uint8 i = 0; i < MP_MAC_MAXSCAN_RESULTS; i++)
    {
        if(MP_CellInfo.CellInfo[i].cnt > 0)
        {
            flag = false;
            break;
        }
    }
    return flag;
}

/* sort by rssi */
void MP_SortCellInfo(void)
{
    MP_cell_info_t *p;
    p = MP_CellInfo.CellInfo;

    for(uint8 i = 0; i < MP_MAC_MAXSCAN_RESULTS; i++)
    {
        for(uint8 j = i + 1; j < MP_MAC_MAXSCAN_RESULTS; j++)
        {
            if(p[i].rssi < p[j].rssi)
            {
                MP_cell_info_t tmp;
                tmp = p[i];
                p[i] = p[j];
                p[j] = tmp;
            }
        }
    }
}

void MP_VoiceScanShedule(void)
{
#define VOICE_TIMEINTERVAL	 (20*VOICE_IDX_THRESHOLD)  // 20ms*12

    if( MP_ScanInfo.isinshedule )
    {
        uint32 tick = osal_GetSystemClock();
        uint32 diff = tick - MP_AudioInfo.peer_tick;

        if(diff > 2*VOICE_TIMEINTERVAL)
        {
            /*peer voice losted more than 2 times, start scan immediately */
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SCAN_EVENT, 5);
            MP_ScanInfo.isinshedule = false;
        }
        else
        {
            uint16 time;
            diff %= VOICE_TIMEINTERVAL;

            if(diff < VOICE_TIMEINTERVAL / 2)
            {
                time = 10;
            }
            else
            {
                time = VOICE_TIMEINTERVAL - diff + 30;
            }
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_PERIODIC_SCAN_EVENT, time);
            MP_ScanInfo.isinshedule = false;
        }
    }
}
void MP_SetPeerNum(const app_termNbr_t *pnbr)//const app_mpCmd_t *pCmd)
{
    if(pnbr == NULL)   /*initial */
    {
        MP_AudioInfo.peernmbr = MP_SHORT_INVALIDNMMBR;
        for(uint8 i=0; i<APP_NMBRDIGIT; i++)
        {
            MP_AudioInfo.peer_termnbr.nbr[i] = 0xFF;
        }
    }
    else
    {
        char str[21];
        num_term2str(str, pnbr);
        //if(num_term_getlen(pnbr) == 4)        //YIRI MP has a 4-digital number
        if((atoul(str)<=NUMBER_PHONE_MAX)&&(atoul(str)>=NUMBER_PHONE_MIN))
        {
            MP_AudioInfo.peernmbr = atoul(str);
            MP_AudioInfo.peer_termnbr = *pnbr;
            MP_AudioInfo.IsTalkWithGateWay = FALSE;
            AudioSetInputGain(INGAIN_PHONE2PHONE);

        }
        else
        {
            MP_AudioInfo.peernmbr = MP_SHORT_GATEWAYNMBR;
            MP_AudioInfo.peer_termnbr = *pnbr;
            AudioSetInputGain(INGAIN_PHONE2GATEWAY);
            MP_AudioInfo.IsTalkWithGateWay = TRUE;
        }
    }

}

void MP_StopSignalRetrys(void)
{
    osal_stop_timerEx(MP_TaskId, MP_DIALUP_RETRY_EVENT);
    osal_clear_event(MP_TaskId, MP_DIALUP_RETRY_EVENT);

    osal_stop_timerEx(MP_TaskId, MP_ACCEPT_RETRY_EVENT);
    osal_clear_event(MP_TaskId, MP_ACCEPT_RETRY_EVENT);
    MP_AudioInfo.accept_cnt = 0;

    osal_stop_timerEx(MP_TaskId, MP_CLOSE_RETRY_EVENT);
    osal_clear_event(MP_TaskId, MP_CLOSE_RETRY_EVENT);
    MP_AudioInfo.close_cnt = 0;

    /* unset all retry bitmaps */
    MP_AudioInfo.retrying_bitmap  = 0;
}
#if 0
unsigned char Front_three_num_isequal(const app_termNbr_t *p1, const app_termNbr_t *p2)
{
   if((p1->nbr[0] == p2->nbr[0]) && ((p1->nbr[1] & 0x0F) == (p2->nbr[1] & 0x0F))
   	&&(p1->nbr[2] == 0xFF)&&(p2->nbr[2] == 0xff))
   	{
	   return 1;
   	}
    else
    {
		return 0;

    }
}
#endif







