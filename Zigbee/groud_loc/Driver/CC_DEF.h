/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
 Special definitions for different C Compiler
******************************************************************************/

#ifndef _CC_DEFINE_H_
#define _CC_DEFINE_H_

//-----------------------------------------------------------------------------

#define NTRX_DEV_ADDR_SIZE  6
#define xNT_SNIFFER

//#ifdef __TARGET_CPU_CORTEX_M3

	typedef unsigned char   uint8 ;
	typedef signed   char   int8 ;
	typedef unsigned short  uint16 ;
	typedef signed   short  int16 ;
	typedef unsigned int    uint32 ;
	typedef signed   int    int32 ;

	typedef	enum {
		False = 0,
		True = 1
	} Bool ;

////增加NULL定义
//	#ifndef	NULL
//	#define NULL 0
//	#endif

	typedef enum {FALSE = 0, TRUE = !FALSE} bool;


	typedef unsigned char NtrxDevAddr[NTRX_DEV_ADDR_SIZE] ;
	typedef unsigned char * NtrxDevPtr ;

	typedef unsigned char NtrxBufferType ;
	typedef NtrxBufferType * NtrxBufferPtr ;

	typedef unsigned char NtrxFlashCode ;

	#define NtrxReadFlash(addr)  (*(unsigned char *)(addr))

	#define CSTR(s) ((const NtrxFlashCode *)(s))

	#define inline
//#endif

//-----------------------------------------------------------------------------

#endif
