#ifndef __DELAY__
#define __DELAY__
#include "hal_types.h"
#include "hal_defs.h"
/*
// data type define
#ifndef uint8
#define uint8   unsigned char
#endif
#ifndef uint16
#define uint16  unsigned short
#endif
#ifndef uint32
#define uint32  unsigned int
#endif
#ifndef int8
#define int8    signed char
#endif
#ifndef int16
#define int16   short
#endif
#ifndef int32
#define int32   int
#endif
*/

#if 0
#define DelayUs(N) st(  \
for(uint16 i=0;i<N;i++){  \
    for(uint16 j=0;j<3;j++){asm("nop");} \
    } \
)

#define  DelayMs(N) st( \
    for (uint16 i=0; i<N; i++) { \
      for (uint16 j=0; j<1200; j++) { \
            asm("nop"); \
      } \
    } \
) 
#endif

// Delay 1 us
__near_func  void DelayUs(uint16 n);
// Delay 1 ms
__near_func  void DelayMs(uint16 n);

#endif  // __DELAY__
