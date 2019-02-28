#ifndef LOCSYS_UTIL_H
#define LOCSYS_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_types.h"

extern uint32 macBackoffTimerCount(void);
#define MAX_BACKOFF_TIMER_COUNT ((uint32)(3 * 16) << 14)
#define Util_GetTime320us()  macBackoffTimerCount()
uint32 Util_Add320us(uint32 a, uint32 b);
uint32 Util_Sub320us(uint32 a, uint32 b);
#define UTIL_MS_TO_320US(x) ((x) * 25 / 8)
#define UTIL_320US_TO_MS(x) ((x) * 8 / 25)

#define Util_SequenceAfter8(a, b) ((int8)(b) - (int8)(a) < 0)
#define Util_SequenceAfter16(a, b) ((int16)(b) - (int16)(a) < 0)
#define Util_SequenceAfter32(a, b) ((int32)(b) - (int32)(a) < 0)
#define Util_TimeAfter(a, b) Util_SequenceAfter32(a, b)

#define Util_BitTest(val, mask)  (!!((val) & (mask)))
#define Util_BitSet(var, mask)   (var |= (mask))
#define Util_BitClear(var, mask) (var &= ~(mask)) 

void Util_DisableSleep(uint8 taskid, uint8 sleepid);
void Util_EnableSleep(uint8 taskid, uint8 sleepid);

#define Util_start_timerEx(taskid, event, timeout) do{if(SUCCESS != osal_start_timerEx(taskid, event, timeout)) SystemReset();}while(0)
#define Util_start_reload_timer(taskid, event, timeout) do{if(SUCCESS != osal_start_reload_timer(taskid, event, timeout)) SystemReset();}while(0)
#define Util_stop_timer(taskid, event) do{osal_stop_timerEx(taskid, event);}while(0);

#ifdef __cplusplus
};
#endif


#endif
