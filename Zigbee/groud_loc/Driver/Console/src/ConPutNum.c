/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "Console.h"

void con_PutNumber( const NtrxFlashCode *ptr, uint16 n )
{
	con_PutString( ptr )	;
	con_PutDecNum( n )	;
	con_PutReturn()	;
}
