/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

/*
 * NtrxGetRTC:
 *
 * NtrxGetRTC() return the RTC time of NanoLoc chip, unit is 30.517578125us
 *
 */

void NtrxGetRTC( NtrxBufferPtr RtcBufferPtr )
{
	NtrxSetField( NA_RtcCmdRd, True ) ;
	Delay_us( 130 ) ;
	NtrxReadSPI( NA_RamRtcReg_O, RtcBufferPtr, 6 ) ;
	NtrxSetField( NA_RtcCmdRd, False ) ;
}

void NtrxGetRTC32(NtrxBufferPtr RtcBufferPtr)
{
    NtrxSetField( NA_RxCmdStop, True ) ;    // Stop receiver

    NtrxSetField( NA_RtcCmdRd, True ) ;
    Delay_us( 130 ) ;
    NtrxReadSPI( NA_RamRtcReg_O, RtcBufferPtr, 4 ) ;
    NtrxSetField( NA_RtcCmdRd, False ) ;

    NtrxSetField(NA_RxCmdStart, True) ;
}

