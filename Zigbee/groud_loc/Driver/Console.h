/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "CC_DEF.h"

#ifdef __cplusplus
extern "C" {
#endif

//========================================================================
//����stdio.h�󣬿���ʹ��printf�������д��ڴ�ӡ
// #ifdef __GNUC__
//   /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
//      set to 'Yes') calls __io_putchar() */
//   #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
// #else
//   #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
// #endif /* __GNUC__ */


//-----------------------------------------------------------------------------
// ���ģ���ṩ���º���
void con_PutDecNum( uint16 n ) ;
void con_PutHexNum( uint8 n ) ;
void con_PutString( const NtrxFlashCode *ptr ) ;
void con_PutNumber( const NtrxFlashCode *ptr, uint16 n ) ;
void con_PutReturn( void ) ;
void con_PutSpace( void ) ;
NtrxBufferPtr con_ReadLine( NtrxBufferPtr ptr, uint8 buffer_size ) ;

//-----------------------------------------------------------------------------
// ���ģ����Ҫ���º���,��CPUģ���ṩ
void con_putchar( uint8 c ) ;
uint8 con_kbhit( void ) ;
uint8 con_getchar( void ) ;

#ifdef __cplusplus
}
#endif	

#endif /* _CONSOLE_H_ */
