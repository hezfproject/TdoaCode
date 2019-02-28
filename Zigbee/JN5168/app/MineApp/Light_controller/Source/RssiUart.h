#ifndef _RSSI_UART_H_
#define _RSSI_UART_H_

/****************************************************************************
* MICROS
 ****************************************************************************/
#define UART_MAX_RX_LEN 256
#define UART_MAX_TX_LEN 2048

#define UART_FIFO_LEVEL 16

// returns
#define UART_SUCCESS 0
#define UART_INVALIDPARAM 1
#define UART_BUSY 2

//baudrate 
#define UART_BAUDRATE_460800	0
#define UART_BAUDRATE_115200 	1
#define UART_BAUDRATE_38400 	2
#define UART_BAUDRATE_9600 	        3


// typedefs
typedef void (*uart_tx_cb_t)(void);
typedef void (*uart_rx_cb_t)(uint8* p, uint16 len);


/****************************************************************************
* FUNCTIONS
 ****************************************************************************/

 PUBLIC void vUart_Init(const uint8 uart_baud, const uart_tx_cb_t tx_callback, const uart_rx_cb_t rx_callback);
 PUBLIC void vUart_SetBaudRate(uint8 u8BaudRate);
 PUBLIC uint16 vUart_GetAddrBackOff(void);
 PUBLIC uint8  u8Uart_StartTx(uint8 *p, uint16 len);
 PUBLIC bool  bUart_IsOnTx(void);

#endif
