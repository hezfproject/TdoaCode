/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "Config.h"
#include "Console.h"
#include "CPU.h"

#include "Utility.h"

#include <stdio.h>

//-----------------------------------------------------------------------------
// 去除前导空格

NtrxBufferPtr LTrim( NtrxBufferPtr Buffer )
{
	while( *Buffer == ' ' )
		Buffer ++ ;
	return Buffer ;
}

//-----------------------------------------------------------------------------
// 计算字符串长度

uint8 GetStrLen( NtrxBufferPtr Buffer )
{
	uint8 l = 0 ;
	if( Buffer == 0 )
		return 0 ;
	while( *Buffer )
	{
		Buffer ++ ;
		l ++ ;
	}
	return l ;
}

//-----------------------------------------------------------------------------
// 将输入的字符串转化为数据(默认十六进制数)
// 正常返回剩余的字符串,出错返回NULL

NtrxBufferPtr ConvertCharToData( NtrxBufferPtr Buffer, uint8 *cData )
{
	uint8 c, Count = 0 ;
	Buffer = LTrim( Buffer ) ;
	*cData = 0 ;
	while( 1 )
	{
		c = *Buffer ;
		if( ( c == 0 ) || ( c == ' ' ) )
			break ;
		if( ( c >= 0x30 ) && ( c <= 0x39 ) )
			c -= 0x30 ;
		else if( ( c >= 'A' ) && ( c <= 'F' ) )
			c -= 0x37 ;
		else if( ( c >= 'a' ) && ( c <= 'f' ) )
			c -= 0x57 ;
		else
			return (NtrxBufferPtr) 0 ;
		if( *cData == 0 )
		{
			*cData = c ;
			Count = 1 ;
		}
		else
		{
			*cData = ((*cData)<<4) + c ;
			Count ++ ;
		}
		Buffer ++ ;
	}
	if( ( Count > 0 ) && ( Count < 3 ) )
		return Buffer ;
	return (NtrxBufferPtr) 0 ;
}

//-----------------------------------------------------------------------------
// 转化距离值为字符串,精度为小数点后一位

NtrxBufferPtr ConvertDistanceToStr( float d, NtrxBufferPtr Buffer )
{
	uint16 n, i = 10000 ;
	uint8 f = 0 ;

	if( d < 0 )
	{
		n = (uint16)((0.05-d)*10) ;
		if( n >= 1 )
			*Buffer++ = '-' ;
	}
	else
	{
		n = (uint16)((d+0.05)*10) ;
	}
	while( i )
	{
		if( f || ( n >= i ) )
		{
			f = 1 ;
			*Buffer++ = 0x30 + n / i ;
		}
		n = n % i	;
		i /= 10 ;
		if( i == 10 )
			f = 1 ;
		else if( i == 1 )
			*Buffer++ = '.' ;
	}
	*Buffer = 0 ;
	return Buffer ;
}


//-----------------------------------------------------------------------------
// 监控串口输出寄存器列表

uint8 NtrxReadReg( uint8 Addr ) ;
uint8 NtrxGetShadowReg( uint8 Addr ) ;

static void ListTableTitle( void )
{
	uint8 i ;
	con_PutString( CSTR( "---" ) ) ;
	for( i = 0 ; i < 16 ; i ++ )
	{
		con_putchar( '-' ) ;
		con_PutHexNum( i ) ;
	}
	con_PutReturn() ;
}

void con_RegisterList( void )
{
	uint8 Addr ;
	con_PutString( CSTR( "Shadow List\n" ) ) ;
	ListTableTitle() ;
	for( Addr = 0x00 ; Addr < 0x80 ; Addr ++ )
	{
		if( ( Addr & 0x0F ) == 0x00 )
		{
			con_PutHexNum( Addr ) ;
			con_putchar( ':' ) ;
			con_PutSpace() ;
		}
		con_PutHexNum( NtrxGetShadowReg( Addr ) ) ;
		if( ( Addr & 0x0F ) == 0x0F )
			con_PutReturn() ;
		else
			con_PutSpace() ;
	}
	con_PutString( CSTR( "SPI Read All\n" ) ) ;
	ListTableTitle() ;

	Addr = 0 ;
	while( 1 )
	{
		if( ( Addr & 0x0F ) == 0x00 )
		{
			con_PutHexNum( Addr ) ;
			con_putchar( ':' ) ;
			con_PutSpace() ;
		}
		con_PutHexNum( NtrxReadReg( Addr ) ) ;
		if( ( Addr & 0x0F ) == 0x0F )
			con_PutReturn() ;
		else
			con_PutSpace() ;
		if( ++Addr == 0 )
			return ;
	}
}

//-----------------------------------------------------------------------------
// 监控串口输出一个寄存器值

void con_PutRegister( uint8 cAddr )
{
	con_putchar( '[' ) ;
	con_PutHexNum( cAddr ) ;
	con_putchar( ']' ) ;
	con_PutString( CSTR( " Shadow=" ) ) ;
	con_PutHexNum( NtrxGetShadowReg( cAddr ) ) ;
	con_PutString( CSTR( " SPI-Read=" ) ) ;
	con_PutHexNum( NtrxReadReg( cAddr ) ) ;
	con_PutReturn() ;
}

uint8 calcFCS(uint8 *pBuf, uint8 len)
{
  uint8 rtrn = 0;

  while (len--)
  {
    rtrn ^= *pBuf++;
  }

  return rtrn;
}














