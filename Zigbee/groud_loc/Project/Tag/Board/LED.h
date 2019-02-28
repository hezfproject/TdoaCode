/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _LED_H_
#define _LED_H_

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
//	#define LED1_On()               ( PORTB |=  (1<<PORTB0) )
//	#define LED1_Off()              ( PORTB &= ~(1<<PORTB0) )
//	#define LED1_Toggle()			( PORTB ^=  (1<<PORTB0) )
//	#define LED2_On()               ( PORTB |=  (1<<PORTB1) )
//	#define LED2_Off()              ( PORTB &= ~(1<<PORTB1) )
//	#define LED2_Toggle()			( PORTB ^=  (1<<PORTB1) )
//
//#endif

//-----------------------------------------------------------------------------
// IO define for STM32F103CB

//#ifdef __TARGET_CPU_CORTEX_M3

	#define RCC_APB2Periph_GPIO_LED RCC_APB2Periph_GPIOB
	#define LED_A_PORT              GPIOB
	#define LED_A_PIN               GPIO_Pin_12
	#define LED_B_PORT              GPIOB
	#define LED_B_PIN               GPIO_Pin_10
	#define LED1_Off()               ( GPIO_SetBits( LED_A_PORT, LED_A_PIN ) )
	#define LED1_On()              ( GPIO_ResetBits( LED_A_PORT, LED_A_PIN ) )
	#define LED1_Toggle()
	#define LED2_Off()               ( GPIO_SetBits( LED_B_PORT, LED_B_PIN ) )
	#define LED2_On()              ( GPIO_ResetBits( LED_B_PORT, LED_B_PIN ) )
	#define LED2_Toggle()

//#endif

//-----------------------------------------------------------------------------
// Function define

#ifdef __cplusplus
}
#endif

#endif /* _LED_H_ */

//-----------------------------------------------------------------------------

