/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/
#include "NtrxConfig.h"
#include "NtrxChip.h"

extern void NtrxInitShadowRegister( void ) ;

void NtrxWarmInit( void )
{
	NtrxInitShadowRegister() ;
	NtrxWriteReg( 0x00, 0x42 ) ;
	if( NtrxSetupTRxMode( CONFIG_NTRX_DEFAULT_MODE ) == 0 )
		ErrorHandler( 4 ) ;
}
