/**************************************************************************************************
  Filename:       mac_radio_def.h
  Revised:        $Date: 2010/10/30 23:40:22 $
  Revision:       $Revision: 2.0 $

  Description:    Describe the purpose and contents of the file.


  Copyright 2006-2007 Texas Instruments Incorporated. All rights reserved.

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

#ifndef MAC_RADIO_DEFS_H
#define MAC_RADIO_DEFS_H

/* ------------------------------------------------------------------------------------------------
 *                                             Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_defs.h"
#include "hal_board.h"
#include "hal_mac_cfg.h"
#include "mac_spec.h"
#include "mac_mcu.h"
#include "mac_mem.h"
#include "mac_csp_tx.h"
#include "mac_assert.h"


/* ------------------------------------------------------------------------------------------------
 *                                      Target Specific Defines
 * ------------------------------------------------------------------------------------------------
 */

/* immediate strobe commands */
#define ISTXCALN      0xE1
#define ISRXON        0xE2
#define ISTXON        0xE3
#define ISTXONCCA     0xE4
#define ISRFOFF       0xE5
#define ISFLUSHRX     0xE6
#define ISFLUSHTX     0xE7
#define ISACK         0xE8
#define ISACKPEND     0xE9

/* FSCTRLL */
#define FREQ_2405MHZ                  0x65

/* RFSTATUS */
#define TX_ACTIVE                     BV(4)
#define FIFO                          BV(3)
#define FIFOP                         BV(2)
#define SFD                           BV(1)
#define CCA                           BV(0)

/* IEN2 */
#define RFIE                          BV(0)

/* MDMCTRL0L */
#define AUTOACK                       BV(4)

/* MDMCTRL0H */
#define PAN_COORDINATOR               BV(4)
#define ADDR_DECODE                   BV(3)

/* MDMCTRL1L */
#define MDMCTRL1L_RESET_VALUE         0x00
#define RX_MODE(x)                    ((x) << 0)
#define RX_MODE_INFINITE_RECEPTION    RX_MODE(2)
#define RX_MODE_NORMAL_OPERATION      RX_MODE(0)

/* FSMSTATE */
#define FSM_FFCTRL_STATE_RX_INF       31      /* infinite reception state - not documented in datasheet */

/* RFPWR */
#define ADI_RADIO_PD                  BV(4)
#define RREG_RADIO_PD                 BV(3)

/* ADCCON1 */
#define RCTRL1                        BV(3)
#define RCTRL0                        BV(2)
#define RCTRL_BITS                    (RCTRL1 | RCTRL0)
#define RCTRL_CLOCK_LFSR              RCTRL0

/* FSMTC1 */
#define ABORTRX_ON_SRXON              BV(5)
#define PENDING_OR                    BV(1)

/* CSPCTRL */
#define CPU_CTRL                      BV(0)
#define CPU_CTRL_ON                   CPU_CTRL
#define CPU_CTRL_OFF                  (!(CPU_CTRL))


/* ------------------------------------------------------------------------------------------------
 *                                    Unique Radio Define
 * ------------------------------------------------------------------------------------------------
 */
#define MAC_RADIO_CC2430
#define MAC_RADIO_FEATURE_HARDWARE_OVERFLOW_NO_ROLLOVER

// TBD : enabling this chip bug workaround appears to mask a separate, unidentified problem
#define MAC_RADIO_RXBUFF_CHIP_BUG


/* ------------------------------------------------------------------------------------------------
 *                                    Common Radio Defines
 * ------------------------------------------------------------------------------------------------
 */
#define MAC_RADIO_CHANNEL_DEFAULT               11
#define MAC_RADIO_TX_POWER_DEFAULT              0x1F
#if defined (HAL_PA_LNA)
#define MAC_RADIO_TX_POWER_MAX_DBM              19
#elif defined (HAL_PA_LNA_CC2590)
#define MAC_RADIO_TX_POWER_MAX_DBM              11
#else
#define MAC_RADIO_TX_POWER_MAX_DBM              25
#endif

#define MAC_RADIO_RECEIVER_SENSITIVITY_DBM      -91 /* dBm */
#define MAC_RADIO_RECEIVER_SATURATION_DBM       10  /* dBm */

/* offset applied to hardware RSSI value to get RF power level in dBm units */
#define MAC_RADIO_RSSI_OFFSET                   HAL_MAC_RSSI_OFFSET
#if defined (HAL_PA_LNA) || defined (HAL_PA_LNA_CC2590)
#define MAC_RADIO_RSSI_LNA_HGM_OFFSET           HAL_MAC_RSSI_LNA_HGM_OFFSET
#define MAC_RADIO_RSSI_LNA_LGM_OFFSET           HAL_MAC_RSSI_LNA_LGM_OFFSET
#define MAC_RADIO_RSSI_LNA_OFFSET(x)            st(  x += P1_1 ? MAC_RADIO_RSSI_LNA_HGM_OFFSET : MAC_RADIO_RSSI_LNA_LGM_OFFSET; )
#endif

/* precise values for small delay on both receive and transmit side */
#define MAC_RADIO_RX_TX_PROP_DELAY_MIN_USEC     3.076  /* usec */
#define MAC_RADIO_RX_TX_PROP_DELAY_MAX_USEC     3.284  /* usec */


/* ------------------------------------------------------------------------------------------------
 *                                      Common Radio Macros
 * ------------------------------------------------------------------------------------------------
 */
#define MAC_RADIO_MCU_INIT()                          macMcuInit()

#define MAC_RADIO_TURN_ON_POWER_START()               st( RFPWR &= ~RREG_RADIO_PD; )
#define MAC_RADIO_TURN_ON_POWER_DONE()                st( while((RFPWR & ADI_RADIO_PD)); )

#define MAC_RADIO_TURN_OFF_POWER()                    st( RFPWR |=  RREG_RADIO_PD; )
#define MAC_RADIO_TURN_ON_OSC()                       MAC_ASSERT(SLEEP & XOSC_STB)/* oscillator should already be stable, it's mcu oscillator too */
#define MAC_RADIO_TURN_OFF_OSC()                      /* don't do anything, oscillator is also mcu oscillator */

#define MAC_RADIO_RX_FIFO_HAS_OVERFLOWED()            ((RFSTATUS & FIFOP) && !(RFSTATUS & FIFO))
#define MAC_RADIO_RX_FIFO_IS_EMPTY()                  (!(RFSTATUS & FIFO) && !(RFSTATUS & FIFOP))

#define MAC_RADIO_SET_RX_THRESHOLD(x)                 st( IOCFG0 = ((x)-1); )
#define MAC_RADIO_RX_IS_AT_THRESHOLD()                (RFSTATUS & FIFOP)
#define MAC_RADIO_ENABLE_RX_THRESHOLD_INTERRUPT()     MAC_MCU_FIFOP_ENABLE_INTERRUPT()
#define MAC_RADIO_DISABLE_RX_THRESHOLD_INTERRUPT()    MAC_MCU_FIFOP_DISABLE_INTERRUPT()
#define MAC_RADIO_CLEAR_RX_THRESHOLD_INTERRUPT_FLAG() st( RFIF = ~IRQ_FIFOP; )

#define MAC_RADIO_TX_ACK()                            st( RFST = ISACK; )
#define MAC_RADIO_TX_ACK_PEND()                       st( RFST = ISACKPEND; )

#define MAC_RADIO_RX_ON()                             st( RFST = ISRXON;    )
#define MAC_RADIO_RXTX_OFF()                          st( RFST = ISRFOFF;   )
#define MAC_RADIO_FLUSH_RX_FIFO()                     st( RFST = ISFLUSHRX; RFST = ISFLUSHRX; )
#define MAC_RADIO_FLUSH_TX_FIFO()                     st( RFST = ISFLUSHTX; )

#define MAC_RADIO_READ_RX_FIFO(pData,len)             macMemReadRxFifo((pData),(uint8)(len))
#define MAC_RADIO_WRITE_TX_FIFO(pData,len)            macMemWriteTxFifo((pData),(uint8)(len))

#define MAC_RADIO_SET_PAN_COORDINATOR(b)              st( MDMCTRL0H = (MDMCTRL0H & ~PAN_COORDINATOR) | (PAN_COORDINATOR * (b!=0)); )
#define MAC_RADIO_SET_CHANNEL(x)                      st( FSCTRLL = FREQ_2405MHZ + 5 * ((x) - 11); )
#define MAC_RADIO_SET_TX_POWER(x)                     st( TXCTRLL = x; )

#define MAC_RADIO_SET_PAN_ID(x)                       st( PANIDL = (x) & 0xFF; PANIDH = (x) >> 8; )
#define MAC_RADIO_SET_SHORT_ADDR(x)                   st( SHORTADDRL = (x) & 0xFF; SHORTADDRH = (x) >> 8; )
#define MAC_RADIO_SET_IEEE_ADDR(p)                    macMemWriteRam((macRam_t *) &IEEE_ADDR0, p, 8)

#define MAC_RADIO_REQUEST_ACK_TX_DONE_CALLBACK()      st( MAC_MCU_TXDONE_CLEAR_INTERRUPT(); MAC_MCU_TXDONE_ENABLE_INTERRUPT(); )
#define MAC_RADIO_CANCEL_ACK_TX_DONE_CALLBACK()       MAC_MCU_TXDONE_DISABLE_INTERRUPT()

#define MAC_RADIO_RANDOM_BYTE()                       macMcuRandomByte()

#define MAC_RADIO_TX_RESET()                          macCspTxReset()
#define MAC_RADIO_TX_PREP_CSMA_UNSLOTTED()            macCspTxPrepCsmaUnslotted()
#define MAC_RADIO_TX_PREP_CSMA_SLOTTED()              macCspTxPrepCsmaSlotted()
#define MAC_RADIO_TX_PREP_SLOTTED()                   macCspTxPrepSlotted()
#define MAC_RADIO_TX_GO_CSMA()                        macCspTxGoCsma()
#define MAC_RADIO_TX_GO_SLOTTED()                     macCspTxGoSlotted()

#define MAC_RADIO_FORCE_TX_DONE_IF_PENDING()          macCspForceTxDoneIfPending()

#define MAC_RADIO_TX_REQUEST_ACK_TIMEOUT_CALLBACK()   macCspTxRequestAckTimeoutCallback()
#define MAC_RADIO_TX_CANCEL_ACK_TIMEOUT_CALLBACK()    macCspTxCancelAckTimeoutCallback()

#define MAC_RADIO_TIMER_TICKS_PER_USEC()              HAL_CPU_CLOCK_MHZ /* never fractional */
#define MAC_RADIO_TIMER_TICKS_PER_BACKOFF()           (HAL_CPU_CLOCK_MHZ * MAC_SPEC_USECS_PER_BACKOFF)
#define MAC_RADIO_TIMER_TICKS_PER_SYMBOL()            (HAL_CPU_CLOCK_MHZ * MAC_SPEC_USECS_PER_SYMBOL)

#define MAC_RADIO_TIMER_CAPTURE()                     macMcuTimerCapture()
#define MAC_RADIO_TIMER_FORCE_DELAY(x)                st( T2TLD = (x) & 0xFF;  T2THD = (x) >> 8; )  /* timer must be running */

#define MAC_RADIO_TIMER_SLEEP()                       st( T2CNF &= ~RUN; while (T2CNF & RUN); )
#define MAC_RADIO_TIMER_WAKE_UP()                     st( T2CNF |=  RUN; while (!(T2CNF & RUN)); )

#define MAC_RADIO_BACKOFF_COUNT()                     macMcuOverflowCount()
#define MAC_RADIO_BACKOFF_CAPTURE()                   macMcuOverflowCapture()
#define MAC_RADIO_BACKOFF_SET_COUNT(x)                macMcuOverflowSetCount(x)
#define MAC_RADIO_BACKOFF_SET_COMPARE(x)              macMcuOverflowSetCompare(x)

#define MAC_RADIO_BACKOFF_COMPARE_CLEAR_INTERRUPT()   st( T2CNF = RUN | SYNC | (~OFCMPIF & T2CNF_IF_BITS); )
#define MAC_RADIO_BACKOFF_COMPARE_ENABLE_INTERRUPT()  macMcuOrT2PEROF2(OFCMPIM)
#define MAC_RADIO_BACKOFF_COMPARE_DISABLE_INTERRUPT() macMcuAndT2PEROF2(~OFCMPIM)

#define MAC_RADIO_RECORD_MAX_RSSI_START()             macMcuRecordMaxRssiStart()
#define MAC_RADIO_RECORD_MAX_RSSI_STOP()              macMcuRecordMaxRssiStop()

#define MAC_RADIO_TURN_ON_RX_FRAME_FILTERING()        st( MDMCTRL0H |=  ADDR_DECODE; )
#define MAC_RADIO_TURN_OFF_RX_FRAME_FILTERING()       st( MDMCTRL0H &= ~ADDR_DECODE; )


/* ------------------------------------------------------------------------------------------------
 *                                    Common Radio Externs
 * ------------------------------------------------------------------------------------------------
 */
extern const uint8 CODE macRadioDefsTxPowerTable[];


/* ------------------------------------------------------------------------------------------------
 *                              Transmit Power Setting Configuration
 * ------------------------------------------------------------------------------------------------
 */

/*
 *  To use actual register values for setting power level, uncomment the following #define.
 *  In this case, the value passed to the set power function will be written directly to TXCTRLL.
 *  Characterized values for this register can be found in the datasheet in the "power settings" table.
 */
//#define HAL_MAC_USE_REGISTER_POWER_VALUES


/**************************************************************************************************
 */
#endif
