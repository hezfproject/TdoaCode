/****************************************************************************
 *
 * MODULE:             enddevice.c
 *
 * COMPONENT:          enddevice.c,v
 *
 * VERSION:
 *
 * REVISION:           1.3
 *
 * DATED:              2006/11/06 10:03:46
 *
 * STATUS:             Exp
 *
 * AUTHOR:
 *
 * DESCRIPTION:
 *
 * LAST MODIFIED BY:
 *                     $Modtime: $
 *
 ****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2009. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>


#include "config.h"
#include "JN5148_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define EVENT_SEND_PACKET 0x0001


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_ACTIVE_SCANNING,
    E_STATE_ASSOCIATING,
    E_STATE_ASSOCIATED
} teState;

typedef struct
{
    teState eState;
    uint8   u8Channel;
    uint8   u8TxPacketSeqNb;
    uint8   u8RxPacketSeqNb;
    uint16  u16Address;
} tsEndDeviceData;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitSystem(void);
PRIVATE void vProcessEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vStartActiveScan(void);
PRIVATE void vHandleActiveScanResponse(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vStartAssociate(void);
PRIVATE void vHandleAssociateResponse(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vTransmitDataPacket(uint8 *pu8Data, uint8 u8Len, uint16 u16DestAdr);
PRIVATE void vProcessReceivedDataPacket(uint8 *pu8Data, uint8 u8Len);

PRIVATE void vProcessAppEvent();


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE void *s_pvMac;
PRIVATE MAC_Pib_s *s_psMacPib;
PRIVATE tsEndDeviceData sEndDeviceData;
PRIVATE uint8 au8Data[DATALEN];

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: AppColdStart
 *
 * DESCRIPTION:
 * Entry point for application from boot loader. Initialises system and runs
 * main loop.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void AppColdStart(void)
{
    /* Disable watchdog if enabled by default */
#ifdef WATCHDOG_ENABLED
    vAHI_WatchdogStop();
#endif

    vInitSystem();

    // vStartActiveScan();
    TimerUtil_eSetCircleTimer(EVENT_SEND_PACKET, 50);

    while (1)
    {
        TimerUtil_vUpdate();
        vProcessEventQueues();
        vProcessAppEvent();
    }
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 * DESCRIPTION:
 * Entry point for application from boot loader. Simply jumps to AppColdStart
 * as, in this instance, application will never warm start.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
    AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitSystem
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitSystem(void)
{
    /* Setup interface to MAC */

    (void)u32AppQApiInit(NULL, NULL, NULL);
    (void)u32AHI_Init();
    /* Enable high power modules */
    vAHI_HighPowerModuleEnable(TRUE, TRUE);

    PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
    PrintfUtil_vPrintf("start\n");

    //vAHI_ProtocolPower(TRUE);
    //vAHI_BbcSetHigherDataRate(E_AHI_BBC_CTRL_DATA_RATE_250_KBPS);

    TimerUtil_vInit();

    /* Initialise end device state */
    sEndDeviceData.eState = E_STATE_IDLE;
    sEndDeviceData.u8TxPacketSeqNb = 0;
    sEndDeviceData.u8RxPacketSeqNb = 0;

    /* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    /* Set Pan ID in PIB (also sets match register in hardware) */
    MAC_vPibSetPanId(s_pvMac, PAN_ID);
    MAC_vPibSetShortAddr(s_pvMac, 0x1234);

    /* Enable receiver to be on when idle */
    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);

    //MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 4);
    //MAC_vPibSetMinBe(s_pvMac, 1);

    vAHI_DioSetDirection(0, E_AHI_DIO8_INT);//DIO 8 output    //RED LED ON
    vAHI_DioSetOutput(0, E_AHI_DIO8_INT);

    s_psMacPib->sCoordExtAddr.u32H	= 0xAAAA;	// fake
    s_psMacPib->sCoordExtAddr.u32L 	= 0xBBBB;	// fake
    s_psMacPib->u16CoordShortAddr 	= 0x0000;	// fake
    s_psMacPib->u8SecurityMode 		= 0;
    s_psMacPib->u8SuperframeOrder = 15;
    s_psMacPib->u8BeaconOrder = 15;

    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, 0x10);


    int i;
    for (i=2; i<DATALEN; i++)
        au8Data[i]=i;

    // EventUtil_vSetEvent(EVENT_SEND_PACKET);
}
/****************************************************************************
 *
 * NAME: vProcessEventQueues
 *
 * DESCRIPTION:
 * Check each of the three event queues and process and items found.
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vProcessEventQueues(void)
{
    MAC_MlmeDcfmInd_s *psMlmeInd;
    MAC_McpsDcfmInd_s *psMcpsInd;
    AppQApiHwInd_s    *psAHI_Ind;

    /* Check for anything on the MCPS upward queue */
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

    /* Check for anything on the MLME upward queue */
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

    /* Check for anything on the AHI upward queue */
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

PRIVATE void vProcessAppEvent()
{
    uint32 event = EventUtil_u32ReadEvents();

    switch(event)
    {
    case EVENT_SEND_PACKET:
    {
        static uint32 seq = 1;
        uint32* p32;
        p32 = (uint32*)au8Data;
        *p32 = 0x36452738;

        memcpy(au8Data+4, &seq, sizeof(seq));
        vTransmitDataPacket(au8Data, 64, 0xFFFF);
        seq++;

        PrintfUtil_vPrintf("Sending %d\n",seq);

        EventUtil_vUnsetEvent(event);
        break;
    }
    default:
        EventUtil_vUnsetEvent(event);
        break;
    }

}

/****************************************************************************
 *
 * NAME: vProcessIncomingHwEvent
 *
 * DESCRIPTION:
 * Process any hardware events.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psAHI_Ind
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind)
{
}

/****************************************************************************
 *
 * NAME: vProcessIncomingMlme
 *
 * DESCRIPTION:
 * Process any incoming managment events from the stack.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psMlmeInd
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd)
{
    /* We respond to several MLME indications and confirmations, depending
       on mode */
    switch (psMlmeInd->u8Type)
    {
        /* Deferred confirmation that the scan is complete */
    case MAC_MLME_DCFM_SCAN:
        if (sEndDeviceData.eState == E_STATE_ACTIVE_SCANNING)
        {
            vHandleActiveScanResponse(psMlmeInd);
        }
        break;

        /* Deferred confirmation that the association process is complete */
    case MAC_MLME_DCFM_ASSOCIATE:
        /* Only respond to this if associating */
        if (sEndDeviceData.eState == E_STATE_ASSOCIATING)
        {
            vHandleAssociateResponse(psMlmeInd);
        }
        break;

    default:
        break;
    }
}

/****************************************************************************
 *
 * NAME: vProcessIncomingData
 *
 * DESCRIPTION:
 * Process incoming data events from the stack.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psMcpsInd
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
{
    /* Only handle incoming data events one device has been started as a
       coordinator */
    if (sEndDeviceData.eState >= E_STATE_ASSOCIATED)
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
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{
    if (psMcpsInd->uParam.sDcfmData.u8Status == MAC_ENUM_SUCCESS)
    {
        /* Data frame transmission successful */
    }
    else
    {
        /* Data transmission falied after 3 retries at MAC layer. */
    }
    //EventUtil_vSetEvent(EVENT_SEND_PACKET);
}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
    MAC_RxFrameData_s *psFrame;

    psFrame = &psMcpsInd->uParam.sIndData.sFrame;

    int8 dbm = SysUtil_vConvertLQI2Dbm(psMcpsInd->uParam.sIndData.sFrame.u8LinkQuality);
}
/****************************************************************************
 *
 * NAME: vProcessReceivedDataPacket
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vProcessReceivedDataPacket(uint8 *pu8Data, uint8 u8Len)
{
}

/****************************************************************************
 *
 * NAME: vStartAssociate
 *
 * DESCRIPTION:
 * Start the association process with the network coordinator.
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * Assumes that a network has been found during the network scan.
 ****************************************************************************/
PRIVATE void vStartAssociate(void)
{
    MAC_MlmeReqRsp_s  sMlmeReqRsp;
    MAC_MlmeSyncCfm_s sMlmeSyncCfm;

    sEndDeviceData.eState = E_STATE_ASSOCIATING;

    /* Create associate request. We know short address and PAN ID of
       coordinator as this is preset and we have checked that received
       beacon matched this */

    sMlmeReqRsp.u8Type = MAC_MLME_REQ_ASSOCIATE;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqAssociate_s);
    sMlmeReqRsp.uParam.sReqAssociate.u8LogicalChan = sEndDeviceData.u8Channel;
    sMlmeReqRsp.uParam.sReqAssociate.u8Capability = 0x80; /* We want short address, other features off */
    sMlmeReqRsp.uParam.sReqAssociate.u8SecurityEnable = FALSE;
    sMlmeReqRsp.uParam.sReqAssociate.sCoord.u8AddrMode = 2;
    sMlmeReqRsp.uParam.sReqAssociate.sCoord.u16PanId = PAN_ID;
    sMlmeReqRsp.uParam.sReqAssociate.sCoord.uAddr.u16Short = COORDINATOR_ADR;

    /* Put in associate request and check immediate confirm. Should be
       deferred, in which case response is handled by event handler */
    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}

/****************************************************************************
 *
 * NAME: vHandleAssociateResponse
 *
 * DESCRIPTION:
 * Handle the response generated by the stack as a result of the associate
 * start request.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psMlmeInd
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vHandleAssociateResponse(MAC_MlmeDcfmInd_s *psMlmeInd)
{
    /* If successfully associated with network coordinator */
    if (psMlmeInd->uParam.sDcfmAssociate.u8Status == MAC_ENUM_SUCCESS)
    {
        sEndDeviceData.u16Address = psMlmeInd->uParam.sDcfmAssociate.u16AssocShortAddr;
        sEndDeviceData.eState = E_STATE_ASSOCIATED;
        //EventUtil_vSetEvent(EVENT_SEND_PACKET);
    }
    else
    {
        vStartActiveScan();
    }
}

/****************************************************************************
 *
 * NAME: vStartActiveScan
 *
 * DESCRIPTION:
 * Start a scan to search for a network to join.
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vStartActiveScan(void)
{
    MAC_MlmeReqRsp_s  sMlmeReqRsp;
    MAC_MlmeSyncCfm_s sMlmeSyncCfm;

    sEndDeviceData.eState = E_STATE_ACTIVE_SCANNING;

    /* Request scan */
    sMlmeReqRsp.u8Type = MAC_MLME_REQ_SCAN;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqScan_s);
    sMlmeReqRsp.uParam.sReqScan.u8ScanType = MAC_MLME_SCAN_TYPE_ACTIVE;
    sMlmeReqRsp.uParam.sReqScan.u32ScanChannels = 0x020000UL;//SCAN_CHANNELS 0x11;
    sMlmeReqRsp.uParam.sReqScan.u8ScanDuration = ACTIVE_SCAN_DURATION;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}

/****************************************************************************
 *
 * NAME: vHandleActiveScanResponse
 *
 * DESCRIPTION:
 * Handle the reponse generated by the stack as a result of the network scan.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psMlmeInd
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vHandleActiveScanResponse(MAC_MlmeDcfmInd_s *psMlmeInd)
{
    MAC_PanDescr_s *psPanDesc;
    uint8 i;

    if (psMlmeInd->uParam.sDcfmScan.u8ScanType == MAC_MLME_SCAN_TYPE_ACTIVE)
    {
        if (psMlmeInd->uParam.sDcfmScan.u8Status == MAC_ENUM_SUCCESS)
        {
            i = 0;

            while (i < psMlmeInd->uParam.sDcfmScan.u8ResultListSize)
            {
                psPanDesc = &psMlmeInd->uParam.sDcfmScan.uList.asPanDescr[i];

                if ((psPanDesc->sCoord.u16PanId == PAN_ID)
                        && (psPanDesc->sCoord.u8AddrMode == 2)
                        && (psPanDesc->sCoord.uAddr.u16Short == COORDINATOR_ADR)
                        /* Check it is accepting association requests */
                        && (psPanDesc->u16SuperframeSpec & 0x8000))
                {
                    sEndDeviceData.u8Channel = psPanDesc->u8LogicalChan;
                    vStartAssociate();
                    return;
                }
                i++;
            }
        }
        vStartActiveScan();
    }
}

/****************************************************************************
*
* NAME: vTransmitDataPacket
*
* DESCRIPTION:
*
* PARAMETERS:      Name            RW  Usage
* None.
*
* RETURNS:
* None.
*
* NOTES:
* None.
****************************************************************************/
PRIVATE void vTransmitDataPacket(uint8 *pu8Data, uint8 u8Len, uint16 u16DestAdr)
{
    MAC_McpsReqRsp_s  sMcpsReqRsp;
    MAC_McpsSyncCfm_s sMcpsSyncCfm;
    uint8 *pu8Payload, i = 0;
    static uint8 u8Handle;
    /* Create frame transmission request */
    sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
    sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);
    /* Set handle so we can match confirmation to request */
    sMcpsReqRsp.uParam.sReqData.u8Handle = u8Handle++;
    /* Use short address for source */
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = PAN_ID;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.u16Short =0x1234;
    /* Use short address for destination */
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = 0xFFFF;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.u16Short = u16DestAdr;
    /* Frame requires ack but not security, indirect transmit or GTS */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = 0;

    pu8Payload = sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu;

    //pu8Payload[0] = sEndDeviceData.u8TxPacketSeqNb++;

    for (i = 0; i < u8Len; i++)
    {
        pu8Payload[i] = *pu8Data++;
    }

    /* Set frame length */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = i;

    /* Request transmit */
    vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
