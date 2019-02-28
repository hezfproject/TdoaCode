/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

void NtrxSetRxMode( uint8 mode )
{
	NtrxWriteReg( NA_RxDataEn_O, ( NtrxGetShadowReg( NA_RxDataEn_O ) & 0xF0 ) | ( mode & 0x0F ) ) ;

	if( mode & ModeRxArqMode )
		NtrxSetField( NA_RxArqMode, NA_RxArqModeCrc2_VC_C ) ;
	else
		NtrxSetField( NA_RxArqMode, NA_RxArqModeNone_VC_C ) ;

	if( mode & ModeRxTimeBAuto )
		NtrxSetField( NA_RtcTimeBAutoMode, True ) ;
	else
		NtrxSetField( NA_RtcTimeBAutoMode, False ) ;
}
