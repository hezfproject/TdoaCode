#ifndef _NANOTRON_H_
#define _NANOTRON_H_

// nanochip spi/cs
#define RCC_AHBPeriph_GPIO_SSN  RCC_AHBPeriph_GPIOA
#define NTRX_SSN_PORT           GPIOA
#define NTRX_SSN_PIN            GPIO_Pin_4
//#define NtrxSetSSN()            ( GPIO_SetBits( NTRX_SSN_PORT, NTRX_SSN_PIN ) )
//#define NtrxClrSSN()            ( GPIO_ResetBits( NTRX_SSN_PORT, NTRX_SSN_PIN ) )

#define NTRX_UCRESET_PORT       GPIOA
#define NTRX_UCRESET_PIN        GPIO_Pin_0

// nanochip reset signal, duration >= 5us
#define RCC_AHBPeriph_GPIO_RST  RCC_AHBPeriph_GPIOA
#define NTRX_RST_PORT           GPIOA
#define NTRX_RST_PIN            GPIO_Pin_1
#define NtrxSetRst()            ( GPIO_SetBits( NTRX_RST_PORT, NTRX_RST_PIN ) )
#define NtrxClrRst()            ( GPIO_ResetBits( NTRX_RST_PORT, NTRX_RST_PIN ) )

#define RCC_AHBPeriph_GPIO_DIO  RCC_AHBPeriph_GPIOB
#define NTRX_DIO_PORT           GPIOB

// nanochip dio0 using to sleep alarm
#define NTRX_DIO0_PIN           GPIO_Pin_0
#define NtrxWakeupDo()          ( GPIO_SetBits( NTRX_DIO_PORT, NTRX_DIO0_PIN ) )
#define NtrxWakeupReady()       ( GPIO_ResetBits( NTRX_DIO_PORT, NTRX_DIO0_PIN ) )

#define NTRX_UCIRQ_PIN          GPIO_Pin_1
#define NTRX_UCIRQ_PORT         GPIOB

// circuitry design of moudle power switch
#define NTRX_DIO14_POWER        GPIO_Pin_14
#define NtrxOpenPA()            ( GPIO_SetBits( NTRX_DIO_PORT, NTRX_DIO14_POWER ) )
#define NtrxClosePA()           ( GPIO_ResetBits( NTRX_DIO_PORT, NTRX_DIO14_POWER ) )

#endif

