/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _OLED_H_
#define _OLED_H_

//#ifdef __AVR__
//	#include <avr\io.h>
//#endif

//#ifdef __TARGET_CPU_CORTEX_M3
	#include "stm32f10x.h"
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
//	#ifdef ENABLE_OLED
//		#define OLED_SetSCL()       ( PORTD |=  (1<<PORTD7) )
//		#define OLED_ClrSCL()       ( PORTD &= ~(1<<PORTD7) )
//		#define OLED_SetSI()        ( PORTC |=  (1<<PORTC0) )
//		#define OLED_ClrSI()        ( PORTC &= ~(1<<PORTC0) )
//		#define OLED_SetCS()        ( PORTC |=  (1<<PORTC1) )
//		#define OLED_ClrCS()        ( PORTC &= ~(1<<PORTC1) )
//		#define OLED_SetRES()       ( PORTC |=  (1<<PORTC6) )
//		#define OLED_ClrRES()       ( PORTC &= ~(1<<PORTC6) )
//		#define OLED_SetA0()        ( PORTC |=  (1<<PORTC7) )
//		#define OLED_ClrA0()        ( PORTC &= ~(1<<PORTC7) )
//	#else
//		#define OLED_ClrRES()
//	#endif
//
//#endif

//-----------------------------------------------------------------------------
// IO define for STM32F103CB

//#ifdef __TARGET_CPU_CORTEX_M3

	#ifdef ENABLE_OLED
		#define RCC_APB2Periph_GPIO_OLED    (RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB)
		#define OLED_SCL_PORT       GPIOB
		#define OLED_SCL_PIN        GPIO_Pin_14
		#define OLED_SI_PORT        GPIOB
		#define OLED_SI_PIN         GPIO_Pin_15
		#define OLED_CS_PORT        GPIOA
		#define OLED_CS_PIN         GPIO_Pin_8
		#define OLED_RES_PORT       GPIOA
		#define OLED_RES_PIN        GPIO_Pin_11
		#define OLED_A0_PORT        GPIOA
		#define OLED_A0_PIN         GPIO_Pin_12
		#define OLED_SetSCL()       ( GPIO_SetBits( OLED_SCL_PORT, OLED_SCL_PIN ) )
		#define OLED_ClrSCL()       ( GPIO_ResetBits( OLED_SCL_PORT, OLED_SCL_PIN ) )
		#define OLED_SetSI()        ( GPIO_SetBits( OLED_SI_PORT, OLED_SI_PIN ) )
		#define OLED_ClrSI()        ( GPIO_ResetBits( OLED_SI_PORT, OLED_SI_PIN ) )
		#define OLED_SetCS()        ( GPIO_SetBits( OLED_CS_PORT, OLED_CS_PIN ) )
		#define OLED_ClrCS()        ( GPIO_ResetBits( OLED_CS_PORT, OLED_CS_PIN ) )
		#define OLED_SetRES()       ( GPIO_SetBits( OLED_RES_PORT, OLED_RES_PIN ) )
		#define OLED_ClrRES()       ( GPIO_ResetBits( OLED_RES_PORT, OLED_RES_PIN ) )
		#define OLED_SetA0()        ( GPIO_SetBits( OLED_A0_PORT, OLED_A0_PIN ) )
		#define OLED_ClrA0()        ( GPIO_ResetBits( OLED_A0_PORT, OLED_A0_PIN ) )
	#else
		#define OLED_ClrRES()
	#endif

//#endif

//-----------------------------------------------------------------------------
// Function define

void OLED_Init( void ) ;
void OLED_TurnOff( void ) ;
void OLED_Clear( void ) ;
void OLED_PutChar( uint8 x, uint8 y, uint8 c ) ;
void OLED_PutString( uint8 x, uint8 y, const NtrxFlashCode *ptr ) ;

#ifdef __cplusplus
}
#endif

#endif /* _OLED_H_ */

//-----------------------------------------------------------------------------

