/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "Console.h"

void con_PutHexNum( uint8 n )
{
	uint8 c = ( n >> 4 ) | 0x30 ;
	if( c > 0x39 )
		c += 7 ;
	con_putchar( c ) ;
	c = ( n & 15 ) | 0x30 ;
	if( c > 0x39 )
		c += 7 ;
	con_putchar( c ) ;
}
