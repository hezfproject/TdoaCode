/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxConfig.h"
#include "NtrxChip.h"

void NtrxSetTransparentMode( uint8 on )
{
	NtrxBufferType Len[2] ;

	if( on != 0 )
	{
		Len[0] = 0x80 ;
		Len[1] = 0 ;
		NtrxSetLongField( NA_RxTransBytes, Len ) ;

		NtrxSetField( NA_TxRxMode, True );
		NtrxSetField( NA_TxRxBbBufferMode0, True );
		NtrxSetField( NA_TxScrambEn, False );
		NtrxSetField( NA_TxScrambInit, 0 );
		NtrxSetField( NA_TxArq, False );
		NtrxSetField( NA_TxArqMax, 0 );
		NtrxSetField( NA_RxArqMode, NA_RxArqModeNone_VC_C );
		NtrxSetField( NA_RxCrc2Mode, False );
	}
	else
	{
		NtrxSetField( NA_TxRxMode, False ) ;
		NtrxSetField( NA_TxScrambEn, True ) ;
		NtrxSetField( NA_TxScrambInit, 0x7F ) ;
		NtrxSetField( NA_TxArq, True ) ;
		NtrxSetField( NA_TxArqMax, NA_MAX_ARQ_CNT ) ;
		NtrxSetField( NA_RxArqMode, NA_RxArqModeCrc2_VC_C ) ;
		NtrxSetField( NA_RxCrc2Mode, True ) ;
	}
}
