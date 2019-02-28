/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxConfig.h"
#include "NtrxChip.h"
#include <string.h>

// Speed of our signal in AIR or CABLE [um/ps]
#define SPEED_OF_AIR 299.792458
#define SPEED_OF_CABLE 299.792458*0.66

// Measurement difference between remote and local station isnt allowed to
// be bigger than MAX_DIFF_ALLOWED
// If its bigger than N [m], it will droped
#define MAX_DIFF_ALLOWED 6

//-----------------------------------------------------------------------------
// GetTransAirTime() calculates the airtime of packet without processing time in chip.
// Par p is one set of data which is necessary for calculation

float GetTransAirTime( uint8 *p )
{
	#define CLK_4MHz   4.0
	#define CLK_32MHz  32.0
	#define CLK_LOD20  (2000.0/244175)	// Scaled 1:20 divider's clock period [MHz]

	float t ;
	uint16 UcSum ;
	uint8 GateOff ;

	t = (float)( ((uint16)p[TXRESPTIME_H]<<8)|p[TXRESPTIME_L] ) / CLK_4MHz ;
	UcSum = ( ((uint16)p[TOAOFFSETMEANACK_H]<<8)|p[TOAOFFSETMEANACK_L] ) +
	        ( ((uint16)p[TOAOFFSETMEANDATA_H]<<8)|p[TOAOFFSETMEANDATA_L] ) ;
	GateOff = ( p[PHASEOFFSETACK]  == 7 ? 7 : 6 - p[PHASEOFFSETACK] ) +
	          ( p[PHASEOFFSETDATA] == 7 ? 7 : 6 - p[PHASEOFFSETDATA] ) ;

	t = ( t - (float)GateOff/CLK_32MHz - (float)UcSum*CLK_LOD20/(2.0*24) ) / 2.0 - NtrxRangeConst ;
	
	// t = ( t - (float)GateOff/CLK_32MHz - (float)UcSum/CLK_32MHz*(2.0*24) ) / 2.0 - NtrxRangeConst ;
	
	return t ;
}

void GetTxData(uint8 * p, uint8 * pout)
{
	uint16 UcSum;
	pout[TX_RESPTIME_L] = p[TXRESPTIME_L] ;
	pout[TX_RESPTIME_H] = p[TXRESPTIME_H] ;
	
	UcSum = ( ((uint16)p[TOAOFFSETMEANACK_H]<<8)|p[TOAOFFSETMEANACK_L] ) + ( ((uint16)p[TOAOFFSETMEANDATA_H]<<8)|p[TOAOFFSETMEANDATA_L] ) ;
	pout[TX_UCSUM_L] = (uint8)UcSum;
	pout[TX_UCSUM_H] = (uint8)(UcSum >> 8);
	pout[TX_GATEOFF] = ( p[PHASEOFFSETACK]  == 7 ? 7 : 6 - p[PHASEOFFSETACK] ) + ( p[PHASEOFFSETDATA] == 7 ? 7 : 6 - p[PHASEOFFSETDATA] ) ;
}
//-----------------------------------------------------------------------------
// GetDistance() calculates indirect the distance.

// Speed of our signal in AIR or CABLE [um/ps]
#define SPEED_OF_AIR   299.792458
#define SPEED_OF_CABLE 299.792458*0.66

// Measurement difference between remote and local station isnt allowed to
// be bigger than MAX_DIFF_ALLOWED
// If its bigger than N [m], it will droped
#define MAX_DIFF_ALLOWED 6

float GetDistance( void )
{
	float d1, d2, d ;
	d1 = GetTransAirTime( NtrxSet1 ) * SPEED_OF_AIR ;
	d2 = GetTransAirTime( NtrxSet2 ) * SPEED_OF_AIR ;
	
	if( d1 > d2 )
	{
		if( d1 - d2 > MAX_DIFF_ALLOWED )
			return -1.0 ;
	}
	else
	{
		if( d2 - d1 > MAX_DIFF_ALLOWED )
			return -1.0 ;
	}
	
	d = ( d1 + d2 ) / 2.0 ;
	if( d < 0 )
	{
		if( d <= -1.0 )
			return -1 ;
		return 0 ;
	}
#if 0
    
	if( d > 2.0 )//不用权重，误差主要不是折射
	{
		if( d1 > d2 )
			d = ( d1 + d2 * 2 ) / 3 ;
		else
			d = ( d1 * 2 + d2 ) / 3 ;
	}
#endif
	return d ;
}

//-----------------------------------------------------------------------------
// NtrxRange() does all necessary steps for a full ranging cycle
//   and return the result to the caller.
// DstAddr : -input- destination address of ranging partner

float NtrxRange( NtrxDevPtr DstAddr )
{
	if( NtrxRangeConst == 0.0 )	// Ranging is disabled.
	{
		ErrorHandler( 2 ) ;
		return -1 ;
	}

	if( NtrxRangingState != RANGING_READY )
		return -1 ;

	NtrxRangingState = RANGING_START ;

	NtrxRangeAddrBackup( DstAddr ) ;

	if( NtrxRangingMode( DstAddr ) == False )   //向目的卡发送信息_
		return -1 ;

	NtrxStartBasebandTimer( RANGING_TIMEOUT ) ; //定时器开启_
	
	do {
		NtrxInterruptService() ;  //开中断更新各种状态标识_

		// Check if nanoNET TRX chip has received valid data
		if( NtrxTransmitState == TxIDLE )
		{
			if( NtrxRxEndIrq )
				NtrxRxReceive() ;
		}

		// Check if is time out
		if( BaseBandTimerIrq )
		{
			NtrxRangingState = RANGING_READY ;
			return -1 ;
		}
	} while( NtrxRangingState != RANGING_SUCCESSFULL ) ;

	NtrxStopBasebandTimer() ;

	NtrxRangingState = RANGING_READY ;

	return GetDistance() ;
}

void GetRawData(NtrxRangeRawDataType *RawData)
{
	memcpy(RawData->set1, NtrxSet1, RANGE_DATALEN);
	memcpy(RawData->set2, NtrxSet2, RANGE_DATALEN);
	RawData->rssi = RangingRssi;
}
//-----------------------------------------------------------------------------
// NtrxRangeRaw() does all necessary steps for a full ranging cycle
//   and return the raw data to the caller.
// DstAddr : -input- destination address of ranging partner
// RawData : RawData buf address

Bool NtrxRangeRaw( NtrxDevPtr DstAddr , NtrxRangeRawDataType *RawData)
{
	if( NtrxRangeConst == 0.0 )	// Ranging is disabled.
	{
		ErrorHandler( 2 ) ;
		return False ;
	}

	if( NtrxRangingState != RANGING_READY )
		return False ;

	NtrxRangingState = RANGING_START ;

	NtrxRangeAddrBackup( DstAddr ) ;

	if( NtrxRangingMode( DstAddr ) == False )   //NtrxSendMessage() in  
		return False ;

	NtrxStartBasebandTimer( RANGING_TIMEOUT ) ;
	
	do {
		NtrxInterruptService() ;

		// Check if nanoNET TRX chip has received valid data
		if( NtrxTransmitState == TxIDLE )
		{
			if( NtrxRxEndIrq )
				NtrxRxReceive() ;
		}

		// Check if is time out
		if( BaseBandTimerIrq )
		{
			NtrxRangingState = RANGING_READY ;
			return False ;
		}
	} while( NtrxRangingState != RANGING_SUCCESSFULL ) ;

	NtrxStopBasebandTimer() ;

	NtrxRangingState = RANGING_READY ;

	GetRawData(RawData) ;

	return True ;
}


