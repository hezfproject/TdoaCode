/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _NTRXFUNC_H_
#define _NTRXFUNC_H_

//=============================================================================

#ifndef NULL
	#define NULL	0
#endif

#define NA_80MHz        0
#define NA_22MHz        1
#define NA_22MHz_HR     2

#define NA_31k25_S		NA_SymbolRate31k25Symbols_VC_C
#define NA_62k5_S       NA_SymbolRate62k5Symbols_VC_C
#define NA_125k_S       NA_SymbolRate125kSymbols_VC_C
#define NA_250k_S       NA_SymbolRate250kSymbols_VC_C
#define NA_500k_S       NA_SymbolRate500kSymbols_VC_C
#define NA_1M_S         NA_SymbolRate1MSymbols_VC_C
#define NA_2M_S         NA_SymbolRate2MSymbols_VC_C

#define NA_500ns        NA_SymbolDur500ns_C
#define NA_1us          NA_SymbolDur1000ns_C
#define NA_2us          NA_SymbolDur2000ns_C
#define NA_4us          NA_SymbolDur4000ns_C
#define NA_8us          NA_SymbolDur8000ns_C
#define NA_16us         NA_SymbolDur16000ns_C

//=============================================================================

#include "NtrxDrv.h"
#include "NtrxRegMap.h"

#define MSB(x)  ((uint8)(x>>8))
#define LSB(x)  ((uint8)x)

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================

#ifdef __MAP_NTRX_VARS__
	#define _EXTERN_
#else
	#define _EXTERN_ extern
#endif

//-----------------------------------------------------------------------------
// NtrxChip.c
_EXTERN_ Bool BaseBandTimerIrq ;
_EXTERN_ Bool NtrxTxEndIrq ;
_EXTERN_ Bool NtrxRxEndIrq ;
_EXTERN_ Bool NtrxCalibration ;

_EXTERN_ float NtrxRangeConst ;

_EXTERN_ uint8 NtrxTransmitState ;
	#define TxIDLE      0x00
	#define TxSEND      0x01
	#define TxWAIT      0x02

_EXTERN_ uint8 NtrxRangingState ;
	#define RANGING_READY       0	// READY means : nothing in process
	#define RANGING_START       1	// START means : try to send first rangingpacket
	#define RANGING_ANSWER1     2	// ANSWER1 means : waiting for first ranging answer
	#define RANGING_ANSWER2     3	// ANSWER2 means : waiting for second ranging answer
	#define RANGING_SUCCESSFULL 4	// SUCCESSFULL means : all data is collected, ready to calculate now

_EXTERN_ uint8 NtrxCurrentChannel ;

_EXTERN_ NtrxBufferType NtrxLRV[] ;
_EXTERN_ NtrxBufferType NtrxSet1[], NtrxSet2[] ;
_EXTERN_ uint8 RangingRssi;
	#define	PHASEOFFSETACK		0
	#define	TOAOFFSETMEANACK_L	1
	#define	TOAOFFSETMEANACK_H	2
	#define	TXRESPTIME_L		3
	#define	TXRESPTIME_H		4
	#define	PHASEOFFSETDATA		5
	#define	TOAOFFSETMEANDATA_L	6
	#define	TOAOFFSETMEANDATA_H	7
	#define	PREVRXID            8
	#define	PREVTXID            9
	#define ACTTXID             10
	#define RANGE_ARRYLEN       11 			// Length of rangingpacket
	#define RANGE_DATALEN       8 			// Length of ranging data

void NtrxReadSPI( uint8 Addr, NtrxBufferPtr Buffer, uint8 Len ) ;
void NtrxWriteSPI( uint8 Addr, NtrxBufferPtr Buffer, uint8 Len ) ;
void NtrxFlashSPI( uint8 Addr, const NtrxFlashCode *Buffer, uint8 Len ) ;
uint8 NtrxReadSingleSPI( uint8 Addr ) ;
void NtrxWriteSingleSPI( uint8 Addr, uint8 cData ) ;
void NtrxWriteReg( uint8 Addr, uint8 Value ) ;
uint8 NtrxReadReg( uint8 Addr ) ;
void NtrxSetShadowReg( uint8 Addr, uint8 Value ) ;
uint8 NtrxGetShadowReg( uint8 Addr ) ;

//-----------------------------------------------------------------------------
// NtrxRegMap.c
Bool NtrxCheckTable( void ) ;
void NtrxSetIndexReg( uint8 Index ) ;
void NtrxSetRamIndex( uint8 Page ) ;
void NtrxSetField( NtrxRegField Field, uint8 Value ) ;
void NtrxSetLongField( NtrxRegField Field, NtrxBufferPtr Buffer ) ;
uint8 NtrxGetField( NtrxRegField Field ) ;
void NtrxGetLongField( NtrxRegField Field, NtrxBufferPtr Buffer ) ;
void NtrxProcessTable( const NtrxFlashCode *TablePtr ) ;

//-----------------------------------------------------------------------------
// NtrxTask.c
void NtrxSetCalInterval( uint16 ci ) ;
void NtrxSetPollMode( Bool mode ) ;
void NtrxRangeAddrBackup( NtrxDevPtr DstAddr ) ;
Bool NtrxMatchRangeAddr( NtrxDevPtr SrcAddr ) ;
void NtrxStartBasebandTimer( uint16 StartValue ) ;
void NtrxStopBasebandTimer( void ) ;
void NtrxAllCalibration( void ) ;
void NtrxInterruptService( void ) ;
void NtrxRxReceive( void ) ;
Bool NtrxTxEnd( void ) ;
Bool NtrxRangingMode( NtrxDevPtr DstAddr ) ;

//-----------------------------------------------------------------------------
// NtrxPara.c
void NtrxSetAgcValues( uint8 bw, uint8 sd, uint8 sr ) ;
void NtrxSetCorrThreshold( uint8 bw, uint8 sd ) ;
void NtrxSetRxIQMatrix( uint8 bw, uint8 sd ) ;
void NtrxSetTxIQMatrix( uint8 bw, uint8 sd ) ;

//=============================================================================

#ifdef __cplusplus
}
#endif

#endif /* _NTRXFUNC_H_ */
