/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _WNTRX_H_
#define _WNTRX_H_

#include "CommonTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// NA5TR1模块提供以下函数给应用程序
#define WARM_STARTUP_TIME   2

//void NtrxSetStaAddress(NtrxDevPtr Addr) ;
//void NtrxSetStaAddress2(NtrxDevPtr Addr) ;
void NtrxInit(void) ;
void NtrxWarmInit(void) ;
void NtrxWarmStart(void);
void NtrxUpdate(void) ;

typedef enum
{
    NtrxMode80MHz500ns125k =  0,
    NtrxMode80MHz500ns1M   =  1,
    NtrxMode80MHz500ns2M   =  2,
    NtrxMode80MHz1us500k   =  3,
    NtrxMode80MHz1us1M     =  4,
    NtrxMode80MHz2us125k   =  5,
    NtrxMode80MHz2us250k   =  6,
    NtrxMode80MHz2us500k   =  7,
    NtrxMode80MHz4us125k   =  8,
    NtrxMode80MHz4us250k   =  9,
    NtrxMode22MHz1us1M     = 10,
    NtrxMode22MHz2us125k   = 11,
    NtrxMode22MHz2us250k   = 12,
    NtrxMode22MHz2us500k   = 13,
    NtrxMode22MHz4us125k   = 14,
    NtrxMode22MHz4us250k   = 15,
    NtrxMode22MHz8us125k   = 16,
    NtrxMode22MHzHR4us125k = 17,
    NtrxMode22MHzHR4us250k = 18
} NtrxTRxMode ;

uint_8 NtrxSetupTRxMode(NtrxTRxMode Mode) ;

typedef enum
{
    PacketTypeData    = 0,
    PacketTypeTimeB   = 2,
    PacketTypeBrdcast = 3,
    PacketTypeRanging = 7
} NtrxPacketType ;


//float NtrxRange(NtrxDevPtr DstAddr, uint_8 data_size);

#define RAWDATALEN 8
#define TX_RESPTIME_L        0
#define TX_RESPTIME_H        1
#define TX_UCSUM_L            2
#define TX_UCSUM_H            3
#define TX_GATEOFF            4
/*typedef struct
{
    NtrxBufferType set1[RAWDATALEN];
    NtrxBufferType set2[RAWDATALEN];
    uint_8 rssi;
} NtrxRangeRawDataType ;*/

//void GetRawData(NtrxRangeRawDataType *RawData);
uint_8 GetRssi(void);
//Bool IsInfoToBeRcv(void);

void GetTxData(uint_8 *pout, uint_8 *p);

void NtrxPowerdownByDio(void);
void NtrxPowerdownMode(uint_32 Seconds) ;
void NtrxPowerdownPadMode(uint_32 s) ;

//void NtrxGetRTC(NtrxBufferPtr RtcBufferPtr) ;
//void NtrxSetRTC(NtrxBufferPtr RtcBufferPtr) ;
//void NtrxGetRTC32(NtrxBufferPtr RtcBufferPtr);

uint_8 NtrxGetChannel(void) ;
void NtrxSetChannel(uint_8 ch) ;

uint_8 NtrxGetOutputPower(void) ;
void NtrxSetOutputPower(uint_8 Value) ;

//uint_8 NtrxDevAddrMatch(NtrxDevPtr SrcAddr, NtrxDevPtr DstAddr) ;
//void NtrxSetSyncWord(NtrxBufferPtr SyncWordPtr) ;

uint_8 NtrxGetRxMode(void) ;
void NtrxSetRxMode(uint_8 mode) ;

#define ModeRxDataEn    0x01
#define ModeRxBrdCastEn 0x02
#define ModeRxTimeBEn   0x04
#define ModeRxAddrMode  0x08
#define ModeRxArqMode   0x10
#define ModeRxTimeBAuto 0x20

//-----------------------------------------------------------------------------
// NA5TR1模块需要应用程序提供以下函数( AVR的CPU还需要ReadFlashCode函数 )

void Delay_ms(uint_16 t) ;
void Delay_us(uint_16 t) ;
uint_32 GetSysClock(void) ;
void ErrorHandler(uint_8 err) ;

//void SPI_ReadBytes(uint_8 Addr, NtrxBufferPtr Buffer, uint_8 Len) ;
//void SPI_WriteBytes(uint_8 Addr, NtrxBufferPtr Buffer, uint_8 Len) ;
//void SPI_WriteCodes(uint_8 Addr, const NtrxFlashCode *Ptr, uint_8 Len) ;

void NtrxSSN_Lo(void) ;
void NtrxSSN_Hi(void) ;
void NtrxResetOn(void) ;
void NtrxResetOff(void) ;
void NtrxCtrlInit(void) ;
//void AppCallback(NtrxDevPtr SrcAddr, NtrxDevPtr DstAddr, uint_8 MsgStatus,
                 //NtrxBufferPtr Payload, uint_8 Len, uint_8 Rssi);

#define NtrxRxPacketTypeMask    0x0F        // MsgStatus Bit[0:3]
#define NtrxRxAddrMatchMask     0x30        // MsgStatus Bit[4:5]
#define NtrxRxCrc1StatMask      0x40        // MsgStatus Bit6
#define NtrxRxCrc2StatMask        0x80        // MsgStatus Bit7

//-----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif /* _WNTRX_H_ */
