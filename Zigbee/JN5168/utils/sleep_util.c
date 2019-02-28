#include <jendefs.h>
#include <AppHardwareApi.h>
#include "event_util.h"
#include "timer_util.h"

#include "sleep_util.h"
#include "printf_util.h"

//NOTE: before go to sleep, all timers and events are cleared.
PUBLIC void SleepUtil_Sleep(uint8 u8Timer, uint32 u32SleepMS, uint8 u8Mode)
{
	/*
	uint32 a, b, c;
	a = u32AHI_TickTimerRead();
	b = u32AHI_WakeTimerCalibrate();
	c = u32AHI_TickTimerRead();
	
	PrintfUtil_vInit();
	PrintfUtil_vPrintf("cal: %d, a: %d, c; %d\n", b, a, c);
	*/
	TimerUtil_vStopAllTimer(); 
	EventUtil_vResetAllEvents();
	float fErrorCal = 10000.0 / u32AHI_WakeTimerCalibrate();
	vAHI_WakeTimerStartLarge(u8Timer, (uint64)(fErrorCal * (u32SleepMS<<5))); //32000 per second;
	vAHI_Sleep(u8Mode);
}


