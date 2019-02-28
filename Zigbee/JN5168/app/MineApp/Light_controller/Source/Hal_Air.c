#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <stdbool.h>
#include <AppApi.h>

#include "JN5148_util.h"
#include "app_protocol.h"
#include "Hal_Air.h"
/**************************************************************************************************
*                                          DEFINES
**************************************************************************************************/

#if (defined DEBUG_COM_RF_QUEUE)
#define DBG(x) do{x}while(0)
#else
#define DBG(x)
#endif

// for errors
#if (defined DEBUG_ERROR)
#define EDBG(x) do{x} while (0);
#else
#define EDBG(x)
#endif


#define HAL_AIR_HEAPLEN             8
#define HAL_AIR_RETRANS_TIME   3
/**************************************************************************************************
*                                          TYPEDEFS
**************************************************************************************************/
typedef struct
{
    uint8 pSend[MAC_MAX_DATA_PAYLOAD_LEN];
    uint16 SendLen;
    uint16 dstPanid;
    uint16 dstAddr;

    bool IsNeedRetrans;
    bool IsUsing;
    bool IsSending;
    uint8 retransCnt;
    uint8 handle;
    uint32 SendTimeMs;   		// the left time for next send
    uint32 SendStartTime;       // the time tick when start send
} Hal_transInfo_t;

static Hal_transInfo_t  Hal_transInfo[HAL_AIR_HEAPLEN];
static uint16 u16SrcPanId = 0xFFFF;
static uint16 u16SrcShortAddr = 0xFFFF;
static uint32 Hal_timeMs;
/**************************************************************************************************
*                                         functions
**************************************************************************************************/
PRIVATE uint8 Hal_SendData(uint8 *pu16Data, uint16 u8Len, uint16 u16DstPanId, uint16 u16DstAddr, uint8 msduHandle, uint8 u8TxOptions);
PRIVATE int8 Hal_HeapFindFirstAvail ( void );

/**************************************************************************************************
*                                         functions
**************************************************************************************************/
void Hal_AirInit (uint16 srcPanId, uint16 srcAddr )
{
    memset ( Hal_transInfo, 0, sizeof ( Hal_transInfo_t ) *HAL_AIR_HEAPLEN );

    uint32 i;
    for(i = 0; i < HAL_AIR_HEAPLEN; i++)
    {
        Hal_transInfo[i].IsUsing = false;
    }

    u16SrcPanId = srcPanId;
    u16SrcShortAddr = srcAddr;
}

bool_t Hal_SendDataToAir ( const uint8 *p, uint8 len, uint16 dstPan, uint16 dstaddr, bool retrans )
{
    if ( p == NULL  || len > MAC_MAX_DATA_PAYLOAD_LEN )
    {
        EDBG(PrintfUtil_vPrintf("Err: Send L%d\n", len););
        return false;
    }

    /* find a available sending buf */
    int8 heapIdx = 0;
    static uint8 fault_count=0;

    if ( ( heapIdx = Hal_HeapFindFirstAvail () ) < 0 )
    {
        EDBG(PrintfUtil_vPrintf("Err: No Heap! count: %d\n",fault_count););
        if(fault_count++>20)
        {
            //vSaveResetType(APP_LS_REPORT_WATCHDOG_RESTART,REPORT_NO_HEAP);
            vAHI_SwReset();
        }
        return false;
    }

    if(fault_count)
    {
        EDBG(PrintfUtil_vPrintf("noheap! %d\n",fault_count););
    }
    fault_count=0;
    /* fill send infos */
    Hal_transInfo[heapIdx].IsNeedRetrans = retrans;
    Hal_transInfo[heapIdx].IsUsing = true;
    Hal_transInfo[heapIdx].IsSending = false;
    Hal_transInfo[heapIdx].retransCnt = 0;
    Hal_transInfo[heapIdx].SendTimeMs = 0; //first send

    /* app_header */
    memcpy ( Hal_transInfo[heapIdx].pSend, ( void * ) p, len );
    Hal_transInfo[heapIdx].SendLen = len;
    Hal_transInfo[heapIdx].dstPanid = dstPan;
    Hal_transInfo[heapIdx].dstAddr  = dstaddr;

    return true;
}

int8 Hal_HeapFindFirstAvail ( void )
{
    uint32 i;
    for (i = 0; i < HAL_AIR_HEAPLEN; i++ )
    {
        if ( Hal_transInfo[i].IsUsing == false)
        {
            return i;
        }
    }
    return -1;
}

void Hal_TimePoll( void )
{
    static uint32 old_tick;
    uint32 tick = u32AHI_TickTimerRead();
    uint32 diff_time = (tick - old_tick)/16000;

    if(diff_time >0 )
    {
        Hal_timeMs += diff_time;
        old_tick = tick;
    }
}

uint32  Hal_GetTimeMs( void )
{
    return Hal_timeMs;
}

/* always safe to call this function in poll */
void  Hal_AirPoll ( void )
{
    static uint32 old_time;
    static uint32 sent_count=0;
    uint32 time = Hal_GetTimeMs();
    uint32 diff_time = time - old_time;
    old_time = time;

    if(diff_time==0)
    {
        return;
    }
    //PrintfUtil_vPrintf("polling, time%d\n", time);

    uint32 i;
    for (i = 0; i < HAL_AIR_HEAPLEN; i++ )
    {
        if ( Hal_transInfo[i].IsUsing == true && Hal_transInfo[i].IsSending == false)
        {
            if(Hal_transInfo[i].SendTimeMs <= diff_time)
            {
                Hal_transInfo[i].SendTimeMs = 0;
                Hal_transInfo[i].IsSending = true;
                Hal_transInfo[i].SendStartTime = Hal_timeMs;  // record the current sending time

                if(Hal_transInfo[i].retransCnt > 0)
                    DBG(PrintfUtil_vPrintf("Sending time:%d, addr:%d, recnt:%d \n", time, Hal_transInfo[i].dstAddr, Hal_transInfo[i].retransCnt););

                sent_count++;
                uint8 msduHandle = (Hal_transInfo[i].handle++) * HAL_AIR_HEAPLEN + i;
                Hal_SendData(Hal_transInfo[i].pSend, Hal_transInfo[i].SendLen, Hal_transInfo[i].dstPanid
                             , Hal_transInfo[i].dstAddr, msduHandle, MAC_TX_OPTION_ACK);
                if(sent_count>1000)
                {
                    sent_count=0;
                    DBG(PrintfUtil_vPrintf("AIR working!\n"););
                }
            }
            else
            {
                Hal_transInfo[i].SendTimeMs -= diff_time;
            }
        }

        /* if current time > send start time + 200ms,  then clear the heap */
        if(Hal_transInfo[i].IsUsing == true && Hal_timeMs - Hal_transInfo[i].SendStartTime > 200)
        {
            EDBG(PrintfUtil_vPrintf("Err: Heap Clear!\n"););
            Hal_transInfo[i].IsUsing =  FALSE;
            Hal_transInfo[i].IsSending =  FALSE;
        }
    }

    return;
}

uint8  hal_ProcessDataCnf (MAC_McpsCfmData_s *pCnf )
{
    uint8 Idx = pCnf->u8Handle % HAL_AIR_HEAPLEN;
    static uint8 error_count=0;
    static uint8 error_count_E0=0;  //MAC_ENUM_BEACON_LOSS
    static uint8 error_count_E1=0;  //MAC_ENUM_CHANNEL_ACCESS_FAILURE
    static uint8 error_count_E8=0;  //MAC_ENUM_INVALID_PARAMETER
    static uint8 error_count_E9=0;  //MAC_ENUM_NO_ACK
    static uint8 error_count_F0=0;  //MAC_ENUM_TRANSACTION_EXPIRED
    static uint8 error_count_F1=0;  //MAC_ENUM_TRANSACTION_OVERFLOW
    static uint8 error_count_other=0;  //

    if ( Hal_transInfo[Idx].IsUsing == true && Hal_transInfo[Idx].IsSending == true )
    {
        Hal_transInfo[Idx].IsSending = false;
        /* send success or do not need retrans or retrans time overflow */
        if ( pCnf->u8Status == MAC_ENUM_SUCCESS || Hal_transInfo[Idx].IsNeedRetrans == false || Hal_transInfo[Idx].retransCnt >= HAL_AIR_RETRANS_TIME )
        {
            /* avail to send new one */
            Hal_transInfo[Idx].IsUsing = false;

            if((pCnf->u8Status == MAC_ENUM_SUCCESS)||(pCnf->u8Status == MAC_ENUM_NO_ACK))
            {
                error_count=0;
                EDBG(
                    error_count_E0=error_count_E1=error_count_E8=error_count_E9=error_count_F0=error_count_F1=error_count_other=0;
                );

            }

        }
        else // resend after a rand time  0-32
        {

            EDBG(
                switch(pCnf->u8Status )
        {
        case MAC_ENUM_BEACON_LOSS:
            error_count_E0++;
            break;

        case MAC_ENUM_CHANNEL_ACCESS_FAILURE:
            error_count_E1++;
            break;

        case MAC_ENUM_INVALID_PARAMETER:
            error_count_E8++;
            break;

        case MAC_ENUM_NO_ACK:
            error_count_E9++;
            break;

        case MAC_ENUM_TRANSACTION_EXPIRED:
            error_count_F0++;
            break;

        case MAC_ENUM_TRANSACTION_OVERFLOW:
            error_count_F1++;
            break;

        default:
            error_count_other++;
            break;

        }

        );
            error_count++;
            {
                vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_DISABLED);
                uint16 randtime = u16AHI_ReadRandomNumber();
                randtime &= 0x1F;
                randtime += 3;

                uint32 timeMs = Hal_GetTimeMs();
                DBG(PrintfUtil_vPrintf("trans fail, time%d, errcode:%d, nexttime:%d\n",timeMs,pCnf->u8Status, randtime););

                Hal_transInfo[Idx].retransCnt++;
                Hal_transInfo[Idx].SendTimeMs = randtime;
            }
        }

        if(error_count >= 30)
        {
            EDBG(PrintfUtil_vPrintf("CNF, E0:%d  E1:%d  E8:%d   E9:%d   F0:%d   F1:%d   other:%d  \n",error_count_E0,error_count_E1,error_count_E8,  \
                             error_count_E9,error_count_F0,error_count_F1,error_count_other););
            error_count=0;
            //vSaveResetType(APP_LS_REPORT_WATCHDOG_RESTART,REPORT_BLAST_ERR_RESTART);
            vAHI_SwReset();
            //AppWarmStart();
        }
    }
    else
    {
        EDBG(PrintfUtil_vPrintf("Err: cnf %d %d %d\n",Hal_transInfo[Idx].IsUsing,Hal_transInfo[Idx].IsSending, pCnf->u8Status););
    }
    return 0;
}

PRIVATE uint8 Hal_SendData(uint8 *pu16Data, uint16 u8Len, uint16 u16DstPanId, uint16 u16DstAddr, uint8 msduHandle, uint8 u8TxOptions)
{
    MAC_McpsReqRsp_s  sMcpsReqRsp;
    MAC_McpsSyncCfm_s sMcpsSyncCfm;

    /* Create frame transmission request */
    sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
    sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);
    /* Set handle so we can match confirmation to request */
    sMcpsReqRsp.uParam.sReqData.u8Handle = msduHandle;
    /* Use short address for source */
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = u16SrcPanId;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.u16Short = u16SrcShortAddr;
    /* Use short address for destination */
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = u16DstPanId;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.u16Short = u16DstAddr;
    /* Frame requires ack but not security, indirect transmit or GTS */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = u8TxOptions;

    u8Len = u8Len > MAC_MAX_DATA_PAYLOAD_LEN ? MAC_MAX_DATA_PAYLOAD_LEN : u8Len;
    memcpy(sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu, pu16Data, u8Len);
    /* Set frame length */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = u8Len;

    /* Request transmit */
    vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);

    return 0;
}

