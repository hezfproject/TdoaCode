#include <c8051f350.h>
#include <stdio.h>
#include <stdarg.h>
#include "c51f350.h"

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
        VOID    UART0_Init(VOID);
static  VOID    UART0_IO_Init(VOID);
static  VOID    UART0_Mode_Init(VOID);
static  VOID    UART0_Baudrate_Init(VOID);
//-----------------------------------------------------------------------------
// variables declaration
//-----------------------------------------------------------------------------
static  BOOL     s_bInit = false;
static  BOOL     s_bRecv_Ok = false;
static  BOOL     s_bSend_Ok = false;
//-----------------------------------------------------------------------------
// Macros Defined
//-----------------------------------------------------------------------------
#define WAIT_SEND_OK()  \
do{                     \
    WATCHDOG_FEED();    \
}while(!s_bSend_Ok)

#define SEND_ENABLE()   (P0 |= (1 << 6))    // P0^6 is high that is send,low is recv

#define SEND_DISABLE() WAIT_SEND_OK();P0 &= ~(1 << 6);

#define SEND_BYTE(byte) (SBUF0 = byte)
#define RECV_BYTE()     (SBUF0)

#define UART0_CR_LF_SEND(ch)    \
do{                             \
    if ('\n' == ch)             \
    {                           \
        WAIT_SEND_OK();         \
        s_bSend_Ok = false;     \
        SEND_BYTE('\r');        \
    }                           \
}while(0)

//-----------------------------------------------------------------------------
// UART0_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Configure the UART0 using Timer1, for <BAUDRATE> and 8-N-1.
//-----------------------------------------------------------------------------
VOID UART0_Init(VOID)
{
    WATCHDOG_FEED();

    UART0_IO_Init();                    // Initialize Port I/O
    UART0_Baudrate_Init();              // Initialize baudrate
    UART0_Mode_Init();                  // Initialize running mode

    s_bSend_Ok = true;
    s_bInit = true;
}

//-----------------------------------------------------------------------------
// UART0_IO_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Configure the Crossbar and GPIO ports.
//
// P0.4   digital   push-pull    UART TX
// P0.5   digital   open-drain   UART RX
// P0.6   digital   push-pull
//-----------------------------------------------------------------------------
static VOID UART0_IO_Init(VOID)
{
    P0MDOUT |= 0x50;                    // Enable UTX as push-pull output
    P0MDIN  |= 0x20;
    XBR0    |= 0x01;                    // Enable UART on P0.4(TX) and P0.5(RX)
    XBR1    |= 0x40;                    // Enable crossbar and weak pull-ups
}

//-----------------------------------------------------------------------------
// UART0_Mode_Init
//-----------------------------------------------------------------------------
//
// configure the running mode register (timer1 uart0's interrupts)
//
//-----------------------------------------------------------------------------
static VOID UART0_Mode_Init(VOID)
{
    TL1 = TH1;                          // init Timer1
    TMOD &= ~0xf0;                      // TMOD: timer 1 in 8-bit autoreload
    TMOD |=  0x20;
    TR1 = 1;                            // START Timer1 (register TCON)
    IP |= 0x10;                         // Make UART high priority
    ES0 = 1;                            // Enable UART0 interrupts (register IE）
}

//-----------------------------------------------------------------------------
// UART0_Baudrate_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Configure the baudrate.
//
//-----------------------------------------------------------------------------
static VOID UART0_Baudrate_Init(VOID)
{
    INT32 s32Clock = (SYSTEMCLOCK / BAUDRATE) >> 1;
    INT32 s32TimerHigh = s32Clock >> 8;     //SYSTEMCLOCK / BAUDRATE / 2 / 256

    WATCHDOG_FEED();

    SCON0 = 0x10;                           // SCON0: 8-bit variable bit rate

    if (s32TimerHigh < 1)
    {
        TH1 = -s32Clock;
        CKCON |=  0x08;                     // T1M = 1; SCA1:0 = xx
    }
    else if (s32TimerHigh < 4)
    {
        TH1 = -(s32Clock >> 2);
        CKCON &= ~0x0B;                     // T1M = 0; SCA1:0 = 01
        CKCON |=  0x01;
    }
    else if (s32TimerHigh < 12)
    {
        TH1 = -(s32Clock / 12);
        CKCON &= ~0x0B;                     // T1M = 0; SCA1:0 = 00
    }
    else
    {
        TH1 = -(s32Clock / 48);
        CKCON &= ~0x0B;                     // T1M = 0; SCA1:0 = 10
        CKCON |=  0x02;
    }
}

//-----------------------------------------------------------------------------
// 重载putchar的功能使printf遵循RS485协议
//
//-----------------------------------------------------------------------------
INT8 putchar(UINT8 ch)
{
    WATCHDOG_FEED();
    UART0_CR_LF_SEND(ch);
    WAIT_SEND_OK();
    s_bSend_Ok = false;
    return SEND_BYTE(ch);
}

//-----------------------------------------------------------------------------
// 重载printf的功能
//
//-----------------------------------------------------------------------------
INT8 print(INT8 *fmt, ...)
{
    INT8 s8Ret;
    va_list args;

    WATCHDOG_FEED();

    if (!s_bInit)
    {
        UART0_Init();
    }

    SEND_ENABLE();
    va_start(args, fmt);
    s8Ret = vprintf(fmt, args);
    va_end(args);
    SEND_DISABLE();

    return s8Ret;
}

/****************************************************************************/
/*       _getkey:  interrupt controlled _getkey                             */
/****************************************************************************/
INT8 _getkey(void)
{
    if (!s_bInit)
    {
        UART0_Init();
    }
    while (!s_bRecv_Ok)
    {
        WATCHDOG_FEED();
    }
    s_bRecv_Ok = false;

    return RECV_BYTE();
}

//-----------------------------------------------------------------------------
// UART0_Interrupt
//-----------------------------------------------------------------------------
//
// This routine is invoked whenever a character is entered or displayed on the
// Hyperterminal.
//
//-----------------------------------------------------------------------------
VOID UART0_Interrupt (VOID) interrupt 4
{
    WATCHDOG_FEED();

    if (RI0 == 1)
    {
        s_bRecv_Ok = true;
        RI0 = 0;                           // Clear interrupt flag
    }
    if (TI0 == 1)                          // Check if transmit flag is set
    {
        s_bSend_Ok = true;
        TI0 = 0;                           // Clear interrupt flag
    }
}
//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
