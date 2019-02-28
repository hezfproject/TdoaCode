#ifndef _UART_H_
#define _UART_H_

/****************************************************************************
* MICROS
 ****************************************************************************/
#define UART_MAX_RX_LEN 128
#define UART_MAX_TX_LEN 128

#define UART_FIFO_LEVEL 16

// returns
#define UART_SUCCESS 0
#define UART_INVALIDPARAM 1
#define UART_BUSY 2
// typedefs
typedef void (*uart_tx_cb_t)(uint8 port);
typedef void (*uart_rx_cb_t)(uint8 port, uint8* p, uint16 len);


/****************************************************************************
* FUNCTIONS
 ****************************************************************************/

 PUBLIC void vUart_Init(const uint8 uart_port, const uint8 uart_baud, const uart_tx_cb_t tx_callback, const uart_rx_cb_t rx_callback);
 PUBLIC uint8  u8Uart_StartTx(const uint8 uart_port, uint8 *p, uint16 len);
 PUBLIC bool  bUart_IsOnTx(const uint8 uart_port);

#endif
