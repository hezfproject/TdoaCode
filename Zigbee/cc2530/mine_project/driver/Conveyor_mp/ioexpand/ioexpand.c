#include "ioexpand.h"
#include "i2c.h"
#include "ZComDef.h"

#define SLAVE_H_ADDRESS 0x21
#define SLAVE_L_ADDRESS 0x20

typedef enum
{
    PORT0_READ = 0x00,
    PORT1_READ,
    PORT0_WRITE,
    PORT1_WRITE,
    PORT0_INVERSION,
    PORT1_INVERSION,
    PORT0_CONFIG,
    PORT1_CONFIG,
} io_command_t;

void ioexpand_init(void)
{
   // I2C_Init();
}

uint8 ioexpand_write(uint8 p0_val, uint8 p1_val)
{
    uint8 slaveaddr = SLAVE_L_ADDRESS;
    uint8 command = PORT0_WRITE;
    uint8 data[2];
    data[0] = p0_val;
    data[1] = p1_val;
    uint8 ret;
    I2C_Init();
    if(I2C_Puts(slaveaddr, &command, 1, data, 2) == 0)
    {
        ret= ZSuccess;
    }
    else
    {
        ret=ZFailure;
    }
    I2C_Close();
    return ret;
}

uint8 ioexpand_read(uint8* p0_val,  uint8* p1_val)
{

    uint8 slaveaddr = SLAVE_L_ADDRESS;
    uint8 command = PORT0_READ;

    uint8 data[2];
    uint8 ret;
    I2C_Init();
    if(I2C_Gets(slaveaddr, &command, 1, data, 2) == 0)
    {
        *p0_val = data[0];
        *p1_val = data[1];
        ret= ZSuccess;
    }
    else
    {
        ret= ZFailure;
    }
    I2C_Close();
    return ret;
}

uint8 ioexpand_setdir(uint8 p0_val, uint8 p1_val)
{
    uint8 slaveaddr = SLAVE_L_ADDRESS;
    uint8 command = PORT0_CONFIG;
    uint8 data[2];
    data[0] = p0_val;
    data[1] = p1_val;
    uint8 ret;
    I2C_Init();

    if(I2C_Puts(slaveaddr, &command, 1, data, 2) == 0)
    {
        ret= ZSuccess;
    }
    else
    {
        ret= ZFailure;
    }
    I2C_Close();
    return ret;
}

uint8 ioexpand_getdir(uint8* p0_val, uint8* p1_val)
{

    uint8 slaveaddr = SLAVE_L_ADDRESS;
    uint8 command = PORT0_CONFIG;

    uint8 data[2];
    uint8 ret;
    I2C_Init();

    if(I2C_Gets(slaveaddr, &command, 1, data, 2) == 0)
    {
        *p0_val = data[0];
        *p1_val = data[1];
        ret= ZSuccess;
    }
    else
    {
        ret= ZFailure;
    }
    I2C_Close();
    return ret;

}

uint8 ioexpand_inv(uint8 p0_val, uint8 p1_val)
{
    uint8 slaveaddr = SLAVE_L_ADDRESS;
    uint8 command = PORT0_INVERSION;
    uint8 data[2];
    data[0] = p0_val;
    data[1] = p1_val;
    uint8 ret;
    I2C_Init();

    if(I2C_Puts(slaveaddr, &command, 1, data, 2) == 0)
    {
        ret= ZSuccess;
    }
    else
    {
        ret= ZFailure;
    }
    I2C_Close();
    return ret;

}


