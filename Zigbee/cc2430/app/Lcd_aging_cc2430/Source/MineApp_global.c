/**************************************************************************************************
Filename:       MineApp_global.c
Revised:        $Date: 2011/07/22 17:38:35 $
Revision:       $Revision: 1.18 $

Description:  User definable common Parameters.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "iocc2430.h"
#include "MineApp_global.h"
#include "MineApp_Local.h"
#include "App_cfg.h"
#include "MineApp_MP.h"
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#include "hal_audio.h"
#include "KeyAudioISR.h"
#include "lcd_interface.h"
#endif
#include "hal_drivers.h"
#include "mac_pib.h"
#include "MacUtil.h"
#include "OnBoard.h"
#include "ZGlobals.h"
#include "string.h"
/*********************************************************************
* LOCAL VARIABLES
*/

/*identify current MP status: idle/calling/called/talking*/
static uint8 nWorkStatus;

/* indicate the system is searching network */
static bool MineApp_SearchingNWK = FALSE;

/*********************************************************************
* FUNCTIONS
*/
bool MineApp_JudgeStatus(uint8 WorkStatus)
{
	return (nWorkStatus == WorkStatus);
}

void MineApp_SetStatus(uint8 WorkStatus)
{
	nWorkStatus = WorkStatus;
}

void MineApp_EndTalk(void)
{
	MineApp_ResetAudio();

#if !(defined AUDIO_TEST)
	AppData.app_Cmdup.msgtype = ZB_CMD_UP;
	AppData.app_Cmdup.cmd = CMD_UP_CLOSE;
	AppData.app_Cmdup.srcnbr = MineApp_NodeAttr.Node_nmbr;
	MineApp_SignalLengh = sizeof(app_CmdUp_t);
	MineApp_SendSignal(MineApp_SignalLengh);
	/*app_CmdUp_t AppData;
	afAddrType_t dstAddr;
	dstAddr.endPoint = MINEAPP_ENDPOINT;
	dstAddr.addrMode = afAddr16Bit;
	dstAddr.addr.shortAddr = 0;
	AppData.msgtype = ZB_CMD_UP;
	AppData.cmd = CMD_UP_CLOSE;
	AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
		MINEAPP_CLUSTERID, sizeof(app_CmdUp_t), (byte *)&AppData,
		&MineApp_TransID, INIT_OPN|AF_ACK_REQUEST, MAX_DEPTH);
	*/
#endif
}

void MineApp_ResetAudio(void)
{
	MineApp_NodeAttr.peernmbr = INVALIDNMMBR;
	memset(MineApp_NodeAttr.peer_termnmbr.Nmbr, 0, NMBRDIGIT);
	MineApp_NodeAttr.IsTalkWithGateWay = FALSE;
	
	AudioSetInputGain(INGAIN_PHONE2PHONE);
	MineApp_ResetFrameblk();
	HalRingClose();
	HalAudioClose();
	uint8 maxFrameRetries = 3;
	MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);
	HalResetBackLightEvent();
}

void MineApp_StartTalk(void)
{
	uint8 maxFrameRetries = 3;
	MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);
	HalResetBackLightEvent();
	HALRING_SHAKEOFF();
	osal_stop_timerEx(Hal_TaskID, MINEAPP_RING_EVENT);
	osal_unset_event(Hal_TaskID, MINEAPP_RING_EVENT);
#ifndef NEW_MENU_LIB	
	DelayMs(65);
#endif
	HalAudioOpen();	
}

void MineApp_StartSearchNWK(void)
{
    if(!MineApp_SearchingNWK)
    {
        ZDApp_StartUpFromApp(APP_STARTUP_ENDEVICE);
        MineApp_SearchingNWK = TRUE;
    }
}
void MineApp_StartSearchNWK2(uint16 panid)
{
	if(!MineApp_SearchingNWK)
	{
		//NLME_JoinRequest(extPanid, panid,channel , CAPINFO_ALLOC_ADDR|CAPINFO_ALTPANCOORD|CAPINFO_DEVICETYPE_RFD|CAPINFO_POWER_AC|CAPINFO_RCVR_ON_IDLE);
		zgConfigPANID= panid;
		ZDApp_StartUpFromApp(APP_STARTUP_ENDEVICE);
		//ZDOInitDevice(0);
		MineApp_SearchingNWK = TRUE;
	}
}

void MineApp_SearchNWKStopped(void)
{
    MineApp_SearchingNWK = FALSE;
}

void  MineApp_set_event(byte taskID, UINT16 event_id)
{
	if(ZSuccess != osal_set_event( taskID,  event_id))
	{
		SystemReset();
	}
}
void MineApp_start_timerEx( byte taskID, UINT16 event_id, UINT16 timeout_value )
{
	if(ZSuccess!=osal_start_timerEx(  taskID,  event_id,  timeout_value ))
	{
		SystemReset();
	}
}

void MineApp_SendSignal(uint8 signalLen)
{
	AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc, 
		MINEAPP_CLUSTERID, signalLen, (uint8 *)&AppData, 
		&MineApp_TransID, INIT_OPN | AF_ACK_REQUEST, 0x01);
}

/* remove from stringutil to reduce codesize */
uint16 MineaApp_atoul(uint8 *str)
{
	uint16 ret = 0;
	uint8* tmpstr = str;
	
	ret = *tmpstr - '0'; 
	
	++tmpstr;
	while (*tmpstr <= '9' && *tmpstr >= '0') 
	{
		ret = ret * 10 + (*tmpstr - '0');
		++tmpstr;
	}

	return (ret);
}

#ifdef MENU_RF_DEBUG
void MineApp_LeaveNWK(uint8* macAddr)
{
	APPWrapper_t AppData;
	AppData.app_LeaveNwk.msgtype = ZB_LEAVE_NOTIFY;
	AppData.app_LeaveNwk.srcnbr = MineApp_NodeAttr.Node_nmbr;
	sAddrExtCpy(AppData.app_LeaveNwk.macAddr, macAddr);
	afAddrType_t dstaddr;
	dstaddr.endPoint = MINEAPP_ENDPOINT;
	dstaddr.addrMode = afAddrBroadcast;
	dstaddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVZCZR;
	AF_DataRequest(&dstaddr, (endPointDesc_t *)&MineApp_epDesc,
		MINEAPP_CLUSTERID, sizeof(app_LeaveNwk_t), (byte *)&AppData,
		&MineApp_TransID, INIT_OPN, 1);	
}
void MineApp_Restart(void)
{
	//Leave current zigbee network, 
	MineApp_LeaveNWK(NLME_GetExtAddr());
	osal_start_timerEx(MineApp_TaskID, MINEAPP_RESTART_EVENT, 200);
}
#endif

