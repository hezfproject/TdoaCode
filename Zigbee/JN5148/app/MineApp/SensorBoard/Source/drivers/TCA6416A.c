/*
 * TCA6416A.c
 *
 *  Created on: 2011-4-12
 *      Author: Dong Biwen
 */

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <AppHardwareApi.h>
#include "TCA6416A.h"

/****************************************************************************
 *
 * NAME: TCA6416A_Write_Regester
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
PUBLIC void TCA6416A_Write_Regester (uint8 command_regester,uint16 data_regester)
{
    vAHI_SiMasterWriteSlaveAddr(TCA6416A_ADDRESS>>1,    //ADS1100 IIC address 1001000_R/W
    						  FALSE);  			  //1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,      //Start followed by Write
    					 E_AHI_SI_NO_STOP_BIT,
    					 E_AHI_SI_NO_SLAVE_READ,
    					 E_AHI_SI_SLAVE_WRITE,
    					 E_AHI_SI_SEND_NACK,
    					 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy

    //if(bAHI_SiMasterCheckRxNack())        {return;} // check to see if we get an ACK back
    //if(bAHI_SiMasterPollArbitrationLost()){return;} // check to see if anyone else has taken the bus

    vAHI_SiMasterWriteData8(command_regester);  //
    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Only write
    				     E_AHI_SI_NO_STOP_BIT,
    				     E_AHI_SI_NO_SLAVE_READ,
    				     E_AHI_SI_SLAVE_WRITE,
    				     E_AHI_SI_SEND_NACK,
    				     E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    //if(bAHI_SiMasterCheckRxNack())        {return;} // check to see if we get an ACK back
    //if(bAHI_SiMasterPollArbitrationLost()){return;} // check to see if anyone else has taken the bus

    vAHI_SiMasterWriteData8((uint8)data_regester);
    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   // Only write
    					     E_AHI_SI_NO_STOP_BIT,
    					     E_AHI_SI_NO_SLAVE_READ,
    					     E_AHI_SI_SLAVE_WRITE,
    					     E_AHI_SI_SEND_NACK,
    					     E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    //if(bAHI_SiMasterCheckRxNack())        {return;} // check to see if we get an ACK back
    //if(bAHI_SiMasterPollArbitrationLost()){return;} // check to see if anyone else has taken the bus

    vAHI_SiMasterWriteData8((uint8)(data_regester>>8));
    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   // write followed stop
    					     E_AHI_SI_STOP_BIT,
    					     E_AHI_SI_NO_SLAVE_READ,
    					     E_AHI_SI_SLAVE_WRITE,
    					     E_AHI_SI_SEND_NACK,
    					     E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
}

/****************************************************************************
 *
 * NAME: TCA6416A_Read_Regester
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
PUBLIC uint16 TCA6416A_Read_Regester (uint8 command_regester)
{
    uint16 Read_Value = 0;
    uint16 Read_Value_temp = 0;
    vAHI_SiMasterWriteSlaveAddr(TCA6416A_ADDRESS>>1,    //ADS1100 IIC address 1001000_R/W
    					  FALSE);  			  //1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,      //Start followed by Write
        				 E_AHI_SI_NO_STOP_BIT,
        				 E_AHI_SI_NO_SLAVE_READ,
        				 E_AHI_SI_SLAVE_WRITE,
        				 E_AHI_SI_SEND_NACK,
        				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    //if(bAHI_SiMasterCheckRxNack())        {return 0;} // check to see if we get an ACK back
    //if(bAHI_SiMasterPollArbitrationLost()){return 0;} // check to see if anyone else has taken the bus

    vAHI_SiMasterWriteData8(command_regester);      //
    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Only write
        				 E_AHI_SI_NO_STOP_BIT,
        				 E_AHI_SI_NO_SLAVE_READ,
        				 E_AHI_SI_SLAVE_WRITE,
        				 E_AHI_SI_SEND_NACK,
        				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    //if(bAHI_SiMasterCheckRxNack())        {return 0;} // check to see if we get an ACK back
    //if(bAHI_SiMasterPollArbitrationLost()){return 0;} // check to see if anyone else has taken the bus

    vAHI_SiMasterWriteSlaveAddr(TCA6416A_ADDRESS>>1,//ADS1100 IIC address 1001000_R/W
    					  TRUE);  			  //1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,      //Start followed by Write
        				 E_AHI_SI_NO_STOP_BIT,
        				 E_AHI_SI_NO_SLAVE_READ,
        				 E_AHI_SI_SLAVE_WRITE,
        				 E_AHI_SI_SEND_NACK,
        				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    //if(bAHI_SiMasterCheckRxNack())        {return 0;} // check to see if we get an ACK back
    //if(bAHI_SiMasterPollArbitrationLost()){return 0;} // check to see if anyone else has taken the bus

    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Read Only
        				 E_AHI_SI_NO_STOP_BIT,
        				 E_AHI_SI_SLAVE_READ,
        				 E_AHI_SI_NO_SLAVE_WRITE,
        				 E_AHI_SI_SEND_ACK,       //Send ACK
        				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    Read_Value = u8AHI_SiMasterReadData8();

    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Read followed by Stop
        				 E_AHI_SI_STOP_BIT,
        				 E_AHI_SI_SLAVE_READ,
        				 E_AHI_SI_NO_SLAVE_WRITE,
        				 E_AHI_SI_SEND_NACK,      //Send NACK
        				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());   // wait while busy
    Read_Value_temp = u8AHI_SiMasterReadData8();
    Read_Value = (Read_Value_temp<<8) + Read_Value;
    return Read_Value;
}

PUBLIC void TCA6416A_Init (void)
{
    TCA6416A_Write_Regester (0x06,0x0000);       //  Write configuration register,configured port1 and port0 as a Output port
}

PUBLIC void TCA6416A_Write_Port (uint16 port_date)
{
    TCA6416A_Write_Regester (0x02,port_date);    //  Write output port register
}







