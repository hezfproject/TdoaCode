/**************************************************************************************************
Filename:       FlashUtil.c
Revised:        $Date: 2010/11/27 04:00:51 $
Revision:       $Revision: 1.6 $

Description:    Flash utils. 
CPU flash operation function. Need disable the interrupt before the functions are started.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "FlashUtil.h"

#define NV_PAGE_SIZE 0x800
#define NV_PAGE_SIZE_EXPONENT   11
#define NV_WORD_SIZE 4   //per word.

// DMA Sturcture
typedef struct
{
	uint8  SRC_HI;
	uint8  SRC_LO;
	uint8  DST_HI;
	uint8  DST_LO;
	uint8  VLEN;
	uint8  LEN;
	uint8  TRIG;
	uint8  INCMODE;
} DMA_t;

static DMA_t  __xdata FlashDMA;
__near_func void  halFlashDmaTrigger(void);
uint16 RamAddr2FlashAddr(uint16 ramAddr);

/**************************************************************************************************
*
* @fn          RamAddr2FlashAddr(uint16 ramAddr)
*
* @brief       This function is helped to transfer a ramAddr to a flashAddr(pg+addrbyword)
*
* @param       ramAddr - The address identified per byte.
*
* @return      16bit - the flashAddr as FADDRH:FADDRL needed.
*
**************************************************************************************************/
uint16 RamAddr2FlashAddr(uint16 ramAddr)
{
	uint8 pg = ramAddr >> NV_PAGE_SIZE_EXPONENT;
	uint16 offset = (ramAddr & (NV_PAGE_SIZE-1));
	offset >>= 2;
	return (uint16)pg << 9 | offset;
}

/**************************************************************************************************
*
* @fn          uint8 FlashWrite(uint16 ramAddr, uint8* pData, uint16 cnt)
*
* @brief       This function is helped to write a block of word into flash.
*
* @param       ramAddr - The address identified per byte.
*                   pData - The data will be transfered to flash.
*                   cnt - The count by word(per 4 bytes).
*
* @return      8bit - flash status.
*
**************************************************************************************************/
__near_func uint8 FlashWrite(uint16 ramAddr, uint8* pData, uint16 cnt)
{

	uint16 AddrByWord = RamAddr2FlashAddr(ramAddr); //transfered to addrbyword for flash conreoller.

        EA = 0;          // disable Intrrupt;
	// OK. do the rest. use DMA Channel 0
	// Set up Flash DMA
      
	FlashDMA.DST_HI  = 0xdf;
	FlashDMA.DST_LO  = 0xaf;   // set Dest = FWDATA
	FlashDMA.VLEN    = 0x00;
	FlashDMA.LEN     = 0x04;
	FlashDMA.TRIG    = 0x12;
	FlashDMA.INCMODE = 0x42;

	DMA0CFGH = (uint16)&FlashDMA >> 8;
	DMA0CFGL = (uint16)&FlashDMA;

	// 'len' must be 0 (mod 4)
	for (uint16 i=0; i<(cnt/4); ++i)  {
		// set buffer pointer in DMA config info
		FlashDMA.SRC_HI  = (uint16)pData >> 8;
		FlashDMA.SRC_LO  = (uint16)pData;
		// Set Flash write address based on fbase
		FADDRH = AddrByWord >> 8;
		FADDRL = AddrByWord;

		DMAARM = 0x01;
		halFlashDmaTrigger();
		while (!(DMAIRQ & 0x01));   // wait for DMA transfer
		while ( FCTL & FLASH_WBUSY );  // wait until Flash controller not busy
		DMAIRQ &= ~0x01;
		AddrByWord++;
		pData += 4;
	}
      EA = 1;          // enable Intrrupt;
	//uint16 AddrByWord = ramAddr/4;
	/*
	uint16 i = 0;
	while (FlashIsBusy());
	Flash_Set_FWT();
	Flash_Set_AddrH(AddrByWord);
	Flash_Set_AddrL(AddrByWord);
	Flash_Set_Fctl(FLASH_WRITE);           // set FCTL.WRITE
	//FCTL &= ~FLASH_WRITE;
	//FCTL |= FLASH_WRITE; 
	while (cnt > i)
	{
	Flash_Set_Fdata(pData[i++]);
	Flash_Set_Fdata(pData[i++]);
	Flash_Set_Fdata(pData[i++]);
	Flash_Set_Fdata(pData[i++]);
	while (FlashIsBusy());    // wait SWBSY free
	}
	*/
	return FLASH_SUCCESS;	
}

/**************************************************************************************************
*
* @fn          uint8 FlashRead(uint16 ramAddr, uint8* pBuf, uint16 cnt)
*
* @brief       This function is helped to write a block of word into flash.
*
* @param       ramAddr - The address identified per byte.
*                   pBuf - The data will hold the flash data.
*                   cnt - The count by word(per 4 bytes).
*
* @return      8bit - flash status.
*
**************************************************************************************************/
__near_func uint8 FlashRead(uint16 ramAddr, uint8*pBuf, uint16 cnt)
{
	return FLASH_SUCCESS;
}

/**************************************************************************************************
*
* @fn          uint8 FlashErase(uint16 ramAddr, uint8* pBuf, uint16 cnt)
*
* @brief       This function is helped to erase a flash page.
*
* @param       ramAddr - The address identified per byte.
*
* @return      8bit - flash status.
*
**************************************************************************************************/
__near_func uint8 FlashErase(uint16 ramAddr)
{
	uint16 AddrByWord = RamAddr2FlashAddr(ramAddr); //transfered to addrbyword for flash controller.
	Flash_Set_AddrH(AddrByWord);
	Flash_Set_Fctl(FLASH_ERASE);           // set ERASE bit
	asm("nop");
	while (FlashIsBusy());    // wait BUSY free

	return FLASH_SUCCESS;
}
