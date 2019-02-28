/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

//=============================================================================
// SPI Initialization
#include "cc_def.h"
#include "stm32f10x.h"
#include <stdio.h>
//#include "Config.h"

#define HSE_SPI 12000000L

//uint32 HSE_SPI;
//#ifdef	HSE_VALUE
//	#define HSE_SPI (uint32)HSE_VALUE
//#else
//	#define HSE_SPI 12000000L
//#endif


void Ntrx_SPI_RCC_Configuration( void )
{
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA, ENABLE ) ;
}

void Ntrx_SPI_Configuration( void )
{
	SPI_InitTypeDef  SPI_InitStructure ;
	GPIO_InitTypeDef GPIO_InitStructure ; 

	// Configure SPI1 pins : SCK, MISO and MOSI
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;  	//50m
	GPIO_Init( GPIOA, &GPIO_InitStructure ) ;

	
	//SPI1 configuration
	//SPI_Cmd(SPI1, DISABLE);	 	//必须先禁能,才能改变MODE

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex ;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master ;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b ;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;			//  SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge ;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft ;	  //由软件在传输前后控制片选的信号高低

//	#if ( 8000000 * 2 * PLL_MUL >= 96000000L )
//		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16 ;	//增加语句
			
//	#if ( 8000000 * 2 * PLL_MUL >= 48000000L )			// * 2, HSE_VALUE是16M . >=
//		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8 ;		//8
//	#elif ( 8000000 * 2 * PLL_MUL > 24000000L )

	#if ( HSE_SPI * PLL_MUL > 48000000L )		//HSE_Value	 HSE_VALUE	HSE_SPI
		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8 ;
	#elif ( HSE_SPI * PLL_MUL > 24000000L )		//HSE_Value	 HSE_VALUE
		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4 ;
	#else
		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2 ;
	#endif
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB ;
	SPI_InitStructure.SPI_CRCPolynomial = 7 ;
	SPI_Init( SPI1, &SPI_InitStructure ) ;

	// Enable SPI1
	SPI_Cmd( SPI1, ENABLE ) ;
}

//=============================================================================
// SPI Read/Write Routines

static uint8 SPI_SendByte( uint8 Data )
{
	// Loop while DR register in not emplty
	while( SPI_I2S_GetFlagStatus( SPI1, SPI_I2S_FLAG_TXE ) == RESET ) ;

	// Send byte through the SPI1 peripheral
	SPI_I2S_SendData( SPI1, Data ) ;

	// Wait to receive a byte
	while( SPI_I2S_GetFlagStatus( SPI1, SPI_I2S_FLAG_RXNE ) == RESET ) ;

	// Return the byte read from the SPI bus
	return SPI_I2S_ReceiveData( SPI1 ) ;
}

void SPI_ReadBytes( uint8 Addr, NtrxBufferPtr Buffer, uint8 Len )
{
	if( ( Len > 0x80 ) || ( Len == 0 ) )
		Len = 1 ;
	SPI_SendByte( Len ) ;
	SPI_SendByte( Addr ) ;
	while( Len-- )
	{
    *Buffer = SPI_SendByte( 0xFF ) ;
		Buffer ++ ;
	}
}

void SPI_WriteBytes( uint8 Addr, NtrxBufferPtr Buffer, uint8 Len )
{
	if( ( Len > 0x80 ) || ( Len == 0 ) )
		Len = 1 ;

	//Ntrx SPI帧的格式
	SPI_SendByte(0x80 | Len) ;  	//发送长度最大128bits，最高位是写使能
	SPI_SendByte( Addr ) ;
    
	while( Len-- )
    {   
		SPI_SendByte( *Buffer ) ;
        Buffer++;
    }

	//写完后可以读取结果
}

void SPI_WriteCodes( uint8 Addr, const NtrxFlashCode *Ptr, uint8 Len )
{
	if( ( Len > 0x80 ) || ( Len == 0 ) )
		Len = 1 ;
	SPI_SendByte( 0x80 | Len ) ;
	SPI_SendByte( Addr ) ;

	while( Len-- )
		SPI_SendByte( NtrxReadFlash( Ptr++ ) ) ;
}

//=============================================================================
