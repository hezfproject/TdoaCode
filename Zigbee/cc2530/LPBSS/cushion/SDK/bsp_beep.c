#include "hal_beeper.h"

VOID BSP_BEEP_Init(VOID)
{
    HalBeepInit();
#ifdef BOOT_DODO
    HalBeepBegin();
    HAL_WaitUs((UINT32)100 * 1000);
    HalBeepStop();
#endif
}

VOID BSP_BEEP_On(VOID)
{
    HalBeepBegin();
}

VOID BSP_BEEP_Off(VOID)
{
    HalBeepStop();
}

