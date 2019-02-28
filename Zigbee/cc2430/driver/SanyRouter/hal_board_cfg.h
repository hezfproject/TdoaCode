/**************************************************************************************************
  Filename:       hal_board_cfg.h
  Revised:        $Date: 2011/07/01 00:29:33 $
  Revision:       $Revision: 1.1.2.1 $

  Description:    Describe the purpose and contents of the file.


  Copyright 2006-2008 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

#ifndef HAL_BOARD_CFG_H
#define HAL_BOARD_CFG_H

/*
 *     =============================================================
 *     |             Chipcon CC2430DB Development Board            |
 *     | --------------------------------------------------------- |
 *     |  mcu   : 8051 core                                        |
 *     |  clock : 32MHz                                            |
 *     =============================================================
 */


/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"


/* ------------------------------------------------------------------------------------------------
 *                                       Board Indentifier
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_BOARD_SANYCOLLECTOR


/* ------------------------------------------------------------------------------------------------
 *                                          Clock Speed
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_CPU_CLOCK_MHZ     32

/* 32 kHz clock source select in CLKCON */
#if !defined (OSC32K_CRYSTAL_INSTALLED) || (defined (OSC32K_CRYSTAL_INSTALLED) && (OSC32K_CRYSTAL_INSTALLED == TRUE))
#define OSC_32KHZ  0x00 /* external 32 KHz xosc */
#else
#define OSC_32KHZ  0x80 /* internal 32 KHz rcosc */
#endif


/* ------------------------------------------------------------------------------------------------
 *                                       LED Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_NUM_LEDS            3
#define HAL_LED_BLINK_DELAY()   st( { volatile uint32 i; for (i=0; i<0x5800; i++) { }; } )

/* D1 - blue */
#define LED1_BV           BV(0)
#define LED1_SBIT         P1_0
#define LED1_DDR          P1DIR
#define LED1_POLARITY     ACTIVE_LOW

/* D2 - blue */
#define LED2_BV           BV(1)
#define LED2_SBIT         P1_1
#define LED2_DDR          P1DIR
#define LED2_POLARITY     ACTIVE_LOW


/* D3 - RED */
#define LED3_BV           BV(2)
#define LED3_SBIT         P1_2
#define LED3_DDR          P1DIR
#define LED3_POLARITY     ACTIVE_LOW


/* ------------------------------------------------------------------------------------------------
 *                                    Push Button Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define ACTIVE_LOW        !
#define ACTIVE_HIGH       !!    /* double negation forces result to be '1' */

/* S1 */
#define PUSH1_BV          BV(1)
#define PUSH1_SBIT        P0_1
#define PUSH1_POLARITY    ACTIVE_LOW

/* Joystick Center Press */
#define PUSH2_BV          BV(0)
#define PUSH2_SBIT        P2_0
#define PUSH2_POLARITY    ACTIVE_HIGH


/* ------------------------------------------------------------------------------------------------
 *                                    OAD Shared Memory Configuration
 * ------------------------------------------------------------------------------------------------
 */

#define NO_DL_IMAGE_ADDRESS  0xFFFFFFFFL

// this is the mailbox value that tells the boot code to flash the downloaded image
// even though the operational image may be sane.
#define MBOX_OAD_ENABLE      0x454E424C           // 'ENBL' enable downloaded code

#define PREAMBLE_MAGIC1        0xA5
#define PREAMBLE_MAGIC2        0x5A

// Space available for storing a downloaded image - limited by XNV size.
#define DL_SPACE_AVAIL         0x20000

// Image size is 2430 flash size less reserved space for NV and boot code.
#define DL_IMAGE_SIZE          0x1E000

#define HAL_OAD_XNV_IS_INTERNAL  TRUE
#define HAL_OAD_XNV_IS_SPI       FALSE


/* ------------------------------------------------------------------------------------------------
 *                          Flash Page size used by OSAL NV and OAD Boot Code
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_NV_PAGE_SIZE               2048
#ifndef HAL_NV_PAGES_USED
  #define HAL_NV_PAGES_USED            2  // Minimum = 2
#endif
#define HAL_NV_PAGE_BEG               (63-HAL_NV_PAGES_USED)  // Reserve last page for OAD boot code.
#define HAL_NV_IEEE_PAGE               63

/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ----------- Board Initialization ---------- */

#define HAL_BOARD_INIT() {                                       \
  uint16 i;                                                      \
                                                                 \
  SLEEP &= ~OSC_PD;                       /* turn on 16MHz RC and 32MHz XOSC */\
  while (!(SLEEP & XOSC_STB));            /* wait for 32MHz XOSC stable */\
  asm("NOP");                             /* chip bug workaround */\
  for (i=0; i<504; i++) asm("NOP");       /* Require 63us delay for all revs */\
  CLKCON = (0x00 | OSC_32KHZ);            /* 32MHz XOSC */\
  while (CLKCON != (0x00 | OSC_32KHZ));                          \
  SLEEP |= OSC_PD;                        /* turn off 16MHz RC */\
                                                                 \
  /* set direction for GPIO outputs  */                          \
  LED1_DDR |= LED1_BV;                                           \
  LED2_DDR |= LED2_BV;                                           \
  LED3_DDR |= LED3_BV;                                           \
                                                                 \
  /* configure tristates */                                      \
  P2INP |= PUSH2_BV;                                             \
                                                                 \
}


/* ----------- Debounce ---------- */
#define HAL_DEBOUNCE(expr)    { int i; for (i=0; i<500; i++) { if (!(expr)) i = 0; } }

/* ----------- Push Buttons ---------- */
#define HAL_PUSH_BUTTON1()        (PUSH1_POLARITY (PUSH1_SBIT))
#define HAL_PUSH_BUTTON2()        (PUSH2_POLARITY (PUSH2_SBIT))
#define HAL_PUSH_BUTTON3()        (0)
#define HAL_PUSH_BUTTON4()        (0)
#define HAL_PUSH_BUTTON5()        (0)
#define HAL_PUSH_BUTTON6()        (0)

/* ----------- LED's ---------- */
#define HAL_TURN_OFF_LED1()       st( LED1_SBIT = LED1_POLARITY (0); )
#define HAL_TURN_OFF_LED2()       st( LED2_SBIT = LED2_POLARITY (0); )
#define HAL_TURN_OFF_LED3()       st( LED3_SBIT = LED3_POLARITY (0); )
#define HAL_TURN_OFF_LED4()       HAL_TURN_OFF_LED1()

#define HAL_TURN_ON_LED1()        st( LED1_SBIT = LED1_POLARITY (1); )
#define HAL_TURN_ON_LED2()        st( LED2_SBIT = LED2_POLARITY (1); )
#define HAL_TURN_ON_LED3()        st( LED3_SBIT = LED3_POLARITY (1); )
#define HAL_TURN_ON_LED4()        HAL_TURN_ON_LED1()

#define HAL_TOGGLE_LED1()         st( if (LED1_SBIT) { LED1_SBIT = 0; } else { LED1_SBIT = 1;} )
#define HAL_TOGGLE_LED2()         st( if (LED2_SBIT) { LED2_SBIT = 0; } else { LED2_SBIT = 1;} )
#define HAL_TOGGLE_LED3()         st( if (LED3_SBIT) { LED3_SBIT = 0; } else { LED3_SBIT = 1;} )
#define HAL_TOGGLE_LED4()         HAL_TOGGLE_LED1()

#define HAL_STATE_LED1()          (LED1_POLARITY (LED1_SBIT))
#define HAL_STATE_LED2()          (LED2_POLARITY (LED2_SBIT))
#define HAL_STATE_LED3()          (LED3_POLARITY (LED3_SBIT))
#define HAL_STATE_LED4()          HAL_STATE_LED1()

/* ------------------------------------------------------------------------------------------------
 *                                     Driver Configuration
 * ------------------------------------------------------------------------------------------------
 */

/* Set to TRUE enable H/W TIMER usage, FALSE disable it */
#define HAL_TIMER FALSE

/* Set to TRUE enable ADC usage, FALSE disable it */
#define HAL_ADC FALSE

/* Set to TRUE enable DMA usage, FALSE disable it */
#define HAL_DMA FALSE

/* Set to TRUE enable AES usage, FALSE disable it */
#define HAL_AES FALSE

/* Set to TRUE enable LCD usage, FALSE disable it */
#define HAL_LCD FALSE

/* Set to TRUE enable LED usage, FALSE disable it */
#ifndef HAL_LED
#define HAL_LED TRUE
#endif
#if (!defined BLINK_LEDS) && (HAL_LED == TRUE)
#define BLINK_LEDS
#endif

/*
  Set to TRUE enable KEY usage, FALSE disable it
  Notes: On 2430EB/DB analog joystick is used to simulate
         keys. Keys won't work unless HAL_ADC is also set
         to TRUE
*/
#define HAL_KEY FALSE

/* Set to TRUE enable UART usage, FALSE disable it */
#define HAL_UART TRUE


#if HAL_UART
  #define HAL_UART_0_ENABLE  TRUE
  #define HAL_UART_1_ENABLE  FALSE
  
#if HAL_UART_0_ENABLE
#define HAL_UART_PRIPO          0x00
#else
#define HAL_UART_PRIPO          0x40
#endif

  #if HAL_DMA
  // The DB is forced to still use ISR since DMA cannot be tested on less than Rev-D chips and all available DB's are older.
  //  #if !defined( HAL_UART_DMA )
  //    /* Can only run DMA on one USART or the other, not both at the same time.
  //     * So define to 1 for USART0, 2 for USART1, or 0 for neither.
  //     */
  // TBD   #define HAL_UART_DMA  2
  //  #endif
  //#else
  //  #undef  HAL_UART_DMA
    #define HAL_UART_DMA 0
  #endif

  /* The preferred method of implementation is by DMA for faster buad rate
   * support. Customer may prefer to use the DMA channels for something else,
   * in which case USART can be driven by ISR. Also, if the 2nd USART is to be
   * used, this module does not currently support using 2 more DMA channels for
   * it, so it must use ISR.
   */
  #if !defined( HAL_UART_ISR )
    #define HAL_UART_ISR     TRUE  // TBD FALSE
  #endif

  /* It is not common for embedded applications to open/close/re-open a USART,
   * so make the close function optional.
   */
  #if !defined( HAL_UART_CLOSE )
    #define HAL_UART_CLOSE  FALSE
  #endif
#else
  #define HAL_UART_0_ENABLE  FALSE
  #define HAL_UART_1_ENABLE  FALSE
  #define HAL_UART_DMA       FALSE
  #define HAL_UART_ISR       FALSE
  #define HAL_UART_CLOSE     FALSE
#endif

/*******************************************************************************************************
*/
#endif
