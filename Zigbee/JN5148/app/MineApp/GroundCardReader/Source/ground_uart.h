#ifndef _GROUND_UART_H_
#define _GROUND_UART_H_

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* TX RX switch */
#define DIO_UART1_TX_ENABLE E_AHI_DIO11_INT
#define UART1_IN_TX()   (u32AHI_DioReadInput() & DIO_UART1_TX_ENABLE)

#define UART1_SWITCH_TX()                                   \
    do{                                                     \
        vAHI_DioSetOutput(DIO_UART1_TX_ENABLE, 0);          \
        TimerUtil_vDelay(1000, E_TIMER_UNIT_MICROSECOND);   \
    } while(!UART1_IN_TX())

#define UART1_SWITCH_RX()                                   \
    do{                                                     \
        TimerUtil_vDelay(1000, E_TIMER_UNIT_MICROSECOND);   \
        vAHI_DioSetOutput(0, DIO_UART1_TX_ENABLE);          \
    } while(UART1_IN_TX())

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

//baudrate
enum
{
    UART_BAUDRATE_460800 = 0,
    UART_BAUDRATE_115200 = 1,
    UART_BAUDRATE_38400 = 2,
    UART_BAUDRATE_9600 = 3,
    UART_BAUDRATE_KIND
};

typedef enum
{
    TX,
    RX
}uart_work_state_e;

struct uarthdl_t;
// typedefs
typedef void (*uart_tx_cb_t)(struct uarthdl_t* pHdl);
typedef void (*uart_rx_cb_t)(struct uarthdl_t* pHdl, uint8);

typedef struct uarthdl_t
{
    uint8 uart_port;
    uint8 baud_rate;

    /* status */
    uint8 work_state;
    uint8 sync_state;

    /* buf */
    uint8 *txBuf;
    uint16 tx_pos;
    uint16 tx_max;

    uint8 *rxBuf;
    uint16 rx_pos;
    uint16 rx_len;

    /* tx and rx done call back */
    uart_tx_cb_t tx_callback;
    uart_rx_cb_t rx_callback;
} UartHdl_t;

/****************************************************************************
* FUNCTIONS
 ****************************************************************************/

PUBLIC bool_t uart_init(UartHdl_t * const uarthdl);
PUBLIC uint8 uart_setbaudrate(UartHdl_t * const pHdl, uint8 u8BaudRate);
PUBLIC uint16 uart_getaddrbackoff(void);
PUBLIC void uart_tx_start(UartHdl_t * const);
PUBLIC bool_t uart_is_tx(UartHdl_t const*const pHdl);

#endif
