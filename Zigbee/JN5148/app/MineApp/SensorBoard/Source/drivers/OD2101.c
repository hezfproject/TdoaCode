/*
 * OD2101.c
 *
 *  Created on: 2011-4-15
 *      Author: Dong Biwen
 */

#include <AppHardwareApi.h>
#include "MainBroad.h"
#include "OD2101.h"

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
//char OD2101_FIFO_UATR [64];
//char OD2101_FIFO_I2C [64];

/****************************************************************************
 *
 * NAME: OD2101_Write_Command
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void OD2101_Write_Command (uint8 u8Command)
{
    vAHI_SiMasterWriteSlaveAddr(OD2101_ADRESS>>1,   // ADS1100 IIC address 1001000_R/W
    					  FALSE);  			  // 1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,      // Start followed by Write
    				 E_AHI_SI_NO_STOP_BIT,
    				 E_AHI_SI_NO_SLAVE_READ,
    				 E_AHI_SI_SLAVE_WRITE,
    				 E_AHI_SI_SEND_NACK,
    				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
    if(bAHI_SiMasterCheckRxNack())        {return ;} // check to see if we get an ACK back
    if(bAHI_SiMasterPollArbitrationLost()){return ;} // check to see if anyone else has taken the bus

    vAHI_SiMasterWriteData8(u8Command);               // ADS1100 ConfigReg BSY 0 0 SC DR1 DR0 PGA1 PGA0
    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,     // Write only
    				 E_AHI_SI_NO_STOP_BIT,
    				 E_AHI_SI_NO_SLAVE_READ,
    				 E_AHI_SI_SLAVE_WRITE,
    				 E_AHI_SI_SEND_NACK,
    				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
    if(bAHI_SiMasterCheckRxNack())        {return ;} // check to see if we get an ACK back
    if(bAHI_SiMasterPollArbitrationLost()){return ;} // check to see if anyone else has taken the bus
}

/****************************************************************************
 *
 * NAME: OD2101_Write_Regester
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void OD2101_Write_Regester (uint8 u8Command,uint8 u8regester)
{
    vAHI_SiMasterWriteSlaveAddr(OD2101_ADRESS>>1,   // OD2101_ADRESS 0x50
    					  FALSE);  			  // 1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,      // Start followed by Write
    				 E_AHI_SI_NO_STOP_BIT,
    				 E_AHI_SI_NO_SLAVE_READ,
    				 E_AHI_SI_SLAVE_WRITE,
    				 E_AHI_SI_SEND_NACK,
    				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
    if(bAHI_SiMasterCheckRxNack())        {return ;} // check to see if we get an ACK back
    if(bAHI_SiMasterPollArbitrationLost()){return ;} // check to see if anyone else has taken the bus

    vAHI_SiMasterWriteData8(u8Command);               // ADS1100 ConfigReg BSY 0 0 SC DR1 DR0 PGA1 PGA0
    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,     // Write only
    				 E_AHI_SI_NO_STOP_BIT,
    				 E_AHI_SI_NO_SLAVE_READ,
    				 E_AHI_SI_SLAVE_WRITE,
    				 E_AHI_SI_SEND_NACK,
    				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
    if(bAHI_SiMasterCheckRxNack())        {return ;} // check to see if we get an ACK back
    if(bAHI_SiMasterPollArbitrationLost()){return ;} // check to see if anyone else has taken the bus

    vAHI_SiMasterWriteData8(u8regester);              // ADS1100 ConfigReg BSY 0 0 SC DR1 DR0 PGA1 PGA0
    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,     // Write flowed by stop
    				 E_AHI_SI_STOP_BIT,
    				 E_AHI_SI_NO_SLAVE_READ,
    				 E_AHI_SI_SLAVE_WRITE,
    				 E_AHI_SI_SEND_NACK,
    				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
    if(bAHI_SiMasterCheckRxNack())        {return ;} // check to see if we get an ACK back
    if(bAHI_SiMasterPollArbitrationLost()){return ;} // check to see if anyone else has taken the bus
}

/****************************************************************************
 *
 * NAME: OD2101_Read_Regester
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE uint8 OD2101_Read_Regester (uint8 u8Command)
{ 
    uint8 i;
    OD2101_Write_Command (u8Command);

    vAHI_SiMasterWriteSlaveAddr(OD2101_ADRESS>>1,     // OD2101_ADRESS (0x50)
    					  TRUE);  			    // 1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,        // Start followed by Write
        				 E_AHI_SI_NO_STOP_BIT,
        				 E_AHI_SI_NO_SLAVE_READ,
        				 E_AHI_SI_SLAVE_WRITE,
        				 E_AHI_SI_SEND_NACK,
        				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
    if(bAHI_SiMasterCheckRxNack())        {return 0;} // check to see if we get an ACK back
    if(bAHI_SiMasterPollArbitrationLost()){return 0;} // check to see if anyone else has taken the bus

    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,     //Read followed by Stop
        				 E_AHI_SI_STOP_BIT,
        				 E_AHI_SI_SLAVE_READ,
        				 E_AHI_SI_NO_SLAVE_WRITE,
        				 E_AHI_SI_SEND_NACK,         //Send NACK
        				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
    i = u8AHI_SiMasterReadData8();
    return i;
}

/****************************************************************************
 *
 * NAME: OD2101_Write
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ***************************************************************************
PUBLIC bool_t OD2101_Write(char *date_pt,uint16 data_num)
{
  uint8 i;
  OD2101_Write_Command (0x00);
  for(i = 0;i < data_num;i++)
  {
   vAHI_SiMasterWriteData8(*(date_pt));
   if(i==(data_num-1))
   {
   bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Write followed with stop
						  E_AHI_SI_STOP_BIT,
						  E_AHI_SI_NO_SLAVE_READ,
						  E_AHI_SI_SLAVE_WRITE,
						  E_AHI_SI_SEND_NACK,
						  E_AHI_SI_NO_IRQ_ACK);
   }
   else
   {
   bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   // Only write
						  E_AHI_SI_NO_STOP_BIT,
						  E_AHI_SI_NO_SLAVE_READ,
						  E_AHI_SI_SLAVE_WRITE,
						  E_AHI_SI_SEND_ACK,
						  E_AHI_SI_NO_IRQ_ACK);}
   while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
   if(bAHI_SiMasterCheckRxNack())        {return 0;} // check to see if we get an ACK back
   if(bAHI_SiMasterPollArbitrationLost()){return 0;} // check to see if anyone else has taken the bus
   date_pt++;
  }
  return (1);
}*/

/****************************************************************************
 *
 * NAME: OD2101_Write_Byte
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PUBLIC bool_t OD2101_Write_Byte(char date)
{
    OD2101_Write_Command (0x00);
    vAHI_SiMasterWriteData8(date);
    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,     //Write followed with stop
        				 E_AHI_SI_STOP_BIT,
        				 E_AHI_SI_NO_SLAVE_READ,
        				 E_AHI_SI_SLAVE_WRITE,
        				 E_AHI_SI_SEND_NACK,
        				 E_AHI_SI_NO_IRQ_ACK);
    while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
    if(bAHI_SiMasterCheckRxNack())        {return 0;} // check to see if we get an ACK back
    if(bAHI_SiMasterPollArbitrationLost()){return 0;} // check to see if anyone else has taken the bus
    return (1);
}

/****************************************************************************
 *
 * NAME: OD2101_Read
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ***************************************************************************
PUBLIC bool_t OD2101_Read(char *date_pt,uint16 data_num)
{
  uint8 i;
  OD2101_Write_Command (0x00);
  vAHI_SiMasterWriteSlaveAddr(OD2101_ADRESS>>1,   // ADS1100 IIC address 1001000_R/W
  							  TRUE);  			  // 1.TRUE - configure a read 2.FALSE - configure a write
  bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,      // Start followed by Write
  						 E_AHI_SI_NO_STOP_BIT,
  						 E_AHI_SI_NO_SLAVE_READ,
  						 E_AHI_SI_SLAVE_WRITE,
  						 E_AHI_SI_SEND_NACK,
  						 E_AHI_SI_NO_IRQ_ACK);
  while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
  if(bAHI_SiMasterCheckRxNack())        {return 0;} // check to see if we get an ACK back
  if(bAHI_SiMasterPollArbitrationLost()){return 0;} // check to see if anyone else has taken the bus

  for(i = 0;i < data_num;i++)
  {
   if(i == (data_num - 1))
   {
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   // Read followed Stop
						   E_AHI_SI_STOP_BIT,
						   E_AHI_SI_SLAVE_READ,
					       E_AHI_SI_NO_SLAVE_WRITE,
						   E_AHI_SI_SEND_NACK,
						   E_AHI_SI_NO_IRQ_ACK);
   }
   else
   {
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,   //Read only
						   E_AHI_SI_NO_STOP_BIT,
						   E_AHI_SI_SLAVE_READ,
						   E_AHI_SI_NO_SLAVE_WRITE,
						   E_AHI_SI_SEND_ACK,       //Send ACK
						   E_AHI_SI_NO_IRQ_ACK);
   }
   while(bAHI_SiMasterPollTransferInProgress());     // wait while busy
   OD2101_FIFO_UATR [i] = u8AHI_SiMasterReadData8();
   date_pt++;
  }
  return (1);
}*/

/****************************************************************************
 *
 * NAME: vNum2String
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vNumString(uint32 u32Number, uint8 u8Base)
{
    char buf[33];
    char *p = buf + 33;
    uint32 c, n;

    *--p = '\0';
    do {
        n = u32Number / u8Base;
        c = u32Number - (n * u8Base);
        if (c < 10) {
            *--p = '0' + c;
        } else {
            *--p = 'a' + (c - 10);
        }
        u32Number /= u8Base;
    } while (u32Number != 0);

    while (*p){
        OD2101_Write_Byte(*p);
        p++;
    }

    return;
}

/****************************************************************************
 *
 * NAME: OD2101_vPrintf
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PUBLIC void OD2101_vPrintf(const char *fmt, ...)
{
    char *bp = (char *)fmt;
    va_list ap;
    char c;
    char *p;
    int32 i;

    va_start(ap, fmt);

    while ((c = *bp++)) {
        if (c != '%') {
            if (c == '\n'){
                OD2101_Write_Byte('\n');
                OD2101_Write_Byte('\r');
            } else {
                OD2101_Write_Byte(c);
            }
            continue;
        }

        switch ((c = *bp++)) {

        /* %d - show a decimal value */
        case 'd':
            vNumString(va_arg(ap, uint32), 10);
            break;

        /* %x - show a value in hex */
        case 'x':
            OD2101_Write_Byte('0');
            OD2101_Write_Byte('x');
            vNumString(va_arg(ap, uint32), 16);
            break;

        case 'X':
            vNumString(va_arg(ap, uint32), 16);
            break;

        /* %b - show a value in binary */
        case 'b':
            OD2101_Write_Byte('0');
            OD2101_Write_Byte('b');
            vNumString(va_arg(ap, uint32), 2);
            break;

        /* %c - show a character */
        case 'c':
            OD2101_Write_Byte(va_arg(ap, int));
            break;

        case 'i':
            i = va_arg(ap, int32);
            if(i < 0){
                OD2101_Write_Byte('-');
                vNumString((~i)+1, 10);
            } else {
                vNumString(i, 10);
            }
            break;

        /* %s - show a string */
        case 's':
            p = va_arg(ap, char *);
            do {
                OD2101_Write_Byte(*p++);
            } while (*p);
            break;

        /* %% - show a % character */
        case '%':
            OD2101_Write_Byte('%');
            break;

        /* %something else not handled ! */
        default:
            OD2101_Write_Byte('?');
            break;
        }
    }
    return;
}


/****************************************************************************
 *
 * NAME: OD2101_Read
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PUBLIC void OD2101_Init(void)
{
    OD2101_Write_Regester(OD2101_CTRL,(((OD2101_Read_Regester(OD2101_CTRL))&0xF0)|0x0F)); //Configured UART as 115200 baud rate
}
