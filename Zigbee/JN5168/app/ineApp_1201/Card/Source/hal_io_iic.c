#include "hal_io_iic.h"
//#include <hal_mcu.h>
//#include <types.h>
#include <jendefs.h>
#include <AppHardwareApi.h>


#define I2C_READ    1
#define I2C_WRITE   0


#define DATA_PIN_INPUT()                                    \
    do {                                                    \
        vAHI_DioSetDirection(I2C_SDA_PIN,0);                            \
    } while (0)



#define DATA_PIN_OUTPUT()                                   \
    do {                                                    \
        vAHI_DioSetDirection(0,I2C_SDA_PIN | I2C_SCL_PIN);  \
    } while (0)



void WAIT_NOP(uint8 n)
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
    //P1SEL &= ~(I2C_SDA_PIN | I2C_SCL_PIN);

	vAHI_DioSetDirection(0,I2C_SDA_PIN | I2C_SCL_PIN);
    //P1DIR |= (I2C_SDA_PIN | I2C_SCL_PIN);
    //P0INP |= (I2C_SDA_PIN | I2C_SCL_PIN);

    //SDA = 1;
    //SCL = 1;

	vAHI_DioSetOutput(I2C_SDA_PIN | I2C_SCL_PIN,0);
}


void HAL_I2C_Start(void)
{
    DATA_PIN_OUTPUT();
	vAHI_DioSetOutput(I2C_SDA_PIN | I2C_SCL_PIN,0);

    //SDA = 1;
    //SCL = 1;
    WAIT_NOP(3);
	vAHI_DioSetOutput(0,I2C_SDA_PIN);
    //SDA = 0;
    WAIT_NOP(3);
	vAHI_DioSetOutput(0,I2C_SCL_PIN);
    //SCL = 0;
    WAIT_NOP(3);
}


void HAL_I2C_Stop(void)
{
    DATA_PIN_OUTPUT();
	vAHI_DioSetOutput(I2C_SCL_PIN,0);
    //SCL = 1;
    vAHI_DioSetOutput(0,I2C_SDA_PIN);
    //SDA = 0;
    WAIT_NOP(3);
	vAHI_DioSetOutput(I2C_SDA_PIN,0);
    //SDA = 1;
    WAIT_NOP(3);
}


void HAL_I2C_ACK(bool bAck)
{
    DATA_PIN_OUTPUT();
	if(bAck)
	{
		vAHI_DioSetOutput(0,I2C_SDA_PIN);
	}
	else
	{
		vAHI_DioSetOutput(I2C_SDA_PIN,0);
	}
    //SDA = !bAck;
    WAIT_NOP(3);
	vAHI_DioSetOutput(I2C_SCL_PIN,0);
    //SCL = 1;
    WAIT_NOP(3);
	vAHI_DioSetOutput(0,I2C_SCL_PIN);
    //SCL = 0;
    WAIT_NOP(3);
}


bool HAL_I2C_SendByte(uint8 u8Byte)
{
    volatile uint8 i;
    bool bAck;

    DATA_PIN_OUTPUT();
    WAIT_NOP(3);

    for (i=0; i<8; i++)
    {
        // MSB
        if ((u8Byte << i) & 0x80)
        {
			vAHI_DioSetOutput(I2C_SDA_PIN,0);
			//SDA = 1;
        }
        else
        {
			vAHI_DioSetOutput(0,I2C_SDA_PIN);
			//SDA = 0;
        }
        WAIT_NOP(3);
		vAHI_DioSetOutput(I2C_SCL_PIN,0);
        //SCL = 1;
        WAIT_NOP(3);
		vAHI_DioSetOutput(0,I2C_SCL_PIN);
        //SCL = 0;
        WAIT_NOP(3);
    }

	vAHI_DioSetOutput(I2C_SDA_PIN,0);
	//SDA = 1;
    DATA_PIN_INPUT();
    WAIT_NOP(3);
	vAHI_DioSetOutput(I2C_SCL_PIN,0);
	//SCL = 1;
    WAIT_NOP(3);

	if(u32AHI_DioReadInput() & I2C_SDA_PIN)
    //if (SDA == 1)
        bAck = FALSE;
    else
        bAck = TRUE;

	vAHI_DioSetOutput(0,I2C_SCL_PIN);
	//SCL = 0;
    WAIT_NOP(3);

    return bAck;
}


uint8 HAL_I2C_RecvByte(void)
{
    uint8 i;
    uint8 u8Byte = 0;

    DATA_PIN_INPUT();

    for(i=0; i<8; i++)
    {
		vAHI_DioSetOutput(0,I2C_SCL_PIN);
		//SCL = 0;
        WAIT_NOP(3);
		vAHI_DioSetOutput(I2C_SCL_PIN,0);
        //SCL = 1;
        WAIT_NOP(3);

        u8Byte <<= 1;

        //if(SDA == 1)
        if(u32AHI_DioReadInput() & I2C_SDA_PIN)
            u8Byte |= 1;
    }
	vAHI_DioSetOutput(0,I2C_SCL_PIN);
    //SCL = 0;
    WAIT_NOP(3);

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
    HAL_I2C_ACK(FALSE);

    HAL_I2C_Stop();

    return u8Byte;
}

/*uint8 HAL_I2C_ReadBytes(uint8 u8Addr,uint8 u8Reg,uint8* pbuf,uint8 u8len)
{
    uint8 i;
    HAL_I2C_Start();
    HAL_I2C_SendByte(u8Addr | I2C_WRITE);
    HAL_I2C_SendByte(u8Reg);
    HAL_I2C_Start();
    HAL_I2C_SendByte(u8Addr | I2C_READ);
    for(i=0;i++;i<u8len)
    {
        *(pbuf+i) = HAL_I2C_RecvByte();
        if(i < (u8len -1))
            HAL_I2C_ACK(TRUE);
    }
    HAL_I2C_ACK(FALSE);
    HAL_I2C_Stop();
    return *pbuf;
}*/



