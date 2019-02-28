#include "hal_io_i2c.h"
#include "stm32l1xx.h"
#include "type_def.h"


#define I2C_READ    1
#define I2C_WRITE   0

#define DATA_PIN_INPUT()                                    \
    do {                                                    \
        GPIO_InitTypeDef  GPIO_InitStructure;               \
                                                            \
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;          \
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;        \
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;   \
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;      \
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;   \
        GPIO_Init(GPIOB, &GPIO_InitStructure);              \
    } while (0)

#define DATA_PIN_OUTPUT()                                   \
    do {                                                    \
        GPIO_InitTypeDef  GPIO_InitStructure;               \
                                                            \
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;          \
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;       \
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;   \
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      \
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;       \
        GPIO_Init(GPIOB, &GPIO_InitStructure);              \
    } while (0)

void WAIT_NOP(uint_8 n)
{
    __IO uint_8 biti = n;

    while (biti != 0)
    {
        biti--;
    }
}

void HAL_I2C_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /*!< Configure I2C2 pins: SCL */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /*!< Configure I2C2 pins: SDA */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    HIGH(SDA);
    HIGH(SCL);
}

void HAL_I2C_Start(void)
{
    HIGH(SDA);
    DATA_PIN_OUTPUT();

    HIGH(SCL);
    WAIT_NOP(1);

    LOW(SDA);

    LOW(SCL);
    WAIT_NOP(1);
}

void HAL_I2C_Stop(void)
{
    LOW(SDA);
    DATA_PIN_OUTPUT();

    HIGH(SCL);
    WAIT_NOP(1);

    HIGH(SDA);
    WAIT_NOP(1);
}

void HAL_I2C_ACK(Bool bAck)
{
    if (!bAck)
    {
        LOW(SDA);
    }
    else
    {
        HIGH(SDA);
    }
    DATA_PIN_OUTPUT();

    HIGH(SCL);
    WAIT_NOP(1);

    LOW(SCL);
    WAIT_NOP(1);
}

Bool HAL_I2C_SendByte(uint_8 u8Byte)
{
	uint_8 i;
    Bool bAck;

    DATA_PIN_OUTPUT();

	for (i=0; i<8; i++)
	{
        // MSB
		if ((u8Byte << i) & 0x80)
		{
			HIGH(SDA);
		}
		else
		{
			LOW(SDA);
		}

		HIGH(SCL);
        WAIT_NOP(1);

        LOW(SCL);
        WAIT_NOP(1);
	}

    HIGH(SDA);
    DATA_PIN_INPUT();

    HIGH(SCL);
    WAIT_NOP(1);

	if (GET_INPUT(SDA) == 1)
		bAck = False;
	else
		bAck = True;

    LOW(SCL);

    return bAck;
}

uint8 HAL_I2C_RecvByte(void)
{
	uint8 i;
    uint8 u8Byte = 0;

    DATA_PIN_INPUT();

	for (i=0; i<8; i++)
	{
        LOW(SCL);
		WAIT_NOP(1);

		HIGH(SCL);
		WAIT_NOP(1);

        u8Byte <<= 1;

        u8Byte |= GET_INPUT(SDA);
	}

    LOW(SCL);

    return u8Byte;
}

void HAL_I2C_WriteReg(uint8 u8I2cAddr, uint8 u8Reg, uint8 u8Byte)
{
    HAL_I2C_Start();
    HAL_I2C_SendByte(u8I2cAddr | I2C_WRITE);
    HAL_I2C_SendByte(u8Reg);
    HAL_I2C_SendByte(u8Byte);
    HAL_I2C_Stop();
}

uint8 HAL_I2C_ReadReg(uint8 u8Addr, uint8 u8Reg)
{
    uint8 u8Byte;

    HAL_I2C_Start();
    HAL_I2C_SendByte(u8Addr | I2C_WRITE);
    HAL_I2C_SendByte(u8Reg);
    HAL_I2C_Start();
    HAL_I2C_SendByte(u8Addr | I2C_READ);
    u8Byte = HAL_I2C_RecvByte();
    HAL_I2C_ACK(False);

    HAL_I2C_Stop();

    return u8Byte;
}

