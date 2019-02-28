/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
 Special definitions for different C Compiler
******************************************************************************/

#ifndef _CC_DEFINE_H_
#define _CC_DEFINE_H_

//-----------------------------------------------------------------------------

#define NTRX_DEV_ADDR_SIZE  6

#ifdef __TARGET_CPU_CORTEX_M3

//typedef unsigned char   uint_8 ;
//typedef signed   char   int_8 ;
//typedef unsigned short  uint_16 ;
//typedef signed   short  int_16 ;
//typedef unsigned int    uint_32 ;
//typedef signed   int    int_32 ;

typedef    enum
{
    False = 0,
    True = 1
} Bool ;





#endif

/* __packed keyword used to decrease the data type alignment to 1-byte */
#if defined ( __CC_ARM )
    #define __PACKED __attribute__((__packed__))
#endif

#endif
