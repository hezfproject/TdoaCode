
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>

#include "config.h"
#include "app_protocol.h"
#include "bsmac.h"
#include "JN5148_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define CHECKIN_WATCHDOG_EVENT	BIT(3)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_STARTED,
}teState;


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitCheckinStationSystem(void);
PRIVATE void vStartCoordinator(void);
PRIVATE void vProcessSysEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessAppEventQueues(void);

PRIVATE void vCheckinStationCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val);

PRIVATE bool_t bArmReady();
PRIVATE void vConvertEndian16(uint16* pu16Data);
PRIVATE void vWriteData2Arm(uint8* pbuf);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE void *s_pvMac;
PRIVATE MAC_Pib_s *s_psMacPib;
PRIVATE MAC_ExtAddr_s psMacAddr;
PRIVATE uint16 u16CheckinShortAddr;
PRIVATE uint16 u16CheckinPanId;
PRIVATE uint8 u8CheckinChannel;

PRIVATE teState teStaionState = E_STATE_IDLE;
PRIVATE tof_station_status_te tCheckinStationStatus = 1;//STATION_STATUS_NORMAL;

PRIVATE uint32 u32Data2Arm[30];	// the data send to arm, max len is 114 (BSMAC_MAX_TX_PAYLOAD_LEN), 30*4 = 120 is enough (28 is enough)
PRIVATE struct nwkhdr* pnwk_data = (struct nwkhdr*)u32Data2Arm;
PRIVATE uint16 u16ArmId;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: AppColdStart
 *
 ****************************************************************************/
PUBLIC void AppColdStart(void)
{
	// watch dog is enable by default
	vInitCheckinStationSystem();
	vStartCoordinator();
	
PrintfUtil_vPrintf("System started. \n\r");

	TimerUtil_eCircleTimer(CHECKIN_WATCHDOG_EVENT, 1000); // feed watch dog every 1 second

	while (1)
	{
		vProcessSysEventQueues();
		vProcessAppEventQueues();
	}
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
	PrintfUtil_u8Init(0, 2);
	PrintfUtil_vPrintf("warm start. \n\r");
    	AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitCheckinStationSystem
 *
 * DESCRIPTION:
 * Init system when cold start, includes mac settings, tof / timer / high power mode functions
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitCheckinStationSystem(void)
{
    (void)u32AppQApiInit(NULL, NULL, NULL); //vHwDeviceIntCallback);
    (void)u32AHI_Init();

PrintfUtil_u8Init(0, 2);

	vAHI_HighPowerModuleEnable(TRUE, TRUE);
	TimerUtil_vEnableTimer();

	MacUtil_vReadExtAddress(&psMacAddr);
	u16CheckinShortAddr = 0x0000;	//psMacAddr.u32L & 0xFFFF;
	u16CheckinPanId = psMacAddr.u32L & 0xFFFF;
	u8CheckinChannel = ((psMacAddr.u32H) >> 24) & 0xFF;

// for arm
	pnwk_data->type = NWK_DATA;
	pnwk_data->ttl = 1;
	pnwk_data->src = u16CheckinPanId;
	vConvertEndian16(&(pnwk_data->src));
//

	s_pvMac = pvAppApiGetMacHandle();
	s_psMacPib = MAC_psPibGetHandle(s_pvMac);

	MAC_vPibSetPanId(s_pvMac, u16CheckinPanId);
	MAC_vPibSetShortAddr(s_pvMac, u16CheckinShortAddr);

	MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);
	MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 2);
	MAC_vPibSetMinBe(s_pvMac, 1);

	s_psMacPib->bAssociationPermit = 0;
	s_psMacPib->bAutoRequest=0;
	s_psMacPib->bGtsPermit = FALSE;
	//FIXME
	s_psMacPib->u8MaxFrameRetries = 1;

	MacUtil_Setting_s sMacUtil;
	sMacUtil.u16SrcPanId 		= u16CheckinPanId;
	sMacUtil.u16SrcShortAddr 	= u16CheckinShortAddr;

	// the following init are fake, because station is 802.15.4
	sMacUtil.u16Profile_id 		= 0x2001; //for backward compatable
	sMacUtil.u8Dst_endpoint 	= 0x21;
	sMacUtil.u8NodeType 		= 0;
	sMacUtil.u8Src_endpoint 	= 0;

	MacUtil_vInit(&sMacUtil);
}

/****************************************************************************
 *
 * NAME: vProcessSysEventQueues
 *
 * DESCRIPTION:
 * Process system's event queues.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessSysEventQueues(void)
{
    MAC_MlmeDcfmInd_s *psMlmeInd;
    MAC_McpsDcfmInd_s *psMcpsInd;
    AppQApiHwInd_s    *psAHI_Ind;

    do
    {
        psMcpsInd = psAppQApiReadMcpsInd();
        if (psMcpsInd != NULL)
        {
            vProcessIncomingMcps(psMcpsInd);
            vAppQApiReturnMcpsIndBuffer(psMcpsInd);
        }
    } while (psMcpsInd != NULL);

    do
    {
        psMlmeInd = psAppQApiReadMlmeInd();
        if (psMlmeInd != NULL)
        {
            vProcessIncomingMlme(psMlmeInd);
            vAppQApiReturnMlmeIndBuffer(psMlmeInd);
        }
    } while (psMlmeInd != NULL);

    do
    {
        psAHI_Ind = psAppQApiReadHwInd();
        if (psAHI_Ind != NULL)
        {
            vProcessIncomingHwEvent(psAHI_Ind);
            vAppQApiReturnHwIndBuffer(psAHI_Ind);
        }
    } while (psAHI_Ind != NULL);
}


/****************************************************************************
 *
 * NAME: vProcessAppEventQueues
 *
 * DESCRIPTION:
 * Process application's event queues.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessAppEventQueues(void)
{
	uint32 u32EventID = EventUtil_u32ReadEvents();
	switch (u32EventID)
	{
		case CHECKIN_WATCHDOG_EVENT:
		{
			vAHI_WatchdogRestart();
			EventUtil_vUnsetEvent(u32EventID);
			break;						
		}

		default:
		{
			EventUtil_vUnsetEvent(u32EventID);
			break;						
		}		
	}
}

/****************************************************************************
 *
 * NAME: vProcessIncomingMlme
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd)
{
}

/****************************************************************************
 *
 * NAME: vProcessIncomingData
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
{
    if (teStaionState >= E_STATE_STARTED)
    {
        switch(psMcpsInd->u8Type)
        {
        case MAC_MCPS_IND_DATA:  /* Incoming data frame */
            vHandleMcpsDataInd(psMcpsInd);
            break;
        case MAC_MCPS_DCFM_DATA: /* Incoming acknowledgement or ack timeout */
            vHandleMcpsDataDcfm(psMcpsInd);
            break;
        default:
            break;
        }
    }
}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataDcfm
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{
}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
	MAC_RxFrameData_s *psFrame;
	psFrame = &psMcpsInd->uParam.sIndData.sFrame;

	RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu); 
	if (APP_PROTOCOL_TYPE_CARD == psAppPkt->tof_head.protocoltype)
	{
		if(psAppPkt->tof_head.msgtype == TOF_CARD_CHECKIN)
		{
			vCheckinStationCast(TOF_STATION_CHECKIN_ACK, psFrame->sSrcAddr.uAddr.u16Short, TOF_CARD_NWK_ADDR, 0, 0); 

PrintfUtil_vPrintf("%x checkin\n", psFrame->sSrcAddr.uAddr.u16Short);			
			if(bArmReady())
			{
				app_tof_checkin_ts app_tof_checkin_data;

				app_tof_checkin_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
				app_tof_checkin_data.app_tof_head.msgtype = APP_TOF_MSG_CHECKIN;
				app_tof_checkin_data.app_tof_head.len = 4;

				app_tof_checkin_data.u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
				app_tof_checkin_data.u8Status = psAppPkt->rf_tof_card_data.u8CardStatus;
				
				vWriteData2Arm((uint8*)&app_tof_checkin_data);
			}
		}
	}
}


/****************************************************************************
 *
 * NAME: vProcessIncomingHwEvent
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind)
{
}


/****************************************************************************
 *
 * NAME: vStartCoordinator
 *
 ****************************************************************************/
PRIVATE void vStartCoordinator(void)
{
    /* Structures used to hold data for MLME request and response */
    MAC_MlmeReqRsp_s   sMlmeReqRsp;
    MAC_MlmeSyncCfm_s  sMlmeSyncCfm;

    teStaionState = E_STATE_STARTED;

    sMlmeReqRsp.u8Type = MAC_MLME_REQ_START;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    sMlmeReqRsp.uParam.sReqStart.u16PanId = u16CheckinPanId;
    sMlmeReqRsp.uParam.sReqStart.u8Channel = u8CheckinChannel;
    sMlmeReqRsp.uParam.sReqStart.u8BeaconOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8SuperframeOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = TRUE;
    sMlmeReqRsp.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8Realignment = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8SecurityEnable = FALSE;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}



/****************************************************************************
 *
 * NAME: vRssiStationCast
 *
 * DESCRIPTION:
 * Station send different msg to air.
 *
 * PARAMETERS:      
 *				u8CmdType - Tof protocol
 *				u16DstAddr, u16DstPanId - destination
 *				u8TxOptions - need retry or not
 *				u8Val - optional value
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
 PRIVATE void vCheckinStationCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val)
{
	MacUtil_SendParams_s sParams;
	sParams.u8Radius		= 1;
	sParams.u16DstAddr	= u16DstAddr;
	sParams.u16DstPanId 	= u16DstPanId;
	sParams.u16ClusterId 	= 0;
	sParams.u8DeliverMode	= MAC_UTIL_UNICAST;

	RfTofWrapper_tu RfTofData;
	RfTofData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
	RfTofData.tof_head.msgtype = u8CmdType;

	switch (u8CmdType)
	{
		case TOF_STATION_CHECKIN_ACK:
		{
			RfTofData.rf_tof_station_signal.u8AvailableMs = u8Val;	// card will send request in random (0 ~ u8Val) ms
			RfTofData.rf_tof_station_signal.u8StationStatus = tCheckinStationStatus;
			RfTofData.rf_tof_station_signal.u16CurSlot = 0;
			RfTofData.rf_tof_station_signal.u8RunMs = 0; 			
			RfTofData.rf_tof_station_signal.u8LocIdle = 0;
			RfTofData.tof_head.len = 6;	//this is not a 4-byte struct !!!!!!!!!!!
			break;
		}

		default:
			break;
	}
	
	MacUtil_vSendData(&sParams, (uint8*)&RfTofData, RfTofData.tof_head.len+sizeof(app_header_t), u8TxOptions);
}


/****************************************************************************
 *
 * NAME: bArmReady
 *
 * DESCRIPTION:
 * Check arm is ready or not every time when send data to arm.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * 		TRUE - got the arm id 
 *		FALSE - has not get the arm id
 *
 ****************************************************************************/
PRIVATE bool_t bArmReady()
{
	if(Jbsmac_vGetLinkStatus() == 1)	// link up
	{
		u16ArmId = Jbsmac_u16GetPeerAddr();
		return TRUE;
	}
	else
		return FALSE;
}


PRIVATE void vConvertEndian16(uint16* pu16Data)
{
	SysUtil_vConvertEndian(pu16Data, sizeof(uint16));
}


/****************************************************************************
 *
 * NAME: vWriteData2Arm
 *
 * DESCRIPTION:
 * Station write the msg to ARM, need convert endian for all 16-bit data first.
 *
 * PARAMETERS:      
 *				pbuf - the buffer which need write to ARM
 *
 * RETURNS:
 * 		void
 *
 ****************************************************************************/
PRIVATE void vWriteData2Arm(uint8* pbuf)
{
	app_header_t* pAppHead = (app_header_t*) pbuf;
	uint16 u16App_len = pAppHead->len;
	uint16 u16CardIndex;
	
	switch (pAppHead->msgtype)
	{
		case APP_TOF_MSG_CHECKIN:
		{
			app_tof_checkin_ts* pCheckin_data = (app_tof_checkin_ts*)pbuf;
			vConvertEndian16(&(pCheckin_data->u16ShortAddr));
			break;
		}

		default:
			break;
	}

	uint16 nwk_len = pAppHead->len + sizeof(app_header_t) + sizeof(struct nwkhdr);
	pnwk_data->len = pAppHead->len + sizeof(app_header_t);
	pnwk_data->dst = u16ArmId;
	
	vConvertEndian16(&(pnwk_data->dst));
	vConvertEndian16(&(pnwk_data->len));
	vConvertEndian16(&(pAppHead->len));
	memcpy(pnwk_data+1, pbuf, u16App_len + sizeof(app_header_t));

uint16 u16Tmp;
uint8 u8Data[120];
memcpy(u8Data, (uint8*)pnwk_data, nwk_len);
for(u16Tmp = 0; u16Tmp < nwk_len; u16Tmp++)
    PrintfUtil_vPrintf("%x  ", u8Data[u16Tmp]);
PrintfUtil_vPrintf("\nLen: %d\n", nwk_len);

	Jbsmac_eWriteData((uint8*)pnwk_data, nwk_len); 
}






