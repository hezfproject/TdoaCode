/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _HAL_H_
#define _HAL_H_

#include "CC_DEF.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Global variables define
extern uint8 Hal_TaskId;

#define HAL_EVENT_KEY					0x0001

//-----------------------------------------------------------------------------
// Function define


//-----------------------------------------------------------------------------

extern void Hal_Init(uint8 taskid);
extern uint16 Hal_ProcessEvent(uint8 taskid, uint16 events);
extern void Hal_Poll(void);
extern void Hal_Sleep(uint32 ms);

#ifdef __cplusplus
}
#endif

#endif /* _HAL_H_ */

//-----------------------------------------------------------------------------

