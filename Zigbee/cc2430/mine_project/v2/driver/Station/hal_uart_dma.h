#ifndef __HAL_UART_DMA__
#define __HAL_UART_DMA__



/*******************************************************************************
** Local macro define
*******************************************************************************/
// init return 
#define HAL_UARTDMA_INIT_SUCCESS                (0)
#define HAL_UARTDMA_INIT_BUF_INVALID            (1)
#define HAL_UARTDMA_INIT_CB_INVALID             (2)
#define HAL_UARTDMA_INIT_UARTCH_INVALID         (3)
#define HAL_UARTDMA_INIT_DMACH_INVALID          (4)
#define HAL_UARTDMA_INIT_HARDWARE_ERR           (5)

// start dma return
#define HAL_UARTDMA_START_SUCCESS            (0)
#define HAL_UARTDMA_START_BUSY            (1)

// check dma state
#define HAL_UARTDMA_FREE                    (0)
#define HAL_UARTDMA_BUSY                    (1)


#define HAL_UARTDMA_RX_LEN          (128)


/*******************************************************************************
** Description:
    call back function to handle received data
** Input param:
    pbuf    : pointer of receiving buf
    len     : received data length
*******************************************************************************/
typedef void (*hal_uartdma_cb)(void);

/*******************************************************************************
** Description:
    initial uart and dma for uart+dma transfer
** Input param:
    uartdma_data_handler    : callback function pointer
    uart_ch                 : channel number of uart(0 or 1)
    dma_ch_tx               : channel number of tx dma(0 to 4)
    dma_ch_rx               : channel number of rx dma(0 to 4)
    pbuf_rx                 : pointer of dma_tx buf
    pbuf_rx                 : pointer of dma_rx buf
** Return value:
    0       : success
    ohters  : fail
*******************************************************************************/
unsigned char hal_uartdma_init(const hal_uartdma_cb cb_rx, const hal_uartdma_cb cb_tx, const unsigned char uart_ch, 
                           const unsigned char dma_ch_tx, const unsigned char dma_ch_rx,
                           const unsigned char *pbuf_tx, const unsigned char *pbuf_rx);



/*******************************************************************************
** Description:
    start tx-dma by writing address, length and arm-bit
** Input param:
    pbuf    : tx buf pointer
    len     : length of tx data
** Return value:
    0       : success
    others  : fail
*******************************************************************************/
unsigned char hal_uartdma_start_tx(void);

/*******************************************************************************
** Description:
    re-config uart baudrate
** Input param:
    rate    : baud rate(0: 100K, 1:1M)
*******************************************************************************/
//void uartdma_change_baudrate(unsigned char rate);

/*******************************************************************************
** Description:
    check tx dma channel busy or not
** Return value:
    0       : not busy
    1       : busy
*******************************************************************************/
unsigned char hal_uartdma_TxBusy(void);

/*******************************************************************************
** Description:
    reset and restart rx-dma

*******************************************************************************/
void hal_uartdma_reset_rx(void);

/*******************************************************************************
** Description:
    set link status to driver
** Input params:
    sta     : link status(0, 1)
*******************************************************************************/
void hal_uartdma_set_linksta(unsigned char sta);

char hal_uartdma_config_tx(const unsigned char *pbuf, const unsigned char tx_len);
char hal_uartdma_config_rx(const unsigned char *pbuf, const unsigned char rx_len );


#endif  // __UART_DMA__
