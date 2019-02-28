/****************************************************************************
 *
 * MODULE:             coordinator.c
 *
 * COMPONENT:          coordinator.c,v
 *
 * VERSION:
 *
 * REVISION:           1.3
 *
 * DATED:              2006/11/06 10:03:22
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
#include <string.h>

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
#define EVENT_MEM_TEST 0x0002
#define EVENT_FLASH_TEST 0x0004
#define EVENT_SEND_PACKET_TEST 0x0008

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_ENERGY_SCANNING,
    E_STATE_COORDINATOR_STARTED,
}teState;

/* Data type for storing data related to all end devices that have associated */
typedef struct
{
    uint16 u16ShortAdr;
    uint32 u32ExtAdrL;
    uint32 u32ExtAdrH;
}tsEndDeviceData;

typedef struct
{
    /* Data related to associated end devices */
    uint16          u16NbrEndDevices;
    tsEndDeviceData sEndDeviceData[MAX_END_DEVICES];

    teState eState;

    uint8   u8Channel;
    uint8   u8TxPacketSeqNb;
    uint8   u8RxPacketSeqNb;

}tsCoordinatorData;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitSystem(void);
PRIVATE void vStartEnergyScan(void);
PRIVATE void vStartCoordinator(void);
PRIVATE void vProcessEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleNodeAssociation(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vHandleEnergyScanResponse(MAC_MlmeDcfmInd_s *psMlmeInd);
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
PRIVATE tsCoordinatorData sCoordinatorData;
PRIVATE uint8 au8Data[80];
PRIVATE MAC_ExtAddr_s psMacAddr;
PRIVATE uint8 u8Channel;
PRIVATE uint16 u16DstAddr;
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

    vStartEnergyScan();

    while (1)
    {
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
    (void)u32AHI_Init();
    (void)u32AppQApiInit(NULL, NULL, NULL);

    vAHI_WatchdogStop();

    PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
    MemUtil_vInit();
    TimerUtil_vEnableTimer();

    vAHI_ProtocolPower(TRUE);
    vAHI_BbcSetHigherDataRate(E_AHI_BBC_CTRL_DATA_RATE_250_KBPS);

    /* Enable high power modules */
    vAHI_HighPowerModuleEnable(TRUE, TRUE);    

    /*init address*/
    MacUtil_vReadExtAddress(&psMacAddr);
    u8Channel = ((psMacAddr.u32H) >> 24) & 0xFF;
    u16DstAddr = psMacAddr.u32L & 0xFFFF;

    /* Initialise coordinator state */
    sCoordinatorData.eState = E_STATE_IDLE;
    sCoordinatorData.u8TxPacketSeqNb  = 0;
    sCoordinatorData.u8RxPacketSeqNb  = 0;
    sCoordinatorData.u16NbrEndDevices = 0;

    /* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    /* Set Pan ID and short address in PIB (also sets match registers in hardware) */
    MAC_vPibSetPanId(s_pvMac, PAN_ID);
    MAC_vPibSetShortAddr(s_pvMac, COORDINATOR_ADR);

    /* Enable receiver to be on when idle */
    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);

    MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 4);
    MAC_vPibSetMinBe(s_pvMac, 1);

    //MAC_vPibSetPromiscuousMode(s_pvMac, TRUE, TRUE);
    
    /* Allow nodes to associate */
    s_psMacPib->bAssociationPermit = 1;


    vAHI_DioSetDirection(0, E_AHI_DIO8_INT);//DIO 8 output    //RED LED ON
    
    int i;
    for (i=0;i<80;i++)
        au8Data[i]=i;

    PrintfUtil_vPrintf("system start\n");

    //FLASH
    bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);

    //EventUtil_vSetEvent(EVENT_MEM_TEST);
    //EventUtil_vSetEvent(EVENT_FLASH_TEST);
    TimerUtil_eSetTimer(EVENT_SEND_PACKET_TEST, 5000);
    
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
    } while (psMcpsInd != NULL);

    /* Check for anything on the MLME upward queue */
    do
    {
        psMlmeInd = psAppQApiReadMlmeInd();
        if (psMlmeInd != NULL)
        {
            vProcessIncomingMlme(psMlmeInd);
            vAppQApiReturnMlmeIndBuffer(psMlmeInd);
        }
    } while (psMlmeInd != NULL);

    /* Check for anything on the AHI upward queue */
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

PRIVATE void vProcessAppEvent()
{
    uint32 event = EventUtil_u32ReadEvents();
    
    switch(event)
    {
        case EVENT_SEND_PACKET:
        {
            if(sCoordinatorData.u16NbrEndDevices > 0)
            {
                //PrintfUtil_vPrintf("T\n");
                vTransmitDataPacket(au8Data, 5, 0x0001);
                EventUtil_vUnsetEvent(event);
            }            
            break;
        }
        case EVENT_SEND_PACKET_TEST:
        {
            PrintfUtil_vPrintf("T\n");
            vTransmitDataPacket(au8Data, 80, u16DstAddr);
            EventUtil_vUnsetEvent(event);
            TimerUtil_eSetTimer(EVENT_SEND_PACKET_TEST, 20);
          
            break;
        }
        case EVENT_MEM_TEST:
        {
            uint8 *p1, *p2, *p3, *p4;
            p1 = MemUtil_pvAlloc(1000);
            p2 = MemUtil_pvAlloc(1000);
            p3 = MemUtil_pvAlloc(1000);            
            p4 = MemUtil_pvAlloc(4000);
            
            PrintfUtil_vPrintf("p1 address %x\n", p1);
            PrintfUtil_vPrintf("p2 address %x\n", p2);
            PrintfUtil_vPrintf("p3 address %x\n", p3);
            PrintfUtil_vPrintf("p4 address %x\n", p4);

            MemUtil_vFree(p1);
            MemUtil_vFree(p2);
            //MemUtil_vFree(p3);
            MemUtil_vFree(p4);

            p2 = MemUtil_pvAlloc(2000);
            PrintfUtil_vPrintf("p2000 address %x\n", p2);
            MemUtil_vFree(p2);

            p4 = MemUtil_pvAlloc(4000);
            PrintfUtil_vPrintf("p4 address %x\n", p4);
            MemUtil_vFree(p4);                        

            MemUtil_vPurge();

            p2 = MemUtil_pvAlloc(2000);
            PrintfUtil_vPrintf("p2000 address after purge%x\n", p2);
            MemUtil_vFree(p2);            

            MemUtil_vFree(p3);

            MemUtil_vPurge();

            p4 = MemUtil_pvAlloc(4000);
            PrintfUtil_vPrintf("p4 address after purge %x\n", p4);
            MemUtil_vFree(p4);

            MemUtil_vPurge();
            
            TimerUtil_eSetTimer(EVENT_MEM_TEST,1000);
            EventUtil_vUnsetEvent(event);
            break;
        }
        case EVENT_FLASH_TEST:
        {
            bool_t ff;
            char str[] = "Hello Flash!";
            char str1[] = "Come on baby, i like you!";            
            char str2[256];

            uint8 d1[256];
            uint8 d2[512];
            uint8 d[256];

            uint32 begin,end;

            uint i;
            for(i=0;i<256;i++)
            {
                d1[i]=0xF;
                d2[i]=0x0F;
            }
            
            
            bAHI_FlashEraseSector(7);

            //ff=bAHI_FullFlashProgram(0x700FE, sizeof(str), (uint8*)str);
            //ff=bAHI_FullFlashProgram(0x700FE, sizeof(str1), (uint8*)str1);
            //bAHI_FullFlashRead(0x700FE, sizeof(str1), (uint8*)str2);

            ff=bAHI_FullFlashProgram(0x700FE, sizeof(d1), (uint8*)d1);
            begin=u32AHI_TickTimerRead();
            ff=bAHI_FullFlashProgram(0x70000, sizeof(d2), (uint8*)d2);
            end=u32AHI_TickTimerRead();                        
            bAHI_FullFlashRead(0x700FE, sizeof(d), (uint8*)d);

            

            //PrintfUtil_vPrintf("%s, %d\n", str2, ff);
            PrintfUtil_vPrintf("%x, %d\n",d[0], ff);
            PrintfUtil_vPrintf("%d, %d\n",begin, end);
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
    switch (psMlmeInd->u8Type)
    {
    case MAC_MLME_IND_ASSOCIATE: /* Incoming association request */
        if (sCoordinatorData.eState == E_STATE_COORDINATOR_STARTED)
        {
            vHandleNodeAssociation(psMlmeInd);
        }
        break;

    case MAC_MLME_DCFM_SCAN: /* Incoming scan results */
        if (psMlmeInd->uParam.sDcfmScan.u8ScanType == MAC_MLME_SCAN_TYPE_ENERGY_DETECT)
        {
            if (sCoordinatorData.eState == E_STATE_ENERGY_SCANNING)
            {
                /* Process energy scan results and start device as coordinator */
                vHandleEnergyScanResponse(psMlmeInd);
            }
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
    if (sCoordinatorData.eState >= E_STATE_COORDINATOR_STARTED)
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

    //EventUtil_vSetEvent(EVENT_SEND_PACKET_TEST);
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

    //vAHI_DioSetOutput(0, E_AHI_DIO8_INT);


    /* Check application layer sequence number of frame and reject if it is
       the same as the last frame, i.e. same frame has been received more
       than once. */

    int8 dbm = SysUtil_vConvertLQI2Dbm(psMcpsInd->uParam.sIndData.sFrame.u8LinkQuality);
    bool f=FALSE;
    int i;
    
    //PrintfUtil_vPrintf("r %i dbm, addr %x, len %d\n", dbm, psMcpsInd->uParam.sIndData.sFrame.sSrcAddr.uAddr.u16Short, psMcpsInd->uParam.sIndData.sFrame.u8SduLength);
    
    for(i=3;i<=DATALEN;i++)
    {
        if(psMcpsInd->uParam.sIndData.sFrame.au8Sdu[i] != i-1)
        {
            f=TRUE;
            break;
        }
    }

    static uint32 count=0,error=0;
    static uint32 count_pre=0,error_pre=0, seq_pre=0;
    static int32 rssi_total = 0;
    uint16 seq = 1;
    if(f)
    {
        error++;
    
    }
    else
    {        
        memcpy(&seq, &(psMcpsInd->uParam.sIndData.sFrame.au8Sdu[1]), sizeof(seq));
    }    

    count++;
    rssi_total += dbm;
    if(seq-seq_pre >= 1000) 
    {
        
        PrintfUtil_vPrintf("total %d rx %d, err%d, avg_rssi %i\n", seq-seq_pre, count-count_pre, error-error_pre, rssi_total/(int32)(count-count_pre));
        seq_pre=seq; count_pre=count; error_pre = error;
        rssi_total = 0;
    }

    vAHI_DioSetOutput(E_AHI_DIO8_INT, 0); 

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
 * NAME: vHandleNodeAssociation
 *
 * DESCRIPTION:
 * Handle request by node to join the network.
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
PRIVATE void vHandleNodeAssociation(MAC_MlmeDcfmInd_s *psMlmeInd)
{
    uint16 u16ShortAdr = 0xffff;
    uint16 u16EndDeviceIndex;

    MAC_MlmeReqRsp_s   sMlmeReqRsp;
    MAC_MlmeSyncCfm_s  sMlmeSyncCfm;

    if (sCoordinatorData.u16NbrEndDevices < MAX_END_DEVICES)
    {
        /* Store end device address data */
        u16EndDeviceIndex    = sCoordinatorData.u16NbrEndDevices;
        u16ShortAdr = END_DEVICE_START_ADR + sCoordinatorData.u16NbrEndDevices;

        sCoordinatorData.sEndDeviceData[u16EndDeviceIndex].u16ShortAdr = u16ShortAdr;

        sCoordinatorData.sEndDeviceData[u16EndDeviceIndex].u32ExtAdrL  =
        psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32L;

        sCoordinatorData.sEndDeviceData[u16EndDeviceIndex].u32ExtAdrH  =
        psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32H;

        sCoordinatorData.u16NbrEndDevices++;

        sMlmeReqRsp.uParam.sRspAssociate.u8Status = 0; /* Access granted */
    }
    else
    {
        sMlmeReqRsp.uParam.sRspAssociate.u8Status = 2; /* Denied */
    }

    /* Create association response */
    sMlmeReqRsp.u8Type = MAC_MLME_RSP_ASSOCIATE;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeRspAssociate_s);
    sMlmeReqRsp.uParam.sRspAssociate.sDeviceAddr.u32H = psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32H;
    sMlmeReqRsp.uParam.sRspAssociate.sDeviceAddr.u32L = psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32L;
    sMlmeReqRsp.uParam.sRspAssociate.u16AssocShortAddr = u16ShortAdr;

    sMlmeReqRsp.uParam.sRspAssociate.u8SecurityEnable = FALSE;

    /* Send association response. There is no confirmation for an association
       response, hence no need to check */
    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);

    PrintfUtil_vPrintf("Associated\n");

}

/****************************************************************************
 *
 * NAME: vStartEnergyScan
 *
 * DESCRIPTION:
 * Starts an enery sacn on the channels specified.
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
PRIVATE void vStartEnergyScan(void)
{
    /* Structures used to hold data for MLME request and response */
    MAC_MlmeReqRsp_s   sMlmeReqRsp;
    MAC_MlmeSyncCfm_s  sMlmeSyncCfm;

    sCoordinatorData.eState = E_STATE_ENERGY_SCANNING;

    /* Start energy detect scan */
    sMlmeReqRsp.u8Type = MAC_MLME_REQ_SCAN;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    sMlmeReqRsp.uParam.sReqScan.u8ScanType = MAC_MLME_SCAN_TYPE_ENERGY_DETECT;
    sMlmeReqRsp.uParam.sReqScan.u32ScanChannels = SCAN_CHANNELS;
    sMlmeReqRsp.uParam.sReqScan.u8ScanDuration = ENERGY_SCAN_DURATION;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}

/****************************************************************************
 *
 * NAME: vHandleEnergyScanResponse
 *
 * DESCRIPTION:
 * Selects a channel with low enery content for use by the wireless UART.
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
PRIVATE void vHandleEnergyScanResponse(MAC_MlmeDcfmInd_s *psMlmeInd)
{
    uint8 i = 0;
    uint8 u8MinEnergy;

	u8MinEnergy = (psMlmeInd->uParam.sDcfmScan.uList.au8EnergyDetect[0]) ;

    sCoordinatorData.u8Channel = CHANNEL_MIN;

	/* Search list to find quietest channel */
    while (i < psMlmeInd->uParam.sDcfmScan.u8ResultListSize)
    {
        if ((psMlmeInd->uParam.sDcfmScan.uList.au8EnergyDetect[i]) < u8MinEnergy)
        {
			u8MinEnergy = (psMlmeInd->uParam.sDcfmScan.uList.au8EnergyDetect[i]);
			sCoordinatorData.u8Channel = i + CHANNEL_MIN;
		}
		i++;
    }
    vStartCoordinator();
}

/****************************************************************************
 *
 * NAME: vStartCoordinator
 *
 * DESCRIPTION:
 * Starts the network by configuring the controller board to act as the PAN
 * coordinator.
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * TRUE if network was started successfully otherwise FALSE
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vStartCoordinator(void)
{
    /* Structures used to hold data for MLME request and response */
    MAC_MlmeReqRsp_s   sMlmeReqRsp;
    MAC_MlmeSyncCfm_s  sMlmeSyncCfm;

    sCoordinatorData.eState = E_STATE_COORDINATOR_STARTED;

    /* Start Pan */
    sMlmeReqRsp.u8Type = MAC_MLME_REQ_START;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    sMlmeReqRsp.uParam.sReqStart.u16PanId = PAN_ID;
    sMlmeReqRsp.uParam.sReqStart.u8Channel = u8Channel;
    sMlmeReqRsp.uParam.sReqStart.u8BeaconOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8SuperframeOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = TRUE;
    sMlmeReqRsp.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8Realignment = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8SecurityEnable = FALSE;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
    PrintfUtil_vPrintf("Coordinator started\n");
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

    /* Create frame transmission request */
    sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
    sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);
    /* Set handle so we can match confirmation to request */
    sMcpsReqRsp.uParam.sReqData.u8Handle = 1;
    /* Use short address for source */
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = PAN_ID;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.u16Short = COORDINATOR_ADR;
    /* Use short address for destination */
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = 0xFFF0;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.u16Short = u16DestAdr;
    /* Frame requires ack but not security, indirect transmit or GTS */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = MAC_TX_OPTION_ACK;

    pu8Payload = sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu;

    pu8Payload[0] = 3; //APP_PROTOCOL_TYPE_CARD;
    pu8Payload[1] = 0xF2; //TOF_CARD_OAD_NEW;
    *((uint16*)(pu8Payload+2)) = 4;

    pu8Payload[6] = 1; //DEVICE_TYPE_CARD;
    *(uint16*)(&pu8Payload+4) = 0xffff;

    /* Set frame length */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = 8;    

    vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);

    return;

    pu8Payload[0] = sCoordinatorData.u8TxPacketSeqNb++;

    for (i = 1; i < (u8Len + 1); i++)
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
