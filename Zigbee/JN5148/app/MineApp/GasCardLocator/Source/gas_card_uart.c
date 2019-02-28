
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>

#include "gas_card_uart.h"
#include "printf_util.h"
#include "MicroSpecific.h"
#include "GasLocate_protocol_V2.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_UART)
#define DBG(x) do{x} while (0);
#else
#define DBG(x)
#endif

#define CONVERT_ENDIAN(x)	SysUtil_vConvertEndian(&(x), sizeof(x))
#define UART_STATE_IDLE 0
#define UART_STATE_SYNC 1
#define UART_STATE_RECV 2
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
    uint16 rxLen;

    /* status */
    bool on_tx;
    // bool on_rx;

    /* tx and rx done call back */
    uart_tx_cb_t tx_callback;
    uart_rx_cb_t rx_callback;

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

uint8  state=UART_STATE_IDLE;
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

    switch (uart_baud)
    {
    case UART_BAUTRATE_1M:
    {
        //100k and 500k both use 15
        vAHI_UartSetClocksPerBit(uart_port, 15);
        vAHI_UartSetBaudDivisor(uart_port, 1);
        break;
    }
    case UART_BAUTRATE_500K:
    {
        //100k and 500k both use 15
        vAHI_UartSetClocksPerBit(uart_port, 15);
        vAHI_UartSetBaudDivisor(uart_port, 2);
        break;
    }
    case UART_BAUTRATE_200K:
    {
        //100k and 500k both use 15
        vAHI_UartSetClocksPerBit(uart_port, 15);
        vAHI_UartSetBaudDivisor(uart_port, 5);
        break;
    }
    case UART_BAUTRATE_115200:
    {
        vAHI_UartSetBaudDivisor(E_AHI_UART_1,46);
        vAHI_UartSetClocksPerBit(E_AHI_UART_1,2);
        break;
    }
    case UART_BAUTRATE_38400:
    {
        vAHI_UartSetBaudDivisor(E_AHI_UART_1,46);
        vAHI_UartSetClocksPerBit(E_AHI_UART_1,6);
        break;
    }
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
        UartHdl0.rxLen = 0;
        UartHdl0.on_tx = FALSE;
        // UartHdl0.on_rx = TRUE;
        UartHdl0.tx_callback = tx_callback;
        UartHdl0.rx_callback = rx_callback;
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
        UartHdl1.rxLen = 0;
        UartHdl1.on_tx = FALSE;
        // UartHdl1.on_rx = TRUE;
        UartHdl1.tx_callback = tx_callback;
        UartHdl1.rx_callback = rx_callback;
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
    if(pHdl->on_tx == TRUE)
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
//
    //PrintfUtil_vPrintf("len:%d\n",UartHdl1.txLen);
    //PrintfUtil_vPrintf("pADDR %x\n ",UartHdl1.txBuf);
    //PrintfUtil_vPrintMem(UartHdl1.txBuf,UartHdl1.txLen);


    for(i=0; i<UART_FIFO_LEVEL; i++ )
       // for(i=0; i<8; i++ )
    {
        vAHI_UartWriteData(pHdl->uart_port, *(pHdl->txBuf));
       
        pHdl->txBuf++;
        pHdl->txLen--;        
        if(pHdl->txLen == 0)
        {
            break;
        }
    }
    //PrintfUtil_vPrintf("len1:%d\n",UartHdl1.txLen);
#if 0
               // uint8 i;
                       PrintfUtil_vPrintf("ph=%d ",UartHdl1.txLen); 
                       for(i=0;i<UartHdl1.txLen;i++)
                       {
                              if(i%8==0)
                              {
                                   PrintfUtil_vPrintf("\n"); 
                              }
                              PrintfUtil_vPrintf("ph=%x ",*(UartHdl1.txBuf+i));
                       }
#endif

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
        //PrintfUtil_vPrintf("UART0\n"); 
        pHdl = &UartHdl0;
    }
    else if(u32Device == E_AHI_DEVICE_UART1)
    {
        //PrintfUtil_vPrintf("UART1\n"); 
        pHdl = &UartHdl1;
    }
    else
    {
        return;
    }

#if 0

    uint8 i;
           PrintfUtil_vPrintf("pAddr%x ",UartHdl1.txBuf); 
           PrintfUtil_vPrintf("ph=%d\n ",UartHdl1.txLen); 
            PrintfUtil_vPrintMem(UartHdl1.txBuf,UartHdl1.txLen);
#endif



    
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

PRIVATE void Uart_vTxHandler( UartHdl_t* pHdl )
{
    uint8 i;
    
    if(pHdl->txLen == 0)  /* send complete */
    {

        //uint32 intstore;
        //MICRO_DISABLE_AND_SAVE_INTERRUPTS(intstore);
        pHdl->on_tx = FALSE;
        if(pHdl->tx_callback!=NULL)
        {
            pHdl->tx_callback();
        }
        //MICRO_RESTORE_INTERRUPTS(intstore);
    }
    else			        /* continue to send */
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
    uint8 i;
    uint8 data;

    static uint8 datalen;

    read_level = u8AHI_UartReadRxFifoLevel(pHdl->uart_port);

    if(read_level == 0)
    {
        return;
    }

    for(i=0; i<read_level; i++)
    {

        data = u8AHI_UartReadData(pHdl->uart_port);

        switch(state)
        {
        case UART_STATE_IDLE:
        {
            if(data == UART_PREAMBLE_H)
            {
                state = UART_STATE_SYNC;
            }

            /* flush the buffer */
            pHdl->rxLen = 0;
            break;
        }
        case UART_STATE_SYNC:
        {
            if(data == UART_PREAMBLE_H)
            {
                state = UART_STATE_SYNC;
                pHdl->rxLen = 0;
            }
            else if(data == UART_PREAMBLE_L)
            {
                state = UART_STATE_RECV;
            }
            else
            {
                /* flush the buffer */
                state = UART_STATE_IDLE;
                pHdl->rxLen = 0;
            }
            break;
        }
        case UART_STATE_RECV:
        {
            break;
        }
        }

        pHdl->rxBuf[pHdl->rxLen++] = data;

        if(pHdl->rxLen < sizeof(Uart_Header_t))
        {
            continue;
        }

        else if(pHdl->rxLen == sizeof(Uart_Header_t))
        {
            Uart_Header_t* pheader = (Uart_Header_t*)pHdl->rxBuf;

            //CONVERT_ENDIAN(pheader->len);
            //CONVERT_ENDIAN(pheader->checksum);
            if(pheader->len <=UART_MAX_RX_LEN)
            {
                datalen = pheader->len;
            }
            else
            {
                state = UART_STATE_IDLE;
                pHdl->rxLen = 0;
            }
        }
        else if(pHdl->rxLen > sizeof(Uart_Header_t))
        {
            if(datalen > 0)
            {
                datalen--;
            }
        }

        if(datalen ==0 || pHdl->rxLen>=UART_MAX_RX_LEN)
        {

            if(pHdl->rx_callback!=NULL)
            {
                pHdl->rx_callback(pHdl->rxBuf, pHdl->rxLen);
            }

            state = UART_STATE_IDLE;
            pHdl->rxLen = 0;
        }
    }
}


PRIVATE void Uart_vRxErrHandler(uint8 uart_port)
{
    //if there are bytes in fifo

    //uint8 lineStatus = u8AHI_UartReadLineStatus(uart_port);
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
