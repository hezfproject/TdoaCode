#ifndef  _PIC16F1825_config_TYPE_ 
#define  _PIC16F1825_config_TYPE_

#define	INTER16MHZCLK		0x7b
#define	INTER32MHZCLK		0xf3

#define DUTYCYCLE50PERCENT 	0x10

#define TXEN_ENABLE			0x20

#define SPEN_ENABLE			0x80
#define CREN_ENABLE			0x10
#define RX9_DISABLE			0x0
#define RECEIVE_INTERRUPT_ENABLE 0x20
#define HIGH_BAUD_RATE_ENABLE	0x1


#define GIE_ENABLE			0x80
#define PEIE_ENABLE			0x40
#define RA2_ANALOG_INPUT	0x04

#define RC2_ANALOG_INPUT	0x04

#define RA0_PULL_UP_DISABLE	0xFE
#define RA1_PULL_UP_DISABLE	0xFD

#define RA2_AS_INPUT 		0x04
#define RC0_AS_INPUT 		0x01
#define RC1_AS_INPUT 		0x02
#define RC2_AS_INPUT 		0x04

#define ADC_SLECT_AN2		(0x02<<2)
#define ADC_ENABLE			0x1
#define	ADC_FOSC_8			0x10
#define ADC_SLECT_AN6		(0x06<<2)
#define ADC_ALIGN_RIGHT		0x1
#define ADC_START			0x1
#define ADC_STOP			0x0

#endif//_PIC16F639_config_TYPE_


