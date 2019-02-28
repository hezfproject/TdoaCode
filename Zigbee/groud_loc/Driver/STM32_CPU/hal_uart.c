
/*********************************************************************
 * INCLUDES
 */

#include "CC_DEF.h"
#include "hal_uart.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

//#include "_hal_uart_isr.c"

/******************************************************************************
 * @fn      HalUARTInit
 *
 * @brief   Initialize the UART
 *
 * @param   none
 *
 * @return  none
 *****************************************************************************/
void HalUARTInit(void)
{
  HalUARTInitISR();
}

/******************************************************************************
 * @fn      HalUARTOpen
 *
 * @brief   Open a port according tp the configuration specified by parameter.
 *
 * @param   port   - UART port
 *          config - contains configuration information
 *
 * @return  Status of the function call
 *****************************************************************************/
uint8 HalUARTOpen(uint8 port, halUARTCfg_t *config)
{
  (void)port;
  (void)config;

  HalUARTOpenISR(config->baudRate);
  
  return HAL_UART_SUCCESS;
}

/*****************************************************************************
 * @fn      HalUARTRead
 *
 * @brief   Read a buffer from the UART
 *
 * @param   port - USART module designation
 *          buf  - valid data buffer at least 'len' bytes in size
 *          len  - max length number of bytes to copy to 'buf'
 *
 * @return  length of buffer that was read
 *****************************************************************************/
uint16 HalUARTRead(uint8 port, uint8 *buf, uint16 len)
{
  (void)port;
  (void)buf;
  (void)len;

  return HalUARTReadISR(buf, len);

}

/******************************************************************************
 * @fn      HalUARTWrite
 *
 * @brief   Write a buffer to the UART.
 *
 * @param   port - UART port
 *          buf  - pointer to the buffer that will be written, not freed
 *          len  - length of
 *
 * @return  length of the buffer that was sent
 *****************************************************************************/
uint16 HalUARTWrite(uint8 port, uint8 *buf, uint16 len)
{
  (void)port;
  (void)buf;
  (void)len;

  return HalUARTWriteISR(buf, len);
}

/******************************************************************************
 * @fn      HalUARTSuspend
 *
 * @brief   Suspend UART hardware before entering PM mode 1, 2 or 3.
 *
 * @param   None
 *
 * @return  None
 *****************************************************************************/
void HalUARTSuspend( void )
{
  HalUARTSuspendISR();
}

/******************************************************************************
 * @fn      HalUARTResume
 *
 * @brief   Resume UART hardware after exiting PM mode 1, 2 or 3.
 *
 * @param   None
 *
 * @return  None
 *****************************************************************************/
void HalUARTResume( void )
{
  HalUARTResumeISR();
}

/***************************************************************************************************
 * @fn      HalUARTPoll
 *
 * @brief   Poll the UART.
 *
 * @param   none
 *
 * @return  none
 *****************************************************************************/
void HalUARTPoll(void)
{
  HalUARTPollISR();
}

/**************************************************************************************************
 * @fn      Hal_UART_RxBufLen()
 *
 * @brief   Calculate Rx Buffer length - the number of bytes in the buffer.
 *
 * @param   port - UART port
 *
 * @return  length of current Rx Buffer
 **************************************************************************************************/
uint16 Hal_UART_RxBufLen( uint8 port )
{
  (void)port;

  return HalUARTRxAvailISR();
}

void UartInitialize(void)
{
#ifdef NT_SNIFFER
  uint32 br = 256000UL;
#else
  uint32 br = 115200UL;
#endif
  HalUARTOpenISR(br);
}

void AVR_UartInitialize(void)
{
  UartInitialize();
}

//#ifdef __AVR__
//static uint8 _UCSR0B;
//#define AVR_ConUartDisable() do{_UCSR0B = UCSR0B ; UCSR0B = 0;}while(0)
//#define AVR_ConUartEnable() do{UCSR0B = _UCSR0B;}while(0)
//#endif

#if 0
void con_putchar(uint8 c)
{
  HalUARTWriteISR(&c, 1);
}
#endif

void HalUARTConfig(halUARTCBack_t uartcb)
{
  HalUARTConfigISR(uartcb);
}

//#ifdef __TARGET_CPU_CORTEX_M3
#if 0
void con_UART_RCC_Configuration( void )
{
	// Enable GPIOA and USART1 clocks
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE ) ;
}

void con_UART_Configuration( void )
{
  UartInitialize();
}
#endif

/******************************************************************************
******************************************************************************/
