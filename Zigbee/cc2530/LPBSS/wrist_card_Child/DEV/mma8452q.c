#include <types.h>
#include <hal_mcu.h>
#include "mma8452q.h"
#include "hal_io_iic.h"


/***********************************************************************************************\
* Public functions
\***********************************************************************************************/

/*********************************************************\
* Put MMA845xQ into Active Mode
\*********************************************************/
void MMA845x_Active(void)
{
    uint8 u8Cmd;

    u8Cmd = HAL_I2C_ReadReg(MMA845x_I2C_ADDRESS, CTRL_REG1) | ACTIVE_MASK;
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG1, u8Cmd);
}

/*********************************************************\
* Put MMA845xQ into Standby Mode
\*********************************************************/
void MMA845x_Standby(void)
{
    uint8 u8RegVal;
    /*
    **  Read current value of System Control 1 Register.
    **  Put sensor into Standby Mode.
    **  Return with previous value of System Control 1 Register.
    */
    u8RegVal = HAL_I2C_ReadReg(MMA845x_I2C_ADDRESS, CTRL_REG1);
    u8RegVal &= ~ACTIVE_MASK;
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG1, u8RegVal);
}

/*********************************************************\
* Initialize MMA845xQ
\*********************************************************/
void MMA845x_Init(void)
{
    UINT8 u8RegVal;

    u8RegVal = MMA845x_ReadReg(WHO_AM_I_REG);
    while (MMA8451Q_ID != u8RegVal
        && MMA8452Q_ID != u8RegVal
        && MMA8453Q_ID != u8RegVal)
      u8RegVal = MMA845x_ReadReg(WHO_AM_I_REG);

    MMA845x_Standby();

    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG1, DATA_RATE_80MS | FREAD_MASK);
    //HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, HP_FILTER_CUTOFF_REG, PULSE_HPF_BYP_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG2, SMODS_MASK | MODS_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG3, WAKE_TRANS_MASK | PP_OD_MASK | WAKE_PULSE_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG4, INT_EN_PULSE_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG5, INT_CFG_PULSE_MASK); //;INT_CFG_PULSE_MASK
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, FF_MT_CFG_1_REG, ELE_MASK | ZEFE_MASK | YEFE_MASK | XEFE_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, FT_MT_THS_1_REG, 0x09);
	HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, FF_MT_COUNT_1_REG, 0x04);	//;*80ms
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, TRANSIENT_CFG_REG, TELE_MASK | ZTEFE_MASK | YTEFE_MASK | XTEFE_MASK | HPF_BYP_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, TRANSIENT_THS_REG, 0x15);//;17
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, PULSE_CFG_REG, PELE_BIT|XSPEFE_BIT|YSPEFE_BIT|ZSPEFE_BIT);
	HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, PULSE_THSX_REG, 0x20);
	HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, PULSE_THSY_REG, 0x20);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, PULSE_THSZ_REG, 0x20);
	HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, PULSE_TMLT_REG, 0x08);//;80ms
	HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, PULSE_LTCY_REG, 0x0A);//;*160ms
	HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, PULSE_WIND_REG, 0x20);//;
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, XYZ_DATA_CFG_REG, FULL_SCALE_8G);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, HP_FILTER_CUTOFF_REG, PULSE_LPF_EN_MASK);

    MMA845x_Active();
}

uint8 MMA845x_ReadReg(uint8 u8Reg)
{
    return HAL_I2C_ReadReg(MMA845x_I2C_ADDRESS, u8Reg);
}

void MMA845x_WriteReg(UINT8 u8Reg, UINT8 u8Byte)
{
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, u8Reg, u8Byte);
}

