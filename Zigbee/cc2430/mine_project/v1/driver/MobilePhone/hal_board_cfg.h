/**************************************************************************************************
  Filename:       hal_board_cfg.h
  Revised:        $Date: 2010/10/31 00:34:25 $
  Revision:       $Revision: 1.1 $

  Description:    Describe the purpose and contents of the file.

**************************************************************************************************/

#ifndef HAL_BOARD_CFG_H
#define HAL_BOARD_CFG_H

/*
 *     =============================================================
 *     |            MobilePhone Board			            |
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
#define HAL_BOARD_MOBILEPHONE


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

/* ------------------------------------------------------------------------------------------------
 *                                    Push Button Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define ACTIVE_LOW        !
#define ACTIVE_HIGH       !!    /* double negation forces result to be '1' */


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
}

/* ----------- Debounce ---------- */
#define HAL_DEBOUNCE(expr)    { int i; for (i=0; i<500; i++) { if (!(expr)) i = 0; } }


/* ------------------------------------------------------------------------------------------------
 *                                     Driver Configuration
 * ------------------------------------------------------------------------------------------------
 */

/* Set to TRUE enable ADC usage, FALSE disable it */
#define HAL_ADC TRUE

/* Set to TRUE enable DMA usage, FALSE disable it */
#define HAL_DMA FALSE

/* Set to TRUE enable AES usage, FALSE disable it */
#define HAL_AES FALSE

/* Set to TRUE enable LED usage, FALSE disable it */
#define HAL_LED FALSE

#define HAL_KEY TRUE

#define HAL_SPI FALSE

#define HAL_AUDIO TRUE

/* Set to TRUE enable UART usage, FALSE disable it */
#define HAL_UART FALSE

  #define HAL_UART_0_ENABLE  FALSE
  #define HAL_UART_1_ENABLE  FALSE
  #define HAL_UART_DMA       FALSE
  #define HAL_UART_ISR       FALSE
  #define HAL_UART_CLOSE     FALSE

/*******************************************************************************************************
*/
#endif


