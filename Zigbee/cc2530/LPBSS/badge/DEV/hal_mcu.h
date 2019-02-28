/*******************************************************************************
  Filename:       hal_mcu.h
  Revised:        $Date: 18:21 2012年5月7日
  Revision:       $Revision: 1.0 $

  Description:    Describe the purpose and contents of the file.

*******************************************************************************/
#ifndef _HAL_MCU_H
#define _HAL_MCU_H

/*******************************************************************************
* Includes
*/
#include <defs.h>
#include <types.h>

/*******************************************************************************
* CONSTANTS AND DEFINES
*/
/* ---------------------- IAR Compiler ---------------------- */
#ifdef __IAR_SYSTEMS_ICC__
#include <ioCC2530.h>

#define HAL_COMPILER_IAR

#define HAL_MCU_LITTLE_ENDIAN()   __LITTLE_ENDIAN__

#else
#error "ERROR: Unknown compiler."
#endif

/* -----------------------------------------------------------------------------
 *                                        Interrupt Macros
 * -----------------------------------------------------------------------------
 */
#define HAL_ENABLE_INTERRUPTS()         st( EA = 1; )

#define HAL_DISABLE_INTERRUPTS()        st( EA = 0; )

#define HAL_INTERRUPTS_ARE_ENABLED()    (EA)

typedef unsigned char halIntState_t;

#define HAL_ENTER_CRITICAL_SECTION(x)   \
    st( x = EA;  HAL_DISABLE_INTERRUPTS(); )

#define HAL_EXIT_CRITICAL_SECTION(x)    \
    st( EA = x; )

#define HAL_CRITICAL_STATEMENT(x)       \
    st(                                 \
        halIntState_t _s;               \
                                        \
        HAL_ENTER_CRITICAL_SECTION(_s); \
            x;                          \
        HAL_EXIT_CRITICAL_SECTION(_s);  \
    )

#ifdef __IAR_SYSTEMS_ICC__
  /* IAR library uses XCH instruction with EA. It may cause the higher priority interrupt to be
   * locked out, therefore, may increase interrupt latency.  It may also create a lockup condition.
   * This workaround should only be used with 8051 using IAR compiler. When IAR fixes this by
   * removing XCH usage in its library, compile the following macros to null to disable them.
   */
  #define HAL_ENTER_ISR()   \
        { halIntState_t _isrIntState = EA; HAL_ENABLE_INTERRUPTS();

  #define HAL_EXIT_ISR()    \
        EA = _isrIntState; }

#else

  #define HAL_ENTER_ISR()

  #define HAL_EXIT_ISR()

#endif /* __IAR_SYSTEMS_ICC__ */

/* -----------------------------------------------------------------------------
 *                                        watchdog Macro
 * -----------------------------------------------------------------------------
 */
#define WD_EN               BV(3)

#define WD_MODE             BV(2)

#define WD_INT_1900_USEC    (BV(0) | BV(1))

#define WD_RESET1           (0xA0 | WD_EN | WD_INT_1900_USEC)

#define WD_RESET2           (0x50 | WD_EN | WD_INT_1900_USEC)

#define DOGTIMER_INTERVAL_1S            0
#define DOGTIMER_INTERVAL_250MS         1
#define DOGTIMER_INTERVAL_15MS          2
#define DOGTIMER_INTERVAL_2MS           3

#define SLEEP_RST_POS                    3

#define WDCTL_INT_POS                    0
#define WDCTL_MODE_POS                    2
#define WDCTL_EN_POS                    3
#define WDCTL_CLR_POS                    4

/* disable interrupts, set watchdog timer, wait for reset */
#define HAL_SYSTEM_RESET()              \
    st(                                 \
        HAL_DISABLE_INTERRUPTS();       \
        WDCTL = WD_RESET1;              \
        WDCTL = WD_RESET2;              \
        for(;;);                        \
    )

/* -----------------------------------------------------------------------------
 *                                       CC2590/CC2591 support
 *
 *                        Define HAL_PA_LNA_CC2590 if CC2530+CC2590EM is used
 *                        Define HAL_PA_LNA if CC2530+CC2591EM is used
 *                        Note that only one of them can be defined
 * -----------------------------------------------------------------------------
 */
#define HAL_PA_LNA
/* -----------------------------------------------------------------------------
 *                                          Clock Speed
 * -----------------------------------------------------------------------------
 */
/* This flag should be defined if the SoC uses the 32MHz crystal
 * as the main clock source (instead of DCO).
 */
#define HAL_CLOCK_CRYSTAL

#define HAL_CPU_CLOCK_MHZ     32

/* 32 kHz clock source select in CLKCONCMD */
#ifdef HAL_CLOCK_CRYSTAL
    #define OSC_32KHZ  0x00 /* external 32 KHz xosc */
#else
    #define OSC_32KHZ  0x80 /* internal 32 KHz rcosc */
#endif

/* CLKCONCMD bit definitions */
#define OSC              BV(6)
#define TICKSPD(x)       (x << 3)
#define CLKSPD(x)        (x << 0)
#define CLKCONCMD_32MHZ  (0)
#define CLKCONCMD_16MHZ  (CLKSPD(1) | TICKSPD(1) | OSC)

#define HAL_CLOCK_STABLE()    \
    st( while (CLKCONSTA != (CLKCONCMD_32MHZ | OSC_32KHZ)); )

/* -----------------------------------------------------------------------------
 *                         OSAL NV implemented by internal flash pages.
 * -----------------------------------------------------------------------------
 */
#if defined NON_BANKED
#define HAL_FLASH_LOCK_BITS        16
#define HAL_NV_PAGE_END            30
#define HAL_NV_PAGE_CNT            2
#else
#define HAL_FLASH_LOCK_BITS        16
#define HAL_NV_PAGE_END            126  //最后两页存放IEEE MAC信息，pg从1开始
#define HAL_NV_PAGE_CNT            8
#endif

// Flash is partitioned into 8 banks of 32 KB or 16 pages.
#define HAL_FLASH_PAGE_PER_BANK    16
// Flash is constructed of 128 pages of 2 KB.
#define HAL_FLASH_PAGE_SIZE        2048
#define HAL_FLASH_WORD_SIZE        4

// CODE banks get mapped into the XDATA range 8000-FFFF.
#define HAL_FLASH_PAGE_MAP         0x8000

#define HAL_FLASH_INFOMATION_SIZE   (HAL_NV_PAGE_CNT * HAL_FLASH_PAGE_SIZE) //1024
#define HAL_FLASH_INFOMATION_PAGE   (HAL_NV_PAGE_END - HAL_NV_PAGE_CNT + 1)
#define HAL_FLASH_INFOMATION_OSET   (0)

// Re-defining Z_EXTADDR_LEN here so as not to include a Z-Stack .h file.
#define HAL_FLASH_IEEE_SIZE        8
#define HAL_FLASH_IEEE_PAGE       (HAL_NV_PAGE_END+1)
#define HAL_FLASH_IEEE_OSET       (HAL_FLASH_PAGE_SIZE - HAL_FLASH_LOCK_BITS - HAL_FLASH_IEEE_SIZE)

#define ACTIVE_LOW        !
#define ACTIVE_HIGH       !!    /* double negation forces result to be '1' */

/* 1 - Red */
#define LED1_BV           BV(2)
#define LED1_SBIT         P1_2
#define LED1_DIR          P1DIR
#define LED1_POLARITY     ACTIVE_LOW
/* 2 - green */
#define LED2_BV           BV(3)
#define LED2_SBIT         P1_3
#define LED2_DIR          P1DIR
#define LED2_POLARITY     ACTIVE_LOW

#define HAL_TURN_OFF_LED1()       st( LED1_SBIT = LED1_POLARITY(0); )
#define HAL_TURN_OFF_LED2()       st( LED2_SBIT = LED2_POLARITY(0); )
#define HAL_TURN_ON_LED1()        st( LED1_SBIT = LED1_POLARITY(1); )
#define HAL_TURN_ON_LED2()        st( LED2_SBIT = LED2_POLARITY(1); )
#define HAL_TOGGLE_LED1()         st( LED1_SBIT = !LED1_SBIT;)
#define HAL_TOGGLE_LED2()         st( LED2_SBIT = !LED2_SBIT;)
#define HAL_STATE_LED1()          (LED1_POLARITY(LED1_SBIT))
#define HAL_STATE_LED2()          (LED2_POLARITY(LED2_SBIT))

#define HAL_LED_BLINK_DELAY()   st(volatile UINT32 i = 0; while (i++ < 0x5800);)

#define HAL_ASSERT(x)               \
    do{                             \
        if (!(x))                   \
        {                           \
            HAL_TOGGLE_LED1();      \
            HAL_TOGGLE_LED2();      \
            HAL_LED_BLINK_DELAY();  \
            HAL_WATCHDOG_Feed();    \
        }                           \
    }while(!(x))

/* -----------------------------------------------------------------------------
 *                                        CC2530 sleep common code
 * -----------------------------------------------------------------------------
 */
/*******************************************************************************
* GLOBAL FUNCTIONS DECLARATION
*/
UINT16 HAL_MCU_Random(VOID);

VOID HAL_MCU_XOSC_Init(VOID);

VOID HAL_WATCHDOG_Feed(VOID);

VOID HAL_WATCHDOG_Start(UINT8 u8IntTime);

VOID HAL_WaitUs(UINT32 u32Usec);

#endif
