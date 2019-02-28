#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "hal_types.h"

void DPrint_init(void);
void DPrint(char* p);
uint32  DPrint_TimeUs(void);
uint32  DPrint_TimeMs(void);
uint32  DPrint_OsalTimeMs(void);
void DPrint_TimeInterVal(void);

#endif
