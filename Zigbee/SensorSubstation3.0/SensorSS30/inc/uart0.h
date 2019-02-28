#ifndef _UART0_H_
#define _UART0_H_
#include "type.h"

#define BAUDRATE        115200           // Baud rate of UART in bps

extern INT8 print(INT8 *, ...);
extern VOID UART0_Init(VOID);
#endif