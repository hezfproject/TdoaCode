/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

/**
 * NtrxSetOutputPower:
 *
 * NtrxSetOutputPower() set output power either for the frame types Data,
 * Broadcast and TimeB, or for the frame types Ack, Request-to-send and
 * Clear-to-send.
 *
 */

void NtrxSetOutputPower( uint8 Value )
{
	if( Value > 0x3F )
		Value = 0x3F ;
	NtrxSetField( NA_TxOutputPower0, Value ) ;
	NtrxSetField( NA_TxOutputPower1, Value ) ;
}
