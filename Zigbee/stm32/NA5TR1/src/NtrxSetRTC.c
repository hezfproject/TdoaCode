/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

// NtrxSetRTC() set the RTC time of NanoLoc chip, unit is 30.517578125us

void NtrxSetRTC( NtrxBufferPtr RtcBufferPtr )
{
	NtrxWriteSPI( NA_RamRtcReg_O, RtcBufferPtr, 6 ) ;
	NtrxSetField( NA_RtcCmdWr, True ) ;
	NtrxSetField( NA_RtcCmdWr, False ) ;
}
