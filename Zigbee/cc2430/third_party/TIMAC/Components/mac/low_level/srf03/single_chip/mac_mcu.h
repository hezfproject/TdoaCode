/**************************************************************************************************
  Filename:       mac_mcu.h
  Revised:        $Date: 2010/10/30 23:40:22 $
  Revision:       $Revision: 2.0 $

  Description:    Describe the purpose and contents of the file.


  Copyright 2006-2009 Texas Instruments Incorporated. All rights reserved.

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

#ifndef MAC_MCU_H
#define MAC_MCU_H

/* ------------------------------------------------------------------------------------------------
 *                                     Compiler Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_mcu.h"
#include "hal_types.h"
#include "hal_defs.h"
#include "hal_board.h"


/* ------------------------------------------------------------------------------------------------
 *                                    Target Specific Defines
 * ------------------------------------------------------------------------------------------------
 */
/* IP0, IP1 */
#define IPX_0                 BV(0)
#define IPX_1                 BV(1)
#define IPX_2                 BV(2)
#define IP_RFERR_RF_DMA_BV    IPX_0
#define IP_RXTX0_T2_BV        IPX_2

/* T2CNF */
#define CMPIF           BV(7)
#define PERIF           BV(6)
#define OFCMPIF         BV(5)
#define SYNC            BV(1)
#define RUN             BV(0)
#define T2CNF_IF_BITS   (CMPIF | PERIF | OFCMPIF)

/* T2PEROF2 */
#define CMPIM           BV(7)
#define PERIM           BV(6)
#define OFCMPIM         BV(5)
#define PEROF2_BITS     (BV(3) | BV(2) | BV(1) | BV(0))

/* RFIF */
#define IRQ_TXDONE      BV(6)
#define IRQ_FIFOP       BV(5)
#define IRQ_SFD         BV(4)
#define IRQ_CSP_STOP    BV(1)
#define IRQ_CSP_INT     BV(0)

/* RFIM */
#define IM_TXDONE       BV(6)
#define IM_FIFOP        BV(5)
#define IM_SFD          BV(4)
#define IM_CSP_STOP     BV(1)
#define IM_CSP_INT      BV(0)

/* IRQSRC */
#define TXACK           BV(0)


/* ------------------------------------------------------------------------------------------------
 *                                       Interrupt Macros
 * ------------------------------------------------------------------------------------------------
 */
#define MAC_MCU_WRITE_RFIF(x)         HAL_CRITICAL_STATEMENT( RFIF = x; S1CON = 0x00; RFIF = 0xFF; )
#define MAC_MCU_OR_RFIM(x)            st( RFIM |= x; )  /* compiler must use atomic ORL instruction */
#define MAC_MCU_AND_RFIM(x)           st( RFIM &= x; )  /* compiler must use atomic ANL instruction */

#define MAC_MCU_FIFOP_ENABLE_INTERRUPT()              MAC_MCU_OR_RFIM(IM_FIFOP)
#define MAC_MCU_FIFOP_DISABLE_INTERRUPT()             MAC_MCU_AND_RFIM(~IM_FIFOP)
#define MAC_MCU_FIFOP_CLEAR_INTERRUPT()               MAC_MCU_WRITE_RFIF(~IRQ_FIFOP)

#define MAC_MCU_TXDONE_ENABLE_INTERRUPT()             MAC_MCU_OR_RFIM(IM_TXDONE)
#define MAC_MCU_TXDONE_DISABLE_INTERRUPT()            MAC_MCU_AND_RFIM(~IM_TXDONE)
#define MAC_MCU_TXDONE_CLEAR_INTERRUPT()              MAC_MCU_WRITE_RFIF(~IRQ_TXDONE)

#define MAC_MCU_CSP_STOP_ENABLE_INTERRUPT()           MAC_MCU_OR_RFIM(IM_CSP_STOP)
#define MAC_MCU_CSP_STOP_DISABLE_INTERRUPT()          MAC_MCU_AND_RFIM(~IM_CSP_STOP)
#define MAC_MCU_CSP_STOP_CLEAR_INTERRUPT()            MAC_MCU_WRITE_RFIF(~IRQ_CSP_STOP)
#define MAC_MCU_CSP_STOP_INTERRUPT_IS_ENABLED()       (RFIM & IM_CSP_STOP)

#define MAC_MCU_CSP_INT_ENABLE_INTERRUPT()            MAC_MCU_OR_RFIM(IM_CSP_INT)
#define MAC_MCU_CSP_INT_DISABLE_INTERRUPT()           MAC_MCU_AND_RFIM(~IM_CSP_INT)
#define MAC_MCU_CSP_INT_CLEAR_INTERRUPT()             MAC_MCU_WRITE_RFIF(~IRQ_CSP_INT)
#define MAC_MCU_CSP_INT_INTERRUPT_IS_ENABLED()        (RFIM & IM_CSP_INT)


/* ------------------------------------------------------------------------------------------------
 *                                       Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void macMcuInit(void);
uint8 macMcuRandomByte(void);
uint8 macMcuTimerCount(void);
uint16 macMcuTimerCapture(void);
uint32 macMcuOverflowCount(void);
uint32 macMcuOverflowCapture(void);
void macMcuOverflowSetCount(uint32 count);
void macMcuOverflowSetCompare(uint32 count);
void macMcuOrT2PEROF2(uint8 value);
void macMcuAndT2PEROF2(uint8 value);
void macMcuRecordMaxRssiStart(void);
int8 macMcuRecordMaxRssiStop(void);
void macMcuRecordMaxRssiIsr(void);
uint16 macMcuPrecisionCount(void);
extern void (* macRadioStuckCallback)(void);


/**************************************************************************************************
 */
#endif
