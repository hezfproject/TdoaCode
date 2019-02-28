/**************************************************************************************************
  Filename:       FlashUtil.h
  Revised:        $Date: 2010/11/27 04:00:51 $
  Revision:       $Revision: 1.2 $
 **************************************************************************************************/
/*********************************************************************
Flash related operation.
*********************************************************************/
#ifndef FLASH_UTIL_H
#define FLASH_UTIL_H

#include "iocc2430.h"
#include "hal_types.h"
#include "hal_defs.h"

/*flash return status.*/
#define FLASH_SUCCESS   0
#define FLASH_WRITE_FAILED 1
#define FLASH_ERASE_FAILED 2
#define FLASH_READ_FAILED 3
#define FLASH_INVALID_ADDR 4

/*FCTL defination.*/
#define FLASH_BSY 0x80
#define FLASH_SWBSY 0x40
#define FLASH_WRITE  0x02
#define FLASH_ERASE  0x01

#define FLASH_WBUSY (FLASH_SWBSY | FLASH_BSY)

#define FlashIsBusy() (FCTL&FLASH_WBUSY)

#ifdef CPU32MHZ
#define FWTIME   0x2a
#else
#define FWTIME   0x15
#endif

#define Flash_Set_FWT() \
st( \
	FWT = FWTIME;\
)

#define Flash_Set_Fctl(ctlBit) \
st (\
	FCTL &= ~FLASH_WRITE;\
	FCTL |= ctlBit; \
)

#define Flash_Set_Fdata(data) (FWDATA = data)

//#define FlashAddrIsInValid(flashAddr) (flashAddr>>15)
#define Flash_Set_AddrH(flashAddr) \
st ( \
	FADDRH = (flashAddr>>8) & 0xff; \
)

#define Flash_Set_AddrL(flashAddr) \
st (\
	FADDRL = flashAddr & 0xff; \
)

__near_func uint8 FlashWrite(uint16 ramAddr,  uint8* pData, uint16 cnt);
__near_func uint8 FlashRead(uint16 ramAddr, uint8*pBuf, uint16 cnt);
__near_func uint8 FlashErase(uint16 ramAddr);

#endif

