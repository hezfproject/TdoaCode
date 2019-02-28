/**************************************************************************************************
  Filename:       SPIMgr.h
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    This header describes the functions that handle the serial port.


  Copyright 2005-2007 Texas Instruments Incorporated. All rights reserved.

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

#ifndef SPIMGR_H
#define SPIMGR_H


#ifdef __cplusplus
extern "C"
{
#endif

/***************************************************************************************************
 *                                               INCLUDES
 ***************************************************************************************************/
#include "Onboard.h"

/***************************************************************************************************
 *                                               MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/
#define SOP_VALUE       0x02
#define SPI_MAX_LENGTH  128

/* Default values */
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  #define SPI_MGR_DEFAULT_PORT           ZTOOL_PORT
#elif defined (ZAPP_P1) || defined (ZAPP_P2)
  #define SPI_MGR_DEFAULT_PORT           ZAPP_PORT
#endif

#if !defined( SPI_MGR_DEFAULT_OVERFLOW )
  #define SPI_MGR_DEFAULT_OVERFLOW       FALSE
#endif

#define SPI_MGR_DEFAULT_BAUDRATE         HAL_UART_BR_38400
#define SPI_MGR_DEFAULT_THRESHOLD        SPI_THRESHOLD
#define SPI_MGR_DEFAULT_MAX_RX_BUFF      SPI_RX_BUFF_MAX
#if !defined( SPI_MGR_DEFAULT_MAX_TX_BUFF )
  #define SPI_MGR_DEFAULT_MAX_TX_BUFF      SPI_TX_BUFF_MAX
#endif
#define SPI_MGR_DEFAULT_IDLE_TIMEOUT     SPI_IDLE_TIMEOUT

/* Application Flow Control */
#define SPI_MGR_ZAPP_RX_NOT_READY         0x00
#define SPI_MGR_ZAPP_RX_READY             0x01

/*
 * Initialization
 */
extern void SPIMgr_Init (void);

/*
 * Process ZTool Rx Data
 */
void SPIMgr_ProcessZToolData ( uint8 port, uint8 event );

/*
 * Process ZApp Rx Data
 */
void SPIMgr_ProcessZAppData ( uint8 port, uint8 event );

/*
 * Calculate the check sum
 */
extern uint8 SPIMgr_CalcFCS( uint8 *msg_ptr, uint8 length );

/*
 * Register TaskID for the application
 */
extern void SPIMgr_RegisterTaskID( byte taskID );

/*
 * Register max length that application can take
 */
extern void SPIMgr_ZAppBufferLengthRegister ( uint16 maxLen );

/*
 * Turn Application flow control ON/OFF
 */
extern void SPIMgr_AppFlowControl ( uint8 status );

/***************************************************************************************************
***************************************************************************************************/

#endif  //SPIMGR_H
