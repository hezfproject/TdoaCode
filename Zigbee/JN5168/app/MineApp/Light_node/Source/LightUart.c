
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>

#include "LightUart.h"
#include "MicroSpecific.h"
#include "app_protocol.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_UART)
#define DBG(x) do{x} while (0);
#else
#define DBG(x)
#endif

/* TX RX switch */

#define  DIO_UART1_TX_ENABLE           E_AHI_DIO11_INT
#define UART1_IN_TX()  (u32AHI_DioReadInput() & DIO_UART1_TX_ENABLE)
#define UART1_SWITCH_TX()  do{vAHI_DioSetOutput(DIO_UART1_TX_ENABLE, 0);}while(!UART1_IN_TX());
#define UART1_SWITCH_RX()  do{vAHI_DioSetOutput(0, DIO_UART1_TX_ENABLE);}while(UART1_IN_TX());

/* RX state machine */
#define UART_STATE_IDLE 	0     /* IDLE STATE */
#define UART_STATE_SYNC1	1  	/* SYNCED 'Y', WANT 'I'*/
#define UART_STATE_SYNC2 	2  	/* SYNCED 'I', WANT 'R'*/
#define UART_STATE_LEN  	3    	/* WANT cmd*/
#define UART_STATE_DATA  		8  /*  RECV DATA*/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct
{
    uint8 uart_port;
    uint8 baud_rate;

    /* state machine */
    uint8       state;

    /* buf */
    uint8 *txBuf;
    uint16 tx_datalen;
    uint16 tx_wantlen;

    uint8 rxBuf[UART_MAX_RX_LEN + 10];

    uint16 rx_datalen;
    uint16 rx_wantlen;

    /* status */
    bool recv_type;
    bool on_tx;
    // bool on_rx;

    /* tx and rx done call back */
    uart_tx_cb_t tx_callback;
    uart_rx_cb_t rx_callback;
} UartHdl_t;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
//UartHdl_t  UartHdl0;
UartHdl_t  UartHdl1;

PRIVATE uint16 u8UartAddrBackOff=100;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
PRIVATE void vUart_HandleUartInterrupt(uint32 u32Device, uint32 u32ItemBitmap);
PRIVATE void Uart_vTxHandler(UartHdl_t* pHdl );
PRIVATE void Uart_vRxHandler(UartHdl_t* pHdl, bool isEnd);
PRIVATE void Uart_vRxErrHandler(uint8 uart_port);

/****************************************************************************
 *
 * NAME: vUART_Init
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PUBLIC void vUart_Init(const uint8 uart_baud, const uart_tx_cb_t tx_callback, const uart_rx_cb_t rx_callback)
{
    uint8 uart_port = E_AHI_UART_1;

    vAHI_UartEnable(uart_port);
    vAHI_UartReset(uart_port, TRUE, TRUE);

    vUart_SetBaudRate(uart_baud);

    //E,8,1; no RTS
    vAHI_UartSetControl(uart_port, E_AHI_UART_EVEN_PARITY, E_AHI_UART_PARITY_DISABLE,
                        E_AHI_UART_WORD_LEN_8, E_AHI_UART_1_STOP_BIT, E_AHI_UART_RTS_LOW);

    //interrupt is also generated if no character has entered the FIFO during a time interval
    //in which at least four characters could potentially have been received.
    vAHI_UartSetInterrupt(uart_port, FALSE, FALSE, TRUE, TRUE, E_AHI_UART_FIFO_LEVEL_8);

    if(uart_port == E_AHI_UART_0)
    {
        vAHI_Uart0RegisterCallback(vUart_HandleUartInterrupt);
    }
    else if(uart_port == E_AHI_UART_1)
    {
        vAHI_Uart1RegisterCallback(vUart_HandleUartInterrupt);
    }
    else
    {
        DBG(PrintfUtil_vPrintf("Init Err\n"););
    }

    UartHdl1.uart_port = uart_port;
    UartHdl1.on_tx = FALSE;
    UartHdl1.tx_callback = tx_callback;
    UartHdl1.rx_callback = rx_callback;
    UartHdl1.state = UART_STATE_IDLE;

    // DIO 11 use as TX RX switch
    vAHI_DioSetDirection(0, DIO_UART1_TX_ENABLE);
    vAHI_DioSetPullup(0, DIO_UART1_TX_ENABLE);
    UART1_SWITCH_RX();

}

/****************************************************************************
 *
 * NAME: vUart_SetBaudRate
 *
 * DESCRIPTION: 设置Uart1的波特率
 *
 * PARAMETERS:      u8BaudRate
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/

PUBLIC void vUart_SetBaudRate(uint8 u8BaudRate)
{
    UartHdl_t* pHdl;

    pHdl = &UartHdl1;
    pHdl->baud_rate = u8BaudRate;
    pHdl->state = UART_STATE_IDLE;
    pHdl->rx_datalen = 0;

    //  note: 1. vAHI_UartSetBaudRate() may error  sometime;
    //            2.  vAHI_UartSetBaudDivisor(uart_x,u16Divisor) must before vAHI_UartSetClocksPerBit(uart_x,u8Cpb);
    //            3.  0<= u8Cpb <=15    0-2 not recommended
    //            4.  BaudRate = 16/(u16Divisor*(u8Cpb+1))

    switch(u8BaudRate)
    {
    case UART_BAUDRATE_460800:
    {
        vAHI_UartSetBaudDivisor(E_AHI_UART_1,5);
        vAHI_UartSetClocksPerBit(E_AHI_UART_1,6);
        u8UartAddrBackOff = 50;
        break;
    }

    case UART_BAUDRATE_115200:
    {
        vAHI_UartSetBaudDivisor(E_AHI_UART_1,10);
        vAHI_UartSetClocksPerBit(E_AHI_UART_1,13);
        u8UartAddrBackOff = 100;
        break;
    }

    case UART_BAUDRATE_38400:
    {
        vAHI_UartSetBaudDivisor(E_AHI_UART_1,26);
        vAHI_UartSetClocksPerBit(E_AHI_UART_1,15);
        u8UartAddrBackOff = 200;
        break;
    }

    case UART_BAUDRATE_9600:
    {
        vAHI_UartSetBaudDivisor(E_AHI_UART_1,119);
        vAHI_UartSetClocksPerBit(E_AHI_UART_1,13);
        u8UartAddrBackOff = 600;
        break;
    }

    default:
    {
        //115200
        vAHI_UartSetBaudDivisor(E_AHI_UART_1,10);
        vAHI_UartSetClocksPerBit(E_AHI_UART_1,13);
        u8UartAddrBackOff = 100;
        break;
    }
    }
}

/****************************************************************************
 *
 * NAME: vUart_GetAddrBackOffTimer
 *
 * DESCRIPTION: 得到当前波特率下回复地址的退避时间
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * u8BackOffTimer.
 *
 * NOTES:
 * None.
 ****************************************************************************/

PUBLIC uint16 vUart_GetAddrBackOff(void)
{
    return u8UartAddrBackOff;
}


/****************************************************************************
 *
 * NAME: vUART_StartTx
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PUBLIC uint8  u8Uart_StartTx(uint8 *p, uint16 len)
{
    uint8 i;
    UartHdl_t* pHdl;

    if( p==NULL ||  len ==0 || len > UART_MAX_TX_LEN)
    {
        PrintfUtil_vPrintf("UART_INVALIDPARAM: %d\n",len);
        return UART_INVALIDPARAM;
    }

    pHdl = &UartHdl1;

    uint32 intstore;
    MICRO_DISABLE_AND_SAVE_INTERRUPTS(intstore);
    if(pHdl->on_tx)
    {
        MICRO_RESTORE_INTERRUPTS(intstore);
        PrintfUtil_vPrintf("UART_BUSY\n");
        return UART_BUSY;
    }

    pHdl->on_tx = TRUE;

    MICRO_RESTORE_INTERRUPTS(intstore);

    //
    UART1_SWITCH_TX();
    vAHI_UartSetInterrupt(E_AHI_UART_1, FALSE, FALSE, TRUE, FALSE, E_AHI_UART_FIFO_LEVEL_8);

    // TimerUtil_vDelay(1, E_TIMER_UNIT_MILLISECOND);

    /* if the tx fifo is not empty, reset tx*/
    if (!(u8AHI_UartReadLineStatus(pHdl->uart_port) & E_AHI_UART_LS_THRE))
    {
        vAHI_UartReset(pHdl->uart_port, TRUE, FALSE);
        //PrintfUtil_vPrintf("T2\n");
    }


    /* start to send */
    MICRO_DISABLE_AND_SAVE_INTERRUPTS(intstore);

    //memcpy(pHdl->txBuf, p, len);
    pHdl->txBuf = p;
    pHdl->tx_wantlen = len;
    pHdl->tx_datalen = 0;

    for(i=0; i<UART_FIFO_LEVEL; i++ )
    {
        vAHI_UartWriteData(pHdl->uart_port, pHdl->txBuf[pHdl->tx_datalen++]);

        if(pHdl->tx_datalen >= pHdl->tx_wantlen)
        {
            break;
        }
    }
    MICRO_RESTORE_INTERRUPTS(intstore);

    return UART_SUCCESS;
}

/****************************************************************************
 *
 * NAME: vUart_HandleUartInterrupt
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vUart_HandleUartInterrupt(uint32 u32Device, uint32 u32ItemBitmap)
{
    UartHdl_t* pHdl;

    if (u32Device == E_AHI_DEVICE_UART1)
    {
        pHdl = &UartHdl1;
    }
    else
    {
        return;
    }
    /* If character transmission is complete */
    if (u32ItemBitmap == E_AHI_UART_INT_TX)
    {
        Uart_vTxHandler(pHdl);
    }
    /* If data has been received */
    else if ((u32ItemBitmap & 0x000000FF) == E_AHI_UART_INT_RXDATA)
    {
        Uart_vRxHandler(pHdl, FALSE);
    }
    else if(u32ItemBitmap == E_AHI_UART_INT_TIMEOUT)
    {
        Uart_vRxHandler(pHdl, TRUE);
    }
    else if(u32ItemBitmap == E_AHI_UART_INT_RXLINE)
    {
        Uart_vRxErrHandler(pHdl->uart_port);
    }

}

PUBLIC bool  bUart_IsOnTx(void)
{
    UartHdl_t* pHdl;

    pHdl = &UartHdl1;

    return pHdl->on_tx;
}

PRIVATE void Uart_vTxHandler(UartHdl_t* pHdl )
{
    uint8 i, read_level;

    if(pHdl->tx_datalen >= pHdl->tx_wantlen)  /* send complete */
    {
        //uint32 intstore;
        //MICRO_DISABLE_AND_SAVE_INTERRUPTS(intstore);
        if(pHdl->tx_callback!=NULL)
        {
            pHdl->tx_callback();
        }
        vAHI_UartSetInterrupt(E_AHI_UART_1, FALSE, FALSE, FALSE, TRUE, E_AHI_UART_FIFO_LEVEL_8);

        pHdl->on_tx = FALSE;

        //TimerUtil_vDelay(5, E_TIMER_UNIT_MILLISECOND);
        UART1_SWITCH_RX();

        /* drop the datas in my recv fifo*/
        read_level = u8AHI_UartReadRxFifoLevel(pHdl->uart_port);

        while(read_level-- !=0)
        {
            u8AHI_UartReadData(pHdl->uart_port);
        }
        //MICRO_RESTORE_INTERRUPTS(intstore);
    }
    else                    /* continue to send */
    {
        for(i=0; i<UART_FIFO_LEVEL; i++)
        {
            vAHI_UartWriteData(pHdl->uart_port, pHdl->txBuf[pHdl->tx_datalen++]);
            if(pHdl->tx_datalen >= pHdl->tx_wantlen)
            {
                break;
            }
        }
    }

}


PRIVATE void Uart_vRxHandler(UartHdl_t* pHdl, bool isEnd)
{
	uint8 read_level;
	uint8 data;
	uint8 i;

	if(pHdl->uart_port!=E_AHI_UART_1 ||pHdl->on_tx )
	{
		DBG(PrintfUtil_vPrintf("Rort Err or on tx\n"););
		return;
	}

	read_level = u8AHI_UartReadRxFifoLevel(pHdl->uart_port);

	if(read_level ==0)
	{
		DBG(PrintfUtil_vPrintf("Recv Err3\n"););
		return;
	}

	for(i=0; i<read_level; i++)
	{
		data = u8AHI_UartReadData(pHdl->uart_port);

		//PrintfUtil_vPrintf("data:%d\n",data);

		switch(pHdl->state)
		{
		case UART_STATE_IDLE:
		{
			pHdl->rx_datalen = 0;
			pHdl->rx_wantlen = 0;

			if(data =='Y')
			{
				pHdl->state = UART_STATE_SYNC1;
				pHdl->rxBuf[pHdl->rx_datalen++] = data;                
			}
			break;
		}
		case UART_STATE_SYNC1:
		{
			if(data == 'I')
			{         
				pHdl->state = UART_STATE_SYNC2;
				pHdl->rxBuf[pHdl->rx_datalen++] = data;
			}
			else if(data =='Y')
			{
				pHdl->rx_datalen = 0;
				pHdl->rx_wantlen = 0;              
				pHdl->state = UART_STATE_SYNC1;
				pHdl->rxBuf[pHdl->rx_datalen++] = data;
			}
			else
			{
				pHdl->state = UART_STATE_IDLE;           
			}
			break;
		}
		case UART_STATE_SYNC2:
		{
			if(data == 'R')
			{
				pHdl->state = UART_STATE_LEN;
				pHdl->rxBuf[pHdl->rx_datalen++] = data;
			}
			else if(data =='Y')
			{
				pHdl->rx_datalen = 0;
				pHdl->rx_wantlen = 0;              
				pHdl->state = UART_STATE_SYNC1;
				pHdl->rxBuf[pHdl->rx_datalen++] = data;
			}            
			else
			{
				pHdl->state = UART_STATE_IDLE;              
			}
			break;
		}		
		case UART_STATE_LEN:
		{
			pHdl->rxBuf[pHdl->rx_datalen++] = data;
			pHdl->rx_wantlen = data;
			pHdl->rx_wantlen += 2; // add for crc
			if(pHdl->rx_wantlen <= (UART_MAX_RX_LEN- sizeof(light_sync_hdr_t) ))
			{
				pHdl->state = UART_STATE_DATA;
			}
			else
			{
				pHdl->state = UART_STATE_IDLE;
			}
			break;
		}
		case UART_STATE_DATA:
		{
			pHdl->rxBuf[pHdl->rx_datalen++]= data;
			if(--pHdl->rx_wantlen == 0)
			{
				if(pHdl->rx_callback!=NULL)
				{
					pHdl->rx_callback(pHdl->rxBuf,  pHdl->rx_datalen);
				}
				pHdl->state = UART_STATE_IDLE;
			}
			else
			{
				pHdl->state = UART_STATE_DATA;
			}
			break;
		}
		}
	}
}

PRIVATE void Uart_vRxErrHandler(uint8 uart_port)
{
    //if there are bytes in fifo

    uint8 lineStatus = u8AHI_UartReadLineStatus(uart_port);
    uint8 dataInFifo = u8AHI_UartReadRxFifoLevel(uart_port);

    DBG(PrintfUtil_vPrintf("ERR: RxErr %d %d\n", lineStatus, dataInFifo););
    while(dataInFifo != 0)
    {
        u8AHI_UartReadData(uart_port);
        dataInFifo--;
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
