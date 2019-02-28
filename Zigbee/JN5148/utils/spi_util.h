#ifndef SPI_UTIL_H
#define SPI_UTIL_H

#include "jendefs.h"

PUBLIC void SpiUtil_vInit(uint32 u32TXRDYport);
PUBLIC bool SpiUtil_bWrite(uint32* pu32data, uint8 u8Len32, bool nonblock);
PUBLIC bool SpiUtil_bWrite128(uint8* pu8data, uint8 u8Len, bool nonblock);

PUBLIC void SpiUtil_vTxReady();

#endif
