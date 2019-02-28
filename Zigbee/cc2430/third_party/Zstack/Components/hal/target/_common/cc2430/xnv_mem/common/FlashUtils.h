/**************************************************************************************************
  Filename:       FlashUtils.h
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

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

#ifndef FLASHUTILS_H
#define FLASHUTILS_H

#define SIZEOF_IEEE_ADDRESS    8
#define SIZEOF_DLIMAGE_INFO    6


#define CRC32_POLYNOMIAL   ((uint32)0xEDB88320)  // reveresed version or 802.3 polynomial 0x04C11DB7
#define FCS_LENGTH         4

// structure to hold OAD persistent memory information. this format is visible only to boot code
// others must use access API defined in mbox.
typedef struct  {
    uint8    OADPM_ServerIEEEAddress[SIZEOF_IEEE_ADDRESS];  // valid only during download session
    uint16   OADPM_ServerNWKAddress;                        // valid only during download session
    uint8    OADPM_ServerEndpoint;                          // valid only during download session
    uint8    OADPM_DLImageInfo[SIZEOF_DLIMAGE_INFO];        // valid only during download session
    uint8    OADPM_DLImagePreambleOffset;                   // valid only if base address and 1st page offset valid
    uint32   OADPM_DLFirstPageOffset;                       // 0xFFFFFFFF if no image present
    uint32   OADPM_DLBaseAddress;                           // 0xFFFFFFFF if no image present
    uint16   OADPM_Status;                                  // bit map of status info
} oadpm_t;

#define OADPM_OS_IEEE_ADDR       (0)
#define OADPM_OS_NWK_ADDR        (OADPM_OS_IEEE_ADDR + SIZEOF_IEEE_ADDRESS)
#define OADPM_OS_ENDPT           (OADPM_OS_NWK_ADDR + sizeof(uint16))
#define OADPM_OS_DLIMG_INFO      (OADPM_OS_ENDPT + sizeof(uint8))
#define OADPM_OS_PREAMBLE_OFFSET (OADPM_OS_DLIMG_INFO + SIZEOF_DLIMAGE_INFO)
#define OADPM_OS_FIRSTPAGE_ADDR  (OADPM_OS_PREAMBLE_OFFSET + sizeof(uint8))
#define OADPM_OS_BADDR           (OADPM_OS_FIRSTPAGE_ADDR + sizeof(uint32))
#define OADPM_OS_STATUS          (OADPM_OS_BADDR + sizeof(uint32))

#define XMEM_WRITE     0
#define XMEM_READ      1

#define CHIP_PAGESIZE     (0x800)
#define CHIP_SHIFTCOUNT   (11)

void FlashInit(void);
void GetFlashRWFunc(int8 (**)(uint8, uint32, uint8 *, uint16));

#endif
