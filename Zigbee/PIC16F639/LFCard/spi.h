#ifndef  __SPI_H__
#define  __SPI_H__
#include <htc.h>
#include "afe.h"

#define CS_IN		{	TRISC1 = 1;}	
#define CS_HIGH		{	RC1 = 1;	TRISC1 = 0;}
#define CS_LOW		{	RC1 = 0;	TRISC1 = 0;}

#define SCLK_IN		{		TRISC2 = 1;}
#define SCLK_HIGH	{	RC2 = 1;	TRISC2 = 0;}
#define SCLK_LOW		{	RC2 = 0;	TRISC2 = 0;}

#define SDIO_IN		{	 TRISC3 = 1;}	
#define SDIO_HIGH	{	RC3 = 1; TRISC3 = 0;}
#define SDIO_LOW	{	RC3 = 0; TRISC3 = 0;}
#define SDIO		RC3
#define SPI_DY		{SPI_DELAY();}
//#define SPI_DY		{asm("nop");}

extern void SPI_DELAY(void);
//extern void SPI_INIT(void);
extern void SPI_WRRITE(unsigned int val);
extern unsigned int SPI_READ(unsigned int val);
extern void SET_AFE();

#endif //__SPI_H__


