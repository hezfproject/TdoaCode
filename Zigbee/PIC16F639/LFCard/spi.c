#include "spi.h"
#include "afe_cfg.h"
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
#if 1
	Uint16 reg0,reg1,reg2,reg3,reg4,reg5,reg6;
	reg0 = AFE_REG0_RMK(AFE_CMD_WRITE,				\
						AFE_WRITE_REG0,             \
						AFE_REG0_OEH_1MS,           \
						AFE_REG0_OEL_4MS,           \
						AFE_REG0_ALARM_EN,         	\
						AFE_REG0_LCZEN_DIS,         \
						AFE_REG0_LCYEN_EN,         \
						AFE_REG0_LCXEN_EN);
	reg1 = AFE_REG1_RMK(AFE_CMD_WRITE,				\
						AFE_WRITE_REG1,             \
						AFE_REG1_DATOUT_MODUL,      \
						LC_TUNE_20PF);				 //REG1_LCXTUN
	reg2 = AFE_REG2_RMK(AFE_CMD_WRITE,				\
						AFE_WRITE_REG2,             \
						AFE_REG2_RSSI_MOS_DW_DIS,   \
						AFE_REG2_CLKDIV4,           \
						LC_TUNE_63PF);				 //REG2_LCYTUN
	reg3 = AFE_REG3_RMK(AFE_CMD_WRITE,      		\
						AFE_WRITE_REG3,             \
						LC_TUNE_0PF);				 //REG3_LCZTUN
	reg4 = AFE_REG4_RMK(AFE_CMD_WRITE,            	\
						AFE_WRITE_REG4,             \
						RX_SENSI_REDUCE_0DB,       	\//REG4_LCXSEN
						RX_SENSI_REDUCE_0DB);		 //REG4_LCYSEN
	reg5 = AFE_REG5_RMK(AFE_CMD_WRITE,            	\
						AFE_WRITE_REG5,             \
						AFE_REG5_AUTOCHSEL_DIS,     \
						AFE_REG5_AGC_ANY,     		\
						AFE_REG5_MODMIN_50,     	\
						RX_SENSI_REDUCE_0DB);		 //REG5_LCZSEN

	reg6 = reg0 ^ reg1;
	reg6 = reg6 ^ reg2;
	reg6 = reg6 ^ reg3;
	reg6 = reg6 ^ reg4;
	reg6 = reg6 ^ reg5;
	reg6 = ~reg6 & 0x1ff;
	reg6 = AFE_CMD(AFE_CMD_WRITE)	|				\
		   AFE_ADD(AFE_WRITE_REG6)	|				\
		   reg6;
//	reg6 = AFE_CMD(AFE_CMD_WRITE)	|				\
//		   AFE_ADD(AFE_WRITE_REG6)	|				\
//		   (~(reg0 ^ reg1 ^ reg2 ^ reg3 ^ reg4 ^ reg5)&0x1ff);

		SPI_WRRITE(reg0); //reg0 111 0000 1010 1010 1
		SPI_WRRITE(reg1); //reg1 111 0001 0000 0000 1
		SPI_WRRITE(reg2); //reg2 111 0010 0000 0000 1
		SPI_WRRITE(reg3); //reg3 111 0011 0000 0000 1
		SPI_WRRITE(reg4); //reg4 111 0100 0000 0000 1
		SPI_WRRITE(reg5); //reg5 111 0101 1100 0000 1
		SPI_WRRITE(reg6); //reg6 111 0110 1001 0101 1
	//	SPI_READ(0xC0AB);
#endif

}




