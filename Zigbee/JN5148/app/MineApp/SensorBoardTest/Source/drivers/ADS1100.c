/*
 * ADS1100.c
 *
 * Created on: 2011-4-2
 * Author: Dong Biwen
 */
#include <AppHardwareApi.h>
#include <math.h>
#include "MainBroad.h"
#include "ADS1100.h"
#include "OD2101.h"
#include "RS485.h"
#include "JN5148_util.h"

#define ABS(x)     ((x) >= 0 ? (x) : (-1)*(x))

static union  ADReg32 ADS1100_Value_State;

/****************************************************************************
 *
 * NAME: ASD1100_SetChannel
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 * None.           channel
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * ****************/
PUBLIC void ADS1100_SetChannel (uint8 u8channel)
{
    if(gCurrent_Channel == gCurrent_Channel_old)
        return;
    else
        gCurrent_Channel_old = gCurrent_Channel;
    vAHI_DioSetDirection(0,0x00040201);   //Enable output DIO_0,DIO_9,DIO_18
	switch(u8channel)
	{
        case MULTIPLEXERS_ONE :   vAHI_DioSetOutput(0x00000000, 0x00040201); //DIO_18 = OFF,DIO_9 = OFF,DIO_0 = OFF
        break;
        case MULTIPLEXERS_TWO :   vAHI_DioSetOutput(0x00000001, 0x00040200); //DIO_18 = OFF,DIO_9 = OFF,DIO_0 = ON
        break;
        case MULTIPLEXERS_THREE : vAHI_DioSetOutput(0x00000200, 0x00040001); //DIO_18 = OFF,DIO_9 = ON ,DIO_0 = OFF
        break;
        case MULTIPLEXERS_FOUR :  vAHI_DioSetOutput(0x00000201, 0x00040000); //DIO_18 = OFF,DIO_9 = ON ,DIO_0 = ON
        break;
        case MULTIPLEXERS_FIVE :  vAHI_DioSetOutput(0x00040000, 0x00000201); //DIO_18 = ON ,DIO_9 = OFF,DIO_0 = OFF
        break;
        case MULTIPLEXERS_SIX :   vAHI_DioSetOutput(0x00040001, 0x00000200); //DIO_18 = ON ,DIO_9 = OFF,DIO_0 = ON
        break;
        case MULTIPLEXERS_SEVEN : vAHI_DioSetOutput(0x00040200, 0x00000001); //DIO_18 = ON ,DIO_9 = ON ,DIO_0 = OFF
        break;
        case MULTIPLEXERS_EIGHT : vAHI_DioSetOutput(0x00040201, 0x00000000); //DIO_18 = ON ,DIO_9 = ON ,DIO_0 = ON
        break;
        default:
        break;
	}
 }

/****************************************************************************
 *
 * NAME: ADS1100_Write
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 * None.           channel
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * ****************/
PUBLIC void ADS1100_Write (uint8 slave_address,uint8 slave_modify)
{
    vAHI_SiMasterWriteSlaveAddr(slave_address>>1,   //ADS1100 IIC address 1001000_R/W
    					  FALSE);  			  //1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,      //Start followed by Write
    				 E_AHI_SI_NO_STOP_BIT,
    				 E_AHI_SI_NO_SLAVE_READ,
    				 E_AHI_SI_SLAVE_WRITE,
    				 E_AHI_SI_SEND_NACK,
    				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy

    if(bAHI_SiMasterCheckRxNack())        {return;} // check to see if we get an ACK back
    if(bAHI_SiMasterPollArbitrationLost()){return;} // check to see if anyone else has taken the bus

    vAHI_SiMasterWriteData8(slave_modify);          // ADS1100 ConfigReg BSY 0 0 SC DR1 DR0 PGA1 PGA0
    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Write followed by Stop
    			     E_AHI_SI_STOP_BIT,
    			     E_AHI_SI_NO_SLAVE_READ,
    			     E_AHI_SI_SLAVE_WRITE,
    			     E_AHI_SI_SEND_NACK,
    			     E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    if(bAHI_SiMasterCheckRxNack())        {return;} // check to see if we get an ACK back
    if(bAHI_SiMasterPollArbitrationLost()){return;} // check to see if anyone else has taken the bus
}

/****************************************************************************
 *
 * NAME: ADS1100_Start
 *
 * DESCRIPTION:     启动ASD1100 配置为单次采样，采样速率8Sps
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * ****************/
PUBLIC void ADS1100_Start (void)
{
    ADS1100_Write (ADS1100_ADDRESS,ADS1100_MODIFY);
}

/****************************************************************************
 *
 * NAME: ADS1100_Init
 *
 * DESCRIPTION:     开机初始化，启动一次单次采样为ADS1100_ReadBlock获得初值
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * ****************/
PUBLIC void ADS1100_Init (void)
{
    ADS1100_Write (0x00,0x06);
    ADS1100_ReadSingle (0);
}

/****************************************************************************
 *
 * NAME: ADS1100_Read ()
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.            ADC convert data
 *
 * NOTES:
 *  ************************************************************************/
PUBLIC void ADS1100_Read (void)
{
    vAHI_SiMasterWriteSlaveAddr(0x48,               //ADS1100 IIC address (1001000_R/W)>>1
    						  TRUE);              //1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,      //Start followed by Write
    					 E_AHI_SI_NO_STOP_BIT,
    					 E_AHI_SI_NO_SLAVE_READ,
    					 E_AHI_SI_SLAVE_WRITE,
    					 E_AHI_SI_SEND_NACK,
    					 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
    if(bAHI_SiMasterCheckRxNack())        
    {
        vAHI_SiMasterDisable();
        vAHI_SiMasterConfigure( FALSE,  //1.Enable pulse suppression filter
    					    FALSE,  //2.Disable Serial Interface interruptEnable
    						0x07 ); //
        ADS1100_Value_State.Reg[0] = 0;
        ADS1100_Value_State.Reg[1] = 0;
        return ;                      // check to see if we get an ACK back
    } 
    if(bAHI_SiMasterPollArbitrationLost())
    {
        vAHI_SiMasterDisable();
        vAHI_SiMasterConfigure( FALSE,  //1.Enable pulse suppression filter
    					    FALSE,  //2.Disable Serial Interface interruptEnable
    						0x07 ); //
    	
        ADS1100_Value_State.Reg[0] = 0;
        ADS1100_Value_State.Reg[1] = 0;
        return ;                    // check to see if anyone else has taken the bus
    } 

    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Read Only
    					 E_AHI_SI_NO_STOP_BIT,
    					 E_AHI_SI_SLAVE_READ,
    					 E_AHI_SI_NO_SLAVE_WRITE,
    					 E_AHI_SI_SEND_ACK,       //Send ACK
    					 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    ADS1100_Value_State.Reg[0] = u8AHI_SiMasterReadData8();

    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Read Only
    					 E_AHI_SI_NO_STOP_BIT,
    					 E_AHI_SI_SLAVE_READ,
    					 E_AHI_SI_NO_SLAVE_WRITE,
    					 E_AHI_SI_SEND_ACK,       //Send ACK
    					 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    ADS1100_Value_State.Reg[1] = u8AHI_SiMasterReadData8();

    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Read followed by Stop
    					 E_AHI_SI_STOP_BIT,
    					 E_AHI_SI_SLAVE_READ,
    					 E_AHI_SI_NO_SLAVE_WRITE,
    					 E_AHI_SI_SEND_ACK,       //Send ACK
    					 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    ADS1100_Value_State.Reg[2] = u8AHI_SiMasterReadData8();
}

/****************************************************************************
 *
 * NAME: ADS1100_ReadSingle
 *
 * DESCRIPTION:     单次采样，此函数需要运行125ms
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 *  ****************/
PUBLIC uint16 ADS1100_ReadSingle (uint8 u8channel)
{
    ADS1100_SetChannel (u8channel);
    ADS1100_Start();
    do
    {
        ADS1100_Read ();
    } while((ADS1100_Value_State.C.AD_State&0x80));
    return ADS1100_Value_State.C.AD_Value;
}

/****************************************************************************
 *
 * NAME: ADS1100_ReadBlock
 *
 * DESCRIPTION:     采集8路数据将8路模拟量存储在 gAD_Data[]数组中
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 *  *************************************************************************/
PUBLIC void ADS1100_ReadBlock ()
{  
	 mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [7].value = 8888;//u16Big_To_Little(ABS((short)ADS1100_Value_State.C.AD_Value));
	 mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [0].value = 8888;//u16Big_To_Little(ABS((short)ADS1100_Value_State.C.AD_Value));
	 mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [1].value = 8888;//u16Big_To_Little(ABS((short)ADS1100_Value_State.C.AD_Value));
	 mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [2].value = 8888;//u16Big_To_Little(ABS((short)ADS1100_Value_State.C.AD_Value));
	 mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [3].value = 8888;//u16Big_To_Little(ABS((short)ADS1100_Value_State.C.AD_Value));
	 mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [4].value = 8888;//u16Big_To_Little(ABS((short)ADS1100_Value_State.C.AD_Value));
	 mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [5].value = 8888;//u16Big_To_Little(ABS((short)ADS1100_Value_State.C.AD_Value));
	 mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [6].value = 8888;//u16Big_To_Little(ABS((short)ADS1100_Value_State.C.AD_Value));
}

