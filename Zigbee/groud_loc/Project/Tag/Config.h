/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
 Description : This file contains defines for CPU module
******************************************************************************/

#ifndef _CONFIG_H_
#define _CONFIG_H_

//-----------------------------------------------------------------------------
// CPU : STM32F103CB

//#ifdef __TARGET_CPU_CORTEX_M3

	#ifndef HSE_VALUE
//		//#define HSE_Value           12000000L	// �ⲿ����Ƶ��	16000000L   //
		#define HSE_VALUE           12000000L	// �ⲿ����Ƶ��	16000000L   //
//		#define HSE_Value           12000000L	// �ⲿ����Ƶ��	16000000L   //
	#endif
	
	#define PLL_MUL             6			// ϵͳʱ�ӱ�Ƶϵ��,ϵͳʱ��=����Ƶ��x��Ƶϵ��	3	//
	#define zWATCHDOG_ENABLE                 // ���Ź�
	#define BASE_TIME_TICK      1          // ��ʱ�ж�ʱ����,��λ:ms		   ÿ���tick�������Ӧ1000
	#define CON_UART_PORT       1           // ʹ��1�Ŵ�����Ϊ���
	#define CON_UART_BAUDRATE   115200      // ������

//#endif

//#define ENABLE_OLED


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



#endif /* _CONFIG_H_ */
