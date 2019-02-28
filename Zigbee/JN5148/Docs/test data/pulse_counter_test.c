
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <AppApiTof.h>
#include <LedControl.h>
#include <Button.h>
#include <OAD.h>

#include "config.h"
#include "app_protocol.h"
#include "JN5148_util.h"
#include "printf_util.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define LED_ON_EVENT		1
#define LED_OFF_EVENT		2
#define LED_PERIOD_MS		500
#define LED_ON_MS			10
#define LED_PIN				E_AHI_DIO8_INT

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitCoordSystem(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessSysEvent();
PRIVATE void vProcessAppEvent();

PRIVATE void vCoordCast(uint16 u16CardShortAddr, uint16 u16CardPanId);
PRIVATE void vSetLed(bool_t bOnOff, uint16 u16OnMs);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE void *s_pvMac;
PRIVATE MAC_Pib_s *s_psMacPib;
PRIVATE MAC_ExtAddr_s psMacAddr;

PRIVATE uint8 u8Channel;			// card's current channel
PRIVATE uint16 u16Count = 0;

/****************************************************************************
 *
 * NAME: AppColdStart
 *
 ****************************************************************************/

PUBLIC void AppColdStart(void)
{
	vAHI_WatchdogStop();
	vInitCoordSystem();

	PrintfUtil_u8Init(0, 2);
	PrintfUtil_vPrintf("cold start\n");

	vAHI_DioSetDirection(0, LED_PIN);		
	TimerUtil_vEnableTimer();
	TimerUtil_eCircleTimer(LED_ON_EVENT, 200);
	
	bAHI_PulseCounterConfigure(E_AHI_PC_0, 0, 0, FALSE, FALSE);
	bAHI_StartPulseCounter(E_AHI_PC_0);

	while (1)
	{
		vProcessSysEvent();
		vProcessAppEvent();
	}
}

/****************************************************************************
 *
 * NAME: vInitCardSystem
 *
 ****************************************************************************/
PRIVATE void vInitCoordSystem(void)
{
	u32AppQApiInit(NULL, NULL, NULL);
	u32AHI_Init();

	MacUtil_vReadExtAddress(&psMacAddr);
	u8Channel = ((psMacAddr.u32H) >> 16) & 0xFF;

	// ***************************************************/
	MacUtil_Setting_s sMacUtil;
	sMacUtil.u16SrcPanId		= COORD_PAN_ID;
	sMacUtil.u8Dst_endpoint 	= 0x21;
	sMacUtil.u8Src_endpoint 	= 0x20;   
	sMacUtil.u16Profile_id 		= 0x2001; 
	sMacUtil.u8NodeType 		= 0x02;
	sMacUtil.u16SrcShortAddr 	= COORD_SHORT_ADDR;

	MacUtil_vInit(&sMacUtil);
	
	s_pvMac = pvAppApiGetMacHandle();
	s_psMacPib = MAC_psPibGetHandle(s_pvMac);

	MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);

	MAC_vPibSetPanId(s_pvMac, COORD_PAN_ID);
	MAC_vPibSetShortAddr(s_pvMac, COORD_SHORT_ADDR);

	s_psMacPib->u8MaxFrameRetries 	= 2;
	s_psMacPib->sCoordExtAddr.u32H	= 0xAAAA;	// fake
    	s_psMacPib->sCoordExtAddr.u32L 	= 0xBBBB;	// fake
   	s_psMacPib->u16CoordShortAddr 	= 0x0000;	// fake
    	s_psMacPib->u8SecurityMode 		= 0;

	eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, u8Channel);

}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
	AppColdStart();
	PrintfUtil_vPrintf("warm start\n");
}

/****************************************************************************
 *
 * NAME: vProcessAppEvent
 *
 ****************************************************************************/
PRIVATE void vProcessAppEvent()
{
	uint32 event = EventUtil_u32ReadEvents();
	if(event)
	{
		switch (event)
		{
			case LED_ON_EVENT:
			{
				uint16 u16CountTmp;
				bAHI_Read16BitCounter(E_AHI_PC_0, &u16CountTmp);
PrintfUtil_vPrintf("Count: %d\n", u16CountTmp - u16Count);
				u16Count = u16CountTmp;
				EventUtil_vUnsetEvent(event);
				break;
			}

			case LED_OFF_EVENT:
			{
				vSetLed(FALSE, 0);
				EventUtil_vUnsetEvent(event);
				break;
			}

			default:
			{
				EventUtil_vUnsetEvent(event);
				break;
			}
				
		}
	}
}

/****************************************************************************
 *
 * NAME: vProcessSysEvent
 *
 ****************************************************************************/
PRIVATE void vProcessSysEvent()
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
 * NAME: vProcessIncomingMcps
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
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


/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
	MAC_RxFrameData_s *psFrame;
	psFrame = &psMcpsInd->uParam.sIndData.sFrame;
	oad_msg_ts* poad_msg = (oad_msg_ts*)(psFrame->au8Sdu);
	if((poad_msg->oad_type == OAD_REQUEST) && (VERSION > poad_msg->u8Version))
	{
		PrintfUtil_vPrintf("%x: %i\n", psFrame->sSrcAddr.uAddr.u16Short, SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality));
		vCoordCast(psFrame->sSrcAddr.uAddr.u16Short, psFrame->sSrcAddr.u16PanId);
	}
}

/****************************************************************************
 *
 * NAME: vCoordCast
 *
 ****************************************************************************/
PRIVATE void vCoordCast(uint16 u16CardShortAddr, uint16 u16CardPanId)
{
	MacUtil_SendParams_s sParams;
	sParams.u8Radius		= 1;
	sParams.u16DstAddr	= u16CardShortAddr;
	sParams.u16DstPanId 	= u16CardPanId;
	sParams.u16ClusterId 	= 0;
	sParams.u8DeliverMode	= MAC_UTIL_UNICAST;

	oad_msg_ts oad_msg;
	oad_msg.oad_type = OAD_ACK;
	oad_msg.u8Version = VERSION;

	MacUtil_vSendData(&sParams, (uint8*)&oad_msg, 2, MAC_TX_OPTION_ACK);
}


/****************************************************************************
 *
 * NAME: vProcessIncomingMlme
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd)
{
    switch (psMlmeInd->u8Type)
    {
    case MAC_MLME_IND_ASSOCIATE: /* Incoming association request */
        break;

    case MAC_MLME_DCFM_SCAN: /* Incoming scan results */
        break;

    default:
        break;
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
 * NAME: vHandleMcpsDataDcfm
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{
}

/****************************************************************************
 *
 * NAME: vSetLed
 *
 ****************************************************************************/
PRIVATE void vSetLed(bool_t bOnOff, uint16 u16OnMs)
{
	if(bOnOff)
		vAHI_DioSetOutput(0, LED_PIN);
	else
		vAHI_DioSetOutput(LED_PIN, 0);

	if(u16OnMs > 0)
		TimerUtil_eSetTimer(LED_OFF_EVENT, u16OnMs);
}

	
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/







