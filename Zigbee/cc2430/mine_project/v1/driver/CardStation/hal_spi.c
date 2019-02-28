/*********************************************************************
 * INCLUDES
 */
#include "hal_spi.h"
#include "hal_dma.h"
#include "OSAL.h"
#include "App_cfg.h"
#include "OnBoard.h"
/*********************************************************************
 * variables
 */
typedef struct
{
    bool   started;
    uint16 bufSize;
    uint8   taskID;
    uint16 eventID;
    uint8 *rxBuf;
    uint8 *txBuf;
} spiCfg_t;

static osal_msg_q_t psend = NULL;
static osal_msg_q_t psendpool = NULL;
static osal_msg_q_t precv = NULL;
static osal_msg_q_t precvpool = NULL;
static spiCfg_t	 spicfg;


uint8 sendbufcnt = 0;
uint8 recvbufcnt = 0;

#ifdef TX_NEXT_CHECK
//Identify the tx wait cycles, after wait several cycles and not receive change of P0IF_6
//we need start the tx manually to avoid block.
static uint8 txCheckCycle = 0;
#endif

bool  ontx;
bool  onrx;

/*********************************************************************
 * defines
 */
#ifndef POOLSIZE
#define POOLSIZE 2
#endif

#ifndef SPI_CHECK_TX_READY
#define SPI_CHECK_TX_READY 2
#endif
#define NO_TASK_ID 0xFF

#ifdef TX_NEXT_CHECK
#define IncrTxCheckCycle() \
st (  \
    txCheckCycle++; \
)
#define ClearTxCheckCycle() \
st (  \
    txCheckCycle = 0; \
)
#define CheckTxCheckCycle() (txCheckCycle < SPI_CHECK_TX_READY)
#define TxQueueIsValid()   (sendbufcnt > 0)
#endif

/* p0_4 is spi0 ssn, disable it when not sending */
#define UART0_SSN_OPEN() \
st(  \
    P0 &= ~0x10;    \
)

#define UART0_SSN_CLOSE() \
st(  \
    P0 |= 0x10; \
)
//macro for DMA rx and tx.
#define DMA_RX( cfg ) \
st(         \
  halDMADesc_t *ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_RX ); \
  HAL_DMA_SET_DEST( ch, cfg.rxBuf ); \
  HAL_DMA_SET_LEN( ch, cfg.bufSize ); \
  HAL_DMA_ARM_CH( HAL_DMA_CH_RX ); \
  DelayUs(10);                      \
)

#define DMA_TX(cfg )  \
st( \
  UART0_SSN_OPEN(); \
  halDMADesc_t *ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_TX ); \
  HAL_DMA_SET_SOURCE( ch, cfg.txBuf ); \
  HAL_DMA_SET_LEN( ch, cfg.bufSize );  \
  HAL_DMA_ARM_CH( HAL_DMA_CH_TX ); \
  DelayUs(80);                      \
  HAL_DMA_START_CH( HAL_DMA_CH_TX );\
)

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
#define SPI_TX_READY_CHECK() ACTIVE_LOW(P0&BV(6))
#define SPI_NEXT_TX_READY_CHECK() P0IFG&BV(6)

/*********************************************************************
 * function proto
 */

__near_func uint8 HalSPITxStart(void);
__near_func uint8 HalSPIRxStart(void);
void HalSpiSendMsg(void);
void DelayUs(uint16 n);
/*********************************************************************
 * functions
 */


void HalSpiInit(void)
{
    /*configuration SPI on p1 for recv, p0 for send.*/

    /* spi1 for recv, loc p1.4 - p1.7 */
    PERCFG |= HAL_SPI_1_PERCFG_BIT;
    P1SEL |= (BV(4)|BV(5)|BV(6)|BV(7));

    U1CSR = CSR_SPI | CSR_RE | CSR_SLAVE;
    U1GCR = GCR_MSB | GCR_BE;
    U1BAUD = BAUD_BM;

    /* spi0 for send, p0.2-p0.5*/
    PERCFG &= ~HAL_SPI_0_PERCFG_BIT;
    P0SEL |= (BV(2)|BV(3)|BV(4)|BV(5));

    U0CSR = CSR_SPI ;
    U0GCR = GCR_MSB | GCR_BE;
    U0BAUD = BAUD_BM;

    /* set the prot0 ss pin to gpio */
    P0SEL &= ~0x10;
    P0DIR |= 0x10;
    UART0_SSN_CLOSE();

    /*init signal lines*/
    SPI_SIGNAL_INIT();

    /*configuration DMA.*/
    halDMADesc_t *ch;
    // Setup Tx by DMA.
    ch = HAL_DMA_GET_DESC1234(HAL_DMA_CH_TX);

    HAL_DMA_SET_DEST(ch, DMA_TXBUF);

    HAL_DMA_SET_VLEN(ch, HAL_DMA_VLEN_USE_LEN);

    HAL_DMA_SET_WORD_SIZE(ch, HAL_DMA_WORDSIZE_BYTE);
    HAL_DMA_SET_TRIG_MODE(ch, HAL_DMA_TMODE_SINGLE);
    HAL_DMA_SET_TRIG_SRC(ch, HAL_DMA_TRIG_UTX0);

    HAL_DMA_SET_SRC_INC(ch, HAL_DMA_SRCINC_1);
    HAL_DMA_SET_DST_INC(ch, HAL_DMA_DSTINC_0);
    HAL_DMA_SET_IRQ(ch, HAL_DMA_IRQMASK_ENABLE);
    HAL_DMA_SET_M8(ch, HAL_DMA_M8_USE_8_BITS);
    HAL_DMA_SET_PRIORITY(ch, HAL_DMA_PRI_GUARANTEED);

    // Setup Rx by DMA.
    ch = HAL_DMA_GET_DESC1234(HAL_DMA_CH_RX);

    HAL_DMA_SET_SOURCE(ch, DMA_RXBUF);
    HAL_DMA_SET_VLEN(ch, HAL_DMA_VLEN_USE_LEN);

    HAL_DMA_SET_WORD_SIZE(ch, HAL_DMA_WORDSIZE_BYTE);
    HAL_DMA_SET_TRIG_MODE(ch, HAL_DMA_TMODE_SINGLE);
    HAL_DMA_SET_TRIG_SRC(ch, HAL_DMA_TRIG_URX1);

    HAL_DMA_SET_SRC_INC(ch, HAL_DMA_SRCINC_0);
    HAL_DMA_SET_DST_INC(ch, HAL_DMA_DSTINC_1);
    HAL_DMA_SET_IRQ(ch, HAL_DMA_IRQMASK_ENABLE);
    HAL_DMA_SET_M8(ch, HAL_DMA_M8_USE_8_BITS);
    HAL_DMA_SET_PRIORITY(ch, HAL_DMA_PRI_GUARANTEED);

    spicfg.taskID = NO_TASK_ID;
    spicfg.started = false;
}

void HalSpiStart(halSPICfg_t* config)
{
    uint8 *buf;

    spicfg.bufSize = config->bufSize;
    spicfg.txBuf = NULL;
    spicfg.rxBuf = NULL;
    spicfg.started = true;
    spicfg.taskID = config->taskID;
    spicfg.eventID = config->eventID;

    if(!psendpool)
    {
        for(uint8 i=0; i < POOLSIZE; ++i)
        {
            buf = osal_msg_allocate(spicfg.bufSize + 10);
            osal_msg_enqueue(&psendpool, buf);
        }
    }
    if(!precvpool)
    {
        for(uint8 i=0; i < POOLSIZE; ++i)
        {
            buf = osal_msg_allocate(spicfg.bufSize + 10);
            osal_msg_enqueue(&precvpool, buf);
        }
    }

    /*Disable DMA intrupt*/
    DMAIE = 0;
    DMAIF = 0;
    DMAIRQ = 0;

    RESET_TX_SIGNAL_LINE();

    /* ready to receive */
    SPI_RX_STOP();
    ontx = false;
    onrx = false;
}


uint8 HalSPIWrite(uint8 *buf, uint16 len)
{
    uint8* txbuf = 0;
    if(buf == NULL || len > spicfg.bufSize)
    {
        return FAILURE;
    }

    osal_event_hdr_t *p = (osal_event_hdr_t *)buf;
    p->event = SPI_RX_MSG;
    p->status = SUCCESS;


    txbuf = osal_msg_dequeue(&psendpool);
    if(txbuf)
    {
        osal_memcpy(txbuf, buf, len);
        if(osal_msg_enqueue_max(&psend, txbuf, POOLSIZE))
        {
            sendbufcnt++;
            HalSPITxStart();
            return HAL_SPI_SUCCESS;
        }
        else
        {
            osal_msg_enqueue(&psendpool, txbuf);
            return HAL_SPI_FULL_QUEUE;
        }
    }
    return HAL_SPI_FULL_QUEUE;
}

uint8 HalSPIRead(uint8 *buf, uint16* plen)
{
    uint8* rxbuf;
    if(buf ==NULL)
    {
        return INVALIDPARAMETER;
    }
    rxbuf  = osal_msg_dequeue(&precv);
    if(rxbuf)
    {
        if(recvbufcnt)
        {
            recvbufcnt--;
        }
        osal_memcpy(buf, rxbuf, spicfg.bufSize);
        if(plen !=NULL)
        {
            *plen = spicfg.bufSize;
        }
        osal_msg_enqueue(&precvpool, rxbuf);
        HalSPIRxStart();
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }
}

__near_func uint8 HalSPITxStart(void)
{
    /*
    *1. Check the port of signal line, actually it's controlled by the other side;
    *2. Check tx state, on tx or not;
    *3. Check if the next Tx is valid.
    *4. Check the tx queue.
    */
    if(!SPI_TX_READY_CHECK())
    {
        return HAL_SPI_HANDLINE_NOREADY;
    }
    if(ontx)
    {
        return HAL_SPI_ON_TX;
    }
    /*FIXME:
    *This check need combine with txmsg, that means: only if txqueue has tx
    *message, we should reset the interrupt bit mask and start the next tx.
    *Here is a trick, we need avoid to be block when debug, so after several
    * times of P0IFG invalid check, we need pass the next tx by several cycles.
    */
#ifdef TX_NEXT_CHECK
    if(TxQueueIsValid())
    {
        if(SPI_NEXT_TX_READY_CHECK())
        {
            RESET_TX_SIGNAL_LINE();
            ClearTxCheckCycle();
        }
        else if(!CheckTxCheckCycle())
        {
            ClearTxCheckCycle();
        }
        else
        {
            IncrTxCheckCycle();
            return HAL_SPI_HANDLINE_NOREADY;
        }
    }
#endif

    spicfg.txBuf = osal_msg_dequeue(&psend);

    if(spicfg.txBuf == NULL)
    {
        return HAL_SPI_EMPTY_QUEUE;
    }
    if(sendbufcnt>0)
    {
        sendbufcnt--;
    }
    halIntState_t intState;
    HAL_ENTER_CRITICAL_SECTION(intState);

    ontx = true;
    DMA_TX(spicfg);

    HAL_EXIT_CRITICAL_SECTION(intState);
    return HAL_SPI_SUCCESS;
}


__near_func uint8 HalSPIRxStart(void)
{
    /*For DMA rx:
    *1. Check rx state;
    *2. Check rx msg count.
    *3. Check rxbuf mem.
    */
    if(onrx)
    {
        return HAL_SPI_ON_RX;
    }

    if(recvbufcnt >= POOLSIZE)
    {
        // SPI_RX_STOP();
        return HAL_SPI_FULL_QUEUE;
    }

    spicfg.rxBuf = osal_msg_dequeue(&precvpool);

    if(spicfg.rxBuf == NULL)
    {
        return HAL_SPI_EMPTY_QUEUE;
    }
    halIntState_t intState;
    HAL_ENTER_CRITICAL_SECTION(intState);

    onrx = true;
    DMA_RX(spicfg);
    SPI_RX_READY(); //revoke the hand line.

    HAL_EXIT_CRITICAL_SECTION(intState);
    return HAL_SPI_SUCCESS;
}

__near_func void HalSPIPoll(void)
{
    if(!spicfg.started)
    {
        return;
    }
    if ( HAL_DMA_CHECK_IRQ( HAL_DMA_CH_RX )
            || (ACTIVE_LOW(P0 & BV(7)) && !HAL_DMA_CHECK_ARM(HAL_DMA_CH_RX) && onrx == true) )
    {
        onrx = false;
        SPI_RX_STOP();

        if(osal_msg_enqueue_max(&precv, spicfg.rxBuf, POOLSIZE))
        {
            recvbufcnt++;
            HalSpiSendMsg();
        }
        else
        {
            osal_msg_enqueue(&precvpool,  spicfg.rxBuf);
        }

        HAL_DMA_CLEAR_IRQ( HAL_DMA_CH_RX );
    }

    if ( HAL_DMA_CHECK_IRQ( HAL_DMA_CH_TX )
            || (!HAL_DMA_CHECK_ARM(HAL_DMA_CH_TX) && ontx))
    {

        osal_msg_enqueue(&psendpool, spicfg.txBuf);
        HAL_DMA_CLEAR_IRQ( HAL_DMA_CH_TX );
        DelayUs(60);
        UART0_SSN_CLOSE();
        ontx = false;
    }


    /* send */

    HalSPITxStart();

    /* recv */

    HalSPIRxStart();

    /*send read event */
    if(spicfg.taskID!=NO_TASK_ID && recvbufcnt > 0)
    {
        HalSpiSendMsg();
    }
}

void HalSpiSendMsg(void)
{
    if(spicfg.taskID!= NO_TASK_ID)
    {
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
        uint8* pMsg = NULL;
        if ((pMsg = osal_msg_allocate(spicfg.bufSize + 10)) != NULL)
        {
            uint16 len;
            if (HalSPIRead(pMsg, &len) == ZSUCCESS)
            {
                if (*pMsg != SPI_RX_MSG)
                {
                    *pMsg = SPI_RX_MSG;
                }
                osal_msg_send(spicfg.taskID, pMsg);
            }
        }
#else
        if(SUCCESS!= osal_set_event(spicfg.taskID, spicfg.eventID))
        {
            SystemReset();
        }
#endif
    }
}
void DelayUs(uint16 n)
{
    while(--n)
    {
        asm("nop");
        asm("nop");
        asm("nop");
    }
}
