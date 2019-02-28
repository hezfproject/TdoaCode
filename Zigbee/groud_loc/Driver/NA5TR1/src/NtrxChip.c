/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#define __MAP_NTRX_VARS__
#include "NtrxChip.h"
//#include "STM32_SPI.c"

#define SHADOW_REG_COUNT        128

NtrxBufferType NtrxShadowReg[SHADOW_REG_COUNT] ;

Bool BaseBandTimerIrq ;
Bool NtrxTxEndIrq ;
Bool NtrxRxEndIrq ;
Bool NtrxCalibration ;
uint8 NtrxCurrentChannel = 0 ;
uint8 NtrxTransmitState ;
uint8 NtrxRangingState ;
float NtrxRangeConst = 0.0 ;

NtrxBufferType NtrxLRV[RANGE_ARRYLEN] ;	// Local ranging values
NtrxBufferType NtrxSet1[RANGE_DATALEN], NtrxSet2[RANGE_DATALEN] ; // this holds the collected data for calc
uint8 RangingRssi;

//=============================================================================

void NtrxReadSPI( uint8 Addr, NtrxBufferPtr Buffer, uint8 Len )
{
	NtrxSSN_Lo() ;
	SPI_ReadBytes( Addr, Buffer, Len ) ;
	NtrxSSN_Hi() ;
}

void NtrxWriteSPI( uint8 Addr, NtrxBufferPtr Buffer, uint8 Len )
{
	NtrxSSN_Lo() ;
	SPI_WriteBytes( Addr, Buffer, Len ) ;
	NtrxSSN_Hi() ;
}

void NtrxFlashSPI( uint8 Addr, const NtrxFlashCode *Ptr, uint8 Len )
{
	NtrxSSN_Lo() ;
	SPI_WriteCodes( Addr, Ptr, Len ) ;
	NtrxSSN_Hi() ;
}

uint8 NtrxReadSingleSPI( uint8 Addr )
{
	NtrxBufferType r ;
	NtrxSSN_Lo() ;
	SPI_ReadBytes( Addr, &r, 1 ) ;
	NtrxSSN_Hi() ;
	return r ;
}

void NtrxWriteSingleSPI( uint8 Addr, uint8 cData )
{
	NtrxBufferType v ;
	v = cData ;
	NtrxSSN_Lo() ;
	SPI_WriteBytes( Addr, &v, 1 ) ;
	NtrxSSN_Hi() ;
}

//=============================================================================

void NtrxWriteReg( uint8 Addr, uint8 Value )
{
	NtrxWriteSingleSPI( Addr, Value ) ;
	if( Addr < SHADOW_REG_COUNT )  		//如果是影寄存器，修改对应的影子寄存器的值
		NtrxShadowReg[Addr] = Value ;
}

uint8 NtrxReadReg( uint8 Addr )
{
	return NtrxReadSingleSPI( Addr ) ;
}

void NtrxSetShadowReg( uint8 Addr, uint8 Value )
{
	if( Addr < SHADOW_REG_COUNT )
		NtrxShadowReg[Addr] = Value ;
}

uint8 NtrxGetShadowReg( uint8 Addr )
{
	if( Addr < SHADOW_REG_COUNT )
		return NtrxShadowReg[Addr] ;
	return 0 ;
}

//=============================================================================

