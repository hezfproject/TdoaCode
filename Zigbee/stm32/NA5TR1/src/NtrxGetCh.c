/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

/**
 * NtrxGetChannel:
 *
 * NtrxGetChannel() returns the channel index selected for transmission and reception.
 *
 * Returns: channel index
 *
 */

uint8 NtrxGetChannel( void )
{
	return NtrxCurrentChannel ;
}

