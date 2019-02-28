/*******************************************************************************
  Filename:     rf.h
  Revised:        $Date: 18:21 2012Äê5ÔÂ7ÈÕ
  Revision:       $Revision: 1.0 $
  Description:  RF library header file

*******************************************************************************/
#ifndef RF_H
#define RF_H

/*******************************************************************************
The "RF" library contains simple functions for packet transmission and
reception with the IEEE 802.15.4 compliant radio devices. The intention of this
library is to demonstrate how the radio devices are operated, and not to provide
a complete and fully-functional packet protocol. The protocol uses 802.15.4
MAC compliant data
and acknowledgment packets, however it contains only a small subset of  the
802.15.4 standard:
- Association, scanning nor beacons are not implemented
- No defined coordinator/device roles (peer-to-peer, all nodes are equal)
- Waits for the channel to become ready, but does not check CCA twice
(802.15.4 CSMA-CA)
- Does not retransmit packets

INSTRUCTIONS:
Startup:
1. Create a basicRfCfg_t structure, and initialize the members:
2. Call RF_Init() to initialize the packet protocol.

Transmission:
1. Create a buffer with the payload to send
2. Call RF_SendPacket()

Reception:
1. Check if a packet is ready to be received by highger layer with
RF_PacketIsReady()
2. Call RF_Receive() to receive the packet by higher layer

FRAME FORMATS:
Data packets (without security):
[Preambles (4)][SFD (1)][Length (1)][Frame control field (2)]
[Sequence number (1)][PAN ID (2)][Dest. address (2)][Source address (2)]
[Payload (Length - 2+1+2+2+2)][Frame check sequence (2)]

Acknowledgment packets:
[Preambles (4)][SFD (1)][Length = 5 (1)][Frame control field (2)]
[Sequence number (1)][Frame check sequence (2)]
*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <types.h>
#include <defs.h>
#include <hal_rf.h>

/*******************************************************************************
* TYPEDEFS
*/

typedef struct
{
    UINT16  u16MyAddr;
    UINT16  u16PanId;
    UINT8   u8Channel;
    BOOL    bAckReq;
}RF_CFG_T;

typedef enum
{
    RF_GAIN_LOW     = HAL_RF_GAIN_LOW,
    RF_GAIN_HIGH    = HAL_RF_GAIN_HIGH
}RF_GAIN_MODE_E;

typedef enum
{
    RF_TXPOWER_4P5_DBM  = HAL_RF_TXPOWER_4P5_DBM,
    RF_TXPOWER_2P5_DBM  = HAL_RF_TXPOWER_2P5_DBM,
    RF_TXPOWER_1_DBM    = HAL_RF_TXPOWER_1_DBM,
    RF_TXPOWER_M0P5_DBM = HAL_RF_TXPOWER_M0P5_DBM,
    RF_TXPOWER_M1P5_DBM = HAL_RF_TXPOWER_M1P5_DBM,
    RF_TXPOWER_M3_DBM   = HAL_RF_TXPOWER_M3_DBM,
    RF_TXPOWER_M6_DBM   = HAL_RF_TXPOWER_M6_DBM,
    RF_TXPOWER_M18_DBM  = HAL_RF_TXPOWER_M18_DBM
}RF_TXPOWER_INDEX_E;

typedef struct
{
    RF_GAIN_MODE_E      emGainMode;
    RF_TXPOWER_INDEX_E  emIndex;
}RF_DEV_T;

typedef struct
{
    UINT16  u16DstAddr;
    UINT16  u16DstPanId;
    UINT16  u16SrcAddr;
    UINT16  u16SrcPanId;
    INT16   s16Rssi;
    INT8    s8Len;
    UINT8   u8Data[];
}RF_DATA_T;

/*******************************************************************************
* GLOBAL FUNCTIONS
*/
VOID    RF_ReceiveOn(VOID);

VOID    RF_ReceiveOff(VOID);

INT8    RF_GetRssi(VOID);

BOOL    RF_PacketIsReady(VOID);

BOOL    RF_PacketIsOK(UINT16 u16SrcPanId, UINT16 u16SrcAddr);

UINT8   RF_Init(RF_CFG_T* pstRfCfg, RF_DEV_T* pstRfDevCfg);

UINT8   RF_Receive(UINT8* pu8RxData, UINT8 u8Len, INT16* p16Rssi);

UINT8   RF_SendPacket(UINT16 u16DestAddr, UINT16 dstpanid,
                            UINT8* pu8Payload, UINT8 u8Length);

RF_DATA_T* RF_ReceivePkt(VOID);

VOID    RF_RevertPkt(RF_DATA_T* pkt);

#endif
