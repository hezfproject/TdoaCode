#include "deca_spi.h"
#include "printf_util.h"
#include "as3933.h"
#include "port.h"
#include "stm32l1xx_spi.h"
#include "spi_nano.h"

//º§¿¯ ‰»Î÷–∂œ
#define BTN_EXCIT_PIN            GPIO_Pin_9
#define BTN_EXCIT_GPIO           GPIOA
#define BTN_EXCIT_EXTI_PIN       EXTI_PinSource9
#define BTN_EXCIT_EXTI_GPIO      EXTI_PortSourceGPIOA
#define BTN_EXCIT_EXTI_LINE      EXTI_Line9
#define BTN_EXCIT_EXTI_IRQ       EXTI9_5_IRQn

#define BTN_EXCIT_DATA_PIN       GPIO_Pin_10

uint8 exchange_data(uint8 data)
{
	uint8 temp=0;
	int i;
	for(i=0;i<8;i++)
	{
		temp |=((data>>i)&0x01)<<(7-i);

		temp &=0xff;
	}
	return temp;
}

static uint_8 SPI2_SendByte(uint_8 Data)
{
    // Loop while DR register in not emplty
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) ;

    // Send byte through the SPI1 peripheral
    SPI_I2S_SendData(SPI2, Data) ;

    // Wait to receive a byte
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET) ;

    // Return the byte read from the SPI bus
    return SPI_I2S_ReceiveData(SPI2) ;
}

void SpiReadWriteRegister(uint8 addr,uint8 data)
{
//		uint16 addr_data,u16data,u16addr;
//		u16addr = ((uint16)addr)<<8;
//		u16data = (uint16)data;
//		addr_data = u16addr + u16data;
//	addr = exchange_data(addr);
//	data = exchange_data(data);
	GPIO_ResetBits(SPIy_CS_GPIO , SPIy_CS);
	GPIO_SetBits(SPIy_CS_GPIO , SPIy_CS);
	//port_SPIy_send_data(addr_data);
	SPI2_SendByte(addr) ;
	SPI2_SendByte(data) ;
	GPIO_ResetBits(SPIy_CS_GPIO , SPIy_CS);

//	SPI2_WriteBytes(addr, &data,8);

}
	
void config_As3933(void)
{
//		SpiReadWriteRegister(WRITE|0xe5,0x55);

	SpiReadWriteRegister(WRITE|0x0,EN_A1|EN_A2|DAT_MASK|PAT32);
	SpiReadWriteRegister(WRITE|0x1,AGC_UD|EN_MANCH|EN_WPAT|EN_XTAL);
	SpiReadWriteRegister(WRITE|0x2,0x00);
	SpiReadWriteRegister(WRITE|0x3,Minimum_Preamble_Length_23MS);
	SpiReadWriteRegister(WRITE|0x4,Shunt_Resistor3K|T_OFF_0MS);
	SpiReadWriteRegister(WRITE|0x5,TS2);
	SpiReadWriteRegister(WRITE|0x6,TS1);
	SpiReadWriteRegister(WRITE|0x7,T_OUT_50ms|T_HBIT_8);
	SpiReadWriteRegister(WRITE|0x8,0x00);
	SpiReadWriteRegister(WRITE|0x9,0x00);
	//SpiReadWriteRegister(WRITE|0xa,0x02);
	//SpiReadWriteRegister(WRITE|0xb,0x00);
	//SpiReadWriteRegister(WRITE|0xc,0x00);
	//SpiReadWriteRegister(WRITE|0xd,0xff);
	//SpiReadWriteRegister(WRITE|0xe,0x1d);
	//SpiReadWriteRegister(WRITE|0xf,0x0);
	SpiReadWriteRegister(WRITE|0x10,0x0);
	SpiReadWriteRegister(WRITE|0x11,0x0);
	SpiReadWriteRegister(WRITE|0x12,0x0);
	SpiReadWriteRegister(WRITE|0x13,0x0);
	//SpiCommandRegister(DIRECT_COMMAND);
}


void EXCIT_Configuration(void)
{
    GPIO_InitTypeDef   GPIO_InitStructure;
    EXTI_InitTypeDef   EXTI_InitStructure;

    /* Configure PB6 pin as input floating */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    /* Configure PB6 pin as input floating */
    GPIO_InitStructure.GPIO_Pin = BTN_EXCIT_PIN;
    GPIO_Init(BTN_EXCIT_GPIO, &GPIO_InitStructure);

    /* Connect EXTI6 Line to PB6 pin */
    SYSCFG_EXTILineConfig(BTN_EXCIT_EXTI_GPIO, BTN_EXCIT_EXTI_PIN);

    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    
    /* Configure EXTI6 line */
    EXTI_InitStructure.EXTI_Line = BTN_EXCIT_EXTI_LINE;
    EXTI_Init(&EXTI_InitStructure);
}


void as3933_data_init(void)
{
    GPIO_InitTypeDef   GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = BTN_EXCIT_DATA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(BTN_EXCIT_GPIO, &GPIO_InitStructure);
	
		GPIO_InitStructure.GPIO_Pin = SPIy_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(SPIy_CS_GPIO_PORT, &GPIO_InitStructure);
}



void init_as3933(void)
{
	EXCIT_Configuration();
	as3933_data_init();
//	while(1)
	config_As3933();
	close_spi2_as3933();
	
}

