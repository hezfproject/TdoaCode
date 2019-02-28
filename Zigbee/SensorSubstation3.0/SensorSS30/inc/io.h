#ifndef _IO_H_
#define _IO_H_

#include "type.h"

#define IO_CURRENT_MODE    0x01
#define IO_FREQ_MODE       0x02
#define IO_STATE_MODE      0x03
#define IO_UART485_MODE    0x04
#define IO_INVALID_MODE    0x0A

extern VOID IO_Init(VOID);

extern VOID IO_Shutting(BOOL bSht);

extern UINT8 IO_GetSwitchState(VOID);

extern BOOL IO_GetStateMode(VOID);

#endif
