/**************************************************************************************************
Filename:       MineController.c
Revised:        $Date: 2009/10/29 22:11:43 $
Revision:       $Revision: 1.4 $

Description:    Controller of mine device.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "MineController_global.h"
#include "MineController.h"
#include "MPDep.h"
#include "mac_pib.h"
#include "AppProtocolWrapper.h"
#include "Mac_radio_defs.h"

#include "App_cfg.h"
#include "MacUtil.h"
#include "Delay.h"
#include "WatchDogUtil.h"
#include "TimeUtil.h"
#include "zGlobals.h"
#if !defined( NONWK )
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#endif
#ifdef TIME_TEST
#include "time.h"
#endif
#include "OSAL.h"
#include "OnBoard.h"

#include "hal_mcu.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "hal_drivers.h"
#include  "SleepUtil.h"

#include "lcd_interface.h"
#include "MenuLibController_global.h"
//#include "hal_audio.h"

/*************************************************************************************************
*CONSTANTS
*/
/*************************************************************************************************
*MACROS
*/
#define MINECONTROLLER_P1INT_ENABLE() 	st(PICTL &= ~0x02; IEN2 |= 0X10;)

/*********************************************************************
* TYPEDEFS
*/
/*********************************************************************
* LOCAL VARIABLES
*/
uint8 MineController_TaskID;
uint8 MineController_TransID;
devStates_t MineController_NwkState;
NodeAddr     MineController_nodeAddr;

APPWrapper_t AppData;
afAddrType_t dstAddr;

#ifdef TIME_TEST
//static uint16 time_test;
#endif
static uint8 settime_count = 10;
static app_Sleep_t MineController_AppSleep;
				  
const cId_t __code MineController_InputClusterList[] = 
{
	MINEAPP_CLUSTERID,
	CARD_CLUSTERID,
};
const cId_t __code MineController_OutputClusterList[] = 
{
	MINEAPP_CLUSTERID,
	CARD_CLUSTERID,
};

const SimpleDescriptionFormat_t __code MineController_SimpleDesc =
{
	MINEAPP_ENDPOINT,
	MINEAPP_PROFID,

	MINEAPP_DEVICEID,

	MINEAPP_VERSION,
	MINEAPP_FLAGS,

	sizeof(MineController_InputClusterList),
	(cId_t*)MineController_InputClusterList,

	sizeof(MineController_OutputClusterList),
	(cId_t*)MineController_OutputClusterList
};

const endPointDesc_t __code MineController_epDesc =
{
	MINEAPP_ENDPOINT,
	&MineController_TaskID,
	(SimpleDescriptionFormat_t *)&MineController_SimpleDesc,
	noLatencyReqs
};

/*********************************************************************
* GLOBAL VARIABLES
*/

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void MineController_HandleKeys( uint16 keys, byte shifts);
static void MineController_ProcessMSGCB(afIncomingMSGPacket_t *MSGpkt);
static byte MineController_ParseAppFrame(const afIncomingMSGPacket_t* MSGpkt);
static uint8 MineController_CvtChannel2Logic(uint32 channellist);

//static uint8 MineController_LeaveNWK(uint8 notify);

/*********************************************************************
* FUNCTIONS
*********************************************************************/

/*********************************************************************
* @fn      MineController_Init
*
* @brief   Initialization function for the MineController OSAL task.
*
* @param   task_id - the ID assigned by OSAL.
*
* @return  none
*/
void MineController_Init( uint8 task_id)
{
#if 1
	MineController_TaskID = task_id;
	MineController_NwkState = DEV_INIT;


	afRegister((endPointDesc_t *)&MineController_epDesc);
	RegisterForKeys(MineController_TaskID);

	HalResetBackLightEvent();
	if(!MP_vdd_check())
	{
		Menu_handle_msg(MSG_NO_POWER, NULL, 0);
		MP_LongDelay(500, 4);
		MP_PowerOFF();
	}

#ifdef MP_VERSION_1_1
	BACKLIGHT_OPEN();
	InitialLcd();
#endif

	Menu_Init();

#ifdef AUDIO_SERIAL
       MINECONTROLLER_P1INT_ENABLE() ;
#endif
	MineController_JoinNWK(MineController_CvtChannel2Logic(zgDefaultChannelList));
	Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
	Menu_UpdateBattery(HalAdcMeasureVdd());
	//osal_start_timerEx(MineController_TaskID, MINECONTROLLER_PERIOD_SLEEP_EVENT, 100);

	//osal_set_event(MineController_TaskID, MINECONTROLLER_UPDATE_EVENT);
#else

#endif

}

/*********************************************************************
* @fn      MineController_ProcessEvent
*
* @brief   Mine controller Task event processor.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events - Bit map of events to process.
*
* @return  none
*/
uint16 MineController_ProcessEvent( uint8 task_id, uint16 events )
{
#if 0
#else
	afIncomingMSGPacket_t* MSGpkt;
	if (events & SYS_EVENT_MSG)
	{
		MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(MineController_TaskID);
		while ( MSGpkt )
		{
			switch ( MSGpkt->hdr.event )
			{
			case ZDO_STATE_CHANGE:
				{
					MineController_NwkState = (devStates_t)(MSGpkt->hdr.status);

					if(MineController_NwkState == DEV_ROUTER)
					{
					}	
					break;
				}

			case KEY_CHANGE:
                    {
				MineController_HandleKeys(((keyChange_t *)MSGpkt)->keys, ((keyChange_t *)MSGpkt)->state);
				break;
                    }
                	case AF_INCOMING_MSG_CMD:
				{
#ifdef TIME_TEST
					INIT_HWTIMER1();
					RESET_T1CNT();
#endif
					MineController_ProcessMSGCB(MSGpkt);
					break;
				}
			}
			osal_msg_deallocate( (uint8 *)MSGpkt );
			MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( MineController_TaskID);
		}
	}
#endif

	#if 1
	if(events & MINECONTROLLER_CARDSETFINISH_EVENT)
	{
             if(settime_count == 0)
             	{
             	       SET_ON_IDLE();
		       //Menu_handle_msg(MSG_SET_SUCCESSFUL, NULL, 0);
             	}
	       else
	       {
	              --settime_count;
		   	osal_start_timerEx(MineController_TaskID, MINECONTROLLER_CARDSETFINISH_EVENT, 60000);
		}
		return events ^ MINECONTROLLER_CARDSETFINISH_EVENT;
	}
	#endif
    	if (events & MINECONTROLLER_POWEROFF_EVENT)
	{	
		MP_PowerOFF();
		return events ^ MINECONTROLLER_POWEROFF_EVENT;
	}
	if (events & MINECONTROLLER_UPDATE_EVENT)
	{
		uint8 battery = HalAdcMeasureVdd();

		Menu_UpdateBattery(battery);
		//Menu_UpdateTime();
		if(battery == 0)
		{ 
			if(!MP_vdd_check())
			{
				Menu_handle_msg(MSG_NO_POWER, NULL, 0);
				MP_LongDelay(500, 4);
				MP_PowerOFF();
			}
		}

		osal_start_timerEx(MineController_TaskID, MINECONTROLLER_UPDATE_EVENT, 10000);
		return events ^ MINECONTROLLER_UPDATE_EVENT;
	}
	if(events & MINECONTROLLER_PERIOD_SLEEP_EVENT)
	{

		bool mFalse = FALSE;
		bool rx_on_idle;

		if(Hal_AllowSleep())  // Idle and hardware not active
		{
			// StopAudio();

			/*LCD consume < 1mA, no need to sleep */
			//LCDIntoSleep();	

			//-----------------------
			MAC_MlmeGetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);
			MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &mFalse);
			while(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP)!=MAC_SUCCESS);

			UtilSleep(CC2430_PM1, MP_WORK_INTERVAL*1000);    

			//halSleep( MP_WORK_INTERVAL*1000);

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
			FeedWatchDog();
#endif
			//LCDWakeUp();
			// DelayMs(250);
			MAC_PwrOnReq();
			MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);

			osal_start_timerEx(MineController_TaskID, MINECONTROLLER_PERIOD_SLEEP_EVENT, MP_WORK_TIMEOUT);
		}
		else // Idle and key is active
		{
			osal_start_timerEx(MineController_TaskID, MINECONTROLLER_PERIOD_SLEEP_EVENT, MP_WORK_INTERVAL*1000);
		}

		// poll for data
		AppData.app_MP_Poll_Req.msgtype= MP_POLL_REQ;
		AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineController_epDesc, 
			MINEAPP_CLUSTERID, sizeof(app_MP_Poll_Req_t), (uint8 *)&AppData, 
			&MineController_TransID, INIT_OPN|AF_ACK_REQUEST, MAX_DEPTH);

		return events ^ MINECONTROLLER_PERIOD_SLEEP_EVENT;
	}  
	if (events & MINECONTROLLER_MENULIB_EVENT)
	{
	
		Menu_ProcessMenuLibEvt();
		return events ^ MINECONTROLLER_MENULIB_EVENT;
	}
	return 0;
}
/*********************************************************************
* @fn      MineController_ParseAppFrame
*
* @brief   This function processes OTA message to app data if needed.
*
* @param   MSGpkt - The incoming msg.
*
* @return  none
*/
byte MineController_ParseAppFrame(const afIncomingMSGPacket_t* MSGpkt)
{
	APPWrapper_t* AppPkt = (APPWrapper_t* )(MSGpkt->cmd.Data);
	byte appflag = AppPkt->app_flag;
	switch (appflag)
	{
	case SSREQ:
		{
			uint8 reqtype = AppPkt->app_ssReq.reqtype;

			/* If poll, Send data to card */
			if (reqtype & SSREQ_POLL)
			{
				if(ON_CARDSETTING())
				{
	                           MacParam_t  MineController_MacParam;		   
	                           MineController_MacParam.cluster_id = CARD_CLUSTERID;
	                           MineController_MacParam.panID = CARD_NWK_ADDR;//CONTROLLER_NWK_ADDR;//the card panid
	                           MineController_MacParam.radius = 0x0A;

				       MAC_UTIL_BuildandSendDataPAN(&MineController_MacParam, (uint8*)&MineController_AppSleep, sizeof(app_Sleep_t), MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr, 
						   	                                         MAC_TXOPTION_NO_RETRANS);
				}
			}
			break;
		}
	default:
		break;
	}
	return ZSUCCESS;
}

void MineController_HandleKeys( uint16 keys, byte shifts)
{
	if(keys == HAL_KEY_CANCEL)
	{
		if(MP_TestLongPress(HAL_KEY_POWER, MINEAPP_POWER_LONGPRESS_TIMEOUT))
		{
                   osal_start_timerEx(MineController_TaskID, MINECONTROLLER_POWEROFF_EVENT, 5);
		}
		else 
		{
			Menu_handle_key(HAL_KEY_CANCEL, KEY_SHORT_PRESS); 

			KeyIntoSleep();
			HalResetBackLightEvent();
		}
		return;
	}
	Menu_handle_key(keys, KEY_SHORT_PRESS); 

	switch (keys)
	{
		case HAL_KEY_SELECT:
		{
			if(ON_CARDDETECTING())
			{

			}
			else if(ON_CARDSETTING())
			{    
			       settime_count = 10;
				Get_Sleeptime_data(&MineController_AppSleep);
				osal_start_timerEx(MineController_TaskID, MINECONTROLLER_CARDSETFINISH_EVENT, 60000);
				Menu_handle_msg(MSG_SET_SUCCESSFUL, NULL, 0);
			}
			else if(ON_SUBSTATIONDETECTING())
			{}
			else if(ON_SUBSTATIONSETTING())
			{}
			break;	
		}
		default:
			break;
	}

	KeyIntoSleep();
	HalResetBackLightEvent();
}

void MineController_ProcessMSGCB(afIncomingMSGPacket_t *MSGpkt)
{
	if(MSGpkt->clusterId == CARD_CLUSTERID)
		MineController_ParseAppFrame(MSGpkt);
}

void MineController_StartMenuLibEvt (uint16 timeout)
{
	osal_start_timerEx(MineController_TaskID, MINECONTROLLER_MENULIB_EVENT, timeout);
}

void MineController_StopMenuLibEvt (void)
{
	osal_stop_timerEx(MineController_TaskID, MINECONTROLLER_MENULIB_EVENT);
}
void  MineController_JoinNWK(uint8 LogicChannel)
{
      MP_SetPowerOn(TRUE);
      
	// todo: add join Network code here, join a virtual  PanID network
	NLME_ResetRequest();

	byte CapabilityInfo = CAPINFO_DEVICETYPE_RFD
#if ( RFD_RCVC_ALWAYS_ON == TRUE)
		| CAPINFO_RCVR_ON_IDLE
#endif
		;

	bool rxonidle = false;
	uint8 maxFrameRetries = 0x01;//0x03;
      uint16 panID = CONTROLLER_NWK_ADDR;
      uint16 shortAddr = 0x0001;
      uint16 coord_shortAddr = 0x0000;//NWK_BROADCAST_SHORTADDR_DEVALL;

	_NIB.nwkDevAddress = shortAddr;
	_NIB.nwkLogicalChannel = LogicChannel;
	_NIB.nwkCoordAddress = coord_shortAddr;
	//osal_cpyExtAddr(_NIB.nwkCoordExtAddress, temp);

      _NIB.nwkPanId = panID;
	_NIB.nwkState = NWK_ROUTER;//NWK_ENDDEVICE;//
	_NIB.beaconOrder = DEF_BEACON_ORDER;
	_NIB.superFrameOrder = DEF_BEACON_ORDER;
	//osal_cpyExtAddr(_NIB.extendedPANID, aExtendedAddress);


	MAC_MlmeResetReq(TRUE);
	MAC_InitDevice();

	MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, (void*)&maxFrameRetries);
	MAC_MlmeSetReq(MAC_COORD_SHORT_ADDRESS, (void *)&coord_shortAddr);
	MAC_MlmeSetReq(MAC_PAN_ID, (void *)&panID);
	MAC_MlmeSetReq(MAC_SHORT_ADDRESS, (void *)&shortAddr);
	MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, (void *)&LogicChannel);
	//MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, (void *)&temp);//aExtendedAddress);

	//MAC_MlmeSetReq(MAC_ASSOCIATED_PAN_COORD,&MACTrue);

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
uint8 MineController_CvtChannel2Logic(uint32 channellist)
{
    uint8 logicchannel = 0;
    while((channellist & 1) == 0)
    {
        channellist>>=1;
        logicchannel++;
    }
    return logicchannel;
}
