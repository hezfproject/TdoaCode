
#ifndef _HAL_IO_IIC_H_
#define _HAL_IO_IIC_H_

//#include <types.h>
#include <jendefs.h>
#include <AppHardwareApi.h>


#define BV(n)      (1 << (UINT32)(n))



//#define SDA             P1_3
//#define SCL             P1_2

//#define I2C_SDA_PIN     BV(3)
//#define I2C_SCL_PIN     BV(2)

#define I2C_SDA_PIN      E_AHI_DIO11_INT
#define I2C_SCL_PIN      E_AHI_DIO12_INT 


PUBLIC void HAL_I2C_Init(void);

PUBLIC void HAL_I2C_WriteReg(uint8 u8I2cAddr, uint8 u8Reg, uint8 u8Byte);

PUBLIC uint8 HAL_I2C_ReadReg(uint8 u8Addr, uint8 u8Reg);

#endif




