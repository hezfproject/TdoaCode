/**************************************************************************************************
Filename:       _hal_uart.c
Revised:        $Date: 2010/08/19 17:58:58 $
Revision:       $Revision: 1.14 $

Description: This file contains the interface to the H/W UART driver.


Copyright 2006-2007 Texas Instruments Incorporated. All rights reserved.

IMPORTANT: Your use of this Software is limited to those specific rights
granted under the terms of a software license agreement between the user
who downloaded the software, his/her employer (which must be your employer)
and Texas Instruments Incorporated (the "License").  You may not use this
Software unless you agree to abide by the terms of the License. The License
limits your use, and you acknowledge, that the Software may not be modified,
copied or distributed unless embedded on a Texas Instruments microcontroller
or used solely and exclusively in conjunction with a Texas Instruments radio
frequency transceiver, which is integrated into your product.  Other than for
the foregoing purpose, you may not use, reproduce, copy, prepare derivative
works of, modify, distribute, perform, display or sell this Software and/or
its documentation for any purpose.

YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

Should you have any questions regarding your right to use this Software,
contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

/*********************************************************************
* INCLUDES
*/

#include "hal_types.h"
#include "hal_assert.h"
#include "hal_board.h"
#include "hal_defs.h"
#if defined( HAL_UART_DMA ) && HAL_UART_DMA
#include "hal_dma.h"
#endif
#include "hal_mcu.h"
#include "hal_uart.h"
#include "osal.h"
#if (defined CFG_AMMETER_MODBUS ) || (defined CFG_AMMETER_IEC)
#include "delay.h"
#endif
/*********************************************************************
* MACROS
*/

#if !defined ( HAL_UART_DEBUG )
#define HAL_UART_DEBUG  FALSE
#endif

#if !defined ( HAL_UART_CLOSE )
#define HAL_UART_CLOSE  FALSE
#endif

#if !defined ( HAL_UART_BIG_TX_BUF )
#define HAL_UART_BIG_TX_BUF  FALSE
#endif

/*
*  The MAC_ASSERT macro is for use during debugging.
*  The given expression must evaluate as "true" or else fatal error occurs.
*  At that point, the call stack feature of the debugger can pinpoint where
*  the problem occurred.
*
*  To disable this feature and save code size, the project should define
*  HAL_UART_DEBUG to FALSE.
*/
#if ( HAL_UART_DEBUG )
#define HAL_UART_ASSERT( expr)        HAL_ASSERT( expr )
#else
#define HAL_UART_ASSERT( expr )
#endif

#define P2DIR_PRIPO               0xC0
#if HAL_UART_0_ENABLE
#define HAL_UART_PRIPO          0x00
#else
#define HAL_UART_PRIPO          0x40
#endif

#define HAL_UART_0_PERCFG_BIT     0x01  // USART0 on P0, so clear this bit.
#define HAL_UART_0_P0_RX_TX       0x0c  // Peripheral I/O Select for Rx/Tx.
#define HAL_UART_0_P0_RTS         0x10  // Peripheral I/O Select for RTS.
#define HAL_UART_0_P0_CTS         0x20  // Peripheral I/O Select for CTS.

#define HAL_UART_1_PERCFG_BIT     0x02  // USART1 on P1, so set this bit.
#define HAL_UART_1_P1_RTS         0x10  // Peripheral I/O Select for RTS.
#define HAL_UART_1_P1_CTS         0x20  // Peripheral I/O Select for CTS.
#define HAL_UART_1_P1_RX_TX       0xC0  // Peripheral I/O Select for Rx/Tx.

#define TX_AVAIL( cfg ) \
	((cfg->txTail == cfg->txHead) ? (cfg->txMax-1) : \
	((cfg->txTail >  cfg->txHead) ? (cfg->txTail - cfg->txHead - 1) : \
	(cfg->txMax - cfg->txHead + cfg->txTail)))

#define RX0_FLOW_ON  ( P0 &= ~HAL_UART_0_P0_CTS )
#define RX0_FLOW_OFF ( P0 |= HAL_UART_0_P0_CTS )
#define RX1_FLOW_ON  ( P1 &= ~HAL_UART_1_P1_CTS)
#define RX1_FLOW_OFF ( P1 |= HAL_UART_1_P1_CTS )

#define RX_STOP_FLOW( cfg ) { \
	if ( !(cfg->flag & UART_CFG_U1F) ) \
  { \
  RX0_FLOW_OFF; \
  } \
  else \
  { \
  RX1_FLOW_OFF; \
  } \
  if ( cfg->flag & UART_CFG_DMA ) \
  { \
  cfg->rxTick = DMA_RX_DLY; \
  } \
  cfg->flag |= UART_CFG_RXF; \
}

#define RX_STRT_FLOW( cfg ) { \
	if ( !(cfg->flag & UART_CFG_U1F) ) \
  { \
  RX0_FLOW_ON; \
  } \
  else \
  { \
  RX1_FLOW_ON; \
  } \
  cfg->flag &= ~UART_CFG_RXF; \
}

#define UART_RX_AVAIL( cfg ) \
	( (cfg->rxHead >= cfg->rxTail) ? (cfg->rxHead - cfg->rxTail) : \
	(cfg->rxMax - cfg->rxTail + cfg->rxHead +1 ) )

/* Need to leave enough of the Rx buffer free to handle the incoming bytes
* after asserting flow control, but before the transmitter has obeyed it.
* At the max expected baud rate of 115.2k, 16 bytes will only take ~1.3 msecs,
* but at the min expected baud rate of 38.4k, they could take ~4.2 msecs.
* SAFE_RX_MIN and DMA_RX_DLY must both be consistent according to
* the min & max expected baud rate.
*/
#if !defined( SAFE_RX_MIN )
#define SAFE_RX_MIN  48  // bytes - max expected per poll @ 115.2k
// 16 bytes @ 38.4 kBaud -> 4.16 msecs -> 138 32-kHz ticks.
#define DMA_RX_DLY  140
//  2 bytes @ 38.4 kBaud -> 0.52 msecs ->  17 32-kHz ticks.
#define DMA_TX_DLY   20
#endif

// The timeout tick is at 32-kHz, so multiply msecs by 33.
#define RX_MSECS_TO_TICKS  33

// The timeout only supports 1 byte.
#if !defined( HAL_UART_RX_IDLE )
#define HAL_UART_RX_IDLE  (6 * RX_MSECS_TO_TICKS)
#endif

// Only supporting 1 of the 2 USART modules to be driven by DMA at a time.
#if HAL_UART_DMA == 1
#define DMATRIG_RX  HAL_DMA_TRIG_URX0
#define DMATRIG_TX  HAL_DMA_TRIG_UTX0
#define DMA_UDBUF   HAL_DMA_U0DBUF
#define DMA_PAD     U0BAUD
#elif HAL_UART_DMA == 2
#define DMATRIG_RX  HAL_DMA_TRIG_URX1
#define DMATRIG_TX  HAL_DMA_TRIG_UTX1
#define DMA_UDBUF   HAL_DMA_U1DBUF
#define DMA_PAD     U1BAUD
#endif

#define DMA_RX( cfg ) { \
	volatile uint8 ft2430 = U0DBUF; \
	\
	halDMADesc_t *ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_RX ); \
	\
	HAL_DMA_SET_DEST( ch, cfg->rxBuf ); \
	\
	HAL_DMA_SET_LEN( ch, cfg->rxMax ); \
	\
	HAL_DMA_CLEAR_IRQ( HAL_DMA_CH_RX ); \
	\
	HAL_DMA_ARM_CH( HAL_DMA_CH_RX ); \
}

#define DMA_TX( cfg ) { \
	halDMADesc_t *ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_TX ); \
	\
	HAL_DMA_SET_SOURCE( ch, (cfg->txBuf + cfg->txTail) ); \
	\
	HAL_DMA_SET_LEN( ch, cfg->txCnt ); \
	\
	HAL_DMA_CLEAR_IRQ( HAL_DMA_CH_TX ); \
	\
	HAL_DMA_ARM_CH( HAL_DMA_CH_TX ); \
	\
	HAL_DMA_START_CH( HAL_DMA_CH_TX ); \
}
#if (defined CFG_AMMETER_MODBUS ) || (defined CFG_AMMETER_IEC)  // P0_1 TxRx switch
#define UART0_SWITCHTO_TX st(P0 |=  0x02;)
#define UART0_SWITCHTO_RX st(P0 &= ~0x02;)
#define UART0_GETTXRXSWITCH 	(P0_1)
#define UART0_TXRXSWITCH_TX 	(1)
#define UART0_TXRXSWITCH_RX 	(0)

#endif

/*********************************************************************
* TYPEDEFS
*/

typedef struct
{
	uint8 *rxBuf;
	uint8 rxHead;
	uint8 rxTail;
	uint8 rxMax;
	uint8 rxCnt;
	uint8 rxTick;
	uint8 rxHigh;

	uint8 *txBuf;
#if HAL_UART_BIG_TX_BUF
	uint16 txHead;
	uint16 txTail;
	uint16 txMax;
	uint16 txCnt;
#else
	uint8 txHead;
	uint8 txTail;
	uint8 txMax;
	uint8 txCnt;
#endif
	uint8 txTick;

	uint8 flag;

	uint8 baudRate;
#if(defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_101) 
	uint8 linkFormat;
#endif

#ifdef CFG_AMMETER_CMDLINE
	uint8 			workMode;
	halUARTCBack_t 	rxCBCmdLine;
#endif
	halUARTCBack_t rxCB;
} uartCfg_t;

/*********************************************************************
* CONSTANTS
*/

// Used by DMA macros to shift 1 to create a mask for DMA registers.
#define HAL_DMA_CH_TX    3
#define HAL_DMA_CH_RX    4

#define HAL_DMA_U0DBUF  0xDFC1
#define HAL_DMA_U1DBUF  0xDFF9

// UxCSR - USART Control and Status Register.
#define CSR_MODE      0x80
#define CSR_RE        0x40
#define CSR_SLAVE     0x20
#define CSR_FE        0x10
#define CSR_ERR       0x08
#define CSR_RX_BYTE   0x04
#define CSR_TX_BYTE   0x02
#define CSR_ACTIVE    0x01

// UxUCR - USART UART Control Register.
#define UCR_FLUSH     0x80
#define UCR_FLOW      0x40
#define UCR_D9        0x20
#define UCR_BIT9      0x10
#define UCR_PARITY    0x08
#define UCR_SPB       0x04
#define UCR_STOP      0x02
#define UCR_START     0x01

#define UTX0IE        0x04
#define UTX1IE        0x08

#define UART_CFG_U1F  0x80  // USART1 flag bit.
#define UART_CFG_DMA  0x40  // Port is using DMA.
#define UART_CFG_FLW  0x20  // Port is using flow control.
#define UART_CFG_SP4  0x10
#define UART_CFG_SP3  0x08
#define UART_CFG_SP2  0x04
#define UART_CFG_RXF  0x02  // Rx flow is disabled.
#define UART_CFG_TXF  0x01  // Tx is in process.



/*********************************************************************
* GLOBAL VARIABLES
*/
static uint32 timeUs=0;
/*********************************************************************
* GLOBAL FUNCTIONS
*/

/*********************************************************************
* LOCAL VARIABLES
*/

#if HAL_UART_0_ENABLE
static uartCfg_t *cfg0;
#endif
#if HAL_UART_1_ENABLE
static uartCfg_t *cfg1;
#endif

/*********************************************************************
* LOCAL FUNCTIONS
*/

#if HAL_UART_DMA
static void pollDMA( uartCfg_t *cfg );
#endif
#if HAL_UART_ISR
static void pollISR( uartCfg_t *cfg );
#endif
static void  HalUartChangeRate(uint8 port, uint8 baudRate);

static void HalUartGetTimePerByte(uint32 *pTime, uint8 baudRate);
#if(defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_101) 
static void  HalUartChangeLinkFormat(uint8 port, uint8 linkFormat);
static uint8  HalUartAddParity(uint8 data, bool isEven);
static uint8  HalUartRemoveParity(uint8 data, bool isEven);
#endif 
#ifdef CFG_AMMETER_CMDLINE
static void halUartPrintPrompt(uint8 port, bool isHavePrompt);
static uint16 HalUARTQuickWrite( uint8 port,uint8 data);
#endif
#if HAL_UART_DMA
/******************************************************************************
* @fn      pollDMA
*
* @brief   Poll a USART module implemented by DMA.
*
* @param   cfg - USART configuration structure.
*
* @return  none
*****************************************************************************/
static void pollDMA( uartCfg_t *cfg )
{
	const uint8 cnt = cfg->rxHead;
	uint8 *pad = cfg->rxBuf+(cfg->rxHead*2);

	// Pack the received bytes to the front of the queue.
	while ( (*pad == DMA_PAD) && (cfg->rxHead < cfg->rxMax) )
	{
		cfg->rxBuf[cfg->rxHead++] = *(pad+1);
		pad += 2;
	}

	if ( !(cfg->flag & UART_CFG_RXF) )
	{
		/* It is necessary to stop Rx flow and wait for H/W-enqueued bytes still
		* incoming to stop before resetting the DMA Rx engine. If DMA Rx is
		* aborted during incoming data, a byte may be lost inside the engine
		* during the 2-step transfer process of read/write.
		*/
		if ( cfg->rxHead >= (cfg->rxMax - SAFE_RX_MIN) )
		{
			RX_STOP_FLOW( cfg );
		}
		// If anything received, reset the Rx idle timer.
		else if ( cfg->rxHead != cnt )
		{
			cfg->rxTick = HAL_UART_RX_IDLE;
		}
	}
	else if ( !cfg->rxTick && (cfg->rxHead == cfg->rxTail) )
	{
		HAL_DMA_ABORT_CH( HAL_DMA_CH_RX );
		cfg->rxHead = cfg->rxTail = 0;
		osal_memset( cfg->rxBuf, ~DMA_PAD, cfg->rxMax*2 );
		DMA_RX( cfg );
		RX_STRT_FLOW( cfg );
	}

	if ( HAL_DMA_CHECK_IRQ( HAL_DMA_CH_TX ) )
	{
		HAL_DMA_CLEAR_IRQ( HAL_DMA_CH_TX );
		cfg->flag &= ~UART_CFG_TXF;
		cfg->txTick = DMA_TX_DLY;

		if ( (cfg->txMax - cfg->txCnt) < cfg->txTail )
		{
			cfg->txTail = 0;  // DMA can only run to the end of the Tx buffer.
		}
		else
		{
			cfg->txTail += cfg->txCnt;
		}
	}
	else if ( !(cfg->flag & UART_CFG_TXF) && !cfg->txTick )
	{
		if ( cfg->txTail != cfg->txHead )
		{
			if ( cfg->txTail < cfg->txHead )
			{
				cfg->txCnt = cfg->txHead - cfg->txTail;
			}
			else  // Can only run DMA engine up to max, then restart at zero.
			{
				cfg->txCnt = cfg->txMax - cfg->txTail + 1;
			}

			cfg->flag |= UART_CFG_TXF;
			DMA_TX( cfg );
		}
	}
}
#endif

#if HAL_UART_ISR
/******************************************************************************
* @fn      pollISR
*
* @brief   Poll a USART module implemented by ISR.
*
* @param   cfg - USART configuration structure.
*
* @return  none
*****************************************************************************/
static void pollISR( uartCfg_t *cfg )
{
	uint8 cnt = UART_RX_AVAIL( cfg );

	if ( !(cfg->flag & UART_CFG_RXF) )
	{
		// If anything received, reset the Rx idle timer.
		if ( cfg->rxCnt != cnt )
		{
			cfg->rxTick = HAL_UART_RX_IDLE;
			cfg->rxCnt = cnt;
		}

		/* It is necessary to stop Rx flow in advance of a full Rx buffer because
		* bytes can keep coming while sending H/W fifo flushes.
		*/
		if ( cfg->rxCnt >= (cfg->rxMax - SAFE_RX_MIN) )
		{
			RX_STOP_FLOW( cfg );
		}
	}
}
#endif

/******************************************************************************
* @fn      HalUARTInit
*
* @brief   Initialize the UART
*
* @param   none
*
* @return  none
*****************************************************************************/
void HalUARTInit( void )
{
#if HAL_UART_DMA
	halDMADesc_t *ch;
#endif

	// Set P2 priority - USART0 over USART1 if both are defined.
	P2DIR &= ~P2DIR_PRIPO;
	P2DIR |= HAL_UART_PRIPO;

#if HAL_UART_0_ENABLE
	// Set UART0 I/O location to P0.
	PERCFG &= ~HAL_UART_0_PERCFG_BIT;

	/* Enable Tx and Rx on P0 */
	P0SEL |= HAL_UART_0_P0_RX_TX;

#if (defined CFG_AMMETER_MODBUS ) || (defined CFG_AMMETER_IEC)
	//Init rxtx switch, P0_1.
	P0SEL &=  ~0x02;
	P0DIR |= 0x02;
	UART0_SWITCHTO_RX;
	//DelayUs(100);
#endif

	/* Make sure ADC doesnt use this */
	ADCCFG &= ~HAL_UART_0_P0_RX_TX;

	/* Mode is UART Mode */
	U0CSR = CSR_MODE;

	/* Flush it */
	U0UCR = UCR_FLUSH;
#endif

#if HAL_UART_1_ENABLE
	// Set UART1 I/O location to P1.
	PERCFG |= HAL_UART_1_PERCFG_BIT;

	/* Enable Tx and Rx on P1 */
	P1SEL  |= HAL_UART_1_P1_RX_TX;

	/* Make sure ADC doesnt use this */
	ADCCFG &= ~HAL_UART_1_P1_RX_TX;

	/* Mode is UART Mode */
	U1CSR = CSR_MODE;

	/* Flush it */
	U1UCR = UCR_FLUSH;
#endif

#if HAL_UART_DMA
	// Setup Tx by DMA.
	ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_TX );

	// The start address of the destination.
	HAL_DMA_SET_DEST( ch, DMA_UDBUF );

	// Using the length field to determine how many bytes to transfer.
	HAL_DMA_SET_VLEN( ch, HAL_DMA_VLEN_USE_LEN );

	// One byte is transferred each time.
	HAL_DMA_SET_WORD_SIZE( ch, HAL_DMA_WORDSIZE_BYTE );

	// The bytes are transferred 1-by-1 on Tx Complete trigger.
	HAL_DMA_SET_TRIG_MODE( ch, HAL_DMA_TMODE_SINGLE );
	HAL_DMA_SET_TRIG_SRC( ch, DMATRIG_TX );

	// The source address is decremented by 1 byte after each transfer.
	HAL_DMA_SET_SRC_INC( ch, HAL_DMA_SRCINC_1 );

	// The destination address is constant - the Tx Data Buffer.
	HAL_DMA_SET_DST_INC( ch, HAL_DMA_DSTINC_0 );

	// The DMA is to be polled and shall not issue an IRQ upon completion.
	HAL_DMA_SET_IRQ( ch, HAL_DMA_IRQMASK_DISABLE );

	// Xfer all 8 bits of a byte xfer.
	HAL_DMA_SET_M8( ch, HAL_DMA_M8_USE_8_BITS );

	// DMA Tx has shared priority for memory access - every other one.
	HAL_DMA_SET_PRIORITY( ch, HAL_DMA_PRI_HIGH );

	// Setup Rx by DMA.
	ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_RX );

	// The start address of the source.
	HAL_DMA_SET_SOURCE( ch, DMA_UDBUF );

	// Using the length field to determine how many bytes to transfer.
	HAL_DMA_SET_VLEN( ch, HAL_DMA_VLEN_USE_LEN );

	/* The trick is to cfg DMA to xfer 2 bytes for every 1 byte of Rx.
	* The byte after the Rx Data Buffer is the Baud Cfg Register,
	* which always has a known value. So init Rx buffer to inverse of that
	* known value. DMA word xfer will flip the bytes, so every valid Rx byte
	* in the Rx buffer will be preceded by a DMA_PAD char equal to the
	* Baud Cfg Register value.
	*/
	HAL_DMA_SET_WORD_SIZE( ch, HAL_DMA_WORDSIZE_WORD );

	// The bytes are transferred 1-by-1 on Rx Complete trigger.
	HAL_DMA_SET_TRIG_MODE( ch, HAL_DMA_TMODE_SINGLE );
	HAL_DMA_SET_TRIG_SRC( ch, DMATRIG_RX );

	// The source address is constant - the Rx Data Buffer.
	HAL_DMA_SET_SRC_INC( ch, HAL_DMA_SRCINC_0 );

	// The destination address is incremented by 1 word after each transfer.
	HAL_DMA_SET_DST_INC( ch, HAL_DMA_DSTINC_1 );

	// The DMA is to be polled and shall not issue an IRQ upon completion.
	HAL_DMA_SET_IRQ( ch, HAL_DMA_IRQMASK_DISABLE );

	// Xfer all 8 bits of a byte xfer.
	HAL_DMA_SET_M8( ch, HAL_DMA_M8_USE_8_BITS );

	// DMA has highest priority for memory access.
	HAL_DMA_SET_PRIORITY( ch, HAL_DMA_PRI_HIGH );
#endif
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
uint8 HalUARTOpen( uint8 port, halUARTCfg_t *config )
{
	uartCfg_t **cfgPP = NULL;
	uartCfg_t *cfg;

#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		cfgPP = &cfg0;
	}
#endif

#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		cfgPP = &cfg1;
	}
#endif

	HAL_UART_ASSERT( cfgPP );

#if HAL_UART_CLOSE
	// Protect against user re-opening port before closing it.
	HalUARTClose( port );
#else
	HAL_UART_ASSERT( *cfgPP == NULL );
#endif

	//HAL_UART_ASSERT( (config->baudRate == HAL_UART_BR_38400) ||
	//                 (config->baudRate == HAL_UART_BR_115200) );

	/* Whereas runtime heap alloc can be expected to fail - one-shot system
	* initialization must succeed, so no check for alloc fail.
	*/
	*cfgPP = (uartCfg_t *)osal_mem_alloc( sizeof( uartCfg_t ) );
	cfg = *cfgPP;
	HAL_UART_ASSERT( cfg );

	cfg->rxMax = config->rx.maxBufSize;

#if !HAL_UART_BIG_TX_BUF
	HAL_UART_ASSERT( (config->tx.maxBufSize < 256) );
#endif
	cfg->txMax = config->tx.maxBufSize;
	cfg->txBuf = osal_mem_alloc( cfg->txMax+1 );

	cfg->rxHead = cfg->rxTail = 0;
	cfg->txHead = cfg->txTail = 0;
	cfg->rxHigh = config->rx.maxBufSize - config->flowControlThreshold;
	cfg->rxCB = config->callBackFunc;
#ifdef CFG_AMMETER_CMDLINE
	cfg->workMode = config->workMode;
	cfg->rxCBCmdLine = config->CallBackFuncCmdLine;
#endif
#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		HalUartChangeRate(port, config->baudRate);
		U0CSR |= CSR_RE;

#if HAL_UART_DMA == 1
		cfg->flag = UART_CFG_DMA;
		HAL_UART_ASSERT( (config->rx.maxBufSize <= 128) );
		HAL_UART_ASSERT( (config->rx.maxBufSize > SAFE_RX_MIN) );
		cfg->rxBuf = osal_mem_alloc( cfg->rxMax*2 );
		osal_memset( cfg->rxBuf, ~DMA_PAD, cfg->rxMax*2 );
		DMA_RX( cfg );
#else
		cfg->flag = 0;
		HAL_UART_ASSERT( (config->rx.maxBufSize < 256) );
		cfg->rxBuf = osal_mem_alloc( cfg->rxMax+1 );
		URX0IE = 1;
		IEN2 |= UTX0IE;
#endif
		// 8 bits/char; no parity; 1 stop bit; stop bit hi.
		if ( config->flowControl )
		{
			cfg->flag |= UART_CFG_FLW;
			U0UCR = UCR_FLOW | UCR_STOP;
			// Must rely on H/W for RTS (i.e. Tx stops when receiver negates CTS.)
			P0SEL |= HAL_UART_0_P0_RTS;
			// Cannot use H/W for CTS as DMA does not clear the Rx bytes properly.
			P0DIR |= HAL_UART_0_P0_CTS;
			RX0_FLOW_ON;
		}
		else
		{
			U0UCR = UCR_STOP;
		}
	}
#endif

#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		// Only supporting 38400 or 115200 for code size - other is possible.
		U1BAUD = (config->baudRate == HAL_UART_BR_38400) ? 59 : 216;
		U1GCR = (config->baudRate == HAL_UART_BR_38400) ? 10 : 11;

		U1CSR |= CSR_RE;

#if HAL_UART_DMA == 2
		cfg->flag = (UART_CFG_U1F | UART_CFG_DMA);
		HAL_UART_ASSERT( (config->rx.maxBufSize <= 128) );
		HAL_UART_ASSERT( (config->rx.maxBufSize > SAFE_RX_MIN) );
		cfg->rxBuf = osal_mem_alloc( cfg->rxMax*2 );
		osal_memset( cfg->rxBuf, ~DMA_PAD, cfg->rxMax*2 );
		DMA_RX( cfg );
#else
		cfg->flag = UART_CFG_U1F;
		HAL_UART_ASSERT( (config->rx.maxBufSize < 256) );
		cfg->rxBuf = osal_mem_alloc( cfg->rxMax+1 );
		URX1IE = 1;
		IEN2 |= UTX1IE;
#endif

		// 8 bits/char; no parity; 1 stop bit; stop bit hi.
		if ( config->flowControl )
		{
			cfg->flag |= UART_CFG_FLW;
			U1UCR = UCR_FLOW | UCR_STOP;
			// Must rely on H/W for RTS (i.e. Tx stops when receiver negates CTS.)
			P1SEL |= HAL_UART_1_P1_RTS;
			// Cannot use H/W for CTS as DMA does not clear the Rx bytes properly.
			P1DIR |= HAL_UART_1_P1_CTS;
			RX1_FLOW_ON;
		}
		else
		{
			U1UCR = UCR_STOP;
		}
	}
#endif

	return HAL_UART_SUCCESS;
}

/******************************************************************************
* @fn      HalUARTClose
*
* @brief   Close the UART
*
* @param   port - UART port
*
* @return  none
*****************************************************************************/
void HalUARTClose( uint8 port )
{
#if HAL_UART_CLOSE
	uartCfg_t *cfg;

#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		U0CSR &= ~CSR_RE;
#if HAL_UART_DMA == 1
		HAL_DMA_ABORT_CH( HAL_DMA_CH_RX );
		HAL_DMA_ABORT_CH( HAL_DMA_CH_TX );
#else
		URX0IE = 0;
#endif
		cfg = cfg0;
		cfg0 = NULL;
	}
#endif
#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		U1CSR &= ~CSR_RE;
#if HAL_UART_DMA == 2
		HAL_DMA_ABORT_CH( HAL_DMA_CH_RX );
		HAL_DMA_ABORT_CH( HAL_DMA_CH_TX );
#else
		URX1IE = 0;
#endif
		cfg = cfg1;
		cfg1 = NULL;
	}
#endif

	if ( cfg )
	{
		if ( cfg->rxBuf )
		{
			osal_mem_free( cfg->rxBuf );
		}
		if ( cfg->txBuf )
		{
			osal_mem_free( cfg->txBuf );
		}
		osal_mem_free( cfg );
	}
#endif
}

/******************************************************************************
* @fn      HalUARTPoll
*
* @brief   Poll the UART.
*
* @param   none
*
* @return  none
*****************************************************************************/
void HalUARTPoll( void )
{
#if ( HAL_UART_0_ENABLE | HAL_UART_1_ENABLE )
	static uint8 tickShdw;
	uartCfg_t *cfg;
	uint8 tick;

#if HAL_UART_0_ENABLE
	if ( cfg0 )
	{
		cfg = cfg0;
	}
#endif
#if HAL_UART_1_ENABLE
	if ( cfg1 )
	{
		cfg = cfg1;
	}
#endif

	// Use the LSB of the sleep timer (ST0 must be read first anyway).
	tick = ST0 - tickShdw;
	tickShdw = ST0;

	do
	{
		if ( cfg->txTick > tick )
		{
			cfg->txTick -= tick;
		}
		else
		{
			cfg->txTick = 0;
		}

		if ( cfg->rxTick > tick )
		{
			cfg->rxTick -= tick;
		}
		else
		{
			cfg->rxTick = 0;
		}

#if HAL_UART_ISR
#if HAL_UART_DMA
		if ( cfg->flag & UART_CFG_DMA )
		{
			pollDMA( cfg );
		}
		else
#endif
		{
			pollISR( cfg );
		}
#elif HAL_UART_DMA
		pollDMA( cfg );
#endif

		/* The following logic makes continuous callbacks on any eligible flag
		* until the condition corresponding to the flag is rectified.
		* So even if new data is not received, continuous callbacks are made.
		*/
		if ( cfg->rxHead != cfg->rxTail )
		{
			uint8 evt;

			if ( cfg->rxHead >= (cfg->rxMax - SAFE_RX_MIN) )
			{
				evt = HAL_UART_RX_FULL;
			}
			else if ( cfg->rxHigh && (cfg->rxHead >= cfg->rxHigh) )
			{
				evt = HAL_UART_RX_ABOUT_FULL;
			}
			else if ( cfg->rxTick == 0 )
			{
				evt = HAL_UART_RX_TIMEOUT;
			}
			else
			{
				evt = 0;
			}

			if ( evt && cfg->rxCB )
			{
				cfg->rxCB( ((cfg->flag & UART_CFG_U1F)!=0), evt );
			}
		}

#if HAL_UART_0_ENABLE
		if ( cfg == cfg0 )
		{
#if HAL_UART_1_ENABLE
			if ( cfg1 )
			{
				cfg = cfg1;
			}
			else
#endif
				break;
		}
		else
#endif
			break;

	} while ( TRUE );
#else
	return;
#endif
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
	uartCfg_t *cfg = NULL;

#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		cfg = cfg0;
	}
#endif
#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		cfg = cfg1;
	}
#endif

	HAL_UART_ASSERT( cfg );

	return UART_RX_AVAIL( cfg );
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
uint16 HalUARTRead( uint8 port, uint8 *buf, uint16 len )
{
	uartCfg_t *cfg = NULL;
	uint8 cnt = 0;

#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		cfg = cfg0;
	}
#endif
#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		cfg = cfg1;
	}
#endif
#if (defined CFG_AMMETER_MODBUS ) || (defined CFG_AMMETER_IEC)
	if(UART0_GETTXRXSWITCH == UART0_TXRXSWITCH_TX)
	{
		return 0;
	}
#endif
	HAL_UART_ASSERT( cfg );

	while ( (cfg->rxTail != cfg->rxHead) && (cnt < len) )
	{
#if(defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_101) 
		if(cfg->linkFormat == HAL_UART_LINKFORMAT_0 || cfg->linkFormat == HAL_UART_LINKFORMAT_2)
		{
			*buf++ = cfg->rxBuf[cfg->rxTail];
		}
		else if(cfg->linkFormat == HAL_UART_LINKFORMAT_1)
		{
			*buf++ = HalUartRemoveParity(cfg->rxBuf[cfg->rxTail], true);
		}
#else
		*buf++ = cfg->rxBuf[cfg->rxTail];
#endif
		if ( cfg->rxTail == cfg->rxMax )
		{
			cfg->rxTail = 0;
		}
		else
		{
			cfg->rxTail++;
		}
		cnt++;
	}

#if HAL_UART_DMA
#if HAL_UART_ISR
	if ( cfg->flag & UART_CFG_DMA )
#endif
	{
		/* If there is no flow control on a DMA-driven UART, the Rx Head & Tail
		* pointers must be reset to zero after every read in order to preserve the
		* full length of the Rx buffer. This implies that every Read must read all
		* of the Rx bytes available, or the pointers will not be reset and the
		* next incoming packet may not fit in the Rx buffer space remaining - thus
		* the end portion of the incoming packet that does not fit would be lost.
		*/
		if ( !(cfg->flag & UART_CFG_FLW) )
		{
			// This is a trick to trigger the DMA abort and restart logic in pollDMA.
			cfg->flag |= UART_CFG_RXF;
		}
	}
#endif

#if HAL_UART_ISR
#if HAL_UART_DMA
	if ( !(cfg->flag & UART_CFG_DMA) )
#endif
	{
		cfg->rxCnt = UART_RX_AVAIL( cfg );

		if ( cfg->flag & UART_CFG_RXF )
		{
			if ( cfg->rxCnt < (cfg->rxMax - SAFE_RX_MIN) )
			{
				RX_STRT_FLOW( cfg );
			}
		}
	}
#endif
	return cnt;

}

/******************************************************************************
* @fn      HalUARTWrite
*
* @brief   Write a buffer to the UART.
*
* @param   port    - UART port
*          pBuffer - pointer to the buffer that will be written, not freed
*          length  - length of
*
* @return  length of the buffer that was sent
*****************************************************************************/
uint16 HalUARTWrite( uint8 port, uint8 *buf, uint16 len )
{
	uartCfg_t *cfg = NULL;
	uint8 cnt;
	
	if(len ==0) return 0;

#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		cfg = cfg0;
	}
#endif
#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		cfg = cfg1;
	}
#endif

	HAL_UART_ASSERT( cfg );
halIntState_t intState;
HAL_ENTER_CRITICAL_SECTION(intState);
	if ( cfg->txHead == cfg->txTail )
	{
#if HAL_UART_DMA
		// When pointers are equal, reset to zero to get max len w/out wrapping.
		cfg->txHead = cfg->txTail = 0;
#endif
#if HAL_UART_ISR
#if HAL_UART_DMA
		if ( !(cfg->flag & UART_CFG_DMA) )
#endif
		{
			cfg->flag &= ~UART_CFG_TXF;
		}
#endif
	}

	// Accept "all-or-none" on write request.
	if ( TX_AVAIL( cfg ) < len )
	{
             HAL_EXIT_CRITICAL_SECTION(intState);      
		return 0;
	}
#if (defined CFG_AMMETER_MODBUS ) || (defined CFG_AMMETER_IEC)
#if HAL_UART_0_ENABLE
	UART0_SWITCHTO_TX;
	// useless
	//DelayUs(2000);
#endif
#endif
	for ( cnt = len; cnt; cnt--)
	{
#if(defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_101) 
		if(cfg->linkFormat == HAL_UART_LINKFORMAT_0 || cfg->linkFormat == HAL_UART_LINKFORMAT_2)
		{
			cfg->txBuf[ cfg->txHead ] = *buf++;
		}
		else if(cfg->linkFormat == HAL_UART_LINKFORMAT_1)
		{
			cfg->txBuf[ cfg->txHead ] = HalUartAddParity(*buf, true);
			buf++;
		}
#else
		cfg->txBuf[ cfg->txHead ] = *buf++;
#endif

		if ( cfg->txHead == cfg->txMax )
		{
			cfg->txHead = 0;
		}
		else
		{
			cfg->txHead++;
		}
	}
 HAL_EXIT_CRITICAL_SECTION(intState);

#if HAL_UART_ISR
#if HAL_UART_DMA
	if ( !(cfg->flag & UART_CFG_DMA) )
#endif
	{
		if ( !(cfg->flag & UART_CFG_TXF) && len )
		{
			cfg->flag |= UART_CFG_TXF;
			if ( !(cfg->flag & UART_CFG_U1F) )
			{
				U0DBUF = cfg->txBuf[cfg->txTail];
			}
			else
			{
				U1DBUF = cfg->txBuf[cfg->txTail];
			}
		}
	}
#endif

	return len;
}

void HalUARTFlushTxBuf( uint8 port)
{
	uartCfg_t *cfg = NULL;
#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		cfg = cfg0;
	}
#endif
#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		cfg = cfg1;
	}
#endif
	cfg->txHead = 0;
	cfg->txTail = 0;
}
void HalUARTFlushRxBuf( uint8 port)
{
	uartCfg_t *cfg = NULL;
#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		cfg = cfg0;
	}
#endif
#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		cfg = cfg1;
	}
#endif	
	cfg->rxHead = 0;
	cfg->rxTail = 0;
}

uint8 HalUARTIoctl ( uint8 port, uint8 cmd, halUARTIoctl_t *pIoctl )
{
	uartCfg_t *cfg = NULL;
#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		cfg = cfg0;
	}
#endif
#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		cfg = cfg1;
	}
#endif	
	/* only support  baudrate at present */

	if(cmd == HAL_UART_IOCTL_SET)
	{
		HalUartChangeRate(port, pIoctl->baudRate);
#if(defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_101) 
		HalUartChangeLinkFormat(port, pIoctl->linkFormat);
#endif
#ifdef CFG_AMMETER_CMDLINE
		cfg->workMode = pIoctl->workMode;
#endif
	}
	else if(cmd == HAL_UART_IOCTL_GET)
	{
		pIoctl->baudRate = cfg->baudRate;
#if(defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_101) 
		pIoctl->linkFormat= cfg->linkFormat;
#endif
#ifdef CFG_AMMETER_CMDLINE
		pIoctl->workMode = cfg->workMode;
#endif

	}
	return 0;
}

void  HalUartChangeRate(uint8 port, uint8 baudRate)
{
	uartCfg_t *cfg=NULL;
#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		cfg = cfg0;
	}
#endif

#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		cfg = cfg1;
	}
#endif
	cfg->baudRate = baudRate;
#if HAL_UART_0_ENABLE

	if ( port == HAL_UART_PORT_0 )
	{
		switch(baudRate)
		{
		case HAL_UART_BR_300:
			{
				U0BAUD = 59;
				U0GCR = 3;
				break;
			}		
		case HAL_UART_BR_600:
			{
				U0BAUD = 59;
				U0GCR = 4;
				break;
			}		
		case HAL_UART_BR_1200:
			{
				U0BAUD = 59;
				U0GCR = 5;
				break;
			}
		case HAL_UART_BR_2400:
			{
				U0BAUD = 59;
				U0GCR = 6;
				break;
			}
		case HAL_UART_BR_4800:
			{
				U0BAUD = 59;
				U0GCR = 7;
				break;
			}
		case HAL_UART_BR_9600:
			{
				U0BAUD = 59;
				U0GCR = 8;
				break;
			}
		case HAL_UART_BR_19200:
			{
				U0BAUD = 59;
				U0GCR = 9;
				break;
			}
		case HAL_UART_BR_38400:
			{
				U0BAUD = 59;
				U0GCR = 10;
				break;
			}
		case HAL_UART_BR_115200:
			{
				U0BAUD = 216;
				U0GCR = 11;	
				break;
			}
		}
	}
#endif
}

#if(defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_101) 
void  HalUartChangeLinkFormat(uint8 port, uint8 linkFormat)
{
	uartCfg_t *cfg;
#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		cfg = cfg0;
	}
#endif

#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
		cfg = cfg1;
	}
#endif
	cfg->linkFormat = linkFormat;

	if(linkFormat == HAL_UART_LINKFORMAT_0 || linkFormat == HAL_UART_LINKFORMAT_1)
	{
		U0UCR = UCR_STOP;
	}
	else if(linkFormat == HAL_UART_LINKFORMAT_2)
	{
		U0UCR = UCR_STOP|UCR_PARITY|UCR_BIT9;
	}
}
uint8  HalUartAddParity(uint8 data, bool isEven)
{
	uint8 ret = data;
	ret &= ~0x80;

	uint8 count;
	count = 0;
	for(uint8 i=0;i<7;i++)
	{
		if(data & 0x01)
		{
			count ++;
		}
		data >>= 0x01;
	}

	if(isEven) // 7+1  add even parity
	{	
		ret |= (count&0x01)<<7;
	}
	else
	{
		ret |= (~(count&0x01))<<7;
	}
	return ret;
}

uint8  HalUartRemoveParity(uint8 data, bool isEven)
{
	return data &= ~0x80;
}
#endif

#ifdef CFG_AMMETER_CMDLINE
void halUartPrintPrompt(uint8 port, bool isHavePrompt)
{
	if(isHavePrompt)
	{
		char* p =  "\r\n>>";
		HalUARTWrite(port, (uint8 *)p, osal_strlen(p));
	}
	else
	{
		char* p =  "\r\n";
		HalUARTWrite(port, (uint8 *)p, osal_strlen(p));
	}
}
uint16 HalUARTQuickWrite( uint8 port,uint8 data)
{
	#if HAL_UART_0_ENABLE
	if ( port == HAL_UART_PORT_0 )
	{
		U0DBUF = data;
	}
#endif
#if HAL_UART_1_ENABLE
	if ( port == HAL_UART_PORT_1 )
	{
			U1DBUF = data;
	}
#endif
	
}
#endif


#if HAL_UART_ISR
/***************************************************************************************************
* @fn      halUart0RxIsr
*
* @brief   UART0 Receive Interrupt
*
* @param   None
*
* @return  None
***************************************************************************************************/
#if HAL_UART_0_ENABLE
HAL_ISR_FUNCTION( halUart0RxIsr, URX0_VECTOR )
{
	uint8 data = U0DBUF;

	cfg0->rxBuf[cfg0->rxHead] = data;

	if ( cfg0->rxHead == cfg0->rxMax )
	{
		cfg0->rxHead = 0;
	}
	else
	{
		cfg0->rxHead++;
	}
#ifdef CFG_AMMETER_CMDLINE
	if(cfg0->workMode == HAL_UART_WORKMODE_REPORT)
	{
		if(data == 0x0d)  //enter
		{
			cfg0->workMode = HAL_UART_WORKMODE_CMDLINE;
			HalUARTFlushRxBuf(HAL_UART_PORT_0);
			halUartPrintPrompt(HAL_UART_PORT_0,TRUE);
		}
	}
	else if(cfg0->workMode == HAL_UART_WORKMODE_CMDLINE)
	{
		//HalUARTQuickWrite(HAL_UART_PORT_0, data);
		HalUARTWrite(HAL_UART_PORT_0, (uint8 *)&data, 1);
		if(data == 0x0d)  //enter
		{
			halUartPrintPrompt(HAL_UART_PORT_0,TRUE);
			cfg0->rxCBCmdLine(HAL_UART_PORT_0, HAL_UART_RX_FULL);
			
		}
	}
#endif
	URX0IF = 0;
}
#endif

// caculate time transmit a byte, the uint is Us

void HalUartGetTimePerByte(uint32*pTime, uint8 baudRate)
{
		uint32 rate;
		switch(baudRate)
		{
		case HAL_UART_BR_300:
			{
				rate = 300;
				break;
			}		
		case HAL_UART_BR_600:
			{
				rate = 600;
				break;
			}		
		case HAL_UART_BR_1200:
			{
				rate = 1200;
				break;
			}
		case HAL_UART_BR_2400:
			{
				rate = 2400;
				break;
			}
		case HAL_UART_BR_4800:
			{
				rate = 4800;
				break;
			}
		case HAL_UART_BR_9600:
			{
				rate = 9600;
				break;
			}
		case HAL_UART_BR_19200:
			{
				rate = 19200;
				break;
			}
		case HAL_UART_BR_38400:
			{
				rate = 38400;
				break;
			}
		case HAL_UART_BR_115200:
			{
				rate = 115200;
				break;
			}
		}
		*pTime =(((uint32)10000000)/rate); //(1000*1000*(8+1+1))/rate

		return ;
}
/***************************************************************************************************
* @fn      halUart1RxIsr
*
* @brief   UART1 Receive Interrupt
*
* @param   None
*
* @return  None
***************************************************************************************************/
#if HAL_UART_1_ENABLE
HAL_ISR_FUNCTION( halUart1RxIsr, URX1_VECTOR )
{
	cfg1->rxBuf[cfg1->rxHead] = U1DBUF;

	if ( cfg1->rxHead == cfg1->rxMax )
	{
		cfg1->rxHead = 0;
	}
	else
	{
		cfg1->rxHead++;
	}
}
#endif

/***************************************************************************************************
* @fn      halUart0TxIsr
*
* @brief   UART0 Transmit Interrupt
*
* @param   None
*
* @return  None
***************************************************************************************************/
#if HAL_UART_0_ENABLE
HAL_ISR_FUNCTION( halUart0TxIsr, UTX0_VECTOR )
{
        
	UTX0IF = 0;

	if ( cfg0->txTail == cfg0->txMax )
	{
		cfg0->txTail = 0;
	}
	else
	{
		cfg0->txTail++;
	}

	if ( cfg0->txTail != cfg0->txHead )
	{
		U0DBUF = cfg0->txBuf[cfg0->txTail];
	}
#if (defined CFG_AMMETER_MODBUS ) || (defined CFG_AMMETER_IEC)
	else
	{
                
		//Switch to uart rx. at least delay to tranmit one byte
		HalUartGetTimePerByte(&timeUs, cfg0->baudRate);
		DelayUs((uint16)(timeUs*106/100));
		//DelayUs(35000);
		
		UART0_SWITCHTO_RX;
	}
#endif
}
#endif

/***************************************************************************************************
* @fn      halUart1TxIsr
*
* @brief   UART1 Transmit Interrupt
*
* @param   None
*
* @return  None
***************************************************************************************************/
#if HAL_UART_1_ENABLE
HAL_ISR_FUNCTION( halUart1TxIsr, UTX1_VECTOR )
{
	UTX1IF = 0;
	U1CSR &= ~CSR_TX_BYTE;  // Rev-D does not require, older does.

	if ( cfg1->txTail == cfg1->txMax )
	{
		cfg1->txTail = 0;
	}
	else
	{
		cfg1->txTail++;
	}

	if ( cfg1->txTail != cfg1->txHead )
	{
		U1DBUF = cfg1->txBuf[cfg1->txTail];
	}
}
#endif
#endif

/******************************************************************************
******************************************************************************/
