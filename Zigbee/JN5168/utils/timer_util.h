#ifndef __TIMER_UTIL_H__
#define __TIMER_UTIL_H__

#include <jendefs.h>
#include <AppHardwareApi.h>

typedef enum
{
	E_TIMERUTIL_SET_SUCCESS,
	E_TIMERUTIL_SET_FAIL,
	E_TIMERUTIL_STOP_SUCCESS,
	E_TIMERUTIL_STOP_NOT_FOUND,
	E_TIMERUTIL_NOT_TIMER,
	E_TIMERUTIL_REACH_SUCCESS
}eTimerUtilStatus;

typedef enum
{
	E_TIMER_UNIT_MICROSECOND,
	E_TIMER_UNIT_MILLISECOND,
}eTimerUtilDelay;

PUBLIC void TimerUtil_vSetDebugPrint(bool_t b);
PUBLIC void TimerUtil_vUpdate();
PUBLIC eTimerUtilStatus TimerUtil_eSetTimer(uint32 u32EventID, uint32 u32Ms) ;
PUBLIC eTimerUtilStatus TimerUtil_eSetCircleTimer(uint32 u32EventID, uint32 u32Ms) ;
PUBLIC eTimerUtilStatus TimerUtil_eStopTimer(uint32 u32EventID);
PUBLIC void TimerUtil_vStopAllTimer();
PUBLIC void TimerUtil_vInit();
PUBLIC void TimerUtil_vDelay(uint16 u16Delay, eTimerUtilDelay eTimeUnit);
PUBLIC uint32 TimerUtil_GetSystemTimer(void);

#if (defined SUPPORT_HARD_WATCHDOG)
PUBLIC void vFeedHardwareWatchDog(void);
#endif

#endif

// end of Timer_util.h

