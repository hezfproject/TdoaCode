/**************************************************************************************************
* INCLUDES
**************************************************************************************************/
#include "iocc2530.h"
#include "OSAL.h"
#include "mac_api.h"
#include "MacUtil.h"
#include "mac_pib.h"
/**************************************************************************************************
* DEFINES
**************************************************************************************************/

/*********************************************************************
* TYPEDEFS
*/

static uint8 *mac_Payload = NULL;

uint8  MAC_UTIL_SendDataPAN(uint16 dstPID, uint8 *p, uint8 len, uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption)
{
    uint8 totalen=0;
    uint8 ret;
    if(mac_Payload == NULL)
    {
        mac_Payload = osal_mem_alloc(MAC_MAX_FRAME_SIZE+ 10); //addsome to avoid error
    }

    if (!mac_Payload)
    {
        return FAILURE;
    }

    if(p==NULL ||  totalen + len > MAC_MAX_FRAME_SIZE)
    {
        return FAILURE;
    }

    osal_memcpy(mac_Payload, p, len);
    totalen += len;

    // send out data
    if(DeliverMode == MAC_UTIL_UNICAST)
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;

        DstAddr.addr.shortAddr      = uniDstAddr; //when unicast , Mac addr always be 0x0000
        ret = MAC_UTIL_McpsDataReq(mac_Payload, totalen, dstPID, DstAddr, txOption);//MAC_TXOPTION_ACK);
    }
    else
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;  /* MAC addMode shoud be 0X02 even broadcast */
        DstAddr.addr.shortAddr      = MAC_UTIL_BROADCAST_SHORTADDR_DEVALL; /* Address should be 0xFF in mac header and 0xFc or 0xFF in NWK header */
        ret = MAC_UTIL_McpsDataReq(mac_Payload, totalen,dstPID, DstAddr, txOption);//MAC_TXOPTION_NO_RETRANS);
    }
    return ret;
}


uint8 MAC_UTIL_McpsDataReq(const uint8* data, uint8 dataLength, uint16  panID, sAddr_t dstAddr,uint8 txOption)
{
    macMcpsDataReq_t  *pData;

    static uint8      handle;

    if ((pData = MAC_McpsDataAlloc(dataLength, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE)) != NULL)
    {
        pData->mac.srcAddrMode = SADDR_MODE_SHORT;
        pData->mac.dstAddr = dstAddr;
        pData->mac.dstPanId = panID;
        pData->mac.msduHandle = handle++;

        pData->mac.txOptions = txOption;
        pData->sec.securityLevel = FALSE;

        osal_memcpy (pData->msdu.p, data, dataLength);
        pData->msdu.len = dataLength;

        MAC_McpsDataReq(pData);
        return SUCCESS;
    }
    return FAILURE;
}



