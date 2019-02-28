#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>
#include <AppApi.h>

#include "mac_util.h"
#include "system_util.h"



/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint16  mac_u16SrcPanId;
PRIVATE uint16  mac_u16SrcShortAddr;
PRIVATE uint8  mac_u8DstEndpoint;
PRIVATE uint8  mac_u8SrcEndpoint;
PRIVATE uint8 mac_u8NodeType;
PRIVATE uint16 mac_u16ProfileId;
PRIVATE uint8 mac_au8Payload[MAC_MAX_DATA_PAYLOAD_LEN_SHORT];

PRIVATE uint8 u8NwkSeq=0;    
PRIVATE uint8 u8ApsSeq=0;
PRIVATE uint8 u8MsduHandle=0;



/****************************************************************************/
/***        Functions Declaration                                               ***/
/****************************************************************************/
PUBLIC uint8 MacUtil_u8BuildNwkHdrZ06(uint8* pu8Hdr, uint16 u16SrcAddr, uint16 u16DstAddr, uint8 u8Radius);
PUBLIC uint8 MacUtil_u8BuildApsHdrZ06(uint8* pu8Hdr, uint16 u16ClusterId, uint8 u8DeliverMode);


/****************************************************************************
 *
 * NAME: MacUtil_vInit
 *
 * DESCRIPTION:
 * Sending data with short address mode.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 *
 ****************************************************************************/

PUBLIC void MacUtil_vInit(MacUtil_Setting_s *psParams)
{
	mac_u8DstEndpoint = psParams->u8Dst_endpoint;
	mac_u8SrcEndpoint = psParams->u8Src_endpoint;
	mac_u16ProfileId = psParams->u16Profile_id;
	mac_u16SrcPanId = psParams->u16SrcPanId;
	mac_u16SrcShortAddr= psParams->u16SrcShortAddr;
	mac_u8NodeType = psParams->u8NodeType;
}

/****************************************************************************
 *
 * NAME: MacUtil_vSendData
 *
 * DESCRIPTION:
 * Sending data with short address mode.
 *
 * PARAMETERS:      Name            RW  Usage
 *                           u8Len                   must be less than MAC_MAX_DATA_PAYLOAD_LEN_SHORT = 116
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * Simple setting with no security.
 ****************************************************************************/
PUBLIC uint8 MacUtil_vSendData(MacUtil_SendParams_s *psParams, 
                                                         uint8* pu16Data, uint8 u8Len, uint8 u8TxOptions)
{
    MAC_McpsReqRsp_s  sMcpsReqRsp;
    MAC_McpsSyncCfm_s sMcpsSyncCfm;

    uint16 u16DstAddr = psParams->u16DstAddr;

    if(u16DstAddr == MAC_UTIL_BROADCAST_SHORTADDR_DEVALL || 
            u16DstAddr == MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR)
    {
        u16DstAddr = MAC_UTIL_BROADCAST_SHORTADDR_DEVALL;
    }

    /* Create frame transmission request */
    sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
    sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);
    /* Set handle so we can match confirmation to request */
    sMcpsReqRsp.uParam.sReqData.u8Handle = u8MsduHandle++;
    /* Use short address for source */
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = mac_u16SrcPanId;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.u16Short = mac_u16SrcShortAddr;
    /* Use short address for destination */
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = psParams->u16DstPanId;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.u16Short = u16DstAddr;
    /* Frame requires ack but not security, indirect transmit or GTS */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = u8TxOptions;

    u8Len = u8Len > MAC_MAX_DATA_PAYLOAD_LEN_SHORT? MAC_MAX_DATA_PAYLOAD_LEN_SHORT : u8Len;
    memcpy(sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu, pu16Data, u8Len);
    /* Set frame length */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = u8Len;

    /* Request transmit */
    vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);

    return u8MsduHandle-1;
}



/****************************************************************************
 *
 * NAME: MacUtil_vSendDataZ06
 *
 * DESCRIPTION:
 * Build the Zigbee 2006 Hdr and send data with short address mode.
 *
 * PARAMETERS:      Name            RW  Usage
 *                           u8Len                   must be less than MAC_MAX_DATA_PAYLOAD_LEN_SHORT - 8 - 8 = 100
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * Simple setting with no security.
 *
 ****************************************************************************/
PUBLIC SeqNum_s MacUtil_vSendDataZ06(MacUtil_SendParams_s *psParams, uint8* pu16Data, uint8 u8Len, uint8 u8TxOptions)
{
    uint8 nwklen;
    uint8 apslen;
    SeqNum_s sSeq;

    nwklen = MacUtil_u8BuildNwkHdrZ06((uint8*)mac_au8Payload, mac_u16SrcShortAddr, psParams->u16DstAddr, psParams->u8Radius);
    apslen = MacUtil_u8BuildApsHdrZ06(((uint8*)mac_au8Payload) + nwklen, psParams->u16ClusterId, psParams->u8DeliverMode);

    //FIXME, send incomplete data is not a good choice!!!
    if(u8Len  > Z06_MAX_DATA_PAYLOAD_LEN_SHORT)
        u8Len = Z06_MAX_DATA_PAYLOAD_LEN_SHORT;

    memcpy((uint8*)mac_au8Payload + nwklen + apslen, pu16Data, u8Len);

    MacUtil_vSendData(psParams, mac_au8Payload, nwklen+apslen+u8Len, u8TxOptions);

    sSeq.u8ApsSeqNum = u8ApsSeq-1;
    sSeq.u8NwkSeqNum = u8NwkSeq-1;
    sSeq.u8MsduHandle = u8MsduHandle-1;

    return sSeq;
}


/****************************************************************************
 *
 * NAME: MacUtil_u8BuildNwkHdrZ06
 *
 * DESCRIPTION:
 * Build the Zigbee 2006 Nwk Hdr.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 * return the hdr length.
 *
 * NOTES:
 * Data frame.
 * Simple setting with no security.
 * Protocol id 0x02
 ****************************************************************************/

PUBLIC uint8 MacUtil_u8BuildNwkHdrZ06(uint8* pu8Hdr, uint16 u16SrcAddr, uint16 u16DstAddr, uint8 u8Radius)
{
    if(!pu8Hdr) return 0;

    Z06_NWK_Hdr_s* psNwkHdr = (Z06_NWK_Hdr_s*)pu8Hdr;
    
    memset(pu8Hdr, 0, sizeof(Z06_NWK_Hdr_s));    

    //fixed FCF for data frame
    psNwkHdr->u16FCF = 0x0800;

    //convert the Jennic byte order in big-endian to packed network byte order (little-endian)
    psNwkHdr->u16SrcAddr = u16SrcAddr;
    SysUtil_vConvertEndian(&psNwkHdr->u16SrcAddr, sizeof(psNwkHdr->u16SrcAddr));

    //convert the Jennic byte order in big-endian to packed network byte order (little-endian)    
    psNwkHdr->u16DstAddr = u16DstAddr;
    SysUtil_vConvertEndian(&psNwkHdr->u16DstAddr, sizeof(psNwkHdr->u16DstAddr));
    
    psNwkHdr->u8BroadcastRadius = u8Radius;
    psNwkHdr->u8BroadcastSeq = u8NwkSeq++;

    //fixed header
    return Z06_NWK_HDR_LEN;
}


/****************************************************************************
 *
 * NAME: MacUtil_u8BuildApsHdrZ06
 *
 * DESCRIPTION:
 * Build the Zigbee 2006 Aps Hdr.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 * return the hdr length.
 *
 * NOTES:
 * Data frame.
 * Simple setting with no security, no Aps ack. 
 ****************************************************************************/
PUBLIC uint8 MacUtil_u8BuildApsHdrZ06(uint8* pu8Hdr, uint16 u16ClusterId, uint8 u8DeliverMode)
{
    if(!pu8Hdr) return 0;

    Z06_APS_Hdr_s* psApsHdr = (Z06_APS_Hdr_s*)pu8Hdr;

    memset(psApsHdr, 0, sizeof(Z06_APS_Hdr_s));
    
    psApsHdr->u8FCF = u8DeliverMode == MAC_UTIL_BROADCAST? 0x08 : 0x00;

    //convert the Jennic byte order in big-endian to packed network byte order (little-endian)
    psApsHdr->u16ClusterID = u16ClusterId;
    SysUtil_vConvertEndian(&psApsHdr->u16ClusterID, sizeof(psApsHdr->u16ClusterID));

    //convert the Jennic byte order in big-endian to packed network byte order (little-endian)
    psApsHdr->u16ProfileID = mac_u16ProfileId;
    SysUtil_vConvertEndian(&psApsHdr->u16ProfileID, sizeof(psApsHdr->u16ProfileID));

    psApsHdr->u8SrcEndPoint = mac_u8SrcEndpoint;
    psApsHdr->u8DstEndPoint = mac_u8DstEndpoint;
    psApsHdr->u8ApsCounter = u8ApsSeq++;

    //Fixed frame length
    return Z06_APS_HDR_LEN;
}


/****************************************************************************
 *
 * NAME: MacUtil_pu8GetApsPayloadZ06
 *
 * DESCRIPTION:
 * get the Z06 APS payload from mac payload
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 * return the pointer to the APS payload.
 *
 * NOTES:
 * the nwk and aps hdr lenght are both 8 bytes
 ****************************************************************************/

PUBLIC uint8* MacUtil_pu8GetApsPayloadZ06(uint8* pu8MacPayload)
{
    return pu8MacPayload+Z06_APS_HDR_LEN+Z06_NWK_HDR_LEN;
}

PUBLIC void MacUtil_vRemoveChild(MAC_ExtAddr_s  sExtAddr, uint16 u16PanId)
{
    MAC_MlmeReqRsp_s sReq;
    MAC_MlmeSyncCfm_s sCfm;

    sReq.u8Type = MAC_MLME_REQ_DISASSOCIATE;
    sReq.u8ParamLength = sizeof(MAC_MlmeReqDisassociate_s);
    
    sReq.uParam.sReqDisassociate.sAddr.u8AddrMode = 3; // ext address
    sReq.uParam.sReqDisassociate.sAddr.u16PanId = u16PanId;
    sReq.uParam.sReqDisassociate.sAddr.uAddr.sExt = sExtAddr;
    sReq.uParam.sReqDisassociate.u8Reason = 1; //Coordinator wishes device to leave the PAN
    sReq.uParam.sReqDisassociate.u8SecurityEnable = FALSE;

    vAppApiMlmeRequest(&sReq, &sCfm);    
}

/****************************************************************************
 *
 * NAME: MacUtil_vReadExtAddress
 *
 * DESCRIPTION:
 * Read 64-bit ext addreass from flash.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PUBLIC void MacUtil_vReadExtAddress(MAC_ExtAddr_s *psExtAddress)
{
    uint32 *pu32Mac = pvAppApiGetMacAddrLocation();
    psExtAddress->u32H = pu32Mac[0];
    psExtAddress->u32L = pu32Mac[1];    
}

/****************************************************************************
 *
 * NAME: MacUtil_vReadExtAddress
 *
 * DESCRIPTION:
 * Read 64-bit ext addreass from flash.
 *
 * PARAMETERS:      Name            RW  Usage
 *                           u8MacPayload       the raw data of mac payload
 * RETURNS:
 *
 * NOTES:
 * assume the nwk and aps header are both 8 bytes according to current usage
 ****************************************************************************/
PUBLIC uint16 MacUtil_u16GetClusterIdZ06(uint8* u8MacPayload)
{
    Z06_APS_Hdr_s *psApsHdr =(Z06_APS_Hdr_s*)(u8MacPayload+8);
    
    //convert packed network byte order (little-endian) to the Jennic byte order in big-endian    
    SysUtil_vConvertEndian(&psApsHdr->u16ClusterID, sizeof(psApsHdr->u16ClusterID));
    return psApsHdr->u16ClusterID;
}

