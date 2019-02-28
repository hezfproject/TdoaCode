#ifndef _HAL_IO_I2C_H_
#define _HAL_IO_I2C_H_

#include "cc_def.h"
#include "CommonTypes.h"

#define SDA             GPIO_Pin_11
#define SCL             GPIO_Pin_10

#define HIGH(pin)       GPIO_SetBits(GPIOB, pin)
#define LOW(pin)        GPIO_ResetBits(GPIOB, pin)
#define GET_INPUT(pin)        GPIO_ReadInputDataBit(GPIOB, pin)

void HAL_I2C_Init(void);

void HAL_I2C_WriteReg(uint_8 u8I2cAddr, uint_8 u8Reg, uint_8 u8Byte);


#endif

