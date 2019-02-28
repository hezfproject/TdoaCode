//#include <types.h>
//#include <hal_mcu.h>
#include "mma8452q.h"
#include "hal_io_iic.h"
#include <jendefs.h>
#include <AppHardwareApi.h>



/***********************************************************************************************\
* Public functions
\***********************************************************************************************/

uint8 bInvalid = 1;


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


uint8 MMA845x_ChipCheck(void)
{
    uint8 u8RegVal = 0;
    uint8 timeout = 0;

    do {
        u8RegVal = HAL_I2C_ReadReg(MMA845x_I2C_ADDRESS, WHO_AM_I_REG);
    } while (MMA8451Q_ID != u8RegVal
            && MMA8452Q_ID != u8RegVal
            && MMA8453Q_ID != u8RegVal
            && ++timeout < 5);

    return bInvalid = (MMA8451Q_ID != u8RegVal
                    && MMA8452Q_ID != u8RegVal
                    && MMA8453Q_ID != u8RegVal);
}


/*********************************************************\
* Initialize MMA845xQ
\*********************************************************/
void MMA845x_Init(void)
{
    uint8 u8RegVal;
    MMA845x_ChipCheck();

    if (bInvalid)
        return;

    MMA845x_Standby();

    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG1, ASLP_RATE_80MS | (DATA_RATE_80MS) | (FREAD_MASK));
    //HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, HP_FILTER_CUTOFF_REG, PULSE_HPF_BYP_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG2, SLPE_MASK | SMODS_MASK | MODS_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG3, WAKE_TRANS_MASK | IPOL_MASK | WAKE_FF_MT_1_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG4, INT_EN_TRANS_MASK | INT_EN_FF_MT_1_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, CTRL_REG5, INT_CFG_TRANS_MASK | INT_CFG_FF_MT_1_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, FF_MT_CFG_1_REG, ELE_MASK  | ZEFE_MASK | YEFE_MASK | XEFE_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, FT_MT_THS_1_REG, 0x08);
	HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, FF_MT_COUNT_1_REG, 0x04);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, TRANSIENT_CFG_REG, TELE_MASK | ZTEFE_MASK | YTEFE_MASK | XTEFE_MASK | HPF_BYP_MASK);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, TRANSIENT_THS_REG, 17);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, XYZ_DATA_CFG_REG, FULL_SCALE_2G);
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, HP_FILTER_CUTOFF_REG, PULSE_LPF_EN_MASK);

    u8RegVal = MMA845x_ReadReg (0x11);
    u8RegVal |= 0x40;
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS,0x11, u8RegVal);

    u8RegVal = MMA845x_ReadReg(0x13);
    u8RegVal &= 0x3F;
    u8RegVal |= 0x00; //This does nothing additional and keeps bits [7:6] = 00
    u8RegVal |= 0x40; //Sets bits[7:6] = 01
    u8RegVal |= 0x80; //Sets bits[7:6] = 02PLBFZCOMP_Data| = 0xC0; //Sets bits[7:6] = 03
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS,0x13,u8RegVal); //Write in the updated Back/Front Angle

    u8RegVal = MMA845x_ReadReg(0x1C); //Read out contents of the register
    u8RegVal &= 0xF8; //Clear the last three bits of the register

    u8RegVal |= 0x00; //This does nothing additional but the Z-lockout selection will remain at 14бу
    u8RegVal |= 0x01; //Set the Z-lockout angle to 18бу
    //PLBFZCOMP_Data| = 0x02; //Set the Z-lockout angle to 21бу
    //PLBFZCOMP_Data| = 0x03; //Set the Z-lockout angle to 25бу
    //PLBFZCOMP_Data| = 0x04; //Set the Z-lockout angle to 29бу
    //PLBFZCOMP_Data| = 0x05; //Set the Z-lockout angle to 33бу
    //PLBFZCOMP_Data| = 0x06; //Set the Z-lockout angle to 37бу
    //PLBFZCOMP_Data| = 0x07; //Set the Z-lockout angle to 42бу
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS,0x13, u8RegVal); //Write in the updated Z-lockout angle


    u8RegVal = MMA845x_ReadReg(0x14);
    u8RegVal &= 0x07; //Clear the Threshold values
    u8RegVal |= (0x07)<<3; //Set Threshold to 15бу
    //u8RegVal |= (0x09)<<3; //Set Threshold to 20бу
    //u8RegVal |= (0x0C)<<3; //Set Threshold to 30бу
    //u8RegVal |= (0x0D)<<3; //Set Threshold to 35бу
    //u8RegVal |= (0x0F)<<3; //Set Threshold to 40бу
    //u8RegVal |= (0x10)<<3; //Set Threshold to 45бу
    //u8RegVal |= (0x13)<<3; //Set Threshold to 55бу
    //u8RegVal |= (0x14)<<3; //Set Threshold to 60бу
    //u8RegVal |= (0x17)<<3; //Set Threshold to 70бу
    //u8RegVal |= (0x19)<<3; //Set Threshold to 75бу
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS,0x14, u8RegVal);

    u8RegVal = MMA845x_ReadReg(0x14);
    u8RegVal &= 0xF8; //Clear the Hysteresis values
    u8RegVal |= 0x01; //Set Hysteresis to б└4бу
    //u8RegVal |= 0x02; //Set Threshold to б└7бу
    //u8RegVal |= 0x03; //Set Threshold to б└11бу
    //u8RegVal |= 0x04; //Set Threshold to б└14бу
    //u8RegVal |= 0x05; //Set Threshold to б└17бу
    //u8RegVal |= 0x06; //Set Threshold to б└21бу
    //u8RegVal |= 0x07; //Set Threshold to б└24бу
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS,0x14, u8RegVal);

    u8RegVal = MMA845x_ReadReg(0x2D); //Read out the contents of the register
    u8RegVal |= 0x10; //Set bit 4
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS,0x2D, u8RegVal); //Set the bit and write into CTRL_REG4

    u8RegVal = MMA845x_ReadReg(0x2E);
    u8RegVal &= 0xEF; //Clear bit 4 to choose the interrupt to route to INT2
    u8RegVal |= 0x10; //Set bit 4 to choose the interrupt to route to INT1
    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS,0x2E, u8RegVal); //Write in the interrupt routing selection

    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS,0x12, 0x05); //This sets the debounce counter to 100 ms at 50 Hz

    MMA845x_Active();
}

uint8 MMA845x_ReadReg(uint8 u8Reg)
{
    if (bInvalid)
        return 0xFF;

    return HAL_I2C_ReadReg(MMA845x_I2C_ADDRESS, u8Reg);
}

uint8 MMA845x_WriteReg(uint8 u8Reg, uint8 u8Byte)
{
    if (bInvalid)
        return 0xFF;

    HAL_I2C_WriteReg(MMA845x_I2C_ADDRESS, u8Reg, u8Byte);
    return 0;
}

