/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _CPU_H_
#define _CPU_H_

#include "CC_DEF.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// CPU模块提供以下函数

void CpuEnterPowerSave( void ) ;
void CpuEnterPowerStop( void ) ;

void CpuInitialize( void ) ;
void CpuEnableInterrupt( void ) ;
void CpuDisableInterrupt( void ) ;
void WatchdogReset( void ) ;
void Delay_ms( uint16 t ) ;
void Delay_us( uint16 t ) ;
uint32 GetSysClock( void ) ;
void SetCountDownTimer( uint16 t ) ;
uint16 GetCountDownTimer( void ) ;

//irq计数值置零
void RestartSysTickIrqCounter(void);
//获取irq计数值
uint16 GetSysTickIrqCounter(void);

void con_putchar( uint8 c ) ;
uint8 con_kbhit( void ) ;
uint8 con_getchar( void ) ;

void ISP_Service( uint8* buf, uint8 len ) ;

#ifdef APP_UART_PORT
	void app_putchar( uint8 c ) ;
	uint8 app_kbhit( void ) ;
	uint8 app_getchar( void ) ;
#endif

void SPI_ReadBytes( uint8 Addr, NtrxBufferPtr Buffer, uint8 Len ) ;
void SPI_WriteBytes( uint8 Addr, NtrxBufferPtr Buffer, uint8 Len ) ;
void SPI_WriteCodes( uint8 Addr, const NtrxFlashCode *Ptr, uint8 Len ) ;

//#ifdef __AVR__
//	uint8 NtrxReadFlash( const NtrxFlashCode *Addr ) ;
//#endif

//-----------------------------------------------------------------------------
// 应用程序应该提供AppTimerTickLoop函数给CPU的基本定时中断调用
void AppTimerTickLoop( void ) ;

void AppPCInt3( void );

//defined in STM32_SPI.c and STM32_USART.c
void con_UART_RCC_Configuration( void );
void con_UART_Configuration( void );
void Ntrx_SPI_RCC_Configuration( void );
void Ntrx_SPI_Configuration( void );

#ifdef __cplusplus
}
#endif

#endif /* _CPU_H_ */
