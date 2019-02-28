#include <c8051f350.h>
#include <stdio.h>
#include "c51f350.h"

#define PCA0_INT_TOTAL 17
#define PCA0L_VALUE 0//(0xFF - 0x91U-30)  // 60049 * 17 = 1020833 ~= SYSTEMCLOCK / 12
#define PCA0H_VALUE (0xFF - 0xECU)//(0xFF - 0xEAU)

//#define PCA0L_VALUE (0xFF - 0x91U)  // 60049 * 17 = 1020833 ~= SYSTEMCLOCK / 12
//#define PCA0H_VALUE (0xFF - 0xEAU)

static volatile UINT8 sg_PCA0_u8Signal;
static volatile UINT8 sg_u8CountCFInt;
static volatile UINT32 sg_u32NewFreqCount;
static volatile UINT32 sg_u32OldFreqCount;

static VOID PCA0_WritePCAHL(UINT8 u8H, UINT8 u8L);

VOID PCA0_Init(VOID)
{
    sg_PCA0_u8Signal = 0;
    sg_u8CountCFInt = 0;
    sg_u32OldFreqCount = 0;
    sg_u32NewFreqCount = 0;

    XBR1 |= 0x41;
    P0SKIP |= 0x40;      // skip P0_6,use p0^7
    PCA0CN = 0x40;      // enable PCA counter,clear mode interrupt flag

    PCA0_WritePCAHL(PCA0H_VALUE, PCA0L_VALUE);

    PCA0CPM0 = 0x21;    // capture positive-edge,enable capture interrupt
    EIE1 |= 0x10;       // enable PCA interrupt
}

VOID PCA0_DeInit(VOID)
{
    EIE1 &= ~0x10;       // disable PCA interrupt
}

static VOID PCA0_WritePCAHL(UINT8 u8H, UINT8 u8L)
{
    UINT8 bitWdte;

    WATCHDOG_SAVE(bitWdte);
    WATCHDOG_OFF();
    PCA0MD |= 0x01;     // enable PCA counter overflow interrupt
    PCA0MD &= ~0x0E;  // clock source = sysclk / 12
    PCA0L = u8L;
    PCA0H = u8H;
    WATCHDOG_SET(bitWdte);
}

UINT32 PCA0_GetCounter(VOID)
{
    while (!sg_PCA0_u8Signal)
        WATCHDOG_FEED();

    sg_PCA0_u8Signal = 0;
    return sg_u32OldFreqCount;
}

#ifdef DEBUG
sbit p06 = P0^6;
BOOL state = true;
#endif
VOID PCA0_Interrupt (VOID) interrupt 11
{
    if (CCF0)
    {
        sg_u32NewFreqCount++;
        CCF0 = 0;
    }
    if (CF)
    {
        if (++sg_u8CountCFInt == PCA0_INT_TOTAL)
        {
#ifdef DEBUG        
            p06 = state;
            state = !state;
#endif
            sg_u32OldFreqCount = sg_u32NewFreqCount;
            sg_u32NewFreqCount = sg_u8CountCFInt = 0;
            sg_PCA0_u8Signal = 1;
        }
        PCA0_WritePCAHL(PCA0H_VALUE, PCA0L_VALUE);
        CF = 0;
    }
}

