/*
 * RS485.c
 *
 *  Created on: 2011-4-27
 *      Author: Administrator
 */
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <AppHardwareApi.h>
#include <malloc.h>
#include <stdlib.h>

#include "MainBroad.h"
#include "JN5148_util.h"
#include "RS485.h"
#include "OD2101.h"
#include "crc.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define MBUS_VERSION (01)             // 0-63
#define RELEASE_VER  (01)             //版本号
//#define MBUS_SLV_SENSOR_ADDRESS (1)   // 1-254
/****************************************************************************/
/***        Typedef                                                       ***/
/****************************************************************************/
typedef enum
{
	RX_LISTEN_STATE = 0,
    RX_CMD_STATE = 1,
    RX_ADDRESS_H_STATE = 2,
	RX_ADDRESS_L_STATE = 3,
    RX_DATA_LEN_H_STATE = 4,
    RX_DATA_LEN_L_STATE = 5,
    RX_CRC_H_STATE = 6,
    RX_CRC_L_STATE = 7,
} eModBus_state;
/*****************************************************************************
* Global variables
*
******************************************************************************/

static eModBus_state  eBus_state;
static mbus_hdr_mstr_t *mbus_hdr_mstr_p,mbus_hdr_mstr;
PUBLIC  mbus_hdr_slv_t_Temp *mbus_hdr_slv_p,mbus_hdr_slv;

PUBLIC uint8 TX_flag = 0;
PUBLIC uint8 fram_control = 0;


PUBLIC  uint8 MbusSlvSensorAddr;
PUBLIC  uint8 SensorAddr;

PUBLIC MAC_ExtAddr_s psMacAddr;
PUBLIC uint16 u16StationPanId;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vUartInterruptHandler(uint32 u32DeviceId, uint32 u32ItemBitmap);
PRIVATE void Senmac_vRxHandler(void);
PRIVATE void Senmac_vTimeoutHandler(void);
PRIVATE void Rx_State (uint8 u8Rx_data);

/****************************************************************************
 *
 * NAME: RS485_Init
 *
 * DESCRIPTION:
 *
 *
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void RS485_Init (void)
{
	mbus_hdr_mstr_p=&mbus_hdr_mstr;
	mbus_hdr_slv_p =&mbus_hdr_slv;
    //Set UART1
	vAHI_UartSetRTSCTS(E_AHI_UART_1,FALSE);
	vAHI_UartEnable(E_AHI_UART_1);
    vAHI_UartReset(E_AHI_UART_1, TRUE, TRUE);
    vAHI_UartReset(E_AHI_UART_1, FALSE, FALSE);

    //vAHI_UartSetBaudRate(E_AHI_UART_1,E_AHI_UART_RATE_115200);
    vAHI_UartSetBaudDivisor(E_AHI_UART_1,46);
    vAHI_UartSetClocksPerBit(E_AHI_UART_1,2);

	vAHI_UartSetControl( E_AHI_UART_1,
						 E_AHI_UART_ODD_PARITY,
						 E_AHI_UART_PARITY_DISABLE,  //Disable parity check
						 E_AHI_UART_WORD_LEN_8,      //Word length (in bits 8)
						 E_AHI_UART_2_STOP_BITS,     //FALSE - 2 stop bit
						 E_AHI_UART_RTS_LOW);        //TRUE - set RTS to high
	vAHI_UartSetInterrupt(E_AHI_UART_1,
			             FALSE,                      //Disable modem status interrupt
			             FALSE,						 //Disable Rx line status interrupt
			             FALSE,                      //Disable interrupt when Tx FIFO becomes empty
			             TRUE,						 //Enable interrupt when Rx data detected
			             E_AHI_UART_FIFO_LEVEL_8);   //Number of received bytes required to trigger an Rx data interrupt
    vAHI_Uart1RegisterCallback(vUartInterruptHandler);
    //Set UART0
}

/****************************************************************************
 *
 * NAME:vUartInterruptHandler
 *
 * DESCRIPTION:
 *
 *
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PRIVATE void vUartInterruptHandler(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
	if (u32DeviceId == E_AHI_DEVICE_UART1)
	{
		switch (u32ItemBitmap)
		{
		case E_AHI_UART_INT_RXDATA:
			Senmac_vRxHandler();
			break;
		case E_AHI_UART_INT_TIMEOUT:
			Senmac_vTimeoutHandler();
			break;
		default:
			break;
		}
	}
}
/****************************************************************************
 *
 * NAME: Senmac_vRxHandler
 *
 * DESCRIPTION:
 * Entry point for application from boot loader. Initializes system and runs
 * main loop.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PRIVATE void Senmac_vRxHandler(void)
{
	uint8 u8fifo_num,u8fifo_count,u8Rx_data;
	u8fifo_num = u8AHI_UartReadRxFifoLevel(E_AHI_UART_1);
    for (u8fifo_count = 0;u8fifo_count < u8fifo_num;u8fifo_count++)
    {
    	u8Rx_data = u8AHI_UartReadData (E_AHI_UART_1);
    	Rx_State (u8Rx_data);
    	//PrintfUtil_vPrintf("Current_RX_DATA: u8Rx_data %x State %d\r\n",u8Rx_data,(uint8)eBus_state);
    }
}
/****************************************************************************
 *
 * NAME:Senmac_vTimeoutHandler
 *
 * DESCRIPTION:
 *
 *
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PRIVATE void Senmac_vTimeoutHandler(void)
{
    uint8 u8fifo_num,u8fifo_count,u8Rx_data;
    u8fifo_num = u8AHI_UartReadRxFifoLevel(E_AHI_UART_1);
    for (u8fifo_count = 0;u8fifo_count < u8fifo_num;u8fifo_count++)
    {
        u8Rx_data = u8AHI_UartReadData (E_AHI_UART_1);
        Rx_State (u8Rx_data);
        //PrintfUtil_vPrintf("Current_RX_TIMEOUT: u8Rx_data %x State %d\r\n",u8Rx_data,(uint8)eBus_state);
    }
}
/****************************************************************************
 *
 * NAME: Rx_State
 *
 * DESCRIPTION:
 *
 *
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/

PRIVATE void Rx_State (uint8 u8Rx_data)
{
	static uint16 u16rx_slv_id = 0; //first send low byte or high byte ??
	static uint16 u16rx_crc = 0;    //first send low byte or high byte ??
	switch (eBus_state)
	{
	case RX_LISTEN_STATE:
		if (MBUS_GET_SLAVE_VERSION(u8Rx_data) == MBUS_VERSION)//SEQ==0 means ARM send
		{
			fram_control = u8Rx_data;
			mbus_hdr_mstr_p->frame_control = u8Rx_data;
			eBus_state = RX_CMD_STATE;
		}
	    break;
	case RX_CMD_STATE:
		if ((u8Rx_data == MBUS_CMD_CLR)||(u8Rx_data == MBUS_CMD_QRY))
        {
            mbus_hdr_mstr_p->cmd = u8Rx_data;
            eBus_state = RX_ADDRESS_H_STATE;
        }
		else
		{
		    eBus_state = RX_LISTEN_STATE;
		}
	    break;
	case RX_ADDRESS_H_STATE:
		 u16rx_slv_id &=0x0000;
		 u16rx_slv_id += u8Rx_data;
		 u16rx_slv_id <<= 8;
		 eBus_state = RX_ADDRESS_L_STATE;
	    break;
	case RX_ADDRESS_L_STATE:
		 u16rx_slv_id &=0xFF00;
		 u16rx_slv_id += u8Rx_data;
		 //PrintfUtil_vPrintf("SLV_ID1 %x \r\n",u16Big_To_Little(u16rx_slv_id));
		 SensorAddr = u16Big_To_Little(u16rx_slv_id);
		if((SensorAddr >= MbusSlvSensorAddr) &&(SensorAddr < (MbusSlvSensorAddr+8)))
		{
            mbus_hdr_mstr_p->slv_id =u16rx_slv_id;
            //PrintfUtil_vPrintf("SLV_ID2 %x \r\n", mbus_hdr_mstr_p->slv_id );
            eBus_state = RX_CRC_H_STATE;
		}
		else
		{
		    eBus_state = RX_LISTEN_STATE;
		}
	    break;
	case RX_CRC_H_STATE :
		u16rx_crc &=0x0000;
		u16rx_crc += u8Rx_data;
		u16rx_crc <<= 8;
		eBus_state = RX_CRC_L_STATE;
	    break;
	case RX_CRC_L_STATE :
		u16rx_crc &=0xFF00;
		u16rx_crc += u8Rx_data;
		//PrintfUtil_vPrintf("Receive_CRC: %x\r\n",u16Big_To_Little(u16rx_crc));
		if( CRC16((uint8 *)mbus_hdr_mstr_p,4,0xFFFF) == u16Big_To_Little(u16rx_crc))
		{
            TX_flag = 1;
            //Send_Pack (fram_control);
		}
		else
		{
		    ;//OD2101_vPrintf("CRC_Error \r\n");
		}
		eBus_state = RX_LISTEN_STATE;
	    break;
	default:
	    break;
	}
}

/****************************************************************************
 *
 * NAME: u16Big_To_Little
 *
 * DESCRIPTION:Convert Big Endian to Little Endian
 *
 *
 *
 * RETURNS:
 * Returns Convert Value.
 *
 ****************************************************************************/
PUBLIC uint16 u16Big_To_Little (uint16 value)
{
    uint16 *p;
    uint8 t1,t2;
    p = &value;
    t1 = *((uint8 *)p);
    t2 = *((uint8 *)p + 1);
    value = 0;
    value += t2;
    value <<= 8;
    value += t1;
    return (value);
}
/****************************************************************************
 *
 * NAME:  Send_Pack
 *
 * DESCRIPTION:
 *
 *
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void Send_Pack (uint8 fram_control)
{
    uint16 count;
    uint16 crc_send_clr =0x0000;
    uint16 crc_send_qry =0x0000;
    uint8 *tx_temp;
    gflash_bit &= ~PORT_LINK_LED;

    mbus_hdr_slv.sync[0] = 'Y';
    mbus_hdr_slv.sync[1] = 'I';
    mbus_hdr_slv.sync[2] = 'R';
    mbus_hdr_slv.sync[3] = 'I';
    mbus_hdr_slv.frame_control = fram_control;                                      //version 01 Data_flag=1
    mbus_hdr_slv.cmd = MBUS_CMD_RSP;
    mbus_hdr_slv.slv_id = u16Big_To_Little(SensorAddr);

    mbus_hdr_slv.mbus_tlv.type = MBUS_TLV_SENSOR_READER_V2;
    mbus_hdr_slv.mbus_tlv.len = u16Big_To_Little(sizeof(mbus_hdr_slv.mbus_tlv) - 3);//不包括 type 和 len 的byte数

    tx_temp =(uint8 *)mbus_hdr_slv_p;

    if (mbus_hdr_mstr_p->cmd == MBUS_CMD_CLR)
    {
        mbus_hdr_slv.data_len = 0x0000;
        crc_send_clr = CRC16((uint8 *)mbus_hdr_slv_p, 10,0xFFFF);              //sync[0]到slv_id 共10个byte
        for (count = 0;count <10;count++,tx_temp++)
        {
            while(!(u8AHI_UartReadLineStatus(E_AHI_UART_1)&E_AHI_UART_LS_THRE)){};
            vAHI_UartWriteData(E_AHI_UART_1,*tx_temp);
        }
        vAHI_UartWriteData(E_AHI_UART_1,(uint8)(crc_send_clr&0x00FF));         //crc_send_clr Low byte
        vAHI_UartWriteData(E_AHI_UART_1,(uint8)((crc_send_clr&0xFF00) >> 8));  //crc_send_clr High byte
        //OD2101_vPrintf("Pack Send_CLR \r\n");
    }
    if (mbus_hdr_mstr_p->cmd == MBUS_CMD_QRY)
    {
        mbus_hdr_slv.data_len = u16Big_To_Little(sizeof(mbus_hdr_slv.mbus_tlv));
        crc_send_qry = CRC16((uint8 *)mbus_hdr_slv_p, sizeof(mbus_hdr_slv_t_Temp),0xFFFF) ; //
        for (count = 0;count <sizeof(mbus_hdr_slv_t_Temp);count++,tx_temp++)
        {
            while(!(u8AHI_UartReadLineStatus(E_AHI_UART_1)&E_AHI_UART_LS_THRE)){};
            vAHI_UartWriteData(E_AHI_UART_1,*tx_temp);
        }
        vAHI_UartWriteData(E_AHI_UART_1,(uint8)(crc_send_qry&0x00FF));       //crc_send_qry Low byte
        vAHI_UartWriteData(E_AHI_UART_1,(uint8)((crc_send_qry&0xFF00) >> 8));  //crc_send_qry High byte
        //OD2101_vPrintf("Pack Send_REQ \r\n");
    }
}











