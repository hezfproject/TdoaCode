/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

/**
 * NtrxSetChannel:
 *
 * @value: -input- channel index
 *
 * NtrxSetChannel() sets the channel for transmission and reception.
 *
 */

void NtrxSetChannel( uint8 ch )
{
	if( ch < 6 )
		NtrxCurrentChannel = ch ;
	NtrxAllCalibration() ;
}

