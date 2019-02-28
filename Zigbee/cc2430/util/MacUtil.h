/**************************************************************************************************
Filename:       MacUtil.h
Revised:        $Date: 2011/01/12 00:42:07 $
Revision:       $Revision: 1.13 $

Description:    This file contains the upper Head build and remove based on mac layer
**************************************************************************************************/

#ifndef MAC_UTIL_H
#define MAC_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

	/**************************************************************************************************
	* INCLUDES
	**************************************************************************************************/
#include "Hal_types.h"
#include "Saddr.h"
#include "Sdata.h"
	/**************************************************************************************************
	* DEFINES
	**************************************************************************************************/
	//need define the node type as the same to AF.h
#define NODETYPE_COORDINATOR    0x00
#define NODETYPE_ROUTER         0x01
#define NODETYPE_DEVICE         0x02

#define MAC_UTIL_UNICAST      0
#define MAC_UTIL_BROADCAST   1

#define MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR     0xFFFC
#define MAC_UTIL_BROADCAST_SHORTADDR_DEVALL       0xFFFF

	/*The following macros are provided to help to construct nwk hdr and aps hdr, So the app can communicate
	*with a full stack app.
	*/
#define MAC_FCF_FRAMETYPE_BEACON 	0
#define MAC_FCF_FRAMETYPE_DATA 		1
#define MAC_FCF_FRAMETYPE_ACK 		2
#define MAC_FCF_FRAMETYPE_CMD 		3

#define MAC_FCF_SEC_ENABLE_OFF 		0
#define MAC_FCF_SEC_ENABLE_ON 		1

#define MAC_FCF_FRAME_PENDING_OFF 	0
#define MAC_FCF_FRAME_PENDING_ON 	1

#define MAC_FCF_ACK_REQ_OFF 		0
#define MAC_FCF_ACK_REQ_ON 		1

#define MAC_FCF_INTRA_PAN_OFF 	0
#define MAC_FCF_INTRA_PAN_ON 		1

#define MAC_FCF_ADDR_MODE_NOTPRESENT 	0x00
	//#define MAC_FCF_ADDR_MODE_RESERVED 		1
#define MAC_FCF_ADDR_MODE_16BIT 			0x02
#define MAC_FCF_ADDR_MODE_64BIT 			0x03


#define MAC_FCF_SET_FRAMETYPE(pMacHdr, fType) \
	st ( \
	pMacHdr->FCF &= ~0x0003; \
	pMacHdr->FCF |= (fType&0x03); \
	)
#define MAC_FCF_SET_SEC_ENABLE(pMacHdr, sec) \
	st ( \
	pMacHdr->FCF &= ~ BV(3); \
	pMacHdr->FCF |= (sec<<3); \
	)
#define MAC_FCF_SET_FRAME_PENDING(pMacHdr, FramePending) \
	st ( \
	pMacHdr->FCF &= ~ BV(4); \
	pMacHdr->FCF |= (FramePending<<4); \
	)

#define MAC_FCF_SET_ACK_REQ(pMacHdr, ackreq) \
	st ( \
	pMacHdr->FCF &= ~ BV(5); \
	pMacHdr->FCF |= (ackreq<<5); \
	)

#define MAC_FCF_SET_IntraPAN(pMacHdr, interaPan) \
	st ( \
	pMacHdr->FCF &= ~ BV(6); \
	pMacHdr->FCF |= (interaPan<<6); \
	)

#define MAC_FCF_SET_DSTADDR_MODE(pMacHdr, daddrMode) \
	st ( \
	pMacHdr->FCF &= ~ (BV(10) | BV(11)); \
	pMacHdr->FCF |= ((uint16)daddrMode<<10); \
	)

#define MAC_FCF_SET_SRCADDR_MODE(pMacHdr, saddrMode) \
	st ( \
	pMacHdr->FCF &= ~ (BV(14) |BV(15)); \
	pMacHdr->FCF |= ((uint16)saddrMode<<14); \
	)


#define NWK_FRAME_DATA   0x00
#define NWK_FRAME_CMD    0x01

#define NWK_FCF_FRAME_TYPE_DATA 0x00
#define NWK_FCF_FRAME_TYPE_CMD 0x01

#define NWK_FCF_PRO_VERSION 0x02

#define NWK_FCF_SUPRESS_DISC_ROUTE   0x00
#define NWK_FCF_ENABLE_DISC_ROUTE   0x01
#define NWK_FCF_FORCE_DISC_ROUTE   0x02

#define NWK_FCF_MULTICAST_ON  0x01
#define NWK_FCF_MULTICAST_OFF  0x00

#define NWK_FCF_SECURITY_ON 0x01
#define NWK_FCF_SECURITY_OFF 0x00

#define NWK_FCF_SRCROUTE_ON 0x01
#define NWK_FCF_SRCROUTE_OFF 0x00

#define NWK_FCS_DSTIEEE_ON 0x01
#define NWK_FCF_DSTIEEE_OFF 0x00

#define NWK_FCF_SRCIEEE_ON 0x01
#define NWK_FCF_SRCIEEE_OFF 0x00

	//bit:0-2
#define NWK_SET_FRAME_TYPE(pNwkHdr, fType) \
	st ( \
	pNwkHdr->FCF &= ~0x0003; \
	pNwkHdr->FCF |= fType; \
	)
#define NWK_GET_FRAME_TYPE(pNwkHdr) \
	st ( \
	pNwkHdr->FCF &= 0x0003; \
	)
	//bit:2-5
#define NWK_SET_PRO_VERSION(pNwkHdr, pv) \
	st ( \
	pNwkHdr->FCF &= ~0x003c; \
	pNwkHdr->FCF |= pv<<2; \
	)
#define NWK_GET_PRO_VERSION(pNwkHdr) \
	st ( \
	(pNwkHdr->FCF &= 0x003c)>>2; \
	)
	//bit:6-7
#define NWK_SET_DISC_ROUTE(pNwkHdr, discroute) \
	st ( \
	pNwkHdr->FCF &= ~0x00c0; \
	pNwkHdr->FCF |= discroute<<6; \
	)
#define NWK_GET_DISC_ROUTE(pNwkHdr) \
	st ( \
	(pNwkHdr->FCF &= 0x00c0)>>6; \
	)
	//bit:8
#define NWK_SET_MULTICAST(pNwkHdr, mflag) \
	st ( \
	pNwkHdr->FCF &= ~0x0100; \
	pNwkHdr->FCF |= mflag<<8; \
	)
#define NWK_GET_MULTICAST(pNwkHdr) \
	st ( \
	(pNwkHdr->FCF &= 0x0100)>>8; \
	)
	//bit:9
#define NWK_SET_SECURITY(pNwkHdr, secflag) \
	st ( \
	pNwkHdr->FCF &= ~0x0200; \
	pNwkHdr->FCF |= secflag<<9; \
	)
#define NWK_GET_SECURITY(pNwkHdr) \
	st ( \
	(pNwkHdr->FCF &= 0x0200)>>9; \
	)
	//bit:10
#define NWK_SET_SRC_ROUTE(pNwkHdr, srcRoute) \
	st ( \
	pNwkHdr->FCF &= ~0x0400; \
	pNwkHdr->FCF |= srcRoute<<10; \
	)
#define NWK_GET_SRC_ROUTE(pNwkHdr) \
	st ( \
	(pNwkHdr->FCF &= 0x0400)>>10; \
	)
	//bit:11
#define NWK_SET_DST_IEEE(pNwkHdr, dstIeee) \
	st ( \
	pNwkHdr->FCF &= ~0x0800; \
	pNwkHdr->FCF |= dstIeee<<11; \
	)
#define NWK_GET_DST_IEEE(pNwkHdr) \
	st ( \
	(pNwkHdr->FCF &= 0x0800)>>11; \
	)
	//bit:12
#define NWK_SET_SRC_IEEE(pNwkHdr, srcIeee) \
	st ( \
	pNwkHdr->FCF &= ~0x1000; \
	pNwkHdr->FCF |= srcIeee<<12; \
	)
#define NWK_GET_SRC_IEEE(pNwkHdr) \
	st ( \
	(pNwkHdr->FCF &= 0x1000)>>12; \
	)
	//bit:13-15. reserverd.

#define APS_FCF_FRAME_TYPE_DATA 0x00
#define APS_FCF_FRAME_TYPE_CMD  0x01
#define APS_FCF_FRAME_TYPE_ACK   0x02

#define APS_FCF_UNICAST                 0x00
#define APS_FCF_INDERECT_ADDR      0x01
#define APS_FCF_BROADCAST            0x02
#define APS_FCF_GROUP_ADDR          0x03

#define APS_FCF_IGNOR_DST_ENDPOINT 0x01
#define APS_FCF_IGNOR_SRC_ENDPOINT 0x00

#define APS_FCF_SECURITY_ON 0x01
#define APS_FCF_SECURITY_OFF 0x00

#define APS_FCF_ACK_ON 0x01
#define APS_FCF_ACK_OFF 0x00



	//bit:0-1
#define APS_SET_FRAME_TYPE(pApsHdr, fType) \
	st ( \
	pApsHdr->FCF &= ~0x03; \
	pApsHdr->FCF |= fType; \
	)
#define APS_GET_FRAME_TYPE(pApsHdr) \
	st ( \
	pApsHdr->FCF &= 0x03; \
	)
	//bit:2-3
#define APS_SET_DELIVERY_MODE(pApsHdr, mode) \
	st ( \
	pApsHdr->FCF &= ~0x0c; \
	pApsHdr->FCF |= mode<<2; \
	)
#define APS_GET_DELIVERY_MODE(pApsHdr) \
	st ( \
	(pApsHdr->FCF &= 0x0c)>>2; \
	)

	//bit:4
#define APS_SET_INDIR_ADDRMODE(pApsHdr, addrmode) \
	st ( \
	pApsHdr->FCF &= ~0x10; \
	pApsHdr->FCF |= addrmode<<4; \
	)
#define APS_GET_INDIR_ADDRMODE(pApsHdr) \
	st ( \
	(pApsHdr->FCF &= 0x10) >> 4; \
	)
	//bit:5
#define APS_SET_SECURITY(pApsHdr, sec) \
	st ( \
	pApsHdr->FCF &= ~0x20; \
	pApsHdr->FCF |= sec<<5; \
	)
#define APS_GET_SECURITY(pApsHdr) \
	st ( \
	(pApsHdr->FCF &= 0x20)>>5; \
	)
	//bit:6
#define APS_SET_ACK(pApsHdr, ackreq) \
	st ( \
	pApsHdr->FCF &= ~0x40; \
	pApsHdr->FCF |= ackreq<<6; \
	)
#define APS_GET_ACK(pApsHdr) \
	st ( \
	(pApsHdr->FCF &= 0x40) >> 6; \
	)
	//bit:7 reserverd.

	//#if (MED_MAC_SUPERFRAME_ORDER > MED_MAC_BEACON_ORDER)
	///#error "ERROR! Superframe order cannot be greater than beacon order."
	//#endif

	//#if ((MED_MAC_SUPERFRAME_ORDER != 15) || (MED_MAC_BEACON_ORDER != 15)) && (MED_DIRECT_MSG_ENABLED == FALSE)
	//#error "ERROR! Cannot run beacon enabled on a polling device"
	//#endif

	//#if (MED_PACKET_LENGTH < 4) || (MED_PACKET_LENGTH > 102)
	//#error "ERROR! Packet length has to be between 4 and 102"
	//#endif

	/*********************************************************************
	* TYPEDEFS
	*/
	typedef struct
	{
		uint16 FCF;
		uint8   seqnum;
		uint16 DestPanID;
		uint16 DestAddr;
		uint16 SrcPanID;
		uint16 SrcAddr;
	}  MAC_hdr_t;

	typedef struct
	{
		uint16 FCF;
		uint16 DstAddr;
		uint16 SrcAddr;
		uint8 BroadcastRadius;
		uint8 BroadcastSeq;
		sData_t AddrCtrlField;
	}NWK_hdr_t;

	typedef struct
	{
		uint8 FCF;
		//sData_t AddrField;
		uint8 DstEndPoint;
		//uint16 GroupAddr;
		uint8 ClusterID;
		uint16 ProfileID;
		uint8 SrcEndPoint;
		uint8 transID;
		uint8 ApsCounter;
	}APS_hdr_t;
	typedef struct
	{
		uint16  panID;
		uint8  dst_endpoint;
		uint8  src_endpoint;
		uint8  cluster_id;
		uint16 profile_id;
		uint8 NodeType;
	} MacUtil_t;

	typedef struct
	{
		uint16 panID;
		uint8 cluster_id;
		uint8 radius;
	} MacParam_t;

	/**************************************************************************************************
	* FUNCTIONS
	**************************************************************************************************/
	extern void MAC_UTIL_INIT(MacUtil_t *p);
	extern void MAC_UTIL_HalInit(uint8 channel, uint8 powerIdx);
	extern uint8 MAC_UTIL_BuildandSendData(uint8 *p, uint8 len, uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption);
	extern uint8 MAC_UTIL_BuildandSendDataPAN(const MacParam_t *param, uint8 *p, uint8 len, uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption);
	extern uint8 MAC_UTIL_BuildNwkHdr(uint8* macPayload,uint16 DstAddr, uint8 radius);
	extern uint8 MAC_UTIL_BuildApsHdr(uint8* ApsPayload,uint8 cluster_id,uint8 DeliverMode);
	extern sData_t MAC_UTIL_RemoveHeader(sData_t In);
	uint8 MAC_UTIL_GetClusterID(const sData_t In);
	void MAC_UTIL_SetPANID(uint16 panId);
	uint16 MAC_UTIL_GetPANID(void);
	extern sData_t MAC_UTIL_RemoveHeader(const sData_t In);
	extern uint8 MAC_UTIL_GetClusterID(const sData_t In);

	extern void MAC_UTIL_McpsDataReq(const uint8* data, uint8 dataLength,uint16 panID,sAddr_t dstAddr,uint8 txOption);

#ifdef __cplusplus
}
#endif

#endif 

