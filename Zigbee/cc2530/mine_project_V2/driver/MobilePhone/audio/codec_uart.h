#ifndef _CODEC_UART_H_
#define _CODEC_UART_H_ 

#include "hal_types.h"


void Codec_Uart_Init(void);

uint8 Codec_Uart_Puts(uint8 *p, uint8 len);

//unsigned char Codec_Uart_Gets(unsigned char SlaveAddr,unsigned char *pSubAddr,unsigned char SubMod,char *dat,unsigned int Size);

void Codec_Uart_Close(void);

#endif //_I2C_H_
