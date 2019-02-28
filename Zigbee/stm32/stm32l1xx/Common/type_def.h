// -------------------------------------------------------------------------------------------------------------------
//
//  File: type_def.h - DecaWave general type definitions
//
//  Copyright 2008 (c) DecaWave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author: Billy Verso, November 2008
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef _TYPES_DEF_H_
#define _TYPES_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include "compiler.h"

#ifndef uint8
#ifndef _DECA_UINT8_
#define _DECA_UINT8_
typedef unsigned char uint8;
#endif
#endif

#ifndef uint16
#ifndef _DECA_UINT16_
#define _DECA_UINT16_
typedef unsigned short uint16;
#endif
#endif

#ifndef uint32
#ifndef _DECA_UINT32_
#define _DECA_UINT32_
typedef unsigned long uint32;
#endif
#endif

#ifndef int8
#ifndef _DECA_INT8_
#define _DECA_INT8_
typedef signed char int8;
#endif
#endif

#ifndef int16
#ifndef _DECA_INT16_
#define _DECA_INT16_
typedef signed short int16;
#endif
#endif

#ifndef int32
#ifndef _DECA_INT32_
#define _DECA_INT32_
typedef signed long int32;
#endif
#endif

typedef   signed       __int64 int64_t;
typedef unsigned       __int64 uint64_t;



typedef uint64_t        uint64 ;

typedef int64_t         int64 ;


#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef NULL
#define NULL    0
#endif


#ifdef __cplusplus
}
#endif

#endif /* DECA_TYPES_H_ */


