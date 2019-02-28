/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _APP_H_
#define _APP_H_

#include "CC_DEF.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Global variables define
#ifdef __MAP_APP_VARS__
	#define EXTERN
#else
	#define EXTERN extern
#endif

EXTERN uint8 KeyCommand ;
EXTERN uint8 KeyStatus ;
	#define KeyPress    1
	#define KeyRepeat   2

#define KEY_SW1         0x81		// 最高位置1区别于串口命令
#define KEY_SW2         0x82
#define KEY_SW3         0x84
#define KEY_SW4         0x88
#define KEY_SW5         0x90

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

// 	#define RCC_APB2Periph_GPIO_KEY (RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC)
// 	#define KEY_SW1_PORT            GPIOC
// 	#define KEY_SW1_PIN             GPIO_Pin_13
// 	#define KEY_SW2_PORT            GPIOC
// 	#define KEY_SW2_PIN             GPIO_Pin_14
// 	#define KEY_SW3_PORT            GPIOB
// 	#define KEY_SW3_PIN             GPIO_Pin_7
// 	#define KEY_SW4_PORT            GPIOB
// 	#define KEY_SW4_PIN             GPIO_Pin_6
// 	#define KEY_SW5_PORT            GPIOB
// 	#define KEY_SW5_PIN             GPIO_Pin_9

//	#ifdef ENABLE_OLED
//		#define RCC_APB2Periph_GPIO_OLED    (RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB)
//		#define OLED_SCL_PORT       GPIOB
//		#define OLED_SCL_PIN        GPIO_Pin_14
//		#define OLED_SI_PORT        GPIOB
//		#define OLED_SI_PIN         GPIO_Pin_15
//		#define OLED_CS_PORT        GPIOA
//		#define OLED_CS_PIN         GPIO_Pin_8
//		#define OLED_RES_PORT       GPIOA
//		#define OLED_RES_PIN        GPIO_Pin_11
//		#define OLED_A0_PORT        GPIOA
//		#define OLED_A0_PIN         GPIO_Pin_12
//		#define OLED_SetSCL()       ( GPIO_SetBits( OLED_SCL_PORT, OLED_SCL_PIN ) )
//		#define OLED_ClrSCL()       ( GPIO_ResetBits( OLED_SCL_PORT, OLED_SCL_PIN ) )
//		#define OLED_SetSI()        ( GPIO_SetBits( OLED_SI_PORT, OLED_SI_PIN ) )
//		#define OLED_ClrSI()        ( GPIO_ResetBits( OLED_SI_PORT, OLED_SI_PIN ) )
//		#define OLED_SetCS()        ( GPIO_SetBits( OLED_CS_PORT, OLED_CS_PIN ) )
//		#define OLED_ClrCS()        ( GPIO_ResetBits( OLED_CS_PORT, OLED_CS_PIN ) )
//		#define OLED_SetRES()       ( GPIO_SetBits( OLED_RES_PORT, OLED_RES_PIN ) )
//		#define OLED_ClrRES()       ( GPIO_ResetBits( OLED_RES_PORT, OLED_RES_PIN ) )
//		#define OLED_SetA0()        ( GPIO_SetBits( OLED_A0_PORT, OLED_A0_PIN ) )
//		#define OLED_ClrA0()        ( GPIO_ResetBits( OLED_A0_PORT, OLED_A0_PIN ) )
//	#else
//		//#define OLED_ClrRES()
//	#endif

	#define InitApplicationIO()     OLED_ClrRES()

//#endif


//-----------------------------------------------------------------------------
// Function define

void PrintVersionMessage( void ) ;

void CpuEnterPowerSave( void ) ;
void CpuEnterPowerStop( void ) ;

uint8 GetKeyStatus( void ) ;

void InitKeyScan( void ) ;
//void KeyScanProcess( void ) ;

#ifdef ENABLE_OLED
void OLED_Init( void ) ;
void OLED_TurnOff( void ) ;
void OLED_Clear( void ) ;
void OLED_PutChar( uint8 x, uint8 y, uint8 c ) ;
void OLED_PutString( uint8 x, uint8 y, const NtrxFlashCode *ptr ) ;
#endif

NtrxBufferPtr LTrim( NtrxBufferPtr Buffer ) ;
uint8 GetStrLen( NtrxBufferPtr Buffer ) ;
NtrxBufferPtr ConvertCharToData( NtrxBufferPtr Buffer, uint8 *Data ) ;
//void ConvertDistanceToStr( float d, NtrxBufferPtr Buffer ) ;
void con_RegisterList( void ) ;
void con_PutRegister( uint8 Addr ) ;


//-----------------------------------------------------------------------------

extern void App_Init(uint8 taskid);
extern uint16 App_ProcessEvent(uint8 taskid, uint16 events);
extern void App_KeyCB(uint8 keys, uint8 state);
extern void App_UartCB(uint8 port, uint8 event);

#ifdef __cplusplus
}
#endif

#endif /* _APP_H_ */

//-----------------------------------------------------------------------------
