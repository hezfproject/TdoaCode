/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "Console.h"
#include "CPU.h"

NtrxBufferPtr con_ReadLine( NtrxBufferPtr ptr, uint8 buffer_size )
{
	static uint8 Index = 0 ;
	uint8 c ;

	if( ( ptr == (NtrxBufferPtr)0 ) || ( buffer_size == 0 ) )
		return (NtrxBufferPtr) 0 ;

	if( ! con_kbhit() )
		return (NtrxBufferPtr) 0 ;

	c = con_getchar() ;
	if( c == '\n' || c == '\r' )
	{
		con_PutReturn() ;
		ptr[Index] = 0 ;
		Index = 0 ;
		return ptr ;
	}
	if( Index < buffer_size - 1 )
	{
		if( c >= 0x20 && c < 0x7F )
		{
			ptr[ Index++ ] = c ;
			con_putchar( c ) ;
		}
	}
	return (NtrxBufferPtr) 0 ;
}

