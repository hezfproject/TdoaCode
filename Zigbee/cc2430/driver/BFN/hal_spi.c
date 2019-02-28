/***********************************************
hal_spi.c 
    Revised:        $Date: 2009/11/20 03:12:32 $
    Revision:       $Revision: 1.10 $

    Description:    This file contains the SPI definitions.
************************************************/
/*********************************************************************
	Use Poll instead of DMA ISR because DMA is everywhere in the system and if a DMA ISR is
existed, every DMA operation will come into the ISR no necessarily.
	For tx, employe queue to hold msg for tx.
	For rx, request a new buffer from heap as possible and App
is responsible to release it.
*********************************************************************/
/*********************************************************************
 * INCLUDES
 */
#include "hal_spi.h"
#include "hal_dma.h"
#include "OSAL.h"

/*********************************************************************
 * CONSTANTS
 */
// Task ID not initialized
#define NO_TASK_ID 0xFF
/*********************************************************************
 * TYPEDEFS
 */
typedef void* Q_DMA_buf_t;

/*********************************************************************
 * LOCAL VARIABLES
 */

static spiCfg_t rxConfigData;
static spiCfg_t* rxConfig = &rxConfigData;

static bool ontx = false; //flag to identify tx state.
static bool onrx = false; //flag to identify rx state.

//Identify the max rxmsg count.
static uint8 rxMaxMsgCnt = 0;

//Identify the txmsg count in tx queue.
static uint8 txMsgCnt = 0;

//Identify the tx wait cycles, after wait several cycles and not receive change of P0IF_6
//we need start the tx manually to avoid block.
static uint8 txCheckCycle = 0;

static byte registeredSpiTaskID = NO_TASK_ID;

/*psend queue hold the tx msg, precv queue hold the rx msg.
*The pbufpool is the sum of buffers for tx. 
*/
static Q_DMA_buf_t precv = NULL;
static Q_DMA_buf_t psend = NULL;
static Q_DMA_buf_t pbufpool = NULL;

/*********************************************************************
 * MACROS
 */
 #ifndef RXMSGTHRESHOLD
 #define RXMSGTHRESHOLD 4
 #endif
 
#ifndef POOLSIZE  
#define POOLSIZE 2
#endif

#ifndef SPI_CHECK_TX_READY
#define SPI_CHECK_TX_READY 2
#endif

#define HalSetTxState(state) \
st (  \
	ontx = state; \
)

#define HalSetRxState(state) \
st (  \
	onrx = state; \
)
#define HalSetTxOn() HalSetTxState(true)
#define HalSetTxOff() HalSetTxState(false)
#define HalSetRxOn() HalSetRxState(true)
#define HalSetRxOff() HalSetRxState(false)
#define HalCheckRxState() (onrx)
#define HalCheckTxState()  (ontx)

#define IncrTxCheckCycle() \
st (  \
	txCheckCycle++; \
)
#define ClearTxCheckCycle() \
st (  \
	txCheckCycle = 0; \
)
#define CheckTxCheckCycle() (txCheckCycle < SPI_CHECK_TX_READY)

#if 0
#define BLOCKRX 0x01

#define HalSpiRxIsBlock()  (SpiStatus & BLOCKRX)
#define HalBlockRx()    SpiStatus |= BLOCKRX
#define HalUnblockRx()  SpiStatus &= ~BLOCKRX

#define DMAIE_FLAG BV(0)
#define ENABLE_DMA_INTERRUPT() (IEN1 |= DMAIE_FLAG)
#define DISABLE_DMA_INTERRUPT() (IEN1 &= ~DMAIE_FLAG)
#endif

#define DEQUEUE_MSG(msg, queue) \
st (  \
	msg = osal_msg_dequeue(&queue);  \
)
	
#define ENQUEUE_MSG(msg, queue) \
st (    \
	osal_msg_enqueue(&queue, msg);  \
)

#define RxQueueFull()   (rxMaxMsgCnt >= RXMSGTHRESHOLD)
#define GET_RX_MSG(msg) \
st ( \
	DEQUEUE_MSG(msg, precv);  \
	if (rxMaxMsgCnt)   \
		rxMaxMsgCnt--; \
)
#define SET_RX_MSG(msg) \
st (  \
	ENQUEUE_MSG(msg, precv);  \
	rxMaxMsgCnt++;  \
)

#define TxQueueIsValid()   (txMsgCnt > 0)
#define GET_TX_MSG(msg) \
st ( \
	 DEQUEUE_MSG(msg, psend);\
	if (txMsgCnt) \
		txMsgCnt--;\
)
#define SET_TX_MSG(msg) \
st (  \
	ENQUEUE_MSG(msg, psend); \
	txMsgCnt++;  \
)
#define GET_BUF(msg) DEQUEUE_MSG(msg, pbufpool)
#define SET_BUF(msg) ENQUEUE_MSG(msg, pbufpool)

//macro for DMA rx and tx.
#define DMA_RX( cfg ) \
st(         \
  halDMADesc_t *ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_RX ); \
  HAL_DMA_SET_DEST( ch, cfg->rxBuf ); \
  HAL_DMA_SET_LEN( ch, cfg->bufSize ); \
  HAL_DMA_ARM_CH( HAL_DMA_CH_RX ); \
)

#define DMA_TX(cfg )  \
st( \
  halDMADesc_t *ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_TX ); \
  HAL_DMA_SET_SOURCE( ch, cfg->txBuf ); \
  HAL_DMA_SET_LEN( ch, cfg->bufSize );  \
  HAL_DMA_ARM_CH( HAL_DMA_CH_TX ); \
  HAL_DMA_START_CH( HAL_DMA_CH_TX );\
)

//Peripheral Control and PxSEL setting.
#define SPI_CFG(uart, port) \
st(        \
          	if ((uart) == 0 && (port) == 0)  \
          	{   \
			PERCFG &= ~HAL_SPI_0_PERCFG_BIT; \
			P##port##SEL = 0x3c;       \
          	}    \
		else if ((uart) == 0 && (port) == 1)  \
		{     \
			PERCFG |= HAL_SPI_0_PERCFG_BIT; \
			P##port##SEL = 0x3c;       \
		}   \
		else if ((uart) == 1 && (port) == 0)  \
		{   \
			PERCFG &= ~HAL_SPI_1_PERCFG_BIT; \
			P##port##SEL = 0x3c;       \
		}    \
		else if ((uart) == 1 && (port) == 1)   \
		{      \
			PERCFG |= HAL_SPI_1_PERCFG_BIT; \
			P##port##SEL = 0xf0;      \
		}     \
)

#define SPI_CFG_SPI0_ON_P0()  SPI_CFG(0, 0)
#define SPI_CFG_SPI1_ON_P1()  SPI_CFG(1, 1)
                                                 
/*
Description for SPI:
There are 4 registers need to be configered
SPI0:
	U0CSR(0x86):	8'b01100000(SPI_MODE | RECEVIER_ENABLE | SPI_SLAVE)
	U0UCR(0xC4):	8'b00000010(Not related to SPI mode)
	U0GCR(0xC5):	8'b00110000(SPI clock polarity: negative,
					SPI clock phase: 0 -> 1,
					MSB first,
					BAUD_E = 16 )
	U0BAUD(0xC2):	8'b00000000(BAUD_M = 0)

SPI1:
	U1CSR(0xF8):	8'b00100000(SPI_MODE | RECEVIER_DISABLE | SPI_MASTER)
	U1UCR(0xFB):	8'b00000010(Not related to SPI mode)
	U1GCR(0xFC):	8'b00110000(SPI clock polarity: negative,
					SPI clock phase: 0 -> 1,
					MSB first,
					BAUD_E = 16 )
	U1BAUD(0xFA):	8'b00000000(BAUD_M = 0)
	
Note: BAUD_E = 16, BAUD_M = 0, so Baudrate = 2M.				
*/
#define SPI_INPUT_CONFIG(uart) \
st(     \
	U##uart##CSR = CSR_SPI | CSR_RE | CSR_SLAVE; \
	U##uart##GCR = GCR_MSB | GCR_BE; \
	U##uart##BAUD = BAUD_BM; \
)

#define SPI_OUTPUT_CONFIG(uart)  \
st(    \
	U##uart##CSR = CSR_SPI ; \
	U##uart##GCR = GCR_MSB | GCR_BE; \
	U##uart##BAUD = BAUD_BM; \
)

#define SPI_INPUT_CONFIG_ON_UART0() SPI_INPUT_CONFIG(0)
#define SPI_INPUT_CONFIG_ON_UART1() SPI_INPUT_CONFIG(1)
#define SPI_OUTPUT_CONFIG_ON_UART1() SPI_OUTPUT_CONFIG(1)
#define SPI_OUTPUT_CONFIG_ON_UART0() SPI_OUTPUT_CONFIG(0)

#define SPI_INPUT_CONFIG_UART0_ON_P0() \
st(  \
	SPI_CFG_SPI0_ON_P0();    \
        \
	SPI_INPUT_CONFIG_ON_UART0();   \
)

#define SPI_INPUT_CONFIG_UART1_ON_P1() \
st(  \
	SPI_CFG_SPI1_ON_P1();    \
        \
	SPI_INPUT_CONFIG_ON_UART1();   \
)

/*
Master is used for output. need set the ss pin to gpio as datasheet description:P1SEL &= ~0x10,
and directory to output:P1DIR |= 0x10.
*/
#define SPI_OUTPUT_CONFIG_UART1_ON_P1() \
st(  \
   	SPI_CFG_SPI1_ON_P1();   \
	\
       SPI_OUTPUT_CONFIG_ON_UART1(); \
         P1SEL &= ~0x10;      \
         P1DIR |= 0x10;   \
)
/*
Master is used for output. need set the ss pin to gpio as datasheet description:P0SEL &= ~0x10,
and directory to output:P0DIR |= 0x10.
*/
#define SPI_OUTPUT_CONFIG_UART0_ON_P0() \
st(  \
   	SPI_CFG_SPI0_ON_P0();   \
	\
       SPI_OUTPUT_CONFIG_ON_UART0(); \
         P0SEL &= ~0x10;      \
         P0DIR |= 0x10;   \
)

#define SPI_MASTER_ON_P1_START()  P1 &= ~0x10
#define SPI_MASTER_ON_P1_STOP() P1 |= 0x10

#define SPI_MASTER_ON_P0_START()  P0 &= ~0x10
#define SPI_MASTER_ON_P0_STOP() P0 |= 0x10

/*Use GPIO P0_6 as master signal line, GPIO P0_7 as slave signal line.
*so in initialization, need set the direction and selection of the two pins.
*before tx need check if P0_6 is valid and check if P0IFG(bit7) is high;
*for rx need set P0_7 invalid when enter isr and set P0_7 valid to notify the 
*other tx port.
*/
#define SPI_SIGNAL_INIT()  \
st (  \
	P0SEL &= ~BV(6);\
	P0DIR &= ~BV(6); \
	P0SEL &= ~BV(7);\
	P0DIR |= BV(7);  \
	P0IFG &= ~BV(6); \
)
#define SPI_RX_READY()  \
st (   \
	P0 &= ~BV(7);  \
)
#define SPI_RX_STOP()  \
st (  \
	P0 |= BV(7);   \
)
#define RESET_TX_SIGNAL_LINE() \
st (  \
	P0IFG &= ~BV(6); \
)
//check signal line of tx.
#define SPI_TX_READY_CHECK() ACTIVE_LOW(P0&BV(6))
#define SPI_NEXT_TX_READY_CHECK() P0IFG&BV(6)

/*********************************************************************
 * LOCAL FUNCTIONS
 */
/*
 * Start a DMA tx.
 */
//__near_func uint8 HalSPITxStart(void);
/*
 * Start a DMA rx.
 */
//__near_func uint8 HalSPIRxStart(void);
static void Spi_SendEvt( void* msg);
/*
Description for DMA: 
	We need 2 dma channels, DMA0 is for SPI Receive, DMA1 is for SPI Transmit.
	To config DMA, just fill one DMA structure(P93-P96 of 2430 SPEC).
Here is detail for it:
DMA0:
	SRCADDR[15:0]:	address of SPI receive register
	DESTADDR[15:0]:	address of user-define dma receive buffer
	VLEN[2:0]:	3'b001, use this mode to eliminate timer interrupt request
	LEN[12:0]:	Maximum transfer size, equal to receiver buf size, make a MACRO here
	WORDSIZE[0]:	1'b0, use byte(8-bit) transfer
	TMODE[1:0]:	2'b10, use single transfer mode
	TRIG[4:0]:	5'b01110(14, URX0)
	SRCINC[1:0]:	2'b00, no increment for srcaddr
	DESTINC[1:0]:	2'b01, 1 byte increment for dstaddr
	IRQMASK[0]:	1'b1, int enable
	M8[0]:		1'b0, use 8 bits for transfer count
	PRIORITY[1:0]	2'b01, middle priority

DMA1:
	SRCADDR[15:0]:	address of user-define dma transmit buffer 
	DESTADDR[15:0]:	SPI transmit register
	VLEN[2:0]:	3'b001, use this mode to eliminate timer interrupt request
	LEN[12:0]:	Maximum transfer size, equal to receiver buf size, make a MACRO here
	WORDSIZE[0]:	1'b0, use byte(8-bit) transfer
	TMODE[1:0]:	2'b00, use single transfer mode
	TRIG[4:0]:	5'b10001(17, UTX1)
	SRCINC[1:0]:	2'b01, 1 byte increment for srcaddr
	DESTINC[1:0]:	2'b00, no increment for dstaddr
	IRQMASK[0]:	1'b1, int enable
	M8[0]:		1'b0, use 8 bits for transfer count
	PRIORITY[1:0]	2'b01, middle priority	

Note: this stucture is user-defined in memory, after filling it, 
write DMA0CFG(or DMA1CFG) with the start address of this structure.
Then write to DMAARM register to arm the channel.
In interrupt handler: 
(1) software clear the interrupt flag bit.
(2) re-arm the dma channel.		
*/

/******************************************************************************
 * @fn      HalSpiInit
 *
 * @brief   Initialize the SPI and DMA config.
 *
 * @param Nobe
 *
 * @return  None
 *****************************************************************************/
void HalSpiInit(void)
{
	/*configuration SPI on p1 for recv, p0 for send.*/
	SPI_INPUT_CONFIG_UART1_ON_P1();
	SPI_OUTPUT_CONFIG_UART0_ON_P0();
	/*init signal lines*/
	SPI_SIGNAL_INIT();

	/*configuration DMA.*/
       halDMADesc_t *ch;
	// Setup Tx by DMA.
       ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_TX );

       HAL_DMA_SET_DEST(ch, DMA_TXBUF);

       HAL_DMA_SET_VLEN(ch, HAL_DMA_VLEN_USE_LEN);
		
       HAL_DMA_SET_WORD_SIZE( ch, HAL_DMA_WORDSIZE_BYTE );
       HAL_DMA_SET_TRIG_MODE( ch, HAL_DMA_TMODE_SINGLE );
       HAL_DMA_SET_TRIG_SRC( ch, HAL_DMA_TRIG_UTX0);
     
       HAL_DMA_SET_SRC_INC( ch, HAL_DMA_SRCINC_1 );
       HAL_DMA_SET_DST_INC( ch, HAL_DMA_DSTINC_0 );
       HAL_DMA_SET_IRQ( ch, HAL_DMA_IRQMASK_ENABLE );
       HAL_DMA_SET_M8( ch, HAL_DMA_M8_USE_8_BITS );
       HAL_DMA_SET_PRIORITY( ch, HAL_DMA_PRI_GUARANTEED);
     
       // Setup Rx by DMA.
       ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_RX );
	   
       HAL_DMA_SET_SOURCE( ch, DMA_RXBUF );
       HAL_DMA_SET_VLEN( ch, HAL_DMA_VLEN_USE_LEN );
	
       HAL_DMA_SET_WORD_SIZE( ch, HAL_DMA_WORDSIZE_BYTE );
       HAL_DMA_SET_TRIG_MODE( ch, HAL_DMA_TMODE_SINGLE );
       HAL_DMA_SET_TRIG_SRC( ch, HAL_DMA_TRIG_URX1);
     
       HAL_DMA_SET_SRC_INC( ch, HAL_DMA_SRCINC_0);  
       HAL_DMA_SET_DST_INC( ch, HAL_DMA_DSTINC_1);
       HAL_DMA_SET_IRQ( ch, HAL_DMA_IRQMASK_ENABLE);
       HAL_DMA_SET_M8( ch, HAL_DMA_M8_USE_8_BITS);
       HAL_DMA_SET_PRIORITY( ch, HAL_DMA_PRI_GUARANTEED );
}

/******************************************************************************
 * @fn      HalSpiStart
 *
 * @brief   startup the configuration specified by parameter.
 *
 * @param config - contains configuration information
 *
 * @return  None
 *****************************************************************************/
void HalSpiStart(halSPICfg_t* config)
{
	rxConfig->bufSize = config->maxBufSize;
	uint8 i = 0;
	byte* buf = 0;
	if (!pbufpool)
	{
		for (; i < POOLSIZE; ++i)
		{
			buf = osal_msg_allocate(rxConfig->bufSize);
			osal_msg_enqueue(&pbufpool, buf);
		}
	}
	
       rxConfig->rxBuf = osal_msg_allocate(rxConfig->bufSize);
	//GET_BUF(rxConfig->txBuf);
	//drive tx.
	SPI_MASTER_ON_P0_START();
	//drive rx.
	SPI_RX_STOP();
	HalSetRxOff();
}

/******************************************************************************
 * @fn     HalSPIWrite
 *
 * @brief  transmit a msg from app to SPI tx queue, which is txbuf featched by HalSPIGetTxBuffer().
 * @param
 *          Buffer - pointer to the input msg.
 *          len  - length of the buffer
 *
 * @return  Status of the function call
 *****************************************************************************/
__near_func uint8 HalSPIWrite(uint8 *buf, uint16 len )
{
	uint8* txbuf = 0;
	GET_BUF(txbuf);
	if (txbuf)
	{
		osal_memcpy(txbuf, buf, len);
		SET_TX_MSG(txbuf);
		HalSPITxStart(); // directly start Tx configure.
		return HAL_SPI_SUCCESS;
	}
	return HAL_SPI_FULL_QUEUE;
}

/******************************************************************************
 * @fn      HalSPITxStart
 *
 * @brief   Config tx data structure and begin a DMA tx.
 *
 * @param
 *                  None
 *
 * @return  Status of the function call
 *****************************************************************************/
__near_func uint8 HalSPITxStart(void)
{
	/*
	*1. Check the port of signal line, actually it's controlled by the other side; 
	*2. Check tx state, on tx or not;
	*3. Check if the next Tx is valid.
	*4. Check the tx queue.
	*/
	if (!SPI_TX_READY_CHECK())
	{
		return HAL_SPI_HANDLINE_NOREADY;
	}
	if (HalCheckTxState())
	{
		return HAL_SPI_ON_TX;
	}
	/*FIXME:
	*This check need combine with txmsg, that means: only if txqueue has tx
	*message, we should reset the interrupt bit mask and start the next tx.
	*Here is a trick, we need avoid to be block when debug, so after several
	* times of P0IFG invalid check, we need pass the next tx by several cycles.
	*/
	if (TxQueueIsValid() && SPI_NEXT_TX_READY_CHECK())
	{
		RESET_TX_SIGNAL_LINE();
	}
	else if (TxQueueIsValid() && !CheckTxCheckCycle())
	{
		ClearTxCheckCycle();
	}
	else
	{
		IncrTxCheckCycle();
		return HAL_SPI_HANDLINE_NOREADY;
	}
	
	GET_TX_MSG(rxConfig->txBuf);
	if (!rxConfig->txBuf)
	{
		return HAL_SPI_EMPTY_QUEUE;
	}
	
	//it's time to prepare tx config and drive tx now.
	HalSetTxOn();
	DMA_TX(rxConfig);
	return HAL_SPI_SUCCESS;
}

/******************************************************************************
 * @fn      HalSPIRxStart
 *
 * @brief   Config rx data structure and begin a DMA rx.
 *
 * @param
 *                None
 *
 * @return  Status of the function call
 *****************************************************************************/
__near_func uint8 HalSPIRxStart(void)
{
	/*For DMA rx:
	*1. Check rx state;
	*2. Check rx msg count. 
	*3. Check rxbuf mem.
	*/
	if (HalCheckRxState())
	{
		return HAL_SPI_ON_RX;
	}
	if (RxQueueFull())
	{
		return HAL_SPI_FULL_QUEUE;
	}
	
	rxConfig->rxBuf = osal_msg_allocate(rxConfig->bufSize);
	if (!rxConfig->rxBuf)
		return HAL_SPI_MEM_FAIL;

	HalSetRxOn();
	DMA_RX(rxConfig);
	SPI_RX_READY(); //revoke the hand line.
	return HAL_SPI_SUCCESS;
}

/******************************************************************************
 * @fn      HaSPIPoll
 *
 * @brief   Poll the SPI Interrupt.
 *
 * @param   none
 *
 * @return  none
 *****************************************************************************/
__near_func void HalSPIPoll( void )
{
  	if (HAL_DMA_CHECK_IRQ(HAL_DMA_CH_RX))
	{
		HAL_DMA_CLEAR_IRQ( HAL_DMA_CH_RX );
		HalSetRxOff();
		SPI_RX_STOP(); //stop the hand line and notify the other side i'm not ok to recv now.
		SET_RX_MSG(rxConfig->rxBuf);
		uint8* rxmsg = 0;
		GET_RX_MSG(rxmsg);
		if (rxmsg)
			Spi_SendEvt(rxmsg);
	}

	if (HAL_DMA_CHECK_IRQ(HAL_DMA_CH_TX))
	{
		HAL_DMA_CLEAR_IRQ( HAL_DMA_CH_TX );
		HalSetTxOff();
		SET_BUF(rxConfig->txBuf);
	}

	//Double check if RX/TX is started normally in App, otherwise start here.
	HalSPIRxStart();
	HalSPITxStart();
}
 
 /*********************************************************************
 * @fn      Spi_SendEvt
 *
 * @brief   Send "Spi" message to application.
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  status

 *********************************************************************/

void Spi_SendEvt( void* msg)
{
	if (registeredSpiTaskID != NO_TASK_ID)
		osal_msg_send(registeredSpiTaskID, (byte *)msg);	
}

 /*********************************************************************
 * Spi Register function
 *
 * The Spi handler is setup to send all spi events to
 * one task (if a task is registered).
 *
 * If a task registers, it will get all the spi. 
 *********************************************************************/

byte RegisterForSpi( byte task_id )
{
	// Allow only the first task
	if ( registeredSpiTaskID == NO_TASK_ID )
	{
		registeredSpiTaskID = task_id;
		return (true);
	}
	else
		return (false);
}
/******************************************************************************
******************************************************************************/

