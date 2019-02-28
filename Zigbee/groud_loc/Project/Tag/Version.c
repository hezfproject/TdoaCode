/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "Console.h"
#include <stdio.h>

void PrintVersionMessage( void )
{
	// 监控端口输出版本信息
	printf("TAG:\n");
	con_PutString( CSTR( "WNtrxDrv SDK Demo 2.0, " ) ) ;
	con_PutString( CSTR( __DATE__ ) ) ;
	con_PutSpace() ;
	con_PutString( CSTR( __TIME__ ) ) ;
	con_PutReturn() ;
}
