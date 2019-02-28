
/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "Console.h"

void con_PutDecNum( uint16 n )
{
	uint16 i = 10000 ;
	Bool f = False ;
	while( i )
	{
		if( f || ( n >= i ) )
		{
			f = True ;
			con_putchar( 0x30 + n / i )	;
		}
		n = n % i	;
		i /= 10 ;
		if( i == 1 )
			f = True ;
	}
}

