/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "Console.h"

void con_PutString( const NtrxFlashCode *ptr )
{
	uint8 c ;
	while( 1 )
	{
		c = NtrxReadFlash( ptr ) ;
		if( ! c )
			return ;
		if( c == '\n' )
			con_PutReturn()	;
		else
			con_putchar( c )	;
		ptr ++ ;
	}
}

