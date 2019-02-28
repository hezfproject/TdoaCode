/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _KEYSCAN_H_
#define _KEYSCAN_H_

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

#define KEY_SW1         0x81		// 最高位置1区别于串口命令
#define KEY_SW2         0x82
#define KEY_SW3         0x84
#define KEY_SW4         0x88
#define KEY_SW5         0x90

//-----------------------------------------------------------------------------
// IO define for ATMEGA644V

//#ifdef __AVR__
//
//	#define KEY_SW1_STATUS()	    ( (PINA^0xFF) & (1<<PINA6) )
//	#define KEY_SW2_STATUS()	    ( (PINA^0xFF) & (1<<PINA5) )
//	#define KEY_SW3_STATUS()	    ( (PINA^0xFF) & (1<<PINA4) )
//	#define KEY_SW4_STATUS()	    ( (PINA^0xFF) & (1<<PINA3) )
//	#define KEY_SW5_STATUS()	    ( (PINA^0xFF) & (1<<PINA2) )
//
//#endif

//-----------------------------------------------------------------------------
// IO define for STM32F103CB

//#ifdef __TARGET_CPU_CORTEX_M3

	#define RCC_APB2Periph_GPIO_KEY (RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC)
	#define KEY_SW1_PORT            GPIOC
	#define KEY_SW1_PIN             GPIO_Pin_13
	#define KEY_SW2_PORT            GPIOC
	#define KEY_SW2_PIN             GPIO_Pin_14
	#define KEY_SW3_PORT            GPIOB
	#define KEY_SW3_PIN             GPIO_Pin_7
	#define KEY_SW4_PORT            GPIOB
	#define KEY_SW4_PIN             GPIO_Pin_6
	#define KEY_SW5_PORT            GPIOB
	#define KEY_SW5_PIN             GPIO_Pin_9

	#define KEY_SW1_PORTSRC         GPIO_PortSourceGPIOC
	#define KEY_SW1_PINSRC          GPIO_PinSource13
	#define KEY_SW2_PORTSRC         GPIO_PortSourceGPIOC
	#define KEY_SW2_PINSRC          GPIO_PinSource14
	#define KEY_SW3_PORTSRC         GPIO_PortSourceGPIOB
	#define KEY_SW3_PINSRC          GPIO_PinSource7
	#define KEY_SW4_PORTSRC         GPIO_PortSourceGPIOB
	#define KEY_SW4_PINSRC          GPIO_PinSource6
	#define KEY_SW5_PORTSRC         GPIO_PortSourceGPIOB
	#define KEY_SW5_PINSRC          GPIO_PinSource9

	#define KEY_SW1_EXTI_LINE       EXTI_Line13
	#define KEY_SW2_EXTI_LINE       EXTI_Line14
	#define KEY_SW3_EXTI_LINE       EXTI_Line7
	#define KEY_SW4_EXTI_LINE       EXTI_Line6
	#define KEY_SW5_EXTI_LINE       EXTI_Line9

//#endif

typedef void (*halKeyCBack_t) (uint8 keys, uint8 state);

extern Bool Hal_KeyIntEnable;
//-----------------------------------------------------------------------------
// Function define

uint8 GetKeyStatus( void ) ;

void InitKeyScan( void ) ;

void KeyScanProcess( void );


void HalKeyInit(void);
void HalKeyConfig(Bool interruptEnable, halKeyCBack_t cback);
uint8 HalKeyRead(void);
void HalKeyPoll(void);

//#ifdef __TARGET_CPU_CORTEX_M3
void Key_NVIC_Configuration( void );
//#endif

#ifdef __cplusplus
}
#endif

#endif /* _KEYSCAN_H_ */

//-----------------------------------------------------------------------------

