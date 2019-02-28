/**************************************************************************************************
  Filename:       SPIMgr.c
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    This module handles anything dealing with the serial port.


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

/***************************************************************************************************
 *                                           INCLUDES
 ***************************************************************************************************/
#include "ZComDef.h"
#include "OSAL.h"
#include "hal_uart.h"
#include "MTEL.h"
#include "SPIMgr.h"
#include "OSAL_Memory.h"

/***************************************************************************************************
 *                                            MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                           CONSTANTS
 ***************************************************************************************************/

/* State values for ZTool protocal */
#define SOP_STATE      0x00
#define CMD_STATE1     0x01
#define CMD_STATE2     0x02
#define LEN_STATE      0x03
#define DATA_STATE     0x04
#define FCS_STATE      0x05

/***************************************************************************************************
 *                                            TYPEDEFS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                         GLOBAL VARIABLES
 ***************************************************************************************************/
byte App_TaskID;

/* ZTool protocal parameters */

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
uint8 state;
uint8  CMD_Token[2];
uint8  LEN_Token;
uint8  FSC_Token;
mtOSALSerialData_t  *SPI_Msg;
uint8  tempDataLen;
#endif //ZTOOL

#if defined (ZAPP_P1) || defined (ZAPP_P2)
uint16  SPIMgr_MaxZAppBufLen;
bool    SPIMgr_ZAppRxStatus;
#endif


/***************************************************************************************************
 *                                          LOCAL FUNCTIONS
 ***************************************************************************************************/

/***************************************************************************************************
 * @fn      SPIMgr_Init
 *
 * @brief
 *
 * @param   None
 *
 * @return  None
***************************************************************************************************/
void SPIMgr_Init ()
{
  halUARTCfg_t uartConfig;

  /* Initialize APP ID */
  App_TaskID = 0;

  /* UART Configuration */
  uartConfig.configured           = TRUE;
  uartConfig.baudRate             = SPI_MGR_DEFAULT_BAUDRATE;
  uartConfig.flowControl          = SPI_MGR_DEFAULT_OVERFLOW;
  uartConfig.flowControlThreshold = SPI_MGR_DEFAULT_THRESHOLD;
  uartConfig.rx.maxBufSize        = SPI_MGR_DEFAULT_MAX_RX_BUFF;
  uartConfig.tx.maxBufSize        = SPI_MGR_DEFAULT_MAX_TX_BUFF;
  uartConfig.idleTimeout          = SPI_MGR_DEFAULT_IDLE_TIMEOUT;
  uartConfig.intEnable            = TRUE;
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  uartConfig.callBackFunc         = SPIMgr_ProcessZToolData;
#elif defined (ZAPP_P1) || defined (ZAPP_P2)
  uartConfig.callBackFunc         = SPIMgr_ProcessZAppData;
#else
  uartConfig.callBackFunc         = NULL;
#endif

  /* Start UART */
#if defined (SPI_MGR_DEFAULT_PORT)
  HalUARTOpen (SPI_MGR_DEFAULT_PORT, &uartConfig);
#else
  /* Silence IAR compiler warning */
  (void)uartConfig;
#endif

  /* Initialize for ZApp */
#if defined (ZAPP_P1) || defined (ZAPP_P2)
  /* Default max bytes that ZAPP can take */
  SPIMgr_MaxZAppBufLen  = 1;
  SPIMgr_ZAppRxStatus   = SPI_MGR_ZAPP_RX_READY;
#endif


}

/***************************************************************************************************
 * @fn      MT_SerialRegisterTaskID
 *
 * @brief
 *
 *   This function registers the taskID of the application so it knows
 *   where to send the messages whent they come in.
 *
 * @param   void
 *
 * @return  void
 ***************************************************************************************************/
void SPIMgr_RegisterTaskID( byte taskID )
{
  App_TaskID = taskID;
}

/***************************************************************************************************
 * @fn      SPIMgr_CalcFCS
 *
 * @brief
 *
 *   Calculate the FCS of a message buffer by XOR'ing each byte.
 *   Remember to NOT include SOP and FCS fields, so start at the CMD
 *   field.
 *
 * @param   byte *msg_ptr - message pointer
 * @param   byte len - length (in bytes) of message
 *
 * @return  result byte
 ***************************************************************************************************/
byte SPIMgr_CalcFCS( uint8 *msg_ptr, uint8 len )
{
  byte x;
  byte xorResult;

  xorResult = 0;

  for ( x = 0; x < len; x++, msg_ptr++ )
    xorResult = xorResult ^ *msg_ptr;

  return ( xorResult );
}


#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/***************************************************************************************************
 * @fn      SPIMgr_ProcessZToolRxData
 *
 * @brief   | SOP | CMD  |   Data Length   | FSC  |
 *          |  1  |  2   |       1         |  1   |
 *
 *          Parses the data and determine either is SPI or just simply serial data
 *          then send the data to correct place (MT or APP)
 *
 * @param   pBuffer  - pointer to the buffer that contains the data
 *          length   - length of the buffer
 *
 *
 * @return  None
 ***************************************************************************************************/
void SPIMgr_ProcessZToolData ( uint8 port, uint8 event )
{
  uint8  ch;

  /* Verify events */
  if (event == HAL_UART_TX_FULL)
  {
    // Do something when TX if full
    return;
  }

  if (event & (HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT))
  {
    while (Hal_UART_RxBufLen(SPI_MGR_DEFAULT_PORT))
    {
      HalUARTRead (SPI_MGR_DEFAULT_PORT, &ch, 1);

      switch (state)
      {
        case SOP_STATE:
          if (ch == SOP_VALUE)
            state = CMD_STATE1;
          break;

        case CMD_STATE1:
          CMD_Token[0] = ch;
          state = CMD_STATE2;
          break;

        case CMD_STATE2:
          CMD_Token[1] = ch;
          state = LEN_STATE;
          break;

        case LEN_STATE:
          LEN_Token = ch;
          if (ch == 0)
            state = FCS_STATE;
          else
            state = DATA_STATE;

          tempDataLen = 0;

          /* Allocate memory for the data */
          SPI_Msg = (mtOSALSerialData_t *)osal_msg_allocate( sizeof ( mtOSALSerialData_t ) + 2+1+LEN_Token );

          if (SPI_Msg)
          {
            /* Fill up what we can */
            SPI_Msg->hdr.event = CMD_SERIAL_MSG;
            SPI_Msg->msg = (uint8*)(SPI_Msg+1);
            SPI_Msg->msg[0] = CMD_Token[0];
            SPI_Msg->msg[1] = CMD_Token[1];
            SPI_Msg->msg[2] = LEN_Token;
          }
          else
          {
            state = SOP_STATE;
            return;
          }

          break;

        case DATA_STATE:
            SPI_Msg->msg[3 + tempDataLen++] = ch;
            if ( tempDataLen == LEN_Token )
              state = FCS_STATE;
          break;

        case FCS_STATE:

          FSC_Token = ch;

          /* Make sure it's correct */
          if ((SPIMgr_CalcFCS ((uint8*)&SPI_Msg->msg[0], 2 + 1 + LEN_Token) == FSC_Token))
          {
            osal_msg_send( MT_TaskID, (byte *)SPI_Msg );
          }
          else
          {
            /* deallocate the msg */
            osal_msg_deallocate ( (uint8 *)SPI_Msg);
          }

          /* Reset the state, send or discard the buffers at this point */
          state = SOP_STATE;

          break;

        default:
         break;
      }


    }
  }
}
#endif //ZTOOL

#if defined (ZAPP_P1) || defined (ZAPP_P2)
/***************************************************************************************************
 * @fn      SPIMgr_ProcessZAppRxData
 *
 * @brief   | SOP | CMD  |   Data Length   | FSC  |
 *          |  1  |  2   |       1         |  1   |
 *
 *          Parses the data and determine either is SPI or just simply serial data
 *          then send the data to correct place (MT or APP)
 *
 * @param   pBuffer  - pointer to the buffer that contains the data
 *          length   - length of the buffer
 *
 *
 * @return  None
 ***************************************************************************************************/
void SPIMgr_ProcessZAppData ( uint8 port, uint8 event )
{

  osal_event_hdr_t  *msg_ptr;
  uint16 length = 0;
  uint16 rxBufLen  = Hal_UART_RxBufLen(SPI_MGR_DEFAULT_PORT);

  /*
     If maxZAppBufferLength is 0 or larger than current length
     the entire length of the current buffer is returned.
  */
  if ((SPIMgr_MaxZAppBufLen != 0) && (SPIMgr_MaxZAppBufLen <= rxBufLen))
  {
    length = SPIMgr_MaxZAppBufLen;
  }
  else
  {
    length = rxBufLen;
  }

  /* Verify events */
  if (event == HAL_UART_TX_FULL)
  {
    // Do something when TX if full
    return;
  }

  if (event & ( HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT))
  {
    if ( App_TaskID )
    {
      /*
         If Application is ready to receive and there is something
         in the Rx buffer then send it up
      */
      if ((SPIMgr_ZAppRxStatus == SPI_MGR_ZAPP_RX_READY ) && (length != 0))
      {
        /* Disable App flow control until it processes the current data */
         SPIMgr_AppFlowControl ( SPI_MGR_ZAPP_RX_NOT_READY );

        /* 2 more bytes are added, 1 for CMD type, other for length */
        msg_ptr = (osal_event_hdr_t *)osal_msg_allocate( length + sizeof(osal_event_hdr_t) );
        if ( msg_ptr )
        {
          msg_ptr->event = SPI_INCOMING_ZAPP_DATA;
          msg_ptr->status = length;

          /* Read the data of Rx buffer */
          HalUARTRead( SPI_MGR_DEFAULT_PORT, (uint8 *)(msg_ptr + 1), length );

          /* Send the raw data to application...or where ever */
          osal_msg_send( App_TaskID, (uint8 *)msg_ptr );
        }
      }
    }
  }
}

/***************************************************************************************************
 * @fn      SPIMgr_ZAppBufferLengthRegister
 *
 * @brief
 *
 * @param   maxLen - Max Length that the application wants at a time
 *
 * @return  None
 *
 ***************************************************************************************************/
void SPIMgr_ZAppBufferLengthRegister ( uint16 maxLen )
{
  /* If the maxLen is larger than the RX buff, something is not right */
  if (maxLen <= SPI_MGR_DEFAULT_MAX_RX_BUFF)
    SPIMgr_MaxZAppBufLen = maxLen;
  else
    SPIMgr_MaxZAppBufLen = 1; /* default is 1 byte */
}

/***************************************************************************************************
 * @fn      SPIMgr_AppFlowControl
 *
 * @brief
 *
 * @param   status - ready to send or not
 *
 * @return  None
 *
 ***************************************************************************************************/
void SPIMgr_AppFlowControl ( bool status )
{

  /* Make sure only update if needed */
  if (status != SPIMgr_ZAppRxStatus )
  {
    SPIMgr_ZAppRxStatus = status;
  }

  /* App is ready to read again, ProcessZAppData have to be triggered too */
  if (status == SPI_MGR_ZAPP_RX_READY)
  {
    SPIMgr_ProcessZAppData ( SPI_MGR_DEFAULT_PORT, HAL_UART_RX_TIMEOUT );
  }

}

#endif //ZAPP

/***************************************************************************************************
***************************************************************************************************/
