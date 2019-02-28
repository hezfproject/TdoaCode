#include <ioCC2430.h>
#include <string.h>
#include "hal_uart_dma.h"
#include "bsmac_timer.h"
#include "crc.h"
/*******************************************************************************
** local macro define
*******************************************************************************/
#define MAX_UART_CHANNEL_NUM        (1)
#define MAX_DMA_CHANNEL_NUM         (4)

#define DMA_TX_LEN_DEFAULT          (128)
#define DMA_RX_LEN_DEFAULT          (128)

// UXCSR 
#define UART_MODE                   (0x80)
#define RECEIVER_ENABLE             (0x40)
// UXUCR
#define HIGH_STOP_BIT               (0x02)
// UXGCR
#define MSB_FIRST                   (0x20)
#define BAUD_E_1M                   (15)
#define BAUD_E_500K                 (14)
#define BAUD_E_100K                 (11)
// UXBAUD
#define BAUD_M_1M                   (0)
#define BAUD_M_500K                 (0)
#define BAUD_M_100K                 (153)
// UXDBUF
#define U0DBUF_H                    (0xDF)
#define U0DBUF_L                    (0xC1)
#define U1DBUF_H                    (0xDF)
#define U1DBUF_L                    (0xF9)

// DMA
#define TMODE_SINGLE                (0)
#define TRIG_URX0                   (14)
#define TRIG_UTX0                   (15)
#define TRIG_URX1                   (16)
#define TRIG_UTX1                   (17)
#define SRCINC_0                    (0x00)
#define SRCINC_1                    (0x40)
#define DESTINC_0                   (0x00)
#define DESTINC_1                   (0x10)
#define IRQMASK_EN                  (0x08)
#define M8_USE8                     (0x00)
#define PRIO_LOW                    (0x00)
#define PRIO_GUAR                   (0x01)
#define PRIO_HIGH                   (0x02)

// dma int
#define DMA_INT_ENABLE              do {IEN1 |= 0x01;} while (0)
#define DMA_INT_DISABLE             do {IEN1 &= ~0x01;} while (0)
#define DMA_INT_CLEAR(ch)           do {DMAIRQ &= ~(1 << ch);} while (0)

/*******************************************************************************
** local struct define
*******************************************************************************/
typedef struct _TDmaConf {
    unsigned char srcaddr_h;        // source address high byte
    unsigned char srcaddr_l;        // source address low byte
    unsigned char destaddr_h;       // destination address high byte
    unsigned char destaddr_l;       // destination address low byte
    unsigned char vlen_lenh5;       // vlen and bit[12:8] of transfer length
    unsigned char len;              // bit[7:0] of transfer length
    unsigned char mode_trig;        // mode and triger
    unsigned char ctrl;             // other control bits
}TDmaConf, *PDMACONF;

typedef struct _TUartDmaHdl {
    TDmaConf tx_dma_conf;               // dma config for tx
    TDmaConf rx_dma_conf;               // dma config for rx
    unsigned char tx_dma_ch;            // dma channel used for tx
    unsigned char rx_dma_ch;            // dma channel used for rx
    unsigned char uart_ch;              // uart channel
    unsigned char rx_buf[DMA_RX_LEN_DEFAULT];   // dma rx buf, coordinated with mac-layber buf to provide double-buf senario
    unsigned char *pbuf_tx;             // tx buf pointer
    unsigned char *pbuf_rx;             // rx buf pointer
    unsigned char link_up;              // linkup, now for debugging
//    unsigned char rx_buf0[DMA_RX_LEN_DEFAULT];  // buf1 for rx_dma_ch
//    unsigned char rx_buf1[DMA_RX_LEN_DEFAULT];  // buf2 for rx_dma_ch    
//    unsigned char rx_buf_num;           // buf number used currently, buf1 and buf2 will be ping-pong
    hal_uartdma_cb tx_cb_hdl;                  // tx call back handler
    hal_uartdma_cb rx_cb_hdl;                  // rx call back handler    
}TUartDmaHdl, *PUARTDMAHANDLE;

static TUartDmaHdl tUartDmaHdl;

/*******************************************************************************
** Local subroutine define
*******************************************************************************/
static char init_uartdma(void);
static void hal_uartdma_start_rx(void);


/*******************************************************************************
** Description:
    initial uart and dma hardware
** Input param:
    rate        : baud rate (0:100K, 1:1M)
** Return value:
    0           : success
    others      : fail
*******************************************************************************/
static char init_uartdma(void)
{
        
    switch (tUartDmaHdl.uart_ch) {
    case (0):
        // P0.2->RX, P0.3->TX, P0.4->CT, P0.5->RT
        P0SEL |= 0x3c;      // P0.2---P0.5 used as peripheral io

        U0CSR = (UART_MODE | RECEIVER_ENABLE);
        U0UCR = HIGH_STOP_BIT;
        U0GCR = BAUD_E_500K;    // ((rate == UARTDMA_BAUDRATE_100K) ? BAUD_E_100K : BAUD_E_1M);         // baud rate = 1M (BAUD_E = 15, BAUD_M = 0)
        U0BAUD = BAUD_M_500K;       // (rate == UARTDMA_BAUDRATE_100K) ? BAUD_M_100K : BAUD_M_1M;
        
        tUartDmaHdl.tx_dma_conf.destaddr_h = U0DBUF_H;
        tUartDmaHdl.tx_dma_conf.destaddr_l = U0DBUF_L;
        tUartDmaHdl.tx_dma_conf.mode_trig = TMODE_SINGLE | TRIG_UTX0;     
        
        tUartDmaHdl.rx_dma_conf.srcaddr_h = U0DBUF_H;
        tUartDmaHdl.rx_dma_conf.srcaddr_l = U0DBUF_L;
        tUartDmaHdl.rx_dma_conf.mode_trig = TMODE_SINGLE | TRIG_URX0;     
        
        break;
    case (1):
        // P1.7->RX, P1.6->TX, P1.5->RT, P1.4->CT
        P1SEL |= 0xf0;      // P1.7---P1.4 used as peripheral io
        
        U1CSR = (UART_MODE | RECEIVER_ENABLE);
        U1UCR = HIGH_STOP_BIT;
        U1GCR = BAUD_E_500K;    // ((rate == UARTDMA_BAUDRATE_100K) ? BAUD_E_100K : BAUD_E_1M);         // baud rate = 1M (BAUD_E = 15, BAUD_M = 0)
        U1BAUD = BAUD_M_500K;       // (rate == UARTDMA_BAUDRATE_100K) ? BAUD_M_100K : BAUD_M_1M;
        
        tUartDmaHdl.tx_dma_conf.destaddr_h = U1DBUF_H;
        tUartDmaHdl.tx_dma_conf.destaddr_l = U1DBUF_L;
        tUartDmaHdl.tx_dma_conf.mode_trig = TMODE_SINGLE | TRIG_UTX1;     
        
        tUartDmaHdl.rx_dma_conf.srcaddr_h = U1DBUF_H;
        tUartDmaHdl.rx_dma_conf.srcaddr_l = U1DBUF_L;
        tUartDmaHdl.rx_dma_conf.mode_trig = TMODE_SINGLE | TRIG_URX1;  
        
        break;
    default:
        return (char)-1;
    }

    hal_uartdma_config_tx(tUartDmaHdl.pbuf_tx, DMA_RX_LEN_DEFAULT);
    hal_uartdma_config_rx(tUartDmaHdl.rx_buf, DMA_RX_LEN_DEFAULT);
    
    // enable dma interrupt
    DMA_INT_ENABLE;
    
    // start rx dma
    hal_uartdma_start_rx();           // need fix length for rx???
    
    return HAL_UARTDMA_INIT_SUCCESS;
}



/*******************************************************************************
** Description:
    config tx buf address and tx length
** Return value:
    0           : success
    others      : fail
*******************************************************************************/
char hal_uartdma_config_tx(const unsigned char *pbuf, const unsigned char tx_len)
{
    tUartDmaHdl.tx_dma_conf.srcaddr_h = ((int)(pbuf) >> 8) & 0xff;
    tUartDmaHdl.tx_dma_conf.srcaddr_l = ((int)pbuf) & 0xff;
    tUartDmaHdl.tx_dma_conf.ctrl = SRCINC_1 | DESTINC_0 | IRQMASK_EN | M8_USE8 | PRIO_GUAR;

    tUartDmaHdl.tx_dma_conf.vlen_lenh5 = (tx_len & 0x1f00) >> 8;
    tUartDmaHdl.tx_dma_conf.len = tx_len & 0xff;

    if (tUartDmaHdl.tx_dma_ch == 0) {
        DMA0CFGH = (((int)(&(tUartDmaHdl.tx_dma_conf))) & 0xff00) >> 8;
        DMA0CFGL = ((int)(&(tUartDmaHdl.tx_dma_conf))) & 0xff;
    } else if (tUartDmaHdl.tx_dma_ch <= MAX_DMA_CHANNEL_NUM) {
        DMA1CFGH = (((int)(&(tUartDmaHdl.tx_dma_conf)) - (tUartDmaHdl.tx_dma_ch-1)*8) & 0xff00) >> 8;
        DMA1CFGL = ((int)(&(tUartDmaHdl.tx_dma_conf)) - (tUartDmaHdl.tx_dma_ch-1)*8) & 0xff;
    } else {
        return (char)-1;
    }

    return 0;
}

/*******************************************************************************
** Description:
    config rx buf address and rx length
** Return value:
    0           : success
    others      : fail
*******************************************************************************/
char hal_uartdma_config_rx(const unsigned char *pbuf, const unsigned char rx_len )
{
    tUartDmaHdl.rx_dma_conf.destaddr_h = ((int)(pbuf) >> 8) & 0xff;
    tUartDmaHdl.rx_dma_conf.destaddr_l = ((int)(pbuf)) & 0xff;
    tUartDmaHdl.rx_dma_conf.ctrl = SRCINC_0 | DESTINC_1 | IRQMASK_EN | M8_USE8 | PRIO_GUAR;

    tUartDmaHdl.rx_dma_conf.vlen_lenh5 = 0;
    tUartDmaHdl.rx_dma_conf.len = rx_len;    

    if (tUartDmaHdl.rx_dma_ch == 0) {
        DMA0CFGH = (((int)(&(tUartDmaHdl.rx_dma_conf))) & 0xff00) >> 8;
        DMA0CFGL = ((int)(&(tUartDmaHdl.rx_dma_conf))) & 0xff;
    } else if (tUartDmaHdl.rx_dma_ch <= MAX_DMA_CHANNEL_NUM) {
        DMA1CFGH = (((int)(&(tUartDmaHdl.rx_dma_conf)) - (tUartDmaHdl.rx_dma_ch-1)*8) & 0xff00) >> 8;
        DMA1CFGL = ((int)(&(tUartDmaHdl.rx_dma_conf)) - (tUartDmaHdl.rx_dma_ch-1)*8) & 0xff;
    } else {
        return (char)-1;
    }

    return 0;
}

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
unsigned char hal_uartdma_start_tx()
{
//    unsigned char rs = 0;
    
    // check dma-tx channel is free or not
//    rs = DMAARM;
//    if ((rs & (1 << tUartDmaHdl.tx_dma_ch)) != 0) {
    if (hal_uartdma_TxBusy() != HAL_UARTDMA_FREE) {
        return HAL_UARTDMA_BUSY;
    }    
    
    DMAARM |= (1 << tUartDmaHdl.tx_dma_ch);
    
    // for tx, need manually triger dma, to start uart tx for the first byte
    DMAREQ |= (1 << tUartDmaHdl.tx_dma_ch);
    
    return HAL_UARTDMA_START_SUCCESS;
}

/*******************************************************************************
** Description:
    start rx-dma by writing address, length and arm-bit

*******************************************************************************/
static void hal_uartdma_start_rx(void)
{   
    // no need check busy, since it's handled sequentially by driver itself
    
//    tUartDmaHdl.rx_dma_conf.vlen_lenh5 = 0;
//    tUartDmaHdl.rx_dma_conf.len = DMA_RX_LEN_DEFAULT;
    
    DMAARM |= (1 << tUartDmaHdl.rx_dma_ch);
    
    //bsmac_timer_start_listen_rxdma();
}


/*******************************************************************************
** Description:
    check tx dma channel busy or not
** Return value:
    0       : not busy
    1       : busy
*******************************************************************************/
unsigned char hal_uartdma_TxBusy(void)
{
    unsigned char rs = 0;

    rs = DMAARM;
    if ((rs & (1 <<  tUartDmaHdl.tx_dma_ch)) != 0) {
        return HAL_UARTDMA_BUSY;
    }  else {
        return HAL_UARTDMA_FREE;
    }
}

/*******************************************************************************
** Description:
    reset and restart rx-dma

*******************************************************************************/
void hal_uartdma_reset_rx(void)
{
    
    DMAARM = 0x80 | (1 << tUartDmaHdl.rx_dma_ch);   // abort rx channel
    
    hal_uartdma_start_rx(); 
}

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
unsigned char hal_uartdma_init(const hal_uartdma_cb cb_rx, const hal_uartdma_cb cb_tx, const unsigned char uart_port, 
                           const unsigned char dma_ch_tx, const unsigned char dma_ch_rx,
                           const unsigned char *pbuf_tx, const unsigned char *pbuf_rx) 
{
    char rs = 0;
 
    // clear and set handle
    memset(&tUartDmaHdl, 0, sizeof(TUartDmaHdl));
    
    // check input parameter
    if ((pbuf_tx == 0) || (pbuf_rx == 0)) {
        return HAL_UARTDMA_INIT_BUF_INVALID;
    }
    if (cb_rx == NULL || cb_tx == NULL) {
        return HAL_UARTDMA_INIT_CB_INVALID;
    } else {
        tUartDmaHdl.rx_cb_hdl= cb_rx;
        tUartDmaHdl.tx_cb_hdl= cb_tx;
    }
    if (uart_port > MAX_UART_CHANNEL_NUM) {
        return HAL_UARTDMA_INIT_UARTCH_INVALID;
    }
    if ( (dma_ch_tx > MAX_DMA_CHANNEL_NUM) || (dma_ch_rx > MAX_DMA_CHANNEL_NUM) || (dma_ch_tx == dma_ch_rx) ||(pbuf_tx == pbuf_rx)) {
        return HAL_UARTDMA_INIT_DMACH_INVALID;
    }
    
    
    tUartDmaHdl.uart_ch = uart_port;
    tUartDmaHdl.tx_dma_ch = dma_ch_tx;
    tUartDmaHdl.rx_dma_ch = dma_ch_rx;
    tUartDmaHdl.pbuf_tx = (unsigned char*)pbuf_tx;
    tUartDmaHdl.pbuf_rx = (unsigned char *)pbuf_rx;
    
    // init uart hardware
    rs = init_uartdma();
    if (rs != 0) {
        return HAL_UARTDMA_INIT_HARDWARE_ERR;
    }
    
    return HAL_UARTDMA_INIT_SUCCESS;
}


/*******************************************************************************
** Description:
    re-config uart baudrate
** Input param:
    rate    : baud rate(0: 100K, 1:1M)
*******************************************************************************/
/*
void uartdma_change_baudrate(unsigned char rate) 
{
    unsigned char rs = 0;
    
    // wait dma free, may cause hang, but don't worry, since timer will reconfig if timeout, and this guy must wait success, or link will not up
    do {
        rs = DMAARM;
    } while ((rs & (1 << tUartDmaHdl.tx_dma_ch)) != 0);
    
    init_uartdma(rate);
}
*/
/*******************************************************************************
** Description:
    set link status to driver
** Input params:
    sta     : link status(0, 1)
*******************************************************************************/
void hal_uartdma_set_linksta(unsigned char sta)
{
    tUartDmaHdl.link_up = sta;
}
/*******************************************************************************
** Description:
    dma interrupt handler
*******************************************************************************/
#pragma vector = DMA_VECTOR
__near_func __interrupt void  DMA_ISR(void)
{
    unsigned char rs = 0;

    //clear interrupt flag first; if there are new DMA interrupt coming, the DMA int flag will be set again and be served again
    IRCON &= ~1;

    rs = DMAIRQ;   
    
    // handle rx_dma interrupt
    if ((rs & (1 << tUartDmaHdl.rx_dma_ch)) != 0) {
        // clear dma int
        DMA_INT_CLEAR(tUartDmaHdl.rx_dma_ch);
        
        // for debugging
        if ((tUartDmaHdl.link_up == 1) && ((tUartDmaHdl.rx_buf[0] != 0x4d) || (tUartDmaHdl.rx_buf[1] != 0x41))) {
            tUartDmaHdl.rx_buf[0] = 0x5a;
        }                

        // copy rx data
        memcpy(tUartDmaHdl.pbuf_rx, tUartDmaHdl.rx_buf, DMA_RX_LEN_DEFAULT);
        // start rx dma
        hal_uartdma_start_rx();  
        
        
        // callback, so dma_rx can be in parallel with callback handling, but need buf lock?       
        tUartDmaHdl.rx_cb_hdl();        

    }
    
    // handle tx_dma interrupt
    if ((rs & (1 << tUartDmaHdl.tx_dma_ch)) != 0) {
        DMA_INT_CLEAR(tUartDmaHdl.tx_dma_ch);

        // callback
        tUartDmaHdl.tx_cb_hdl();
    }
}


