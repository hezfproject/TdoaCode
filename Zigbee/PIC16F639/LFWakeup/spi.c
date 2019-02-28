#include "spi.h"

/*******************************
**
**	SPI操作延时
**
********************************/

void SPI_DELAY(void)
{	unsigned int vv=2;
	while(--vv)
	{
		asm("nop");
    }
}

/*
void SPI_INIT(void)
{
	SDIO_IN;		//先置为输入
	CS_OUT;		//CS高时SDIO是LFDATA输出
	CLK_OUT;
}
*/


/***************************************
**
**	SPI写
**
***************************************/
void SPI_WRRITE(unsigned int val)
{
	unsigned char Bitcount;
	SCLK_LOW;
	SPI_DY;
	CS_LOW;	
	for(Bitcount = 16; Bitcount >0; Bitcount--)
	{
		SPI_DY;
		if(val & 0x8000)
		{
		    SDIO_HIGH;
		}
		else 
		{
		    SDIO_LOW;
		}			
		SCLK_HIGH;
		SPI_DY;
		SPI_DY;
		SCLK_LOW;
		val = val<<1;
	}
	SDIO_IN;
	SPI_DY;
	CS_HIGH;
	SPI_DY;
	SCLK_IN;

}

/***************************************
**
**	SPI读
**
***************************************/

unsigned int SPI_READ(unsigned int val)
{

	unsigned char Bitcount;
	SCLK_LOW;
	SPI_DY;
	CS_LOW;	
	for(Bitcount = 16; Bitcount >0; Bitcount--)
	{
		SPI_DY;
		if(val & 0x8000)
		{
		    SDIO_HIGH;
		}
		else 
		{
		    SDIO_LOW;
		}			
		SCLK_HIGH;
		SPI_DY;
		SPI_DY;
		SCLK_LOW;
		val = val<<1;
    }
//	SPI_DY;
	SDIO_IN;
	SPI_DY;
	CS_HIGH;
	SPI_DY;
	CS_LOW;

	for(Bitcount = 16; Bitcount >0; Bitcount--)
	{
		SPI_DY;
		val = val<<1;
		SCLK_HIGH;
		SPI_DY;
		if(SDIO)
		{
			val = val+1;
		}	
		SCLK_LOW;
	}	
	SPI_DY;
	CS_HIGH;
	SCLK_IN;
    return val;
}


/***************************************
**
**	设置AFE
**
***************************************/
void SET_AFE()
{
#if 0
	 // 是可以读写的
		SPI_WRRITE(0xe141); //reg0 111 0000 1010 0000 1
		SPI_WRRITE(0xe201); //reg1 111 0001 0000 0000 1
		SPI_WRRITE(0xe401); //reg2 111 0010 0000 0000 1
		SPI_WRRITE(0xe601); //reg3 111 0011 0000 0000 1
		SPI_WRRITE(0xe801); //reg4 111 0100 0000 0000 1
		SPI_WRRITE(0xeb81); //reg5 111 0101 1100 0000 1
		SPI_WRRITE(0xed3f); //reg6 111 1110 1001 1111 1
	
	SPI_READ(0xC0AB);
	
	
#endif
	
#if 1
		SPI_WRRITE(0xe141); //reg0 111 0000 1010 1010 1
		SPI_WRRITE(0xe201); //reg1 111 0001 0000 0000 1
		SPI_WRRITE(0xe401); //reg2 111 0010 0000 0000 1
		SPI_WRRITE(0xe601); //reg3 111 0011 0000 0000 1
		SPI_WRRITE(0xe801); //reg4 111 0100 0000 0000 1
		SPI_WRRITE(0xeb81); //reg5 111 0101 1100 0000 1
		SPI_WRRITE(0xed3f); //reg6 111 0110 1001 0101 1
	//	SPI_READ(0xC0AB);
	
#endif



}




