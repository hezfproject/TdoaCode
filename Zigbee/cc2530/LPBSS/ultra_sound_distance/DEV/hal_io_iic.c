#include "hal_io_iic.h"
#include <hal_mcu.h>
#include <types.h>


#define I2C_READ    1
#define I2C_WRITE   0

#define DATA_PIN_INPUT()                                    \
    do {                                                    \
        P1DIR &= ~(I2C_SDA_PIN);                            \
    } while (0)

#define DATA_PIN_OUTPUT()                                   \
    do {                                                    \
        P1DIR |= (I2C_SDA_PIN | I2C_SCL_PIN);                            \
    } while (0)

void WAIT_NOP(uint16 n)
{
    volatile uint16 biti = n;

    while (biti != 0)
    {
        biti--;
    }
}

void HAL_I2C_Init(void)
{
    // init configration the pin is output
    P1SEL &= ~(I2C_SDA_PIN | I2C_SCL_PIN);
    P1DIR |= (I2C_SDA_PIN | I2C_SCL_PIN);
    //P0INP |= (I2C_SDA_PIN | I2C_SCL_PIN);

    SDA = 1;
    SCL = 1;
}


void HAL_I2C_Start(void)
{
    DATA_PIN_OUTPUT();
    SDA = 1;
    SCL = 1;
    WAIT_NOP(8);
    SDA = 0;
    WAIT_NOP(6);
    SCL = 0;
    WAIT_NOP(6);
}


void HAL_I2C_Stop(void)
{
    DATA_PIN_OUTPUT();
    SCL = 1;
    SDA = 0;
    WAIT_NOP(6);
    SDA = 1;
    WAIT_NOP(6);
}


void HAL_I2C_ACK(BOOL bAck)
{
    DATA_PIN_OUTPUT();
    SDA = !bAck;
    WAIT_NOP(6);
    SCL = 1;
    WAIT_NOP(6);
    SCL = 0;
    WAIT_NOP(6);
}


BOOL HAL_I2C_SendByte(uint8 u8Byte)
{
    volatile uint8 i;
    BOOL bAck;

    DATA_PIN_OUTPUT();
    WAIT_NOP(5);

    for (i=0; i<8; i++)
    {
        // MSB
        if ((u8Byte << i) & 0x80)
        {
            SDA = 1;
        }
        else
        {
            SDA = 0;
        }
        WAIT_NOP(6);

        SCL = 1;
        WAIT_NOP(6);

        SCL = 0;
        WAIT_NOP(6);
    }

    SDA = 1;
    DATA_PIN_INPUT();
    WAIT_NOP(6);
    SCL = 1;
    WAIT_NOP(6);

    if (SDA == 1)
        bAck = false;
    else
        bAck = true;

    SCL = 0;
    WAIT_NOP(6);

    return bAck;
}


uint8 HAL_I2C_RecvByte(void)
{
    uint8 i;
    uint8 u8Byte = 0;

    DATA_PIN_INPUT();

    for(i=0; i<8; i++)
    {
        SCL = 0;
        WAIT_NOP(6);
        SCL = 1;
        WAIT_NOP(6);

        u8Byte <<= 1;

        if(SDA == 1)
            u8Byte |= 1;
    }

    SCL = 0;
    WAIT_NOP(6);

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
    HAL_I2C_ACK(false);

    HAL_I2C_Stop();

    return u8Byte;
}

UINT8 HAL_I2C_ReadBytes(UINT8 u8Addr,UINT8 u8Reg,UINT8* pbuf,UINT8 u8len)
{
    UINT8 i;
    HAL_I2C_Start();
    HAL_I2C_SendByte(u8Addr | I2C_WRITE);
    HAL_I2C_SendByte(u8Reg);
    HAL_I2C_Start();
    HAL_I2C_SendByte(u8Addr | I2C_READ);
    for(i=0;i<u8len;i++)
    {
        *(pbuf+i) = HAL_I2C_RecvByte();
        if(i < (u8len -1))
            HAL_I2C_ACK(true);
    }
    HAL_I2C_ACK(false);
    HAL_I2C_Stop();
    return *pbuf;
}


UINT16 HAL_I2C_ReadValue(UINT8 u8Addr, UINT8 u8Reg, UINT8 u8Cmd)
{
    UINT16 value = 0;

    HAL_I2C_WriteReg(u8Addr, u8Reg, u8Cmd);
    WAIT_NOP(600 * 2); //delay 2ms

    while (!SCL)   //wait SCL -> 0
    {
        WAIT_NOP(6);
    }

    value   = HAL_I2C_ReadReg(u8Addr, u8Reg);
    value <<= 8;
    value  += HAL_I2C_ReadReg(u8Addr, u8Reg+1);

    return value;
}

BOOL HAL_I2C_TEST()
{

    DATA_PIN_OUTPUT();

    while (1)
    {
        SCL = 1;
        WAIT_NOP(600); //60 1ms

        SCL = 0;
        WAIT_NOP(600);
    }
}
