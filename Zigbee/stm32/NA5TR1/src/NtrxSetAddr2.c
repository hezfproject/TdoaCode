/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

/**
 * NtrxSetStaAddress2() sets the second MAC address in the chip.
 */

void NtrxSetStaAddress2( NtrxDevPtr Address )
{
	NtrxSetLongField( NA_RamStaAddr1, Address ) ;
}
