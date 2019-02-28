/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

/**
 * NtrxGetOutputPower:
 *
 * NtrxGetOutputPower() reads output power either for the frame types Data,
 * Broadcast and TimeB, or for the frame types Ack, Request-to-send and
 * Clear-to-send.
 *
 * Returns:  the value of the output power (0 ... 0x3F)
 */

uint8 NtrxGetOutputPower( void )
{
	return NtrxGetField( NA_TxOutputPower0 ) ;
}
