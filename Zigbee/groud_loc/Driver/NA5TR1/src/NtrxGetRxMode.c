/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

uint8 NtrxGetRxMode( void )
{
	uint8 mode ;

	mode = NtrxGetShadowReg( NA_RxDataEn_O ) & 0x0F ;

	if( NtrxGetField( NA_RxArqMode ) != 0 )
		mode |= ModeRxArqMode ;

	if( NtrxGetField( NA_RtcTimeBAutoMode ) != 0 )
		mode |= ModeRxTimeBAuto ;

	return mode ;
}
