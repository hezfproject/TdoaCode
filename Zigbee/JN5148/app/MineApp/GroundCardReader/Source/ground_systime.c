#include <jendefs.h>
#include <AppHardwareApi.h>
#include "ground_track.h"

static uint32 u32SysMs = 0;
static uint32 u32SysTick = 0;

PUBLIC void init_system_ms(void)
{
    u32SysTick = u32AHI_TickTimerRead();
}

PUBLIC uint32 get_system_ms(void)
{
    uint32 u32CurrTick;
    uint32 ms;

    u32CurrTick = u32AHI_TickTimerRead();
    ms = u32CurrTick - u32SysTick;
    ms = ms / 16000;

    if (ms > 0)
    {
        u32SysMs += ms;
        u32SysTick = u32CurrTick;
    }

    return u32SysMs;
}

