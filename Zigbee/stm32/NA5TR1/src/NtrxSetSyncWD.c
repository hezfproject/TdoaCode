/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

// NtrxSetSyncWord() set the sync word of NanoLoc chip
// Default Sync Word : 0xAB69CA9492D52CAB
//     SyncWordPtr[0]=0xAB, SyncWordPtr[1]=0x2C, ......

void NtrxSetSyncWord( NtrxBufferPtr SyncWordPtr )
{
	NtrxSetLongField( NA_SyncWord, SyncWordPtr ) ;
}
