/**************************************************************************************************
  Filename:       Ammeter_IEC.c
  Protocal          IEC62056-21
  Revised:        $Date: 2010/06/03 06:03:02 $
  Revision:       $Revision: 1.1 $

  Description
  IEC62056-21 Device related operations, including Baud rate change, data transmit and stop operation
  
***************************************************************************************************/
#include "Ammeter_IEC.h"
#include "ZComDef.h"
#include "hal_uart.h"

static uint8 u8aIEC_serverDISC[12] = {0x7E, 0xA0, 0x0A, 0x00, 0x22, 0x18, 0xF1, 0x61, 0x53, 0x2D, 0x8D, 0x7E};
static uint8 u8aIEC_clientUA[12]      = {0x7E, 0xA0, 0x0A, 0x61, 0x00, 0x22, 0x18, 0xF1, 0x73, 0x12, 0x45, 0x7E};
static uint8 u8IEC_dataStart = 0x7E;
static uint8 u8IEC_dataEnd = 0x7E;

static uint8 transferBaudRate(uint8 data);


/**************************************************************************
 *  
 *  NAME : IEC_ParseFrame
 *
 *  DESCRIPTION:
 *  parse the Frame type.
 *  only need support type in teFrameType
 *
 *  PARAMETERS:
 *  frameData          len
 *
**************************************************************************/

/*FIXME, need check frame tail ???*/
teIECframeType IEC_ParseFrame(uint8 *frameData, uint8 len, uint8 nodeType, tsIEC_info* info)
{
	uint8 i = 0;
	uint8* cmd;

	if(info->protocol == D_IEC_PROTOCOL_E)
	{
		if(frameData[0] != u8IEC_dataStart) return E_IEC_FRAME_DATA_ERROR;
		if(frameData[len-1] != u8IEC_dataEnd) return E_IEC_FRAME_DATA_UNFINISHED;
		
		if(len != 12) return E_IEC_FRAME_DATA_FINISHED;

		cmd = nodeType==D_IEC_SERVER? u8aIEC_serverDISC : u8aIEC_clientUA;
		while(cmd[i] == frameData[i])
			i++;

		if(i != 12) return E_IEC_FRAME_DATA_FINISHED;

		return nodeType==D_IEC_SERVER? E_IEC_FRAME_SERVER_DISC: E_IEC_FRAME_CLIENT_UA;
	}
	
	if(nodeType == D_IEC_SERVER)
	{
		/* "/?" initializes a start request	*/	
		if(frameData[0] == 0x2F && frameData[1] == 0x3F)
		{
			return E_IEC_FRAME_SERVER_START;
		}

		/* 06 32 initializes a ACK*/
		/*FIXME, not compatible with IEC62056-21??? should be 06 30 ??? */
		if(frameData[0] == 0x06 && frameData[1] == 0x32)
		{
			info->com_baud_rate = transferBaudRate(frameData[2]);
			info->baud_switch = 1;
			return E_IEC_FRAME_SERVER_ACK;
		}
	}
	else
	{
		if(frameData[0] == 0x2F)
		{			
			if(info){ 
				info->max_baud_rate = transferBaudRate(frameData[4]);
				info->device_id[0] = frameData[1];
				info->device_id[1] = frameData[2];
				info->device_id[2] = frameData[3];
			}
			
			return E_IEC_FRAME_CLIENT_ID;
		}		
	}

	return E_IEC_FRAME_DATA_ERROR;	
}

static uint8 transferBaudRate(uint8 data)
{
	uint8 baud_rate;
	switch (data)
	{
		case '0':
			baud_rate = HAL_UART_BR_300;
			break;
		case '1':
			baud_rate = HAL_UART_BR_600;
			break;
		case '2':
			baud_rate = HAL_UART_BR_1200;
			break;
		case '3':
			baud_rate = HAL_UART_BR_2400;
			break;
		case '4':
			baud_rate = HAL_UART_BR_4800;
			break;
		case '5':
			baud_rate = HAL_UART_BR_9600;
			break;
		case '6':
			baud_rate = HAL_UART_BR_19200;				
			break;
		default:
			/*FIXME, 7~9 reserved for extensions*/
			baud_rate = HAL_UART_BR_300;
			break;
	}
	
	return baud_rate;
}


/* change baud rate*/
void IEC_ChangeBaudRate(uint8 port, tsIEC_info* info)
{
	halUARTIoctl_t UARTconfig;

	if(info->baud_switch  == 1)
	{			
		HalUARTIoctl(port, HAL_UART_IOCTL_GET, &UARTconfig);
		UARTconfig.baudRate = info->com_baud_rate;
		HalUARTIoctl(port, HAL_UART_IOCTL_SET, &UARTconfig);
		info->protocol = D_IEC_PROTOCOL_E;

		info->baud_switch  = 0;
	}
}

/*************************************************************************
*  end of file
*************************************************************************/
