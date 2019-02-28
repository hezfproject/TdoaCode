

#ifndef HAL_UART_H
#define HAL_UART_H

#include "CC_DEF.h"

#ifdef __cplusplus
extern "C"
{
#endif

/***************************************************************************************************
 *                                             INCLUDES
 ***************************************************************************************************/
//#include "hal_board.h"

/***************************************************************************************************
 *                                              MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                            CONSTANTS
 ***************************************************************************************************/


/* UART Ports */

/*
   Serial Port Baudrate Settings
   Have to match with baudrate table
  
  Note that when using interrupt service based UART configuration (as opposed to DMA)
  higher baudrate such as 115200bps may have problem when radio is operational at the same time.
*/
#define HAL_UART_BR_9600   0x00
#define HAL_UART_BR_19200  0x01
#define HAL_UART_BR_38400  0x02
#define HAL_UART_BR_57600  0x03
#define HAL_UART_BR_115200 0x04

/* Frame Format constant */

/* Stop Bits */
#define HAL_UART_ONE_STOP_BIT       0x00
#define HAL_UART_TWO_STOP_BITS      0x01

/* Parity settings */
#define HAL_UART_NO_PARITY          0x00
#define HAL_UART_EVEN_PARITY        0x01
#define HAL_UART_ODD_PARITY         0x02

/* Character Size */
#define HAL_UART_8_BITS_PER_CHAR    0x00
#define HAL_UART_9_BITS_PER_CHAR    0x01

/* Flow control */
#define HAL_UART_FLOW_OFF   0x00
#define HAL_UART_FLOW_ON    0x01

/* Ports */
#define HAL_UART_PORT_0   0x00
#define HAL_UART_PORT_1   0x01
#define HAL_UART_PORT_MAX 0x02

/* UART Status */
#define  HAL_UART_SUCCESS        0x00
#define  HAL_UART_UNCONFIGURED   0x01
#define  HAL_UART_NOT_SUPPORTED  0x02
#define  HAL_UART_MEM_FAIL       0x03
#define  HAL_UART_BAUDRATE_ERROR 0x04

/* UART Events */
#define HAL_UART_RX_FULL         0x01
#define HAL_UART_RX_ABOUT_FULL   0x02
#define HAL_UART_RX_TIMEOUT      0x04
#define HAL_UART_TX_FULL         0x08
#define HAL_UART_TX_EMPTY        0x10

/***************************************************************************************************
 *                                             TYPEDEFS
 ***************************************************************************************************/

typedef void (*halUARTCBack_t) (uint8 port, uint8 event);

typedef struct
{
  // The head or tail is updated by the Tx or Rx ISR respectively, when not polled.
  volatile uint16 bufferHead;
  volatile uint16 bufferTail;
  uint16 maxBufSize;
  uint8 *pBuffer;
} halUARTBufControl_t;

typedef struct
{
  Bool                configured;
  uint32              baudRate;
  Bool                flowControl;
  uint16              flowControlThreshold;
  uint8               idleTimeout;
  halUARTBufControl_t rx;
  halUARTBufControl_t tx;
  Bool                intEnable;
  uint32              rxChRvdTime;
  halUARTCBack_t      callBackFunc;
}halUARTCfg_t;

/***************************************************************************************************
 *                                           GLOBAL VARIABLES
 ***************************************************************************************************/


/***************************************************************************************************
 *                                            FUNCTIONS - API
 ***************************************************************************************************/
/*
 *  Initialize UART at the startup
 */
extern void HalUARTInit ( void );

/*
 * Open a port based on the configuration
 */
extern uint8 HalUARTOpen ( uint8 port, halUARTCfg_t *config );

/*
 * Close a port
 */
extern void HalUARTClose ( uint8 port );

/*
 * Read a buffer from the UART
 */
extern uint16 HalUARTRead ( uint8 port, uint8 *pBuffer, uint16 length );

/*
 * Write a buff to the uart *
 */
extern uint16 HalUARTWrite ( uint8 port, uint8 *pBuffer, uint16 length );

/*
 * This to support polling
 */
extern void HalUARTPoll( void );

/*
 * Return the number of bytes in the Rx buffer
 */
extern uint16 Hal_UART_RxBufLen ( uint8 port );

/*
 * Return the number of bytes in the Tx buffer
 */
extern uint16 Hal_UART_TxBufLen ( uint8 port );

/*
 * Abort UART when entering sleep mode
 */
extern void HalUARTSuspend(void);

/*
 * Resume UART after wakeup from sleep
 */
extern void HalUARTResume(void);

extern void HalUARTConfig(halUARTCBack_t uartcb);


/***************************************************************************************************
***************************************************************************************************/

/*********************************************************************
 * LOCAL FUNCTIONS
 */

void HalUARTInitISR(void);
void HalUARTOpenISR(uint32 baudRate);
uint16 HalUARTReadISR(uint8 *buf, uint16 len);
uint16 HalUARTWriteISR(uint8 *buf, uint16 len);
void HalUARTPollISR(void);
uint16 HalUARTRxAvailISR(void);
void HalUARTSuspendISR(void);
void HalUARTResumeISR(void);

void HalUARTConfigISR(halUARTCBack_t uartcb);

#ifdef __cplusplus
}
#endif

#endif
