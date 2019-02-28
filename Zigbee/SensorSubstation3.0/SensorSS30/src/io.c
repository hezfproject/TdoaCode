#include <c8051f350.h>
#include "c51f350.h"


#define FREQ_MODE       0x0E    //1110
#define CURRENT_MODE    0x0D    //1101
#define STATE_MODE      0x0B    //1011
#define UART485_MODE    0x07    //0111

VOID IO_Init(VOID)
{
    P1MDIN |= 0x1F;             // set GPIO p1.0-p1.4 input
    P1MDOUT &= ~0x20;            // set GPIO p1.5 output,open Drain
    XBR1 |= 0x40;               // Enable crossbar and enable weak pull-ups
    P1 |= 0x20;
}

VOID IO_Shutting(BOOL bSht)
{
    if (bSht)
        P1 &= ~0x20;
    else
        P1 |= 0x20;
}

UINT8 IO_GetSwitchState(VOID)
{
    UINT8 u8Mode = IO_INVALID_MODE;
    
    WATCHDOG_FEED();

    switch (0x0F & P1)          // get GPIO p1.0-p1.3 state
    {
        case FREQ_MODE:
            u8Mode = IO_FREQ_MODE;
            break;
        case CURRENT_MODE:
            u8Mode = IO_CURRENT_MODE;
            break;
        case STATE_MODE:
            u8Mode = IO_STATE_MODE;
            break;
        case UART485_MODE:
            u8Mode = IO_UART485_MODE;
            break;
    }
    
    return u8Mode;
}

BOOL IO_GetStateMode(VOID)
{
    return ((P1 & 0x10) != 0);
}

