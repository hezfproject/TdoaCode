/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _BOARDCONFIG_H_
#define _BOARDCONFIG_H_

//#ifdef __AVR__
//	#include <avr\io.h>
//#endif

//#ifdef __TARGET_CPU_CORTEX_M3
//	#include "stm32f10x.h"
//#endif

#include "Config.h"
#include "CC_DEF.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Global variables define

//-----------------------------------------------------------------------------
// IO define for ATMEGA644V

//#ifdef __AVR__
//
//	#define InitNtrxCtrlPort()      ( DDRB |= (1<<DDB4), DDRD |= (1<<DDD6) )
//
//	#define NtrxSetSSN()            ( PORTB |=  (1<<PORTB4) )
//	#define NtrxClrSSN()            ( PORTB &= ~(1<<PORTB4) )
//
//	#define NtrxSetRst()            ( PORTD |=  (1<<PORTD6) )
//	#define NtrxClrRst()            ( PORTD &= ~(1<<PORTD6) )
//
//	#define NtrxSetDio0()           ( PORTB |=  (1<<PORTB2) )
//	#define NtrxClrDio0()           ( PORTB &= ~(1<<PORTB2) )
//
//	#define NtrxAmpPowerOn()    ( PORTB |=  (1<<PORTB3) )
//	#define NtrxAmpPowerOff()   ( PORTB &= ~(1<<PORTB3) )
//
//	#define InitApplicationIO() \
//		{   DDRA &= ~((1<<DDA2)|(1<<DDA3)|(1<<DDA4)|(1<<DDA5)|(1<<DDA6)) ; \
//			PORTA |= ((1<<PORTA2)|(1<<PORTA3)|(1<<PORTA4)|(1<<PORTA5)|(1<<PORTA6)) ; \
//			DDRB |= ((1<<DDB3)|(1<<DDB2)|(1<<DDB1)|(1<<DDB0)) ; \
//			DDRD |= (1<<DDD7) ; \
//			DDRC |= ((1<<DDC0)|(1<<DDC1)|(1<<DDC6)|(1<<DDC7)) ; \
//			NtrxAmpPowerOn() ; \
//			LED1_Off() ; \
//			LED2_Off() ; \
//			OLED_ClrRES() ; \
//		}
//
//#endif

//-----------------------------------------------------------------------------
// IO define for STM32F103CB

//#ifdef __TARGET_CPU_CORTEX_M3

	#define RCC_APB2Periph_GPIO_SSN RCC_APB2Periph_GPIOA
	#define NTRX_SSN_PORT           GPIOA
	#define NTRX_SSN_PIN            GPIO_Pin_4
	#define NtrxSetSSN()            ( GPIO_SetBits( NTRX_SSN_PORT, NTRX_SSN_PIN ) )
	#define NtrxClrSSN()            ( GPIO_ResetBits( NTRX_SSN_PORT, NTRX_SSN_PIN ) )

	#define RCC_APB2Periph_GPIO_RST RCC_APB2Periph_GPIOA
	#define NTRX_RST_PORT           GPIOA
	#define NTRX_RST_PIN            GPIO_Pin_1
	#define NtrxSetRst()            ( GPIO_SetBits( NTRX_RST_PORT, NTRX_RST_PIN ) )
	#define NtrxClrRst()            ( GPIO_ResetBits( NTRX_RST_PORT, NTRX_RST_PIN ) )

	#define RCC_APB2Periph_GPIO_DIO RCC_APB2Periph_GPIOB
	#define NTRX_DIO_PORT           GPIOB
	#define NTRX_DIO0_PIN           GPIO_Pin_0
	#define NtrxSetDio0()           ( GPIO_SetBits( NTRX_DIO_PORT, NTRX_DIO0_PIN ) )
	#define NtrxClrDio0()           ( GPIO_ResetBits( NTRX_DIO_PORT, NTRX_DIO0_PIN ) )

	#define InitApplicationIO()     OLED_ClrRES()

//#endif


#ifdef __cplusplus
}
#endif

#endif /* _BOARDCONFIG_H_ */

//-----------------------------------------------------------------------------

