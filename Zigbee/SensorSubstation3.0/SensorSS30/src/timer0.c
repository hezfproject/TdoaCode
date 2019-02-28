#include <C8051F350.h>                 // SFR declarations
#include "c51f350.h"

/**************************************************************************
** (SYSTEMCLOCK - TIMER0_1S_LIMIT) / TIMER0_PRESCALER / TIMER0_1S_COUNTER
**
*/
#define TIMER0_1S_PRESCALER 0x02
#define TIMER0_PRESCALER    48
#define TIMER0_1S_TOTAL     2//4  2 = 0.5s  4 = 1s
#define TIMER0_1S_COUNTER   63802
#define TIMER0_1S_LIMIT     32
//-----------------------------------------------------------------------------
// variable defined
//-----------------------------------------------------------------------------
static UINT16 s_u16SecCnt;
static UINT16 s_u16SecCntCopy;
static UINT8 s_u8IntCnt;
static BOOL s_bSystemUpDate;
//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
static VOID Timer0_Start(VOID);

VOID Timer0_ISR (VOID) interrupt 1
{
    WATCHDOG_FEED();
    TCON &= ~(0x10);                                  // Timer0 stop

    if (TIMER0_1S_TOTAL == ++s_u8IntCnt)
    {
        s_u8IntCnt = 0;

        if (0 == --s_u16SecCnt)
        {
            s_u16SecCnt = s_u16SecCntCopy;
            s_bSystemUpDate = true;
        }
    }

    TH0 = (65536 - TIMER0_1S_COUNTER) >> 8;           // Init Timer0 High register
    TL0 = (65536 - TIMER0_1S_COUNTER) & 0xFF;         // Init Timer0 Low register
    TCON |= 0x10;                                     // Timer0 ON
}

BOOL SysUpdateState(VOID)
{
    BOOL bRet = s_bSystemUpDate;
    
    if (s_bSystemUpDate)
        s_bSystemUpDate = false;
        
    return bRet;
}

/****************************************************************************
* u16Sec    : 0 is default 1s
* u8LoopCnt : 0 is all time loop
*/
VOID Timer0_Init(UINT16 u16Sec)
{
    WATCHDOG_FEED();

    s_u8IntCnt = 0;
    
    if (!u16Sec)
        s_u16SecCnt = 1;
    else
        s_u16SecCnt = u16Sec;

    s_u16SecCntCopy = s_u16SecCnt;
    
    TMOD &= ~(3);
    TMOD |= 0x01;                        // Timer0 in 16-bit mode
    CKCON &= ~(1 << 2);
    CKCON |= TIMER0_1S_PRESCALER;        // Timer0 uses a 1:48 prescaler
    
    Timer0_Start();
}

static VOID Timer0_Start(VOID)
{
    TH0 = TIMER0_1S_COUNTER >> 8;           // Init Timer0 High register
    TL0 = TIMER0_1S_COUNTER & 0xFF;           // Init Timer0 Low register
    ET0 = 1;                            // Timer0 interrupt enabled
    TCON |= 0x10;                        // Timer0 ON
}
