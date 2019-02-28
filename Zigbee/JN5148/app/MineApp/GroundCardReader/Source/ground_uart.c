
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>

#include "MicroSpecific.h"
#include "MbusProto.h"
#include "ground_track.h"
#include "ground_uart.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void _uart_HandleUartInterrupt(uint32 u32Device, uint32 u32ItemBitmap);
PRIVATE uint8 _uart_tx_fifo(UartHdl_t *const pHdl);
PRIVATE void _uart_vTxHandler(UartHdl_t* pHdl );
PRIVATE void _uart_vRxHandler(UartHdl_t* pHdl, bool isEnd);
PRIVATE void _uart_vRxErrHandler(uint8 uart_port);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
const uint8 bauddivisor_remap[UART_BAUDRATE_KIND] = {
    [UART_BAUDRATE_460800] = 5,
    [UART_BAUDRATE_115200] = 10,
    [UART_BAUDRATE_38400]  = 26,
    [UART_BAUDRATE_9600]   = 119
};

const uint8 baudperbit_remap[UART_BAUDRATE_KIND] = {
    [UART_BAUDRATE_460800] = 6,
    [UART_BAUDRATE_115200] = 13,
    [UART_BAUDRATE_38400]  = 15,
    [UART_BAUDRATE_9600]   = 13
};

const uint16 baudbackoff_remap[UART_BAUDRATE_KIND] = {
    [UART_BAUDRATE_460800] = 50,
    [UART_BAUDRATE_115200] = 100,
    [UART_BAUDRATE_38400]  = 200,
    [UART_BAUDRATE_9600]   = 600
};

PRIVATE UartHdl_t* s_pstUart1;

PRIVATE uint16 u16UartAddrBackOff=100;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vUART_Init
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *                  uarthdl         R
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PUBLIC bool_t uart_init(UartHdl_t * const uartcfg)
{
    ASSERT(uartcfg != NULL);

    DBG_JUDGE(uartcfg->tx_callback == NULL, "warning: uart init lose TX"
        " callbanck");
    DBG_JUDGE(uartcfg->rx_callback == NULL, "warning: uart init lose RX"
        " callbanck");

    if (uartcfg->uart_port != E_AHI_UART_1)
    {
        DBG_WARN("uart init ERR:uart port %d not support\n",
            uartcfg->uart_port);
        return FALSE;
    }

    s_pstUart1 = uartcfg;

    vAHI_UartEnable(uartcfg->uart_port);
    vAHI_UartReset(uartcfg->uart_port, TRUE, TRUE);

    uartcfg->baud_rate = uart_setbaudrate(uartcfg, uartcfg->baud_rate);

    //E,8,1; no RTS
    vAHI_UartSetControl(uartcfg->uart_port,
                        E_AHI_UART_EVEN_PARITY,
                        E_AHI_UART_PARITY_DISABLE,
                        E_AHI_UART_WORD_LEN_8,
                        E_AHI_UART_1_STOP_BIT,
                        E_AHI_UART_RTS_LOW);

    //interrupt is also generated if no character has entered the FIFO during a time interval
    //in which at least four characters could potentially have been received.
    vAHI_UartSetInterrupt(uartcfg->uart_port,
                        FALSE,
                        FALSE,
                        TRUE,
                        TRUE,
                        E_AHI_UART_FIFO_LEVEL_8);

    vAHI_Uart1RegisterCallback(_uart_HandleUartInterrupt);

    // DIO 11 use as TX RX switch
    vAHI_DioSetDirection(0, DIO_UART1_TX_ENABLE);
    vAHI_DioSetPullup(0, DIO_UART1_TX_ENABLE);
    UART1_SWITCH_RX();

    return TRUE;
}

/****************************************************************************
 *
 * NAME: uart_SetBaudRate
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
PUBLIC uint8 uart_setbaudrate(UartHdl_t * const pHdl, uint8 u8BaudRate)
{
    ASSERT(pHdl != NULL);

    if (u8BaudRate >= UART_BAUDRATE_KIND)
    {
        DBG_WARN("uart baudrate config error, by default 115200\n");
        u8BaudRate = UART_BAUDRATE_115200;
    }

    pHdl->baud_rate = u8BaudRate;

    /*
    * note: 1. vAHI_UartSetBaudRate() may error  sometime;
    *       2. vAHI_UartSetBaudDivisor(uart_x,u16Divisor)
    *          must before vAHI_UartSetClocksPerBit(uart_x,u8Cpb);
    *       3. 0<= u8Cpb <=15    0-2 not recommended
    *       4. BaudRate = 16/(u16Divisor*(u8Cpb+1))
    */
    vAHI_UartSetBaudDivisor(pHdl->uart_port, bauddivisor_remap[u8BaudRate]);
    vAHI_UartSetClocksPerBit(pHdl->uart_port, baudperbit_remap[u8BaudRate]);
    u16UartAddrBackOff = baudbackoff_remap[u8BaudRate];

    return u8BaudRate;
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
PUBLIC uint16 uart_getaddrbackoff(void)
{
    return u16UartAddrBackOff;
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
PUBLIC void uart_tx_start(UartHdl_t * const pHdl)
{
    uint16 send_ready;

    ASSERT(pHdl != NULL);

    // go to rx
    vAHI_UartSetInterrupt(E_AHI_UART_1, FALSE, FALSE, FALSE, FALSE,
                        E_AHI_UART_FIFO_LEVEL_8);

    pHdl->work_state = TX;
    UART1_SWITCH_TX();

    /* if the tx fifo is not empty, reset tx*/
    if (!(u8AHI_UartReadLineStatus(pHdl->uart_port) & E_AHI_UART_LS_THRE))
    {
        vAHI_UartReset(pHdl->uart_port, TRUE, TRUE);
    }

    // trigger uart send
    send_ready = _uart_tx_fifo(pHdl);

    vAHI_UartSetInterrupt(pHdl->uart_port, FALSE, FALSE, TRUE, FALSE,
                         E_AHI_UART_FIFO_LEVEL_8);

    DBG_JUDGE(send_ready == 0, "txfifo = %d\n", send_ready);
}

PUBLIC bool_t uart_is_tx(UartHdl_t const* const pHdl)
{
    ASSERT(pHdl != NULL);

    return (bool_t)(pHdl->work_state == TX);
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
PRIVATE void _uart_HandleUartInterrupt(uint32 u32Device, uint32 u32ItemBitmap)
{
    UartHdl_t* pHdl;

    if (u32Device == E_AHI_DEVICE_UART1)
    {
        ASSERT_RET(s_pstUart1 != NULL);
        pHdl = s_pstUart1;
    }
    else
    {
        return;
    }

    /* If character transmission is complete */
    if (u32ItemBitmap == E_AHI_UART_INT_TX)
    {
        _uart_vTxHandler(pHdl);
    }
    /* If data has been received */
    else if ((u32ItemBitmap & 0x000000FF) == E_AHI_UART_INT_RXDATA)
    {
        _uart_vRxHandler(pHdl, FALSE);
    }
    else if (u32ItemBitmap == E_AHI_UART_INT_TIMEOUT)
    {
        _uart_vRxHandler(pHdl, TRUE);
    }
    else if (u32ItemBitmap == E_AHI_UART_INT_RXLINE)
    {
        _uart_vRxErrHandler(pHdl->uart_port);
    }
}

PRIVATE uint8 _uart_tx_fifo(UartHdl_t *const pHdl)
{
    uint8 i;
    uint16 send_ready, tx_level;

    send_ready = pHdl->tx_max - pHdl->tx_pos;

    if (send_ready == 0)
        return 0;

    //tx_level = 16;  // 5148 tx fifo is 16byte
    tx_level = u8AHI_UartReadTxFifoLevel(pHdl->uart_port);

    DBG_LOG("send ready = %d , txFifo = %d\n", send_ready, tx_level);

    tx_level = 16 - tx_level;

    if (send_ready > tx_level)
        send_ready = tx_level;

    for (i=0; i<send_ready; i++)
    {
        vAHI_UartWriteData(pHdl->uart_port, pHdl->txBuf[pHdl->tx_pos++]);
    }

    return send_ready;
}

PRIVATE void _uart_vTxHandler(UartHdl_t* pHdl)
{
    if (pHdl->tx_pos >= pHdl->tx_max)  /* send complete */
    {
        if (pHdl->tx_callback != NULL)
        {
            pHdl->tx_callback(pHdl);
        }
    }
    else                    /* continue to send */
    {
        _uart_tx_fifo(pHdl);
    }
}

PRIVATE void _uart_vRxHandler(UartHdl_t* pHdl, bool isEnd)
{
    uint8 read_level;
    uint8 data;
    uint8 i;

    if(pHdl->uart_port != E_AHI_UART_1)
    {
        DBG_WARN("Recv port Err\n");
        return;
    }

    if(pHdl->work_state == TX)
    {
        DBG_WARN("Recv can't on tx\n");
        return;
    }

    read_level = u8AHI_UartReadRxFifoLevel(pHdl->uart_port);

    if(read_level == 0)
    {
        DBG_WARN("Read Uart FIFO LEVEL ERR %s:%d\n", __FUNCTION__,
            __LINE__);
        return;
    }

    for(i=0; i<read_level; i++)
    {
        data = u8AHI_UartReadData(pHdl->uart_port);

        pHdl->rx_callback(pHdl, data);
    }
}

PRIVATE void _uart_vRxErrHandler(uint8 uart_port)
{
    //if there are bytes in fifo
    uint8 lineStatus = u8AHI_UartReadLineStatus(uart_port);
    uint8 dataInFifo = u8AHI_UartReadRxFifoLevel(uart_port);

    lineStatus = lineStatus;
    dataInFifo = dataInFifo;
    DBG_WARN("UART: RxErr %d %d\n", lineStatus, dataInFifo);
    vAHI_UartReset(uart_port, FALSE, TRUE);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
