
#ifndef _HAL_IO_IIC_H_
#define _HAL_IO_IIC_H_

#include <types.h>

#define BV(n)      (1 << (UINT32)(n))



#define SDA             P1_3
#define SCL             P1_2
#define MMA_INT         P1_4

#define I2C_SDA_PIN     BV(3)
#define I2C_SCL_PIN     BV(2)

void HAL_I2C_Init(void);

void HAL_I2C_WriteReg(uint8 u8I2cAddr, uint8 u8Reg, uint8 u8Byte);

uint8 HAL_I2C_ReadReg(uint8 u8Addr, uint8 u8Reg);

#endif




