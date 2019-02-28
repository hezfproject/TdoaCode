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

#define SEND_MODE_MAC			0
#define SEND_MODE_HAL			1
/*********************************************************************
* TYPEDEFS
*/

/**************************************************************************************************
* DEFINES
**************************************************************************************************/
static uint16  mac_PanId;
static uint8  mac_dst_endpoint;
static uint8  mac_src_endpoint;
static uint8  mac_cluster_id;
static uint8 mac_node_type;
static uint16 mac_profile_id;
static uint8 *mac_Payload = NULL;
static 	char pwrVal[16]= {0xFF/*0.6dBm*/, 0xDF/*0.5dBm*/, 0xBF/*0.3dBm*/, 0x9F/*0.2dBm*/,
                          0x7F/*-0.1dBm*/, 0x5F/*-0.4dBm*/, 0x3F/*-0.9dBm*/, 0x1F/*-1.5dBm*/,
                          0x1B/*-2.7dBm*/, 0x17/*-4.0dBm*/, 0x13/*-5.7dBm*/, 0x0F/*-7.9dBm*/,
                          0x0B/*-10.8dBm*/, 0x07/*-15.4dBm*/, 0x06/*-18.6dBm*/, 0x03/*-25.2dBm*/
                         };

//static uint8 mac_Payload[MAC_PAYLOAD_LEN];

static uint8 MAC_UTIL_BuildMacHdr(uint8* pHead, uint16 destPanID, uint16 destAddr, uint16 srcPanID, uint16 srcAddr);

void MAC_UTIL_INIT(MacUtil_t *p)
{
    mac_dst_endpoint = p->dst_endpoint;
    mac_src_endpoint = p->src_endpoint;
    mac_cluster_id = p->cluster_id;
    mac_profile_id = p->profile_id;
    mac_PanId = p->panID;
    mac_node_type = p->NodeType;
}

/**************************************************************************************************
*
* @fn      MAC_UTIL_BuildData(uint8* pdata, uint8 len)
*
* @brief   Build data to communicate with a full stack app.
*
* @param   data
*              pdata-pointer hold the pure data.
*              datalen
*              DeliverMode: should be MAC_UTIL_UNICAST or MAC_UTIL_MULTICAST
*              uniDstAddr:  Dest Address when using Unicast mode
* @return  total datalen
*
**************************************************************************************************/

uint8 MAC_UTIL_BuildandSendData(uint8 *p, uint8 len, uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption)
{
    MacParam_t param;
    param.panID = mac_PanId;
    param.cluster_id = mac_cluster_id;
    param.radius = 0x01;

    return  MAC_UTIL_BuildandSendDataPAN(&param, p,  len,  DeliverMode,  uniDstAddr,  txOption);
}

uint8  MAC_UTIL_BuildandSendDataPAN(const MacParam_t *param, uint8 *p, uint8 len, uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption)
{
    uint8 nwkhdrlen,totalen=0;
    uint8 ret;
    if(mac_Payload == NULL)
    {
        mac_Payload = osal_mem_alloc(MAC_MAX_FRAME_SIZE+ 10); //addsome to avoid error
    }

    if (!mac_Payload)
    {
        return FAILURE;
    }
    if(DeliverMode == MAC_UTIL_UNICAST)
    {
        nwkhdrlen= MAC_UTIL_BuildNwkHdr(mac_Payload,uniDstAddr,param->radius);
    }
    else
    {
        if(uniDstAddr == MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR)
        {
            nwkhdrlen= MAC_UTIL_BuildNwkHdr(mac_Payload,MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR,param->radius);
        }
        else
        {
            nwkhdrlen= MAC_UTIL_BuildNwkHdr(mac_Payload,MAC_UTIL_BROADCAST_SHORTADDR_DEVALL,param->radius);
        }

    }
    uint8 apshdrlen = MAC_UTIL_BuildApsHdr(mac_Payload+nwkhdrlen, param->cluster_id,DeliverMode);
    totalen = nwkhdrlen + apshdrlen;

    if(p==NULL ||  totalen + len > MAC_MAX_FRAME_SIZE)
    {
        return FAILURE;
    }

    osal_memcpy(mac_Payload+nwkhdrlen+apshdrlen, p, len);
    totalen += len;

    // send out data
    if(DeliverMode == MAC_UTIL_UNICAST)
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;
        if (mac_node_type == NODETYPE_DEVICE)
            DstAddr.addr.shortAddr = 0; //if an enddevice, need to set mac dstaddr to coordinator's addr.
        else
            DstAddr.addr.shortAddr      = uniDstAddr; //when unicast , Mac addr always be 0x0000
        ret = MAC_UTIL_McpsDataReq(mac_Payload, totalen, param->panID, DstAddr, txOption);//MAC_TXOPTION_ACK);
    }
    else
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;  /* MAC addMode shoud be 0X02 even broadcast */
        DstAddr.addr.shortAddr      = MAC_UTIL_BROADCAST_SHORTADDR_DEVALL; /* Address should be 0xFF in mac header and 0xFc or 0xFF in NWK header */
        ret = MAC_UTIL_McpsDataReq(mac_Payload, totalen,param->panID, DstAddr, txOption);//MAC_TXOPTION_NO_RETRANS);
    }
    return ret;
}


/**************************************************************************************************
*
* @fn      MAC_UTIL_BuildMacHdr(uint8* p)
*
* @brief   Build MacHdr.
*
* @param   p
*
* @return  MacHdrLen
*
**************************************************************************************************/
uint8 MAC_UTIL_BuildMacHdr(uint8* pHead, uint16 destPanID, uint16 destAddr, uint16 srcPanID, uint16 srcAddr)
{
    static uint8 seqnum;
    MAC_hdr_t* pMacHdr = (MAC_hdr_t *)(pHead);
    osal_memset((uint8 *)pHead, 0, sizeof(MAC_hdr_t));

    if (pHead)
    {
        MAC_FCF_SET_FRAMETYPE(pMacHdr, MAC_FCF_FRAMETYPE_DATA);
        MAC_FCF_SET_SEC_ENABLE(pMacHdr, 0);
        MAC_FCF_SET_FRAME_PENDING(pMacHdr, 0);
        MAC_FCF_SET_ACK_REQ(pMacHdr, 0);

        if(destPanID == srcPanID)
        {
            MAC_FCF_SET_IntraPAN(pMacHdr, 1);
        }
        else
        {
            MAC_FCF_SET_IntraPAN(pMacHdr, 0);
        }
        MAC_FCF_SET_DSTADDR_MODE(pMacHdr, MAC_FCF_ADDR_MODE_16BIT);
        MAC_FCF_SET_SRCADDR_MODE(pMacHdr, MAC_FCF_ADDR_MODE_16BIT);

        pMacHdr->seqnum = seqnum++;
        pMacHdr->DestPanID = destPanID;
        pMacHdr->DestAddr = destAddr;
        pMacHdr->SrcPanID = srcPanID;
        pMacHdr->SrcAddr = srcAddr;
    }
    uint8 maclen = 2+1+2+2+2+2;
    return maclen;
}
/**************************************************************************************************
*
* @fn      MAC_UTIL_BuildNwkHdr(uint8* macPayload)
*
* @brief   Build NwkHdr.
*
* @param   macPayload
*
*
* @return  machdrlen
*
**************************************************************************************************/
uint8 MAC_UTIL_BuildNwkHdr(uint8* macPayload,uint16 DstAddr, uint8 radius)
{
    //NWK_hdr_t* pNwkHdr = (NWK_hdr_t *)osal_mem_alloc(sizeof(NWK_hdr_t));
    NWK_hdr_t* pNwkHdr = (NWK_hdr_t *)macPayload;

    osal_memset(pNwkHdr, 0, sizeof(NWK_hdr_t));
    uint8 nwklen = 0;
    static uint8 nwkseq = 0;
    if (pNwkHdr)
    {
        NWK_SET_FRAME_TYPE(pNwkHdr, NWK_FCF_FRAME_TYPE_DATA);
        NWK_SET_PRO_VERSION(pNwkHdr, NWK_FCF_PRO_VERSION);
        NWK_SET_DISC_ROUTE(pNwkHdr, NWK_FCF_SUPRESS_DISC_ROUTE);
        NWK_SET_MULTICAST(pNwkHdr, NWK_FCF_MULTICAST_OFF);
        NWK_SET_SECURITY(pNwkHdr, NWK_FCF_SECURITY_OFF);
        NWK_SET_SRC_ROUTE(pNwkHdr, NWK_FCF_SRCROUTE_OFF);
        NWK_SET_DST_IEEE(pNwkHdr, NWK_FCF_DSTIEEE_OFF);
        NWK_SET_SRC_IEEE(pNwkHdr, NWK_FCF_SRCIEEE_OFF);

        pNwkHdr->DstAddr = DstAddr;
        /*
        if(DeliverMode==MAC_UTIL_UNICAST) // When unicast, send to its parent
        {
        MAC_UTIL_MlmeGetReq(MAC_UTIL_COORD_SHORT_ADDRESS, (void *)&(pNwkHdr->DstAddr));
        }
        else if(DeliverMode==MAC_UTIL_BROADCAST)  When broadcast,send to all routers and coords
        {
        pNwkHdr->DstAddr = MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR;
        }
        */

        MAC_MlmeGetReq(MAC_SHORT_ADDRESS, (void *)&(pNwkHdr->SrcAddr));

        pNwkHdr->BroadcastRadius = radius;
        pNwkHdr->BroadcastSeq = nwkseq++;
        nwklen = 2+2+2+1+1;

        //mac_nwkhdrlen += nwklen; //FCF, DstAddr, SrcAddr, Radius, Seq number.
        //osal_memcpy(macPayload, (void *)pNwkHdr, nwklen);
        //osal_mem_free(pNwkHdr);
    }
    return nwklen;
}

/**************************************************************************************************
*
* @fn      MAC_UTIL_BuildApsHdr(uint8* ApsPayload)
*
* @brief   Build ApsHdr.
*
* @param   ApsPayload
*
*
* @return  Apshdrlen
*
**************************************************************************************************/
uint8 MAC_UTIL_BuildApsHdr(uint8* ApsPayload, uint8 cluster_id,uint8 DeliverMode)
{
    uint8 apshdrlen = 0;
    APS_hdr_t* pApsHdr = (APS_hdr_t*)ApsPayload;//osal_mem_alloc(sizeof(APS_hdr_t)+5);
    osal_memset(pApsHdr, 0, sizeof(APS_hdr_t));

    static uint8 ApdSeq = 0;
    if (pApsHdr)
    {
        APS_SET_FRAME_TYPE(pApsHdr, APS_FCF_FRAME_TYPE_DATA);
        if(DeliverMode == MAC_UTIL_UNICAST)
        {
            APS_SET_DELIVERY_MODE(pApsHdr, APS_FCF_UNICAST);
        }
        else if(DeliverMode == MAC_UTIL_BROADCAST)
        {
            APS_SET_DELIVERY_MODE(pApsHdr, APS_FCF_BROADCAST);
        }
        APS_SET_SECURITY(pApsHdr, APS_FCF_SECURITY_OFF);
        APS_SET_ACK(pApsHdr, APS_FCF_ACK_OFF);

        pApsHdr->DstEndPoint = mac_dst_endpoint;
        pApsHdr->ClusterID = cluster_id;
        pApsHdr->ProfileID = mac_profile_id;
        pApsHdr->SrcEndPoint = mac_src_endpoint;

        pApsHdr->transID = 0;
        pApsHdr->ApsCounter = ApdSeq++;
        /*
        pApsHdr->AddrField.p = (uint8 *)(pApsHdr + 1);
        uint8 *p = pApsHdr->AddrField.p;
        *p++ = mac_dst_endpoint;
        *p++ = cluster_id;
        *p++ = LO_UINT16(mac_profile_id);
        *p++ = HI_UINT16(mac_profile_id);
        *p = mac_src_endpoint;
        pApsHdr->AddrField.len= 5;
        uint8* incrAddr = ApsPayload;
        osal_memcpy(incrAddr, &(pApsHdr->FCF), sizeof(uint8));
        incrAddr += sizeof(uint8);
        osal_memcpy(incrAddr, (void *)(pApsHdr->AddrField.p), pApsHdr->AddrField.len);
        incrAddr += pApsHdr->AddrField.len;
        osal_memcpy(incrAddr, &(pApsHdr->transID), sizeof(uint8));
        incrAddr += sizeof(uint8);
        osal_memcpy(incrAddr, &(pApsHdr->ApsCounter), sizeof(uint8));
        osal_mem_free(pApsHdr);
        */
        apshdrlen = 1+1+1+2+1+2; //FCF, dstendpoint, clusteid, profileid, srcendpoint, apscnt, apsblk
    }
    return apshdrlen;
}
/**************************************************************************************************
*
* @fn      MED_McpsDataReq()
*
* @brief   This routine calls the Data Request
*
* @param   data       - contains the data that would be sent
*          dataLength - length of the data that will be sent
*
* @return  None
*
**************************************************************************************************/

/*
void MAC_UTIL_McpsDataReq(uint8* data, uint8 dataLength, sAddr_t dstAddr,uint8 txOption)
{
MAC_UTIL_McpsDataReqPAN( mac_PanId,data,  dataLength,  dstAddr, txOption);
}
*/
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

/* Remove head of packet, note that it doesn't free and alloc memory */
sData_t MAC_UTIL_RemoveHeader(const  sData_t In)
{
    sData_t data;
    uint8 headerlen = 0;
    //-------------- Remove NWK header
    uint8 nwklen = 2+2+2+1+1; //FCF, DstAddr, SrcAddr, Radius, Seq number.
    headerlen += nwklen;
    //-------------- Remove App header
    uint8 apshdrlen = 1+1+1+2+1+2; //FCF, dstendpoint, clusteid, profileid, srcendpoint, apscnt, apsblk
    headerlen += apshdrlen;
    //--------------
    if(In.len <= headerlen)
    {
        data.p = NULL;
        data.len= 0;
    }
    else
    {
        data.p = In.p + headerlen;
        data.len = In.len -headerlen;
    }
    return data;
}

uint8 MAC_UTIL_GetClusterID(const sData_t In)
{
    //--------------  NWK header
    uint8 nwklen = 2+2+2+1+1; //FCF, DstAddr, SrcAddr, Radius, Seq number.
    //--------------  App header
    //FCF, dstendpoint, clusteid, profileid, srcendpoint, apscnt, apsblk
    return *(In.p+nwklen+1+1);
}

void MAC_UTIL_SetPANID(uint16 panId)
{
    mac_PanId = panId;
}

uint16 MAC_UTIL_GetPANID(void)
{
    return mac_PanId;
}
