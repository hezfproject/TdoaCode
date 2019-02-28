#ifndef __CODEC_FM11GE300__
#define __CODEC_FM11GE300__

#include "hal_types.h"

// data type define
/*
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

// Initial codec(fm11ge300)
int8 InitialCodec(void);
int8 CloseCodec(void);

#endif  // __CODEC_FM11GE300__
