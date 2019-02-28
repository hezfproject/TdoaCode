#ifndef _SLEEP_H_
#define _SLEEP_H_

//#include "CC_DEF.h"
#include "type_def.h"


#define RTC_USE_LSE

/*
    The devices have the following two secondary clock sources:
¡ñ 37 kHz low speed internal RC (LSI RC) which drives the independent watchdog
    and optionally the RTC used for Auto-wakeup from Stop/Standby mode.
¡ñ 32.768 kHz low speed external crystal (LSE crystal) which optionally drives
    the real-time clock (RTCCLK)
*/
// RTC_WakeUpClock_RTCCLK_Div16 equals /16
#ifdef RTC_USE_LSE
#define WAKEUP_CLK_SRC  (32768u)
#else
#define WAKEUP_CLK_SRC  (37000u)
#endif

#define MAX_WATCHDOG_MS     (28339u) // ms(0x1000u * 256u / 37000u * 1000u)
#define SLEEP_RESOLUTION    (8)  // 7.8125ms
#define NANO_INIT_TIME      (10) // about 9.8ms
#define STM32_CLOCK_SWITCH_TIME (4) // about 3.5ms + nano sleep config time 152us, misc code 252us

#define SLEEP_TIME_MIN      (NANO_INIT_TIME + STM32_CLOCK_SWITCH_TIME)// + SLEEP_RESOLUTION
#define SLEEP_TIME_MAX      (0x10000u * 16 / WAKEUP_CLK_SRC * 1000u) // 32000ms

#if MAX_WATCHDOG_MS < SLEEP_TIME_MAX
#define MAX_SLEEP_EACH      (26000)     // 2sec margin
#else
#define MAX_SLEEP_EACH      SLEEP_TIME_MAX
#endif

#define CONVER_MS_TO_SLEEPTICK(ms)      (((ms * WAKEUP_CLK_SRC) >> 4) / 1000)
#define CONVER_SLEEPTICK_TO_MS(tick)    (((tick * 1000) << 4) / WAKEUP_CLK_SRC)

void SleepInit(void);

//×îÉÙÐÝÃß28ms
void system_sleep(uint32 ms);

//void Sleep(uint32 ms);

void system_powersave_sleep(uint32 ms);

void system_powersave_wakeup(void);

#endif

