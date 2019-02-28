#ifndef  __AFE_H__
#define  __AFE_H__
#include <htc.h>
#include "spi.h"



#define OPENRA0		{	TRISA0 = 0;	RA0 = 1;}
#define OPENRA1		{	TRISA1 = 0;	RA1 = 1;}
#define OPENRA2		{	TRISA2 = 0;	RA2 = 1;}
#define OPENRA3 		{	TRISA3 = 0;	RA3 = 1;}
#define OPENRA4		{	TRISA4 = 0;	RA4 = 1;}
#define OPENRA5 		{	TRISA5 = 0;	RA5 = 1;}

#define CLOSERA0	{	TRISA0 = 0;	RA0 = 0;}
#define CLOSERA1	{	TRISA1 = 0;	RA1 = 0;}
//#define CLOSERA2	{	TRISA2 = 0;	RA2 = 0;}
#define CLOSERA3 	{	TRISA3 = 0;	RA3 = 0;}
#define CLOSERA4	{	TRISA4 = 0;	RA4 = 0;}
#define CLOSERA5 	{	TRISA5 = 0;	RA5 = 0;}

#define CLEARRA0	{	TRISA0 = 1;	RA0 = 1;}
#define CLEARRA1	{	TRISA1 = 1;	RA1 = 1;}
//#define CLEARRA2	{	TRISA2 = 1;	RA2 = 1;}
#define CLEARRA3 	{	TRISA3 = 1;	RA3 = 1;}
#define CLEARRA4	{	TRISA4 = 1;	RA4 = 1;}
#define CLEARRA5 	{	TRISA5 = 1;	RA5 = 1;}


#define OPENRC0		{	TRISC0 = 0;	RC0 = 1;}
#define OPENRC1		{	TRISC1 = 0;	RC1 = 1;}
#define OPENRC2		{	TRISC2 = 0;	RC2 = 1;}
#define OPENRC3 		{	TRISC3 = 0;	RC3 = 1;}
#define OPENRC4		{	TRISC4 = 0;	RC4 = 1;}
#define OPENRC5 		{	TRISC5 = 0;	RC5 = 1;}

//#define CLOSERC0	{	TRISC0 = 0;	RC0 = 0;}
#define CLOSERC1	{	TRISC1 = 0;	RC1 = 0;}
#define CLOSERC2	{	TRISC2 = 0;	RC2 = 0;}
#define CLOSERC3 	{	TRISC3 = 0;	RC3 = 0;}
#define CLOSERC4	{	TRISC4 = 0;	RC4 = 0;}
#define CLOSERC5 	{	TRISC5 = 0;	RC5 = 0;}

//#define CLEARRC0	{	TRISC0 = 1;	RC0 = 1;}
#define CLEARRC1	{	TRISC1 = 1;	RC1 = 1;}
#define CLEARRC2	{	TRISC2 = 1;	RC2 = 1;}
#define CLEARRC3 	{	TRISC3 = 1;	RC3 = 1;}
#define CLEARRC4	{	TRISC4 = 1;	RC4 = 1;}
#define CLEARRC5 	{	TRISC5 = 1;	RC5 = 1;}

#define I2C_SCL_L		{	TRISC0 = 0;	RC0 = 0;}
#define I2C_SCL_H 	{	TRISC0 = 1;	RC0 = 1;}
#define I2C_SDA_L	{	TRISA1 = 0;	RA1 = 0;}
#define I2C_SDA_H	{	TRISA1 = 0;	RA1 = 1;}

#define OPENLED1	{	CLOSERA4;	}
#define CLOSELED1 	{	OPENRA4;	}

#define OPENLED2	{	CLOSERA5;	}
#define CLOSELED2 	{	OPENRA5;	}


#define data_1	16	// 数字1，1000uS
#define data_0	8	//数字0，500uS
#define time_max	56		//
#define time_min		8
#define time_mid		32		//time_mid以下是0	2ms
#define time_hi_mask  9
/*
typedef enum{
				LFstart,	//起始
				LFaddr,		//标识
				LFdata		//数据
}_LFSTATE;
*/
typedef enum{
				LFstart,
				RCV_11,
				RCV_22,
				RCV_33,
				RCV_44,
				RCV_55
}_LFSTATE;

typedef enum{
				LFidle,
				Upload,
				Download,
				Verify
}_LFCMD;

extern unsigned int rec_count;

extern void start_LFReceiver(void);
extern void OSC_INIT(void);
extern void PORT_INIT(void);
extern void delay(unsigned int c);
extern void delay_200us(unsigned char c);
extern void IIC_Init(void);

extern void INT_AFE_INIT(void);
extern void AFE_NEW_data(void);



#endif //__AFE_H__




