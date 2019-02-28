/**************************************************************************************************
  Filename:       hal_board_cfg.h
  Revised:        $Date: 2011/07/12 17:42:03 $
  Revision:       $Revision: 1.7 $

  Description:    Declarations for the CC2530EM used on the SmartRF05EB.


  Copyright 2006-2010 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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


/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"

/* ------------------------------------------------------------------------------------------------
 *                                       CC2590/CC2591 support
 *
 *                        Define HAL_PA_LNA_CC2590 if CC2530+CC2590EM is used
 *                        Define HAL_PA_LNA if CC2530+CC2591EM is used
 *                        Note that only one of them can be defined
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_PA_LNA
//#define xHAL_PA_LNA_CC2590


/* ------------------------------------------------------------------------------------------------
 *                                       Board Indentifier
 *
 *      Define the Board Identifier to HAL_BOARD_CC2530EB_REV13 for SmartRF05 EB 1.3 board
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_BOARD_MOBILEPHONE


/* ------------------------------------------------------------------------------------------------
 *                                          Clock Speed
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_CPU_CLOCK_MHZ     32

/* This flag should be defined if the SoC uses the 32MHz crystal
 * as the main clock source (instead of DCO).
 */
#define HAL_CLOCK_CRYSTAL

/* 32 kHz clock source select in CLKCONCMD */
#if !defined (OSC32K_CRYSTAL_INSTALLED) || (defined (OSC32K_CRYSTAL_INSTALLED) && (OSC32K_CRYSTAL_INSTALLED == TRUE))
  #define OSC_32KHZ  0x00 /* external 32 KHz xosc */
#else
  #define OSC_32KHZ  0x80 /* internal 32 KHz rcosc */
#endif

#define HAL_CLOCK_STABLE()    st( while (CLKCONSTA != (CLKCONCMD_32MHZ | OSC_32KHZ)); )

/* ------------------------------------------------------------------------------------------------
 *                                       LED Configuration
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                    Push Button Configuration
 * ------------------------------------------------------------------------------------------------
 */

#define ACTIVE_LOW        !
#define ACTIVE_HIGH       !!    /* double negation forces result to be '1' */

/* ------------------------------------------------------------------------------------------------
 *                         OSAL NV implemented by internal flash pages.
 * ------------------------------------------------------------------------------------------------
 */

// Flash is partitioned into 8 banks of 32 KB or 16 pages.
#define HAL_FLASH_PAGE_PER_BANK    16
// Flash is constructed of 128 pages of 2 KB.
#define HAL_FLASH_PAGE_SIZE        2048
#define HAL_FLASH_WORD_SIZE        4

// CODE banks get mapped into the XDATA range 8000-FFFF.
#define HAL_FLASH_PAGE_MAP         0x8000

// The last 16 bytes of the last available page are reserved for flash lock bits.
// NV page definitions must coincide with segment declaration in project *.xcl file.
#if defined NON_BANKED
#define HAL_FLASH_LOCK_BITS        16
#define HAL_NV_PAGE_END            30
#define HAL_NV_PAGE_CNT            2
#else
#define HAL_FLASH_LOCK_BITS        16
#define HAL_NV_PAGE_END            126
#define HAL_NV_PAGE_CNT            8//6
#endif

// Re-defining Z_EXTADDR_LEN here so as not to include a Z-Stack .h file.
#define HAL_FLASH_IEEE_SIZE        8
#define HAL_FLASH_IEEE_PAGE       (HAL_NV_PAGE_END+1)
#define HAL_FLASH_IEEE_OSET       (HAL_FLASH_PAGE_SIZE - HAL_FLASH_LOCK_BITS - HAL_FLASH_IEEE_SIZE)
#define HAL_INFOP_IEEE_OSET        0xC

#define HAL_FLASH_DEV_PRIVATE_KEY_OSET     0x7D2
#define HAL_FLASH_CA_PUBLIC_KEY_OSET       0x7BC
#define HAL_FLASH_IMPLICIT_CERT_OSET       0x78C

#define HAL_NV_PAGE_BEG           (HAL_NV_PAGE_END-HAL_NV_PAGE_CNT+1)

// Used by DMA macros to shift 1 to create a mask for DMA registers.
#define HAL_NV_DMA_CH              0
#define HAL_DMA_CH_RX              3
#define HAL_DMA_CH_TX              4

#define HAL_NV_DMA_GET_DESC()      HAL_DMA_GET_DESC0()
#define HAL_NV_DMA_SET_ADDR(a)     HAL_DMA_SET_ADDR_DESC0((a))

/* ------------------------------------------------------------------------------------------------
 *  Serial Boot Loader: reserving the first 4 pages of flash and other memory in cc2530-sb.xcl.
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_SB_IMG_ADDR       0x2000
#define HAL_SB_CRC_ADDR       0x2090
// Size of internal flash less 4 pages for boot loader, 6 pages for NV, & 1 page for lock bits.
#define HAL_SB_IMG_SIZE      (0x40000 - 0x2000 - 0x3000 - 0x0800)

/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ----------- RF-frontend Connection Initialization ---------- */
#if defined HAL_PA_LNA || defined HAL_PA_LNA_CC2590
extern void MAC_RfFrontendSetup(void);
#define HAL_BOARD_RF_FRONTEND_SETUP() MAC_RfFrontendSetup()
#else
#define HAL_BOARD_RF_FRONTEND_SETUP()
#endif

/* ----------- Cache Prefetch control ---------- */
#define PREFETCH_ENABLE()     st( FCTL = 0x08; )
#define PREFETCH_DISABLE()    st( FCTL = 0x04; )

/* ----------- Board Initialization ---------- */
#define HAL_BOARD_INIT()                                         \
{                                                                \
  uint16 i;                                                      \
                                                                 \
  SLEEPCMD &= ~OSC_PD;                       /* turn on 16MHz RC and 32MHz XOSC */                \
  while (!(SLEEPSTA & XOSC_STB));            /* wait for 32MHz XOSC stable */                     \
  asm("NOP");                                /* chip bug workaround */                            \
  for (i=0; i<504; i++) asm("NOP");          /* Require 63us delay for all revs */                \
  CLKCONCMD = (CLKCONCMD_32MHZ | OSC_32KHZ); /* Select 32MHz XOSC and the source for 32K clock */ \
  while (CLKCONSTA != (CLKCONCMD_32MHZ | OSC_32KHZ)); /* Wait for the change to be effective */   \
  SLEEPCMD |= OSC_PD;                        /* turn off 16MHz RC */                              \
                                                                 \
  /* Turn on cache prefetch mode */                              \
 PREFETCH_ENABLE();                                             \
                                                                 \
  /* setup RF frontend if necessary */                           \
  HAL_BOARD_RF_FRONTEND_SETUP();                                 \
}

/* ----------- Debounce ---------- */
#define HAL_DEBOUNCE(expr)    { int i; for (i=0; i<500; i++) { if (!(expr)) i = 0; } }

/* ----------- Minimum safe bus voltage ---------- */

// Vdd/3 / Internal Reference X ENOB --> (Vdd / 3) / 1.15 X 127
#define VDD_2_0  74   // 2.0 V required to safely read/write internal flash.
#define VDD_2_7  100  // 2.7 V required for the Numonyx device.

#define VDD_MIN_RUN   VDD_2_0
#define VDD_MIN_NV   (VDD_2_0+4)  // 5% margin over minimum to survive a page erase and compaction.
#define VDD_MIN_XNV  (VDD_2_7+5)  // 5% margin over minimum to survive a page erase and compaction.

/* ------------------------------------------------------------------------------------------------
 *                                     Driver Configuration
 * ------------------------------------------------------------------------------------------------
 */

/* Set to TRUE enable H/W TIMER usage, FALSE disable it */
#ifndef HAL_TIMER
#define HAL_TIMER FALSE
#endif

/* Set to TRUE enable ADC usage, FALSE disable it */
#ifndef HAL_ADC
#define HAL_ADC TRUE
#endif

/* Set to TRUE enable DMA usage, FALSE disable it */
#ifndef HAL_DMA
#define HAL_DMA TRUE
#endif

/* Set to TRUE enable Flash access, FALSE disable it */
#ifndef HAL_FLASH
#define HAL_FLASH TRUE
#endif

/* Set to TRUE enable AES usage, FALSE disable it */
#ifndef HAL_AES
#define HAL_AES FALSE
#endif

#ifndef HAL_AES_DMA
#define HAL_AES_DMA FALSE
#endif

/* Set to TRUE enable LCD usage, FALSE disable it */
#ifndef HAL_LCD
#define HAL_LCD FALSE
#endif

/* Set to TRUE enable LED usage, FALSE disable it */
#ifndef HAL_LED
#define HAL_LED FALSE
#endif

#if (!defined BLINK_LEDS) && (HAL_LED == TRUE)
#define BLINK_LEDS
#endif

/* Set to TRUE enable KEY usage, FALSE disable it */
#ifndef HAL_KEY
#define HAL_KEY TRUE
#endif

/* Set to TRUE enable UART usage, FALSE disable it */
#ifndef HAL_UART
#define HAL_UART TRUE
#endif

#define HAL_UART_ISR  1


/* USB is not used for CC2530 configuration */
#define HAL_UART_USB  0
#endif
/*******************************************************************************************************
*/
