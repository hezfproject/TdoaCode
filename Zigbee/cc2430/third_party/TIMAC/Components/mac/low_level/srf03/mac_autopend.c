/**************************************************************************************************
  Filename:       mac_autopend.c
  Revised:        $Date: 2010/10/30 23:40:21 $
  Revision:       $Revision: 2.0 $

  Description:    This file implements the Autopend feature.


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

/* low-level */
#include "mac_api.h"


/* ------------------------------------------------------------------------------------------------
 *                                         Global Variables
 * ------------------------------------------------------------------------------------------------
 */
bool macSrcMatchIsEnabled;
bool macSrcMatchIsAllPending;



/*********************************************************************
 * @fn          MAC_SrcMatchEnable
 *
 * @brief      Enabled AUTOPEND and source address matching. if number of source
 *             address table entries asked for is more than the hardware
 *             supports. It will allocate maximum number of entries and return 
 *             MAC_INVALID_PARAMETER. This function shall be not be called from 
 *             ISR. It is not thread safe.
 *
 * @param      addressType - address type that the application uses
 *                           SADDR_MODE_SHORT or SADDR_MODE_EXT
 * @param      num - number of source address table entries to be used
 *
 * @return     MAC_SUCCESS or MAC_INVALID_PARAMETER
 */
uint8 MAC_SrcMatchEnable ( uint8 addrType, uint8 num  )
{   
  /* Verify the address type */
  if( addrType != SADDR_MODE_SHORT && addrType != SADDR_MODE_EXT )
  {
    return MAC_INVALID_PARAMETER;
  }
  
  macSrcMatchIsEnabled = TRUE;

  return MAC_SUCCESS;
}

/*********************************************************************
 * @fn          MAC_SrcMatchAddEntry
 *
 * @brief       Add a short or extended address to source address table. This 
 *              function shall be not be called from ISR. It is not thread safe.
 *
 * @param       addr - a pointer to sAddr_t which contains addrMode 
 *                     and a union of a short 16-bit MAC address or an extended 
 *                     64-bit MAC address to be added to the source address table. 
 * @param       panID - the device PAN ID. It is only used when the addr is 
 *                      using short address 

 * @return      MAC_NO_RESOURCES (source address table full) 
 */
uint8 MAC_SrcMatchAddEntry ( sAddr_t *addr, uint16 panID )
{
  return MAC_NO_RESOURCES;   // Table is always full  
}

/*********************************************************************
 * @fn         MAC_SrcMatchDeleteEntry
 *
 * @brief      Delete a short or extended address from source address table. 
 *             This function shall be not be called from ISR. It is not thread safe.
 *
 * @param      addr - a pointer to sAddr_t which contains addrMode 
 *                    and a union of a short 16-bit MAC address or an extended 
 *                    64-bit MAC address to be deleted from the source address table. 
 * @param      panID - the device PAN ID. It is only used when the addr is 
 *                     using short address  
 *
 * @return     MAC_INVALID_PARAMETER (address to be deleted 
 *                  cannot be found in the source address table).
 */
uint8 MAC_SrcMatchDeleteEntry ( sAddr_t *addr, uint16 panID  )
{
  return MAC_INVALID_PARAMETER;  
}
                  

/*********************************************************************
 * @fn          MAC_SrcMatchAckAllPending
 *
 * @brief       Enabled/disable acknowledging all packets with pending bit set
 *              The application normally enables it when adding new entries to 
 *              the source address table fails due to the table is full, or 
 *              disables it when more entries are deleted and the table has
 *              empty slots.
 *
 * @param       option - TRUE (acknowledging all packets with pending field set)
 *                       FALSE (acknowledging all packets with pending field cleared) 
 *
 * @return      none
 */
void MAC_SrcMatchAckAllPending ( uint8 option  )
{
  macSrcMatchIsAllPending = option;
}

/*********************************************************************
 * @fn          MAC_SrcMatchCheckAllPending
 *
 * @brief       Check if acknowledging all packets with pending bit set
 *              is enabled. 
 *
 * @param       none 
 *
 * @return      MAC_AUTOACK_PENDING_ALL_ON or MAC_AUTOACK_PENDING_ALL_OFF
 */
uint8 MAC_SrcMatchCheckAllPending ( void )
{
  if( macSrcMatchIsAllPending == TRUE )
  {
    return MAC_AUTOACK_PENDING_ALL_ON; 
  }
  
  return MAC_AUTOACK_PENDING_ALL_OFF;
}

