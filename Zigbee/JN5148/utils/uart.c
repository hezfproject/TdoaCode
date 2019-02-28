
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>

#include "uart.h"
#ifdef NEED_I2C_PRINT
#include "i2c_printf_util.h"
#else
#include "printf_util.h"
#endif
#include "MicroSpecific.h"
#include "Bsmac_header.h"
#include "bsmac.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_UART)
#define DBG(x) do{x} while (0);
#else
#define DBG(x)
#endif

#if (defined DEBUG_ERROR)
#define EDBG(x) do{x} while (0);
#else
#define EDBG(x)
#endif

#ifdef NEED_I2C_PRINT
#define  PRINTF(x...) i2c_vPrintf(x)
#define  PRINTMEM(x...) i2c_vPrintMem(x)
#else
#define  PRINTF(x...) PrintfUtil_vPrintf(x)
#define  PRINTMEM(x...) PrintfUtil_vPrintMem(x)
#endif

#define UARTSTATE_PREAMBLE_H 0
#define UARTSTATE_PREAMBLE_L 1
#define UARTSTATE_FRAME_CONTROL 2
#define UARTSTATE_RESERVERD 3
#define UARTSTATE_FRAME_COUNT_H 4
#define UARTSTATE_FRAME_COUNT_L 5
#define UARTSTATE_SRC_ADDR_H 6
#define UARTSTATE_SRC_ADDR_L 7
#define UARTSTATE_DST_ADDR_H 8
#define UARTSTATE_DST_ADDR_L 9
#define UARTSTATE_DATA_LEN_H 10
#define UARTSTATE_DATA_LEN_L 11
#define UARTSTATE_DATA 12
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct
{
    uint8 uart_port;

    /* buf */
    uint8* txBuf;
    uint16 txLen;

    uint8 rxBuf[UART_MAX_RX_LEN + 10];  //add some guard

    /* status */
    bool on_tx;
    // bool on_rx;

    /* state machine */
    uint8       state;

    /* bsmac header point*/
    bsmac_header_t *pbsmac;

    /* tx and rx done call back */
    uart_tx_cb_t tx_callback;
    uart_rx_cb_t rx_callback;

    uint8 rx_wantlen;
    uint8 rx_datalen;

} UartHdl_t;;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
UartHdl_t  UartHdl0;
UartHdl_t  UartHdl1;

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
PUBLIC void vUart_Init(const uint8 uart_port, const uint8 uart_baud, const uart_tx_cb_t tx_callback, const uart_rx_cb_t rx_callback)
{

    vAHI_UartEnable(uart_port);
    vAHI_UartReset(uart_port, TRUE, TRUE);

    switch(uart_baud)
    {
        case BSMAC_UART_BAUD_500k:
            vAHI_UartSetBaudDivisor(uart_port, 2);
            //100k and 500k both use 15
            vAHI_UartSetClocksPerBit(uart_port, 15);
            break;

        case BSMAC_UART_BAUD_100k:
            vAHI_UartSetBaudDivisor(uart_port, 10);
            //100k and 500k both use 15
            vAHI_UartSetClocksPerBit(uart_port, 15);
            break;

        case BSMAC_UART_BAUD_115200:
            vAHI_UartSetBaudRate(uart_port,E_AHI_UART_RATE_115200);
            break;

        case BSMAC_UART_BAUD_460800:
            vAHI_UartSetBaudDivisor(uart_port, 5);
            vAHI_UartSetClocksPerBit(uart_port, 6);
            break;

        default:
            EDBG(PRINTF("Set Uart baud Error\n"););
            break;
    }


    //E,8,1; no RTS
    vAHI_UartSetControl(uart_port, E_AHI_UART_EVEN_PARITY, E_AHI_UART_PARITY_DISABLE,
                        E_AHI_UART_WORD_LEN_8, E_AHI_UART_1_STOP_BIT, E_AHI_UART_RTS_LOW);

    //interrupt is also generated if no character has entered the FIFO during a time interval
    //in which at least four characters could potentially have been received.
    vAHI_UartSetInterrupt(uart_port, FALSE, FALSE, TRUE, TRUE, E_AHI_UART_FIFO_LEVEL_8);

    if(uart_port == E_AHI_UART_0)
    {
        vAHI_Uart0RegisterCallback(vUart_HandleUartInterrupt);

        //TX0EN
        //vAHI_DioSetDirection(0, E_AHI_DIO10_INT);
        //vAHI_DioSetOutput(E_AHI_DIO10_INT, 0);

        UartHdl0.uart_port = E_AHI_UART_0;
        UartHdl0.txBuf = NULL;
        UartHdl0.txLen = 0;
        UartHdl0.on_tx = FALSE;
        // UartHdl0.on_rx = TRUE;
        UartHdl0.tx_callback = tx_callback;
        UartHdl0.rx_callback = rx_callback;
        UartHdl0.state = UARTSTATE_PREAMBLE_H;
        UartHdl0.pbsmac = (bsmac_header_t *)UartHdl0.rxBuf;
    }
    else
    {
        vAHI_Uart1RegisterCallback(vUart_HandleUartInterrupt);

        //TX1EN
        // vAHI_DioSetDirection(0, E_AHI_DIO11_INT);
        // vAHI_DioSetOutput(E_AHI_DIO11_INT, 0);

        UartHdl1.uart_port = E_AHI_UART_1;
        UartHdl1.txBuf = NULL;
        UartHdl1.txLen = 0;
        UartHdl1.on_tx = FALSE;
        // UartHdl1.on_rx = TRUE;
        UartHdl1.tx_callback = tx_callback;
        UartHdl1.rx_callback = rx_callback;
        UartHdl1.state = UARTSTATE_PREAMBLE_H;
        UartHdl1.pbsmac = (bsmac_header_t *)UartHdl1.rxBuf;


    }
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
PUBLIC uint8  u8Uart_StartTx(const uint8 uart_port, uint8 *p, uint16 len)
{
    uint8 i;
    UartHdl_t* pHdl;

    if( p==NULL ||  len ==0 || len > UART_MAX_TX_LEN)
    {
        return UART_INVALIDPARAM;
    }

    if(uart_port == E_AHI_UART_0)
    {
        pHdl = &UartHdl0;
    }
    else if(uart_port == E_AHI_UART_1)
    {
        pHdl = &UartHdl1;
    }
    else
    {
        return UART_INVALIDPARAM;
    }

    uint32 intstore;
    MICRO_DISABLE_AND_SAVE_INTERRUPTS(intstore);
    if(pHdl->on_tx)
    {
        MICRO_RESTORE_INTERRUPTS(intstore);
        return UART_BUSY;
    }
    pHdl->on_tx = TRUE;
    MICRO_RESTORE_INTERRUPTS(intstore);

    /* if the tx fifo is not empty, reset tx*/
    if (!(u8AHI_UartReadLineStatus(pHdl->uart_port) & E_AHI_UART_LS_THRE))
    {
        vAHI_UartReset(pHdl->uart_port, TRUE, FALSE);
    }


    /* start to send */
	  MICRO_DISABLE_AND_SAVE_INTERRUPTS(intstore);
    pHdl->txBuf = p;
    pHdl->txLen = len;

    for(i=0; i<UART_FIFO_LEVEL; i++ )
    {
        vAHI_UartWriteData(pHdl->uart_port, *(pHdl->txBuf));
        pHdl->txBuf++;
        pHdl->txLen--;
        if(pHdl->txLen == 0)
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

    if (u32Device == E_AHI_DEVICE_UART0)
    {
        pHdl = &UartHdl0;
    }
    else if(u32Device == E_AHI_DEVICE_UART1)
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
    else
    {
        EDBG(PRINTF("Uartint Err %d %d\n", u32Device, u32ItemBitmap););
        Uart_vRxErrHandler(pHdl->uart_port);
    }
}

PUBLIC bool  bUart_IsOnTx(const uint8 uart_port)
{
    UartHdl_t* pHdl;

    if(uart_port == E_AHI_UART_0)
    {
        pHdl = &UartHdl0;
    }
    else if(uart_port == E_AHI_UART_1)
    {
        pHdl = &UartHdl1;
    }
    else
    {
        return FALSE;
    }
    return pHdl->on_tx;
}

PRIVATE void Uart_vTxHandler(UartHdl_t* pHdl )
{
    uint8 i;

    if(pHdl->txLen == 0)  /* send complete */
    {
        //uint32 intstore;
        //MICRO_DISABLE_AND_SAVE_INTERRUPTS(intstore);
        if(pHdl->tx_callback!=NULL)
        {
            pHdl->tx_callback(pHdl->uart_port);
        }
        pHdl->on_tx = FALSE;
        //MICRO_RESTORE_INTERRUPTS(intstore);
    }
    else                    /* continue to send */
    {
        for(i=0; i<UART_FIFO_LEVEL; i++)
        {
            vAHI_UartWriteData(pHdl->uart_port, *pHdl->txBuf);
            pHdl->txBuf++;
            pHdl->txLen--;
            if(pHdl->txLen == 0)
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
    read_level = u8AHI_UartReadRxFifoLevel(pHdl->uart_port);

    if(read_level ==0)
    {
        return;
    }

    for(i=0; i<read_level; i++)
    {
        data = u8AHI_UartReadData(pHdl->uart_port);
        //DBG(PRINTF("%X ", data););
        switch(pHdl->state)
        {
        case UARTSTATE_PREAMBLE_H:
        {
            if(data == BSMAC_PREAMBLE_H)
            {
                pHdl->pbsmac->preamble_H = data;
                pHdl->state = UARTSTATE_PREAMBLE_L;
            }
            break;
        }
        case UARTSTATE_PREAMBLE_L:
        {
            if(data == BSMAC_PREAMBLE_L)
            {
                pHdl->pbsmac->preamble_L  = data;
                pHdl->state = UARTSTATE_FRAME_CONTROL;
            }
            else
            {
                pHdl->state = UARTSTATE_PREAMBLE_H;
            }
            break;
        }
        case UARTSTATE_FRAME_CONTROL:
        {
            pHdl->pbsmac->frame_control= data;
            pHdl->state = UARTSTATE_RESERVERD;
            break;
        }
        case UARTSTATE_RESERVERD:
        {
            pHdl->pbsmac->reserverd = data;
            pHdl->state = UARTSTATE_FRAME_COUNT_H;
            break;

        }
        case UARTSTATE_FRAME_COUNT_H:
        {
            pHdl->pbsmac->frame_count_H = data;
            pHdl->state = UARTSTATE_FRAME_COUNT_L;
            break;
        }
        case UARTSTATE_FRAME_COUNT_L:
        {
            pHdl->pbsmac->frame_count_L = data;
            pHdl->state = UARTSTATE_SRC_ADDR_H;
            break;
        }
        case UARTSTATE_SRC_ADDR_H:
        {
            pHdl->pbsmac->src_addr_H= data;
            pHdl->state = UARTSTATE_SRC_ADDR_L;
            break;
        }

        case UARTSTATE_SRC_ADDR_L:
        {
            pHdl->pbsmac->src_addr_L = data;
            pHdl->state = UARTSTATE_DST_ADDR_H;
            break;
        }
        case UARTSTATE_DST_ADDR_H:
        {
            pHdl->pbsmac->dst_addr_H = data;
            pHdl->state = UARTSTATE_DST_ADDR_L;
            break;
        }

        case UARTSTATE_DST_ADDR_L:
        {
            pHdl->pbsmac->dst_addr_L = data;
            pHdl->state = UARTSTATE_DATA_LEN_H;
            break;
        }
        case UARTSTATE_DATA_LEN_H:
        {
            pHdl->pbsmac->data_len_H = data;
            pHdl->state = UARTSTATE_DATA_LEN_L;
            break;
        }
        case UARTSTATE_DATA_LEN_L:
        {
            pHdl->pbsmac->data_len_L = data;

            pHdl->rx_wantlen = (pHdl->pbsmac->data_len_H<<8 | pHdl->pbsmac->data_len_L); // + 2; len is including crc

            if((pHdl->rx_wantlen <= (UART_MAX_RX_LEN - sizeof(bsmac_header_t))) && (pHdl->rx_wantlen != 0))
            {
                pHdl->rx_datalen =0;
                pHdl->state = UARTSTATE_DATA;
            }
            else
            {
                pHdl->state = UARTSTATE_PREAMBLE_H;
            }
            break;
        }
        case UARTSTATE_DATA:
        {
            pHdl->rxBuf[sizeof(bsmac_header_t) + pHdl->rx_datalen] = data;
            if(++pHdl->rx_datalen >= pHdl->rx_wantlen)
            {
                if(pHdl->rx_callback!=NULL)
                {
                    pHdl->rx_callback(pHdl->uart_port, pHdl->rxBuf, sizeof(bsmac_header_t) + pHdl->rx_datalen);
                }
                pHdl->state = UARTSTATE_PREAMBLE_H;
            }
            else
            {
                pHdl->state = UARTSTATE_DATA;
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

    EDBG(PRINTF("UART: RxErr %d %d\n", lineStatus, dataInFifo););
    while(dataInFifo != 0)
    {
        u8AHI_UartReadData(uart_port);
        dataInFifo--;
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
