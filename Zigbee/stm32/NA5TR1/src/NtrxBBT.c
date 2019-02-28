/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

//=============================================================================
// Starts a countdown from startvalue to zero ( in us )
// BaseBandTimerIrq will be set to 1 when the timer reaches zero

void NtrxStartBasebandTimer( uint16 StartValue )
{
	NtrxBufferType Buffer[2] ;
	Buffer[0] = (uint8)(StartValue&0xFF) ;
	Buffer[1] = (uint8)(StartValue>>8) ;
	NtrxSetLongField( NA_BasebandTimerStartValue, Buffer ) ;
	BaseBandTimerIrq = False ;
}

