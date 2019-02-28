
/*********************************************************************
 * INCLUDES
 */
#include "CC_DEF.h"
#include "hal_uart.h"
#include "CPU.h"

#include "LED.h"

//=============================================================================
//stm32 lib header file
#include "stm32f10x.h"			 
#include "system_stm32f10x.h"


/*********************************************************************
 * MACROS
 */
// 定义串口0相关寄存器
//#ifdef __AVR__
//#ifndef UCSR0A
//	#define UBRR0H      UBRRH
//	#define UBRR0L      UBRRL
//	#define UCSR0A      UCSRA
//	#define UCSR0B      UCSRB
//	#define UCSR0C      UCSRC
//	#define RXCIE0      RXCIE
//	#define UDR0        UDR
//	#define TXC0        TXC
//	#define RXC0        RXC
//	#define RXEN0       RXEN
//	#define TXEN0       TXEN
//	#define UPM01		UPM1
//	#define	UPM00		UPM0
//	#define UCSZ01      UCSZ1
//	#define UCSZ00      UCSZ0
//	#define URSEL0      URSEL
//#endif
//#ifndef	SIG_USART0_RECV
//	#define SIG_USART0_RECV	SIG_USART_RECV
//#endif
//#ifndef SIG_USART0_DATA
//	#define SIG_USART0_DATA SIG_USART_DATA
//#endif
//#endif

//#define HAL_UART_ASSERT(expr)        HAL_ASSERT((expr))
#define HAL_UART_ASSERT(expr)

#define HAL_UART_ISR_RX_AVAIL() \
  (isrCfg.rxTail >= isrCfg.rxHead) ? \
  (isrCfg.rxTail - isrCfg.rxHead) : \
  (HAL_UART_ISR_RX_MAX - isrCfg.rxHead + isrCfg.rxTail)

#define HAL_UART_ISR_TX_AVAIL() \
  (isrCfg.txHead > isrCfg.txTail) ? \
  (isrCfg.txHead - isrCfg.txTail - 1) : \
  (HAL_UART_ISR_TX_MAX - isrCfg.txTail + isrCfg.txHead - 1)

/*********************************************************************
 * CONSTANTS
 */
//#ifdef __AVR__
//#define ConBaudReg(b)          (((F_CPU*10)/(16UL*b)-5)/10)
//#endif


#if !defined HAL_UART_ISR_RX_MAX
#ifdef NT_SNIFFER
#define HAL_UART_ISR_RX_MAX        256
#else
#define HAL_UART_ISR_RX_MAX        256
#endif
#endif
#if !defined HAL_UART_ISR_TX_MAX
#define HAL_UART_ISR_TX_MAX        HAL_UART_ISR_RX_MAX
#endif
#if !defined HAL_UART_ISR_HIGH
#define HAL_UART_ISR_HIGH         (HAL_UART_ISR_RX_MAX / 2 - 16)
#endif
#if !defined HAL_UART_ISR_IDLE
#define HAL_UART_ISR_IDLE         4 //4ms
#endif


/*********************************************************************
 * TYPEDEFS
 */

typedef struct
{
  uint8 rxBuf[HAL_UART_ISR_RX_MAX];

  volatile uint8 rxHead;
  volatile uint8 rxTail;

  uint8 rxTick;
  uint8 rxShdw;

  uint8 txBuf[HAL_UART_ISR_TX_MAX];

  volatile uint8 txHead;
  volatile uint8 txTail;

  uint8 txMT;

  halUARTCBack_t uartCB;
} uartISRCfg_t;

/*********************************************************************
 * GLOBAL VARIABLES

 */
extern volatile uint8 systicks;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static uartISRCfg_t isrCfg;

//=============================================================================
// 串口接收中断

	#ifndef CON_UART_BUFFER_SIZE
		#define CON_UART_BUFFER_SIZE    64
	#endif

#if CON_UART_PORT != 0


	#if CON_UART_BUFFER_SIZE > 0

		static __IO Bool ConBufferFull = False ;
		static __IO uint8_t AT_CommandIndex = 0, IAP_CommandIndex = 0 ;

		#if CON_UART_BUFFER_SIZE == 1
			static __IO uint8_t  ConRecvBuffer ;
		#else
			static __IO uint8_t  ConRecvBuffer[ CON_UART_BUFFER_SIZE ] ;
			static __IO uint16_t ConReceiveIndex = 0, ConReadOutIndex = 0 ;
		#endif

//在_hal_uart_irq.c中已定义
#if 0
		void USART1_IRQHandler( void )
		{
			uint8_t c ;
			if( USART_GetITStatus( USART1, USART_IT_RXNE ) == RESET )
				return ;
			c = USART_ReceiveData( USART1 ) ;
			#if CON_UART_BUFFER_SIZE == 1
				ConRecvBuffer = c ;
				ConBufferFull = True ;
			#else
				if( ! ConBufferFull )
				{
					ConRecvBuffer[ConReceiveIndex++] = c ;
					if( ConReceiveIndex >= CON_UART_BUFFER_SIZE )
						ConReceiveIndex = 0 ;
					if( ConReceiveIndex == ConReadOutIndex )
						ConBufferFull = True ;
				}
			#endif
			if( AT_CommandIndex == 0 )
			{
				if( c == AT_CommandKey[0] )
					AT_CommandIndex = 1 ;
			}
			else if( AT_CommandIndex < sizeof(AT_CommandKey)-1 )
			{
				if( c == AT_CommandKey[AT_CommandIndex] )
					AT_CommandIndex ++ ;
				else
					AT_CommandIndex = 0 ;
			}
			if( IAP_CommandIndex == 0 )
			{
				if( c == IAP_EnterKey[0] )
					IAP_CommandIndex = 1 ;
			}
			else if( IAP_CommandIndex < sizeof(IAP_EnterKey)-1 )
			{
				if( c == IAP_EnterKey[IAP_CommandIndex] )
					IAP_CommandIndex ++ ;
				else
					IAP_CommandIndex = 0 ;
			}
		}
#endif

	#endif

#endif

//-----------------------------------------------------------------------------
// 初始化

void con_UART_RCC_Configuration( void )
{
	#if CON_UART_PORT != 0
		// Enable GPIOA and USART1 clocks
		RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE ) ;
	#endif
}

void con_UART_Configuration( void )
{
	#if CON_UART_PORT != 0	   //使用串口1

		NVIC_InitTypeDef  NVIC_InitStructure ;
		GPIO_InitTypeDef  GPIO_InitStructure ;
		USART_InitTypeDef USART_InitStructure ;

		// USART1 Interrupt
		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn ;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1 ;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE ;
		NVIC_Init( &NVIC_InitStructure ) ;

		// Configure USART1 Rx (PA.10) as input floating
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 ;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING ;
		GPIO_Init( GPIOA, &GPIO_InitStructure ) ;

		// Configure USART1 Tx (PA.09) as alternate function push-pull
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP ;
		GPIO_Init( GPIOA, &GPIO_InitStructure ) ;

		// USART1 Configuration
		USART_InitStructure.USART_BaudRate = 115200; //CON_UART_BAUDRATE ;
		USART_InitStructure.USART_StopBits = USART_StopBits_1 ;
		USART_InitStructure.USART_Parity = USART_Parity_No ;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b ;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None ;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx ;
		USART_Init( USART1, &USART_InitStructure ) ;

		// Enable the UART Receive interrupt
		#if CON_UART_BUFFER_SIZE > 0
			USART_ITConfig( USART1, USART_IT_RXNE, ENABLE ) ;
		#endif

		// Enable UART
		USART_Cmd( USART1, ENABLE ) ;
	#endif
}

//-----------------------------------------------------------------------------
// 串口输入输出函数

void con_putchar( uint8 c )
{
	#if CON_UART_PORT != 0
		USART_SendData( USART1, c ) ;
		while( USART_GetFlagStatus( USART1, USART_FLAG_TC ) == RESET ) ;
	#endif
}

uint8 con_kbhit( void )
{
	#if ( CON_UART_PORT != 0 ) && ( CON_UART_BUFFER_SIZE > 0 )
		if( ConBufferFull == True )
			return 1 ;
		#if CON_UART_BUFFER_SIZE > 1
			if( ConReadOutIndex != ConReceiveIndex )
				return 1 ;
		#endif
	#endif
	return 0 ;
}

uint8 con_getchar( void )
{
	#if ( CON_UART_PORT != 0 ) && ( CON_UART_BUFFER_SIZE > 0 )
		#if CON_UART_BUFFER_SIZE == 1
			if( ConBufferFull = True )
			{
				ConBufferFull = False ;
				return ConRecvBuffer ;
			}
		#else
			uint8 c ;
			if( con_kbhit() )
			{
				c = ConRecvBuffer[ConReadOutIndex] ;
				if( ++ConReadOutIndex >= CON_UART_BUFFER_SIZE )
					ConReadOutIndex = 0 ;
				ConBufferFull = False ;
				return c ;
			}
		#endif
	#endif
	return 0 ;
}

/******************************************************************************
 * @fn      HalUARTInitISR
 *
 * @brief   Initialize the UART
 *
 * @param   none
 *
 * @return  none
 *****************************************************************************/
void HalUARTInitISR(void)
{
	
}
#if 1

/******************************************************************************
 * @fn      HalUARTOpenISR
 *
 * @brief   Open a port according tp the configuration specified by parameter.
 *
 * @param   config - contains configuration information
 *
 * @return  none
 *****************************************************************************/
void HalUARTOpenISR(uint32 baudRate)
{
  isrCfg.uartCB = 0;
  isrCfg.rxHead = isrCfg.rxTail = isrCfg.txHead = isrCfg.txTail = 0;
  // Only supporting subset of baudrate for code size - other is possible.

//#ifdef __AVR__
//  UCSR0A = 0;
//  UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);
//
//#if defined(URSEL) || defined(URSEL0)
//	#define USE_URSEL   (1<<URSEL0)
//#else
//	#define USE_URSEL   0
//#endif
//
//  UCSR0C = USE_URSEL|(1<<UCSZ01)|(1<<UCSZ00);
//  UBRR0H = (uint8)(ConBaudReg(baudRate) >> 8) ;
//  UBRR0L = (uint8)(ConBaudReg(baudRate) & 0xFF) ;
//#endif

//#ifdef __TARGET_CPU_CORTEX_M3
  {
    NVIC_InitTypeDef  NVIC_InitStructure ;
    GPIO_InitTypeDef  GPIO_InitStructure ;
    USART_InitTypeDef USART_InitStructure ;

    // USART1 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn ;//USART1_IRQChannel ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1 ;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE ;
    NVIC_Init( &NVIC_InitStructure ) ;

    // Configure USART1 Rx (PA.10) as input floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING ;
    GPIO_Init( GPIOA, &GPIO_InitStructure ) ;

    // Configure USART1 Tx (PA.09) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP ;
    GPIO_Init( GPIOA, &GPIO_InitStructure ) ;

    // USART1 Configuration
    USART_InitStructure.USART_BaudRate = baudRate ;
    USART_InitStructure.USART_StopBits = USART_StopBits_1 ;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None ;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx ;
    USART_Init( USART1, &USART_InitStructure ) ;

    // Enable the UART Receive interrupt
    USART_ITConfig( USART1, USART_IT_RXNE, ENABLE ) ;

    // Enable UART
    USART_Cmd( USART1, ENABLE ) ;
  }
//#endif
}

/*****************************************************************************
 * @fn      HalUARTReadISR
 *
 * @brief   Read a buffer from the UART
 *
 * @param   buf  - valid data buffer at least 'len' bytes in size
 *          len  - max length number of bytes to copy to 'buf'
 *
 * @return  length of buffer that was read
 *****************************************************************************/
uint16 HalUARTReadISR(uint8 *buf, uint16 len)
{
  uint16 cnt = 0;

  while ((isrCfg.rxHead != isrCfg.rxTail) && (cnt < len))
  {
    *buf++ = isrCfg.rxBuf[isrCfg.rxHead++];
    if (isrCfg.rxHead >= HAL_UART_ISR_RX_MAX)
    {
      isrCfg.rxHead = 0;
    }
    cnt++;
  }

  return cnt;
}

/******************************************************************************
 * @fn      HalUARTWriteISR
 *
 * @brief   Write a buffer to the UART.
 *
 * @param   buf - pointer to the buffer that will be written, not freed
 *          len - length of
 *
 * @return  length of the buffer that was sent
 *****************************************************************************/
uint16 HalUARTWriteISR(uint8 *buf, uint16 len)
{
  uint16 cnt;

  // Enforce all or none.
  uint8 avail = HAL_UART_ISR_TX_AVAIL();
  if (avail < len)
  {
    return 0;
  }

  for (cnt = 0; cnt < len; cnt++)
  {
    isrCfg.txBuf[isrCfg.txTail] = *buf++;
    isrCfg.txMT = 0;

    if (isrCfg.txTail >= HAL_UART_ISR_TX_MAX-1)
    {
      isrCfg.txTail = 0;
    }
    else
    {
      isrCfg.txTail++;
    }

    // Keep re-enabling ISR as it might be keeping up with this loop due to other ints.
//#ifdef __AVR__
//    UCSR0B |= (1<<UDRIE0);
//#endif
//#ifdef __TARGET_CPU_CORTEX_M3
    USART_ITConfig( USART1, USART_IT_TXE, ENABLE);
//#endif
  }

  return cnt;
}

/******************************************************************************
 * @fn      HalUARTPollISR
 *
 * @brief   Poll a USART module implemented by ISR.
 *
 * @param   none
 *
 * @return  none
 *****************************************************************************/
void HalUARTPollISR(void)
{
  if (isrCfg.uartCB)
  {
    uint16 cnt;
    uint8 evt, tmp;
    CpuDisableInterrupt();
    cnt = HAL_UART_ISR_RX_AVAIL();
    CpuEnableInterrupt();
    evt = 0;

    tmp = systicks;

    if (isrCfg.rxTick)
    {
      uint8 decr = tmp - isrCfg.rxShdw;

      if (isrCfg.rxTick > decr)
      {
        isrCfg.rxTick -= decr;
      }
      else
      {
        isrCfg.rxTick = 0;
      }
    }
    isrCfg.rxShdw = tmp;

    if (cnt >= HAL_UART_ISR_RX_MAX-1)
    {
      evt = HAL_UART_RX_FULL;
    }
	#if 0
    else if (cnt >= HAL_UART_ISR_HIGH)
    {
      evt = HAL_UART_RX_ABOUT_FULL;
    }
	#endif
    else if (cnt && !isrCfg.rxTick)
    {
      evt = HAL_UART_RX_TIMEOUT;
    }

    if (isrCfg.txMT)
    {
      isrCfg.txMT = 0;
      evt |= HAL_UART_TX_EMPTY;
    }

    if (evt)
    {
      isrCfg.uartCB(0, evt);
    }
  }
}

/**************************************************************************************************
 * @fn      HalUARTRxAvailISR()
 *
 * @brief   Calculate Rx Buffer length - the number of bytes in the buffer.
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 **************************************************************************************************/
uint16 HalUARTRxAvailISR(void)
{
  return HAL_UART_ISR_RX_AVAIL();
}

/******************************************************************************
 * @fn      HalUARTSuspendISR
 *
 * @brief   Suspend UART hardware before entering PM mode 1, 2 or 3.
 *
 * @param   None
 *
 * @return  None
 *****************************************************************************/
void HalUARTSuspendISR( void )
{
}

/******************************************************************************
 * @fn      HalUARTResumeISR
 *
 * @brief   Resume UART hardware after exiting PM mode 1, 2 or 3.
 *
 * @param   None
 *
 * @return  None
 *****************************************************************************/
void HalUARTResumeISR( void )
{
}
#endif

void HalUARTConfigISR(halUARTCBack_t uartcb)
{
  isrCfg.uartCB = uartcb;
}

/***************************************************************************************************
 * @fn      halUartRxIsr
 *
 * @brief   UART Receive Interrupt
 *
 * @param   None
 *
 * @return  None
 ***************************************************************************************************/
#ifdef __AVR__
SIGNAL( SIG_USART0_RECV )
{
  uint8 tmp = UDR0;
  isrCfg.rxBuf[isrCfg.rxTail] = tmp;

  // Re-sync the shadow on any 1st byte received.
  if (isrCfg.rxHead == isrCfg.rxTail)
  {
    isrCfg.rxShdw = systicks;
  }

  if (++isrCfg.rxTail >= HAL_UART_ISR_RX_MAX)
  {
    isrCfg.rxTail = 0;
  }

  isrCfg.rxTick = HAL_UART_ISR_IDLE;
}
#endif

/***************************************************************************************************
 * @fn      halUartTxIsr
 *
 * @brief   UART Transmit Interrupt
 *
 * @param   None
 *
 * @return  None
 ***************************************************************************************************/
#ifdef __AVR__
SIGNAL( SIG_USART0_DATA )
{
  if (isrCfg.txHead == isrCfg.txTail)
  {
    UCSR0B &= ~(1<<UDRIE0);
    isrCfg.txMT = 1;
  }
  else
  {
    UDR0 = isrCfg.txBuf[isrCfg.txHead++];

    if (isrCfg.txHead >= HAL_UART_ISR_TX_MAX)
    {
      isrCfg.txHead = 0;
    }
  }
}
#endif

//#ifdef __TARGET_CPU_CORTEX_M3
void USART1_IRQHandler( void )
{
  //receive
  if( USART_GetITStatus( USART1, USART_IT_RXNE ) == SET ){
    uint8 tmp = USART_ReceiveData( USART1 ) ;
    isrCfg.rxBuf[isrCfg.rxTail] = tmp;

    // Re-sync the shadow on any 1st byte received.
    if (isrCfg.rxHead == isrCfg.rxTail){
      isrCfg.rxShdw = systicks;
    }

    if (++isrCfg.rxTail >= HAL_UART_ISR_RX_MAX){
      isrCfg.rxTail = 0;
    }

    isrCfg.rxTick = HAL_UART_ISR_IDLE;
  }

  //send
  if( USART_GetITStatus( USART1, USART_IT_TXE ) == SET ){
    if (isrCfg.txHead == isrCfg.txTail){
      USART_ITConfig( USART1, USART_IT_TXE , DISABLE);
      isrCfg.txMT = 1;
    }else{
      USART_SendData( USART1, isrCfg.txBuf[isrCfg.txHead++]);

      if (isrCfg.txHead >= HAL_UART_ISR_TX_MAX){
        isrCfg.txHead = 0;
      }
    }
  }

  USART_ClearITPendingBit( USART1, USART_IT_TXE | USART_IT_RXNE);

  return ;
}
//#endif




/******************************************************************************
******************************************************************************/
