#ifndef _SPI_SLAVE_H_
#define _SPI_SLAVE_H_

#include "type.h"

#define SPI0_HEAD   "YIRI"
#define SPI0_HEAD_SIZE      4
#define SPI0_BUFFER_SIZE    12           // Maximum buffer Master will send
#define SPI0_TRANS_LEN      (SPI0_HEAD_SIZE + SPI0_BUFFER_SIZE)
#define SPI0_ARM_COMMAND_TYPE   100
#define SPI0_ARM_COMMAND_SHT  100
#define SPI0_ARM_COMMAND_RESU   200

#define SPI_ENTRY_CRITICAL(u8INT)    u8INT = ESPI0; ESPI0 = 0
#define SPI_EXIT_CRITICAL(u8INT)     ESPI0 = u8INT

extern  VOID SPI0_Init(VOID);
extern  BOOL SPI0_GetSpiData(UINT8 *pu8Data);
extern  BOOL SPI0_SetSpiData(UINT8 *pu8Data);

#endif