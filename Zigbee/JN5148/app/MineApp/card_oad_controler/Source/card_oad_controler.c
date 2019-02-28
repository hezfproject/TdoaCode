
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>

#include <LedControl.h>
#include <Button.h>

#include "config.h"
#include "app_protocol.h"
#include "JN5148_util.h"
#include "printf_util.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define LED_ON_EVENT		1
#define LED_OFF_EVENT		2
#define SEND_UPDATE_EVENT	4

#define LED_PERIOD_MS		500
#define LED_ON_MS			10
#define LED_PIN				E_AHI_DIO8_INT

#define __OAD_I__

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

PRIVATE void vCardOadControllerCast(uint16 u16CardShortAddr, uint16 u16CardPanId);
PRIVATE void vSetLed(bool_t bOnOff, uint16 u16OnMs);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE void *s_pvMac;
PRIVATE MAC_Pib_s *s_psMacPib;
PRIVATE MAC_ExtAddr_s psMacAddr;

PRIVATE uint8 u8Channel;			// card's current channel
PRIVATE uint16 u16CardOadControllerPan;

PRIVATE uint8 u8OadHandle = 0x0F;
PRIVATE uint8 u8OadRetries = 0;
PRIVATE uint8 u8OadIndex = 0;
PRIVATE uint8 u8OadCards[] = {0x2D, 0x17, 0x22};
PRIVATE uint8 u8Base = 20;
PRIVATE uint8 u8Num;

/****************************************************************************
 *
 * NAME: AppColdStart
 *
 ****************************************************************************/

PUBLIC void AppColdStart(void)
{
#ifdef __OAD_I__
    u8Num = sizeof(u8OadCards)/sizeof(uint8);
#else
    u8Num = 20;
#endif
    PrintfUtil_u8Init(0, 2);
	PrintfUtil_vPrintf("cold start\n");
	vAHI_WatchdogStop();

    TimerUtil_vInit();
    vInitCoordSystem();

    vAHI_DioSetDirection(0, LED_PIN|E_AHI_DIO9_INT);

    TimerUtil_eSetCircleTimer(LED_ON_EVENT, LED_PERIOD_MS);

    //TimerUtil_eCircleTimer(LED_ON_EVENT, LED_PERIOD_MS);	// check button every 10 ms

    vAHI_HighPowerModuleEnable(TRUE, TRUE);

    vCardOadControllerCast(0x1234, 0x5678);
    PrintfUtil_vPrintf("..\n");
    while (1)
    {
        TimerUtil_vUpdate();
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
    //u8Channel = ((psMacAddr.u32H) >> 24) & 0xFF;
    //u16CardOadControllerPan = psMacAddr.u32L & 0xFFFF;
    
    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);
    u16CardOadControllerPan = tmp32%100000;
    u8Channel = ((psMacAddr.u32L) >> 16) & 0xFF;
	
    // ***************************************************/
    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId		= u16CardOadControllerPan;
    sMacUtil.u8Dst_endpoint 	= 0x21;
    sMacUtil.u8Src_endpoint 	= 0x20;
    sMacUtil.u16Profile_id 		= 0x2001;
    sMacUtil.u8NodeType 		= 0x02;
    sMacUtil.u16SrcShortAddr 	= 0x0000;

    MacUtil_vInit(&sMacUtil);

    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);

    MAC_vPibSetPanId(s_pvMac, u16CardOadControllerPan);
    MAC_vPibSetShortAddr(s_pvMac, 0x0000);

    s_psMacPib->u8MaxFrameRetries 	= 2;
    s_psMacPib->sCoordExtAddr.u32H	= 0xAAAA;	// fake
    s_psMacPib->sCoordExtAddr.u32L 	= 0xBBBB;	// fake
    s_psMacPib->u16CoordShortAddr 	= 0x0000;	// fake
    s_psMacPib->u8SecurityMode 		= 0;

    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, u8Channel);
	
    PrintfUtil_vPrintf("System Start, PanID: %d Channel:%d\n", u16CardOadControllerPan, u8Channel);
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
        case SEND_UPDATE_EVENT:
        {
            if(u8OadIndex < u8Num)
            {
#ifdef __OAD_I__
                vCardOadControllerCast(0x0A00|u8OadCards[u8OadIndex], TOF_CARD_NWK_ADDR);
#else
                vCardOadControllerCast(0x0A00|u8Base, TOF_CARD_NWK_ADDR);
#endif
                if(u8OadRetries++ > 250)
                {
                    u8OadRetries = 0;
                    u8Base++;
                    ++u8OadIndex;
                }

                TimerUtil_eSetTimer(SEND_UPDATE_EVENT, 20);
            }
            else
                vAHI_DioSetOutput(E_AHI_DIO9_INT, 0);

            EventUtil_vUnsetEvent(event);
            break;
        }

        case LED_ON_EVENT:
        {
            vSetLed(TRUE, LED_ON_MS);
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
    }
    while (psMcpsInd != NULL);

    do
    {
        psMlmeInd = psAppQApiReadMlmeInd();
        if (psMlmeInd != NULL)
        {
            vProcessIncomingMlme(psMlmeInd);
            vAppQApiReturnMlmeIndBuffer(psMlmeInd);
        }
    }
    while (psMlmeInd != NULL);

    do
    {
        psAHI_Ind = psAppQApiReadHwInd();
        if (psAHI_Ind != NULL)
        {
            vProcessIncomingHwEvent(psAHI_Ind);
            vAppQApiReturnHwIndBuffer(psAHI_Ind);
        }
    }
    while (psAHI_Ind != NULL);
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
    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);

    // not for me or len is error
    if((APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype)
            || (psFrame->u8SduLength != sizeof(app_header_t) + psAppPkt->tof_head.len))
        return;

    if (TOF_CARD_CHECKIN == psAppPkt->tof_head.msgtype) //&& psAppPkt->tof_head.len == sizeof(rf_tof_card_data_ts) - sizeof(app_header_t) )
        vCardOadControllerCast(psFrame->sSrcAddr.uAddr.u16Short, psFrame->sSrcAddr.u16PanId);
}

/****************************************************************************
 *
 * NAME: vCardOadControllerCast
 *
 ****************************************************************************/
PRIVATE void vCardOadControllerCast(uint16 u16CardShortAddr, uint16 u16CardPanId)
{
    MacUtil_SendParams_s sParams;
    sParams.u8Radius		= 1;
    sParams.u16DstAddr	= u16CardShortAddr;
    sParams.u16DstPanId 	= u16CardPanId;
    sParams.u16ClusterId 	= 0;
    sParams.u8DeliverMode	= MAC_UTIL_UNICAST;

    rf_tof_oad_data_ts oad_msg;
    oad_msg.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    oad_msg.tof_head.len = 4;

    oad_msg.tof_head.msgtype = TOF_CARD_OAD;
    oad_msg.u16Version = OAD_CARD_VERSION;
    oad_msg.u8DeviceType = DEVICE_TYPE_CARD;

    u8OadHandle = MacUtil_vSendData(&sParams, (uint8*)&oad_msg, sizeof(rf_tof_oad_data_ts), MAC_TX_OPTION_ACK);
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
    if((psMcpsInd->uParam.sDcfmData.u8Status == MAC_ENUM_SUCCESS) && (u8OadHandle == psMcpsInd->uParam.sDcfmData.u8Handle))
    {
        PrintfUtil_vPrintf("status: %d\n", psMcpsInd->uParam.sDcfmData.u8Status);
        u8OadRetries = 0;
        u8Base++;
        ++u8OadIndex;
    }
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







