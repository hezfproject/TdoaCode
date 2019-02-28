#include <jendefs.h>
#include <AppHardwareApi.h>
#include <Utilities.h>

#include "bsmac.h"
#include "crc.h"

#ifdef NEED_I2C_PRINT
#include "i2c_printf_util.h"
#else
#include "printf_util.h"
#endif
#include "system_util.h"
#include "MicroSpecific.h"
#include "uart.h"
#include "Nwk_protocol.h"
/*****************************************************************************
 * Macros
 *
 ******************************************************************************/
//#define DEBUG_BSMAC

#if (defined DEBUG_BSMAC)
#define DBG(x) do{x} while (0);
#else
#define DBG(x)
#endif

// for errors
#if (defined DEBUG_ERROR)
#define EDBG(x) do{x} while (0);
#else
#define EDBG(x)
#endif

#ifdef NEED_I2C_PRINT
#define  PRINTF(x...) i2c_vPrintf(x)
#define  PRINTMEM(x...) i2c_vPrintMem(x)
#else
#define  PRINTF(x...) PrintfUtil_vPrintf(x)
#define  PRINTMEM(x...) PrintfUtil_vPrintMem(x)
#endif

/*****************************************************************************
 * Typedef
 *
 ******************************************************************************/
#define JBSMAC_TX_BUFF_SIZE 20
#define JBSMAC_RX_BUFF_SIZE 20
#define UART_FIFO_MAX_LEN  (E_AHI_TX_FIFO_FULL - E_AHI_TX_FIFO_EMPTY)

#define JBSMAC_TIMER_PRESCALE_1ms (14)   //16M/(2^14) = 1kHz

#define JBSMAC_SEND_LIVE_TIMEOUT  1000   // send live to every port every 1000ms
#define LINK_TIMEOUT_COUNT (30) //30s
#define ERROR_TXFIFO            (0x1)
#define ERROR_ONSEND           (0x2)
#define ERROR_SENTLEN           (0x4)

#define JBSMAC_BUFTYPE_TX  			0
#define JBSMAC_BUFTYPE_SHORT_TX  	1
#define JBSMAC_BUFTYPE_RX  			2

#define BLAST_NWK_ADDRESS           0xFFFF
//#define ERROR(x) do{tBsMacHdl.error_status |= (x);} while (0);
typedef struct
{
    unsigned char buf[BSMAC_TX_LEN_DEFAULT];
    uint8 frame_len;
    uint8 frame_type;
} tx_buf_t;

typedef struct
{
    unsigned char buf[BSMAC_HEADER_LEN + BSMAC_FOOTER_LEN + 2];   // add 2 bytes for alignment
} tx_shortbuf_t;

typedef struct
{
    unsigned char buf[BSMAC_TX_LEN_DEFAULT];
    uint8 frame_len;
} rx_buf_t;


typedef struct
{
    //make sure the start of the following buffers aligned on 4-byte boundary
    tx_buf_t  tx_buf[2][JBSMAC_TX_BUFF_SIZE]; // buf for tx, or can use up-layer provided buf? if can,
    tx_shortbuf_t  tx_short_buf[2][JBSMAC_TX_BUFF_SIZE];
    rx_buf_t  rx_buf[2][JBSMAC_RX_BUFF_SIZE]; // buf for rx
    unsigned char tx_live_buf[2][BSMAC_TX_LEN_DEFAULT]; // buf for send live, for port 0,1

    unsigned char device_type; 	  //device type, Card_station or COM_station
    unsigned char up_port_known;  //have known upload port
    bool is_arm_neighbor;   // if I am the neighbor of arm. 1:yes  0:no
    unsigned char up_port;  	  //upload uart port
    unsigned short mac_addr; 	  // mac address of itself
    unsigned short peer_addr[2]; // mac address of device in uart 0 and 1
    volatile bool peer_rdy[2];   //uart port peer is ready
    unsigned char link_up[2]; 	 // indicate link is up or down
    unsigned char linkStatus_counter[2]; //if 0, means link down
    unsigned short armid;
    unsigned char use_ack; // switch for ack
    // bool use_flowCtrl;  // switch for ack
    // unsigned char use_retrans; // switch for retransmit
    // unsigned char wait_ack; // status indicating receive ack or not

    unsigned short tx_data_frame_cnt[2]; // tx frame count inside mac header
    unsigned short last_tx_data_frame_cnt[2]; // frame cnt of last data frame

    unsigned short rx_frame_cnt[2]; // received frame cnt
    unsigned short last_rx_frame_cnt[2]; // frame cnt of previous received frame

    //unsigned short rx_frame_cnt_tmbk; // timer-backup rx frame cnt for link status checking
    unsigned short rx_frame_total_num[2]; //number of total received frames
    unsigned short frame_lost_num[2]; //number of lost frames
    unsigned short frame_header_err; //  header error count
    unsigned short crc_err_cnt; // count of crc error
    unsigned short frame_dupl_err_cnt; // error count for received duplicate frame

    PR_BSMAC_RX_CALLBACK cb_hdl; // call back function handler

    unsigned char tx_sending_frame_type[2];

    // unsigned char volatile tx_buf_empty;
    unsigned char tx_buf_head[2];
    unsigned char tx_buf_tail[2];

    // unsigned char volatile tx_short_buf_empty;
    unsigned char tx_short_buf_head[2];
    unsigned char tx_short_buf_tail[2];

    // unsigned char volatile rx_buf_empty;
    unsigned char rx_buf_head[2];
    unsigned char rx_buf_tail[2];

    unsigned char tx_live[2];

    unsigned char tx_data_count[2];
    // unsigned char tx_sending_index;
    // unsigned char tx_len;
    // unsigned char live_counter;

    //uint32 error_status;
    uint32 last_tx_tick[2];  //last send DATA time

    uint32 system_time_ms;    // system time in ms
} TBsMacHdl_t, *PBSMACHDL;

/* add informations in live frame */
typedef struct
{
    bool   is_downdir;
    unsigned short armid;
} live_header_t;

#define LOCK() uint32 intstore; MICRO_DISABLE_AND_SAVE_INTERRUPTS(intstore)
#define UNLOCK()  MICRO_RESTORE_INTERRUPTS(intstore)

/*******************************************************************************
 * function declaration
 *******************************************************************************/
PRIVATE void Jbsmac_eTimerInit();
PRIVATE void vTimer0InterruptHandler(uint32 u32DeviceId,uint32 u32ItemBitmap);
PRIVATE char Jbsmac_eSendData(uint8 port, unsigned char *pbuf, const unsigned short len, const unsigned char frame_type);
PRIVATE eJbsmacRetVal Jbsmac_queue_transfer(uint8 transport);
PRIVATE void Jbsmac_paresRX(void);
PRIVATE void Jbsmac_LivePoll(void);
PRIVATE void Jbsmac_TimerUpdate(void);
PRIVATE void Jbsmac_SendLive(uint8 port);

PRIVATE void Jbsmac_tx_callback(uint8 port);
PRIVATE void Jbsmac_rx_callback(uint8 port, uint8* p, uint16 len);
PRIVATE bool Jbsmac_BufEmpty(uint8 port, uint8 buf_type);
PRIVATE bool Jbsmac_BufFull(uint8 port, uint8 buf_type);
PRIVATE void Jbsmac_EncHeader(uint8 port, uint8 buf_type);
PRIVATE void Jbsmac_EncTail(uint8 port, uint8 buf_type);
/*****************************************************************************
 * Global variables
 *
 ******************************************************************************/

static TBsMacHdl_t tBsMacHdl;

PUBLIC eJbsmacRetVal Jbsmac_u8Init(const PR_BSMAC_RX_CALLBACK cb_hdl, uint8 port, const uint8 uart_baud,
                                   const unsigned short mac_addr, const uint8 device_type)
{
    memset(&tBsMacHdl, 0, sizeof(tBsMacHdl));

    // initial variable

    // this function may be called after warm start,
    // do not reset link_up up_port_known arm_id and so on
    // so bsmac can work rapidly as it never restart

    //tBsMacHdl.up_port_known = 0;

    tBsMacHdl.use_ack = TRUE;
    // tBsMacHdl.use_flowCtrl = TRUE;
    if(port == E_AHI_UART_0  || port == E_AHI_UART_1)
    {
        tBsMacHdl.peer_rdy[port] = TRUE;
    }
    else  //for both port 0 and 1
    {
        tBsMacHdl.peer_rdy[0] = TRUE;
        tBsMacHdl.peer_rdy[1] = TRUE;
    }
    // tBsMacHdl.use_retrans = FALSE;

    tBsMacHdl.mac_addr = mac_addr;

    // check input parameter
    if (cb_hdl == NULL)
    {
        return JBSMAC_INIT_CB_INVALID;
    }
    else
    {
        tBsMacHdl.cb_hdl = cb_hdl;
    }

    if (device_type != BSMAC_DEVICE_TYPE_COM && device_type!= BSMAC_DEVICE_TYPE_LOC)
    {
        return JBSMAC_INIT_DEVICE_INVALID;
    }
    else
    {
        tBsMacHdl.device_type = device_type;
    }

    // fix me, no use now
    Jbsmac_eTimerInit();

    //init uart 0 and uart 1
    if(port == E_AHI_UART_0)
    {
        //TX0EN
        vAHI_DioSetDirection(0, E_AHI_DIO10_INT);
        vAHI_DioSetOutput(E_AHI_DIO10_INT, 0);
        vUart_Init(E_AHI_UART_0, uart_baud, Jbsmac_tx_callback, Jbsmac_rx_callback);

    }
    else if(port == E_AHI_UART_1)
    {
        //TX1EN
        vAHI_DioSetDirection(0, E_AHI_DIO11_INT);
        vAHI_DioSetOutput(E_AHI_DIO11_INT, 0);
        vUart_Init(E_AHI_UART_1, uart_baud, Jbsmac_tx_callback, Jbsmac_rx_callback);
    }
    else
    {
        //TX0EN
        vAHI_DioSetDirection(0, E_AHI_DIO10_INT);
        vAHI_DioSetOutput(E_AHI_DIO10_INT, 0);
        vUart_Init(E_AHI_UART_0, uart_baud, Jbsmac_tx_callback, Jbsmac_rx_callback);
        //TX1EN
        vAHI_DioSetDirection(0, E_AHI_DIO11_INT);
        vAHI_DioSetOutput(E_AHI_DIO11_INT, 0);
        vUart_Init(E_AHI_UART_1, uart_baud, Jbsmac_tx_callback, Jbsmac_rx_callback);
    }
    return JBSMAC_INIT_SUCCESS;
}

// Mp_station WarmStart,reinit bsmac but tBsMacHdl
PUBLIC eJbsmacRetVal Jbsmac_u8WarmStartInit(const PR_BSMAC_RX_CALLBACK cb_hdl, uint8 port, const uint8 uart_baud,
                                   const unsigned short mac_addr, const uint8 device_type)
{
    // check input parameter
    if (cb_hdl == NULL)
    {
        return JBSMAC_INIT_CB_INVALID;
    }
    else
    {
        tBsMacHdl.cb_hdl = cb_hdl;
    }

    if (device_type != BSMAC_DEVICE_TYPE_COM && device_type!= BSMAC_DEVICE_TYPE_LOC)
    {
        return JBSMAC_INIT_DEVICE_INVALID;
    }
    else
    {
        tBsMacHdl.device_type = device_type;
    }

    // fix me, no use now
    Jbsmac_eTimerInit();

    //init uart 0 and uart 1
    if(port == E_AHI_UART_0)
    {
        //TX0EN
        vAHI_DioSetDirection(0, E_AHI_DIO10_INT);
        vAHI_DioSetOutput(E_AHI_DIO10_INT, 0);
        vUart_Init(E_AHI_UART_0, uart_baud, Jbsmac_tx_callback, Jbsmac_rx_callback);

    }
    else if(port == E_AHI_UART_1)
    {
        //TX1EN
        vAHI_DioSetDirection(0, E_AHI_DIO11_INT);
        vAHI_DioSetOutput(E_AHI_DIO11_INT, 0);
        vUart_Init(E_AHI_UART_1, uart_baud, Jbsmac_tx_callback, Jbsmac_rx_callback);
    }
    else
    {
        //TX0EN
        vAHI_DioSetDirection(0, E_AHI_DIO10_INT);
        vAHI_DioSetOutput(E_AHI_DIO10_INT, 0);
        vUart_Init(E_AHI_UART_0, uart_baud, Jbsmac_tx_callback, Jbsmac_rx_callback);
        //TX1EN
        vAHI_DioSetDirection(0, E_AHI_DIO11_INT);
        vAHI_DioSetOutput(E_AHI_DIO11_INT, 0);
        vUart_Init(E_AHI_UART_1, uart_baud, Jbsmac_tx_callback, Jbsmac_rx_callback);
    }
    return JBSMAC_INIT_SUCCESS;
}


PRIVATE void Jbsmac_eTimerInit()
{
    vAHI_TimerDIOControl(E_AHI_TIMER_0, FALSE);
    vAHI_TimerEnable(E_AHI_TIMER_0, JBSMAC_TIMER_PRESCALE_1ms, FALSE, TRUE, FALSE);
    vAHI_TimerClockSelect(E_AHI_TIMER_0, FALSE, FALSE);
    vAHI_TimerStartRepeat(E_AHI_TIMER_0, 128, 256);
    vAHI_Timer0RegisterCallback(vTimer0InterruptHandler);
}
PUBLIC void Jbsmac_vPoll()
{
    Jbsmac_TimerUpdate();
    Jbsmac_LivePoll();
    Jbsmac_paresRX();
    Jbsmac_queue_transfer(0);  //send port 0 and port 1
    Jbsmac_queue_transfer(1);
}

PRIVATE char Jbsmac_buildPacket(uint8 port, unsigned char * pbuf,
                                const unsigned char * pdata, unsigned short len,
                                const unsigned char frame_type)
{
    // add mac header
    if (pbuf == NULL || len > BSMAC_MAX_TX_PAYLOAD_LEN  || port > E_AHI_UART_1)
    {
        EDBG(PRINTF("Build Failed pbuf%X len%d port%d\n",pbuf,len,port););
        return (char) -1;
    }

    bsmac_header_t *ph = (bsmac_header_t *) pbuf;

    ph->preamble_H = BSMAC_PREAMBLE_H;
    ph->preamble_L = BSMAC_PREAMBLE_L;

    BSMAC_SET_DEVICETYPE(ph->frame_control, tBsMacHdl.device_type);
    BSMAC_SET_RDY(ph->frame_control, 1);
    BSMAC_SET_FRAMETYPE(ph->frame_control, frame_type);
    BSMAC_SET_PRIORITY(ph->frame_control, 1);

    if (frame_type == BSMAC_FRAME_TYPE_ACK || frame_type == BSMAC_FRAME_TYPE_DATA_ACK) // for ack, use recieved frame_cnt
    {
        ph->frame_count_H = (tBsMacHdl.rx_frame_cnt[port] & 0xff00) >> 8;
        ph->frame_count_L = tBsMacHdl.rx_frame_cnt[port] & 0xff;
    }
    else
    {
        static uint16 tx_frame_cnt[2];
        ph->frame_count_H = (tx_frame_cnt[port] & 0xff00) >> 8; // framecnt_h
        ph->frame_count_L = tx_frame_cnt[port] & 0xff; // framecnt_l
        tx_frame_cnt[port]++;
    }

    ph->src_addr_H = (tBsMacHdl.mac_addr >> 8) & 0xff;
    ph->src_addr_L = (tBsMacHdl.mac_addr) & 0xff; // source mac address
    ph->dst_addr_H = 0;
    ph->dst_addr_L = 0;

    /* ack do not need payload, Live may have payload */
    if (len != 0 && pdata && (frame_type != BSMAC_FRAME_TYPE_ACK && frame_type != BSMAC_FRAME_TYPE_DATA_ACK))
    {
        memcpy((void*) (pbuf + BSMAC_HEADER_LEN), pdata, len);
    }

    //LIVE packet needs to be a long frame
    if (frame_type == BSMAC_FRAME_TYPE_LIVE)
    {
        len = BSMAC_MAX_TX_PAYLOAD_LEN;
    }
    else if(frame_type == BSMAC_FRAME_TYPE_ACK || frame_type == BSMAC_FRAME_TYPE_DATA_ACK)
    {
        len = 0;
    }

    unsigned short tx_len = len + BSMAC_FOOTER_LEN; // length = payload+footer
    ph->data_len_H = (tx_len >> 8) & 0xff; //
    ph->data_len_L = tx_len & 0xff; //

    unsigned short crc = CRC16((unsigned char *)(pbuf+2), len+BSMAC_HEADER_LEN-2, 0xffff);   // caculate header and payload

    // padding footer
    pbuf[len+BSMAC_HEADER_LEN] = (crc >> 8) & 0xff;
    pbuf[len+BSMAC_HEADER_LEN+1] = crc & 0xff;

    return 0;
}

PRIVATE eJbsmacRetVal Jbsmac_queue_transfer(uint8 transport)
{
    unsigned char *p=NULL;
    unsigned short len=0;
    unsigned char frame_type=0;
    uint8 port = 0;
    bool send =FALSE;

    //send port 0 and port a
    //the sending priority is ACK > DATA > LIVE
    for(port = 0; port < 2; port++)
    {
        // if trans port == 0 or 1, only trans port==transport, otherwise trans port 0 and port 1
        if( (transport == 0 &&  port == 1)  || (transport == 1 &&  port == 0) )
        {
            continue;
        }
        if(bUart_IsOnTx(port))
        {
            continue;
        }
        if (!Jbsmac_BufEmpty(port, JBSMAC_BUFTYPE_SHORT_TX))
        {
            uint8 tail = tBsMacHdl.tx_short_buf_tail[port];
            p =  tBsMacHdl.tx_short_buf[port][tail].buf;
            frame_type = BSMAC_FRAME_TYPE_ACK;
            len = BSMAC_HEADER_LEN + BSMAC_FOOTER_LEN;
            send = TRUE;
        }
        else if (!Jbsmac_BufEmpty(port, JBSMAC_BUFTYPE_TX))
        {
            uint8 tail = tBsMacHdl.tx_buf_tail[port];
            p = tBsMacHdl.tx_buf[port][tail].buf;
            len = tBsMacHdl.tx_buf[port][tail].frame_len;
            frame_type = tBsMacHdl.tx_buf[port][tail].frame_type;
            send = TRUE;

            // only send when peer is ready
            if(tBsMacHdl.peer_rdy[port]!= 1)
            {
                //EDBG(PRINTF("nrdy\n"););
                send = FALSE;
            }

            //do not send data in 3 ms
            uint32 curTick = u32AHI_TickTimerRead();
            if (curTick - tBsMacHdl.last_tx_tick[port] < 48000)
            {
                //DBG(PRINTF("ntk\n"););
                send = FALSE;
            }
        }

        //if do not send data or ack, then send live
        if (send==FALSE && tBsMacHdl.tx_live[port] )
        {
            p = (unsigned char*) (tBsMacHdl.tx_live_buf[port]);
            len = BSMAC_TX_LEN_DEFAULT;
            frame_type = BSMAC_FRAME_TYPE_LIVE;
            send = TRUE;
            //tBsMacHdl.tx_live[port] = 0;
        }

        if(send)
        {
            Jbsmac_eSendData(port, p, len, frame_type);
        }
    }
    return JBSMAC_QUEUE_SENDING;
}

/* do not use this function in interrupt!!!! */
PRIVATE eJbsmacRetVal Jbsmac_tx_enqueue(const unsigned char port, const unsigned char *pbuf, const unsigned short len, const unsigned char frame_type)
{
    char rs = JBSMAC_WRITE_SUCCESS;
    unsigned char *p;

    if (frame_type == BSMAC_FRAME_TYPE_ACK || frame_type == BSMAC_FRAME_TYPE_DATA_ACK)
    {
        if (Jbsmac_BufFull(port, JBSMAC_BUFTYPE_SHORT_TX))
        {
            return JBSMAC_WRITE_BUSY;
        }
        uint8 head = tBsMacHdl.tx_short_buf_head[port];

        p = tBsMacHdl.tx_short_buf[port][head].buf;
        Jbsmac_buildPacket(port, p, pbuf, len, frame_type);


        Jbsmac_EncHeader( port, JBSMAC_BUFTYPE_SHORT_TX);

    }
    else if (frame_type == BSMAC_FRAME_TYPE_DATA)
    {
        if (Jbsmac_BufFull(port, JBSMAC_BUFTYPE_TX))
        {
            return JBSMAC_WRITE_BUSY;
        }

        uint8 head = tBsMacHdl.tx_buf_head[port];

        p = tBsMacHdl.tx_buf[port][head].buf;
        Jbsmac_buildPacket(port, p, pbuf, len, frame_type);
        tBsMacHdl.tx_buf[port][head].frame_len = len + BSMAC_HEADER_LEN + BSMAC_FOOTER_LEN;
        tBsMacHdl.tx_buf[port][head].frame_type = frame_type;

        Jbsmac_EncHeader( port, JBSMAC_BUFTYPE_TX);

    }
    else if (frame_type == BSMAC_FRAME_TYPE_LIVE)
    {
        live_header_t live_header;
        /* tell other port I am the up dir */
        if(tBsMacHdl.up_port_known== 1 && port != tBsMacHdl.up_port)
        {
            live_header.is_downdir = TRUE;
            live_header.armid = tBsMacHdl.armid;
        }
        else
        {
            live_header.is_downdir = FALSE;
        }
        p = (unsigned char*) (&tBsMacHdl.tx_live_buf[port]);
        Jbsmac_buildPacket(port, p, (void*) &live_header, sizeof(live_header), frame_type);
    }

    /* try to start transfer after inqueque*/
    Jbsmac_queue_transfer(port);
    return rs;
}

PRIVATE void Jbsmac_vSetLinkStatus(uint8 port, uint8 u8State)
{
    uint8 idx = (port==E_AHI_UART_0 ? 0:1);
    tBsMacHdl.link_up[idx] = u8State;

    if(tBsMacHdl.up_port_known && port == tBsMacHdl.up_port && u8State==0)
    {
        tBsMacHdl.up_port_known = 0;
    }
}

PUBLIC int8 Jbsmac_i8GetUpPort(void)
{
    if(tBsMacHdl.up_port_known)
    {
        return tBsMacHdl.up_port;
    }
    else
    {
        return -1;
    }
}
PUBLIC uint8 Jbsmac_u8GetLinkStatus(uint8 port)
{
    DBG(PRINTF("Link %d %d %d %d %d\n",tBsMacHdl.up_port_known, tBsMacHdl.up_port, tBsMacHdl.link_up[0],tBsMacHdl.link_up[1],tBsMacHdl.armid););

    if(port == E_AHI_UART_0 || port == E_AHI_UART_1)
    {
        return tBsMacHdl.link_up[port];
    }
    else
    {
        return 0;
    }
}
PUBLIC uint16 Jbsmac_u16GetPeerAddr(uint8 port)
{
    if(port == E_AHI_UART_0 || port == E_AHI_UART_1)
    {
        return tBsMacHdl.peer_addr[port];
    }
    else
    {
        return 0;
    }

}

PUBLIC bool Jbsmac_bIsArmNeighbor()
{
    return tBsMacHdl.is_arm_neighbor;
}

PUBLIC void Jbsmac_vGetErrCnt(uint8 port, uint16* ptotalcnt, uint16* perrcnt)
{
    if(port == E_AHI_UART_0 || port == E_AHI_UART_1)
    {
        *ptotalcnt = tBsMacHdl.rx_frame_total_num[port] ;
        *perrcnt = tBsMacHdl.frame_lost_num[port];
        return ;
    }
}
PUBLIC uint16 Jbsmac_u16GetArmid(void)
{
    if(tBsMacHdl.up_port_known)
    {
        return tBsMacHdl.armid;
    }
    else
    {
        return 0;
    }
}

PUBLIC void Jbsmac_vSetUseAck(bool bUseAck)
{
    tBsMacHdl.use_ack = bUseAck;
}

PUBLIC void Jbsmac_vSetUseFlowCtrl(bool bFlowCtrl)
{
//    tBsMacHdl.use_flowCtrl = bFlowCtrl;
}

PUBLIC eJbsmacRetVal Jbsmac_eWriteData(uint8 *pbuf, uint8 len)
{
    unsigned char rs = 0;

    if (pbuf==NULL || len > BSMAC_MAX_TX_PAYLOAD_LEN)
    {
        EDBG(PRINTF("eWrite Failed\n"););
        return JBSMAC_WRITE_LENGTH_INVALID;
    }

    if(tBsMacHdl.up_port_known)
    {
        rs = Jbsmac_tx_enqueue(tBsMacHdl.up_port, pbuf, len, BSMAC_FRAME_TYPE_DATA);
    }
    else
    {
        EDBG(PRINTF("eWrite error\n"););
    }

    return rs;
}

PRIVATE void Jbsmac_paresRX()
{
    rx_buf_t* prxbuf;
    bsmac_header_t *ph;

    uint8 frame_type, device_type, len;
    uint16 crc, crc_recv;
    uint16 rx_fc = 0;
    struct nwkhdr *pNwkHdr;

    bool_t flag3;
    uint8 port;
    uint16 nwkdst;
    uint16 phy_srcaddr;

    for(port=0; port <2; port++)
    {
        if (Jbsmac_BufEmpty(port, JBSMAC_BUFTYPE_RX))
        {
            continue;
        }
        LOCK();
        uint8 tail = tBsMacHdl.rx_buf_tail[port];
        prxbuf = &tBsMacHdl.rx_buf[port][tail];
        UNLOCK();

        ph = (bsmac_header_t *) prxbuf->buf;

        flag3 = (ph->preamble_H != (unsigned char) BSMAC_PREAMBLE_H)
                || (ph->preamble_L != (unsigned char) BSMAC_PREAMBLE_L);
        // check preamble
        if (flag3)
        {
            if (tBsMacHdl.link_up[port] == 1)
            {
                tBsMacHdl.frame_header_err++;
            }
            EDBG(
                PRINTF("BSmac: header error \n");
            );
            Jbsmac_EncTail( port, JBSMAC_BUFTYPE_RX);
            continue;
        }

        // check frame type and device type
        frame_type = BSMAC_GET_FRAMETYPE(ph->frame_control);
        device_type = BSMAC_GET_DEVICETYPE(ph->frame_control);
        if ((frame_type > BSMAC_FRAME_TYPE_LIVE) )
        {
            tBsMacHdl.frame_header_err++;
            EDBG(PRINTF("BSmac: frame type error\n"););
            Jbsmac_EncTail( port, JBSMAC_BUFTYPE_RX);
            continue;
        }

        // check length
        if(prxbuf->frame_len > BSMAC_RX_LEN_DEFAULT)
        {
            EDBG(PRINTF("BSmac:  len %d error\n", prxbuf->frame_len ););
            Jbsmac_EncTail( port, JBSMAC_BUFTYPE_RX);
            continue;
        }
        len = (ph->data_len_H << 8) | ph->data_len_L;
        if ((frame_type == BSMAC_FRAME_TYPE_LIVE))
        {
            if(len!=BSMAC_RX_LEN_DEFAULT - BSMAC_HEADER_LEN)
            {
                EDBG(PRINTF("BSmac: live len %d error\n", len););
                Jbsmac_EncTail( port, JBSMAC_BUFTYPE_RX);
                continue;
            }
        }
        else if(frame_type == BSMAC_FRAME_TYPE_ACK ||frame_type == BSMAC_FRAME_TYPE_DATA_ACK)  // fixme: FPGA send a long ACK?
        {
            len = BSMAC_FOOTER_LEN;
        }
        else
        {
            if (len > (BSMAC_RX_LEN_DEFAULT - BSMAC_HEADER_LEN))
            {
                tBsMacHdl.frame_header_err++;
                EDBG(PRINTF("BSmac: phy len %d error\n", len););
                Jbsmac_EncTail( port, JBSMAC_BUFTYPE_RX);
                continue;
            }
        }

        // sometimes due to the damaged uart device, will receive signal from myself, filter them to avoid link LED up
        phy_srcaddr = (ph->src_addr_H << 8) | ph->src_addr_L;
        if( phy_srcaddr == tBsMacHdl.mac_addr)
        {
            tBsMacHdl.frame_header_err++;
            EDBG(PRINTF("BSmac: myself!%d \n",phy_srcaddr););
            //EDBG(PRINTMEM(prxbuf->buf, prxbuf->frame_len););
            Jbsmac_EncTail( port, JBSMAC_BUFTYPE_RX);
            continue;
        }

        pNwkHdr = (struct nwkhdr *)(ph+1);
        nwkdst = pNwkHdr->dst;
        SysUtil_vConvertEndian(&(nwkdst), sizeof(nwkdst));

        // check crc
        if (frame_type == BSMAC_FRAME_TYPE_LIVE
                ||frame_type == BSMAC_FRAME_TYPE_DATA  )
        {
            crc = CRC16((unsigned char *) (prxbuf->buf+ 2), len + BSMAC_HEADER_LEN
                        - BSMAC_FOOTER_LEN - 2, 0xffff); // caculate header and payload
            crc_recv = (prxbuf->buf[len + BSMAC_HEADER_LEN - BSMAC_FOOTER_LEN] << 8)
                       | prxbuf->buf[len + BSMAC_HEADER_LEN - BSMAC_FOOTER_LEN + 1];

            if (crc != crc_recv)
            {
                tBsMacHdl.crc_err_cnt++;
                EDBG(PRINTF("BSmac: crc error\n"););
			   // EDBG(PRINTMEM(prxbuf->buf, prxbuf->frame_len););
                Jbsmac_EncTail( port, JBSMAC_BUFTYPE_RX);
                continue;
            }

            // get peer address
            tBsMacHdl.peer_addr[port] = (ph->src_addr_H << 8) | ph->src_addr_L;
        }

        // check ready;
        tBsMacHdl.peer_rdy[port] = BSMAC_GET_RDY(ph->frame_control);

        // check frame_cnt
        rx_fc = (ph->frame_count_H << 8) | ph->frame_count_L;

        if ((frame_type != BSMAC_FRAME_TYPE_ACK) && (frame_type != BSMAC_FRAME_TYPE_DATA_ACK))
        {
            tBsMacHdl.last_rx_frame_cnt[port] = tBsMacHdl.rx_frame_cnt[port]; // bkup last frame_cnt
            tBsMacHdl.rx_frame_cnt[port] = rx_fc;

            if(++tBsMacHdl.rx_frame_total_num[port] == 0)  // if total frame num round over, clear lost num
            {
                tBsMacHdl.frame_lost_num[port] = 0;
            }
            if ((tBsMacHdl.link_up[port] == 1)  // connected
                    && (tBsMacHdl.rx_frame_cnt[port] != tBsMacHdl.last_rx_frame_cnt[port])  //repeat
                    && tBsMacHdl.rx_frame_cnt[port] != (tBsMacHdl.last_rx_frame_cnt[port] + 1)  //normal
                    && tBsMacHdl.last_rx_frame_cnt[port]!=0  //the first frame
                    && tBsMacHdl.rx_frame_cnt[port]!=0)    //other port reset
            {
                tBsMacHdl.frame_lost_num[port] += tBsMacHdl.rx_frame_cnt[port]-tBsMacHdl.last_rx_frame_cnt[port]-1; // no return, need further processing
            }
        }

        switch (frame_type)
        {
        case (BSMAC_FRAME_TYPE_DATA):

            DBG(PRINTF("RD P%d %X\n", port, tBsMacHdl.rx_frame_cnt[port]););

            // if the port is connecting to FPGA, sending ACK, otherwise not
            if((device_type == BSMAC_DEVICE_TYPE_BS_EP || device_type == BSMAC_DEVICE_TYPE_BS_OP)
                    &&(tBsMacHdl.use_ack))
            {
                Jbsmac_tx_enqueue(port, NULL, 0, BSMAC_FRAME_TYPE_ACK);
            }
            else
            {
                Jbsmac_tx_enqueue(port, NULL, 0, BSMAC_FRAME_TYPE_DATA_ACK);
            }

            /* look at the dst addr in network header, if dst address is not me, send to other port */
            if( nwkdst!= tBsMacHdl.mac_addr)
            {
                DBG(PRINTF("FW dst%X  L%d\n",nwkdst, len-2););
                //DBG(PRINTMEM((uint8*)pNwkHdr, len-2););
                uint16 other_port = (port==E_AHI_UART_0 ? E_AHI_UART_1 : E_AHI_UART_0);
                Jbsmac_tx_enqueue(other_port, (uint8*)pNwkHdr, len-2, BSMAC_FRAME_TYPE_DATA);
            }
            if(nwkdst == tBsMacHdl.mac_addr || nwkdst == BLAST_NWK_ADDRESS)
            {
                static unsigned short rx_data_frame_cnt[2]; // received data frame cnt
                static unsigned short last_rx_data_frame_cnt[2]; // frame cnt of previous received data frame

                //tBsMacHdl.rx_frame_total_cnt++;
                last_rx_data_frame_cnt[port] = rx_data_frame_cnt[port]; // bkup last frame_cnt
                rx_data_frame_cnt[port] = rx_fc;

                // frame duplicated, may caused by re-transmit with ack lost, so ignore it
                if (rx_data_frame_cnt[port]  == last_rx_data_frame_cnt[port])
                {
                    tBsMacHdl.frame_dupl_err_cnt++;
                    EDBG(PRINTF("Dup\n"););
                    //            return;
                }
                else if( tBsMacHdl.cb_hdl!=NULL)
                {
                    tBsMacHdl.cb_hdl((unsigned char*) (&(prxbuf->buf[BSMAC_HEADER_LEN])), len - BSMAC_FOOTER_LEN); // call back
                }
            }

            break;

        case (BSMAC_FRAME_TYPE_LIVE):
            DBG(PRINTF("RL P%d %X\n",port, rx_fc););

            //always send live ack
            if (tBsMacHdl.use_ack)
            {
                Jbsmac_tx_enqueue(port, NULL, 0, BSMAC_FRAME_TYPE_ACK);
            }
            /* parse live packet to know the up_port dir */
            if(device_type == BSMAC_DEVICE_TYPE_BS_EP || device_type == BSMAC_DEVICE_TYPE_BS_OP)
            {
                tBsMacHdl.up_port = port;
                tBsMacHdl.up_port_known = 1;
                tBsMacHdl.armid = tBsMacHdl.peer_addr[port];
                tBsMacHdl.is_arm_neighbor = 1;
                DBG(PRINTF("UPA P%d\n", port););
            }
            else
            {
                live_header_t* plive = (live_header_t*)(ph+1);
                if(plive->is_downdir)
                {
                    tBsMacHdl.up_port = port;
                    tBsMacHdl.up_port_known = 1;
                    tBsMacHdl.armid = plive->armid;
                    tBsMacHdl.is_arm_neighbor = 0;
                    DBG(PRINTF("UPB P%d\n", port););
                }
            }
            break;

        case (BSMAC_FRAME_TYPE_ACK):
            DBG(PRINTF("RA P%d %X\n", port, rx_fc););

            Jbsmac_vSetLinkStatus(port, 1);
            tBsMacHdl.linkStatus_counter[port] = LINK_TIMEOUT_COUNT;

            // if (rx_fc == tBsMacHdl.last_tx_data_frame_cnt[port])
            {
                //ACK received; OK to send next data packet
                tBsMacHdl.last_tx_tick[port] -= 48000;
            }
            break;

        case (BSMAC_FRAME_TYPE_DATA_ACK):

            tBsMacHdl.tx_data_count[port] = 0;
            Jbsmac_EncTail( port, JBSMAC_BUFTYPE_TX);
            break;
        }

        // udpate rx buffer
        Jbsmac_EncTail( port, JBSMAC_BUFTYPE_RX);
    }
}

PRIVATE char Jbsmac_eSendData(uint8 port, unsigned char *pbuf, const unsigned short len, const unsigned char frame_type)
{
    bsmac_header_t *ph;

    if (pbuf == NULL || len < BSMAC_HEADER_LEN || len > BSMAC_TX_LEN_DEFAULT)
    {
        EDBG(PRINTF("Send Failed %x %d\n",pbuf,len););
        return JBSMAC_WRITE_LENGTH_INVALID;
    }

    //record tx data frame_type
    ph = (bsmac_header_t *)pbuf;

    if (frame_type == BSMAC_FRAME_TYPE_DATA)
    {
        tBsMacHdl.last_tx_data_frame_cnt[port] = tBsMacHdl.tx_data_frame_cnt[port]; // bkup tx_data_frame_cnt for data frame
        tBsMacHdl.tx_data_frame_cnt[port] =  (ph->frame_count_H << 8) | ph->frame_count_L;
    }

    DBG(PRINTF("US P%d L%d %X\n",port, len, (ph->frame_count_H << 8) | ph->frame_count_L););

    if (u8Uart_StartTx(port, pbuf, len) == UART_SUCCESS)
    {
        tBsMacHdl.tx_sending_frame_type[port] = frame_type;
        if(frame_type == BSMAC_FRAME_TYPE_LIVE)
        {
            tBsMacHdl.tx_live[port] = 0;
        }
    }
    return 0;
}

PRIVATE void Jbsmac_TimerUpdate(void)
{
    static uint32 old_tick;
    uint32 tick = u32AHI_TickTimerRead();
    uint32 diff_time = (tick - old_tick) / 16000; /* in ms */

    if (diff_time > 0)
    {
        tBsMacHdl.system_time_ms += diff_time;
        old_tick = tick;
    }
}
PRIVATE void Jbsmac_LivePoll(void)
{
    static uint32 old_time;
    uint32 diff_time = tBsMacHdl.system_time_ms - old_time;

    static uint32 live_timeout = 0;
    static uint32 cnt;

    if(diff_time > 0)
    {
        if(diff_time >= live_timeout)
        {
            if(cnt++ %2 ==0)
            {
                Jbsmac_SendLive(E_AHI_UART_0);
            }
            else
            {
                Jbsmac_SendLive(E_AHI_UART_1);
            }
            live_timeout = JBSMAC_SEND_LIVE_TIMEOUT/2;
        }
        else
        {
            live_timeout -= diff_time;
        }
    }

    old_time = tBsMacHdl.system_time_ms;
}

PRIVATE void Jbsmac_SendLive(uint8 port)
{
    if( !tBsMacHdl.tx_live[port])
    {
        //DBG(PRINTF("Live EnQ P%d TM%d\n",port, tBsMacHdl.system_time_ms););
        tBsMacHdl.tx_live[port] = 1;
        Jbsmac_tx_enqueue(port, NULL, BSMAC_MAX_TX_PAYLOAD_LEN, BSMAC_FRAME_TYPE_LIVE);
    }

    tBsMacHdl.linkStatus_counter[port]--;
    if (tBsMacHdl.linkStatus_counter[port] == 0)
    {
        Jbsmac_vSetLinkStatus(port, 0);
    }
}

PRIVATE void Jbsmac_tx_callback(uint8 port)
{
    //LOCK();
    if (tBsMacHdl.tx_sending_frame_type[port] == BSMAC_FRAME_TYPE_DATA)
    {
        //record tick; do not send data too fast
        tBsMacHdl.last_tx_tick[port] = u32AHI_TickTimerRead();
        if(((tBsMacHdl.up_port == port) && (tBsMacHdl.is_arm_neighbor == 1)) || tBsMacHdl.tx_data_count[port]++ > 2)
        {
            Jbsmac_EncTail( port, JBSMAC_BUFTYPE_TX);
            tBsMacHdl.tx_data_count[port] = 0;
        }

    }
    else if (tBsMacHdl.tx_sending_frame_type[port]  == BSMAC_FRAME_TYPE_ACK)
    {
        Jbsmac_EncTail( port, JBSMAC_BUFTYPE_SHORT_TX);

        // do not send data in interrupt
        //Jbsmac_queue_transfer(port);
    }
    else if (tBsMacHdl.tx_sending_frame_type[port]  == BSMAC_FRAME_TYPE_LIVE)
    {
    }

    // UNLOCK();
}
PRIVATE void Jbsmac_rx_callback(uint8 port, uint8* p, uint16 len)
{
    uint8* pbuf;
    uint8 head;

    if(port != E_AHI_UART_0 && port != E_AHI_UART_1 )
    {
        EDBG(PRINTF("RFail %d\n", len););
    }

    if (p == NULL || len <BSMAC_HEADER_LEN|| len > BSMAC_RX_LEN_DEFAULT
            || Jbsmac_BufFull(port, JBSMAC_BUFTYPE_RX))
    {
        EDBG(PRINTF("RFail %d\n",len););
        return;
    }
    head = tBsMacHdl.rx_buf_head[port];

    pbuf = tBsMacHdl.rx_buf[port][head].buf;
    memcpy(pbuf, p, len);
    tBsMacHdl.rx_buf[port][head].frame_len = len;

    Jbsmac_EncHeader( port, JBSMAC_BUFTYPE_RX);


}
PRIVATE bool Jbsmac_BufEmpty(uint8 port, uint8 buf_type)
{
    uint8 head, tail;
    LOCK();
    if(buf_type == JBSMAC_BUFTYPE_TX)
    {
        head = tBsMacHdl.tx_buf_head[port];
        tail = tBsMacHdl.tx_buf_tail[port];
    }
    else if(buf_type == JBSMAC_BUFTYPE_SHORT_TX)
    {
        head = tBsMacHdl.tx_short_buf_head[port];
        tail = tBsMacHdl.tx_short_buf_tail[port];
    }
    else if(buf_type == JBSMAC_BUFTYPE_RX)
    {
        head = tBsMacHdl.rx_buf_head[port];
        tail = tBsMacHdl.rx_buf_tail[port];
    }
    else
    {
        UNLOCK();
        EDBG(PRINTF("type err!\n"););
        return TRUE;
    }
    UNLOCK();
    if (head == tail)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
PRIVATE bool Jbsmac_BufFull(uint8 port, uint8 buf_type)
{
    uint8 head, tail;
    uint16 buf_size;
    LOCK();
    if(buf_type == JBSMAC_BUFTYPE_TX)
    {
        head = tBsMacHdl.tx_buf_head[port];
        tail = tBsMacHdl.tx_buf_tail[port];
        buf_size = JBSMAC_TX_BUFF_SIZE;
    }
    else if(buf_type == JBSMAC_BUFTYPE_SHORT_TX)
    {
        head = tBsMacHdl.tx_short_buf_head[port];
        tail = tBsMacHdl.tx_short_buf_tail[port];
        buf_size = JBSMAC_TX_BUFF_SIZE;
    }
    else if(buf_type == JBSMAC_BUFTYPE_RX)
    {
        head = tBsMacHdl.rx_buf_head[port];
        tail = tBsMacHdl.rx_buf_tail[port];
        buf_size = JBSMAC_RX_BUFF_SIZE;
    }
    else
    {
        UNLOCK();
        EDBG(PRINTF("type err!\n"););
        return TRUE;
    }

    UNLOCK();
    if (head + 1 == tail || (tail == 0 && head == buf_size - 1))
    {
        EDBG(PRINTF("BufFull %d %d \n", port, buf_type););
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}
PRIVATE void Jbsmac_EncHeader(uint8 port, uint8 buf_type)
{
    if(buf_type == JBSMAC_BUFTYPE_TX)
    {
        LOCK();
        tBsMacHdl.tx_buf_head[port]++;
        if (tBsMacHdl.tx_buf_head[port] >= JBSMAC_TX_BUFF_SIZE)
        {
            tBsMacHdl.tx_buf_head[port] = 0;
        }
        UNLOCK();
        //DBG(PRINTF("TIn P%d %d %d\n",  port, tBsMacHdl.tx_buf_head[port],  tBsMacHdl.tx_buf_tail[port]););
    }
    else if(buf_type == JBSMAC_BUFTYPE_SHORT_TX)
    {
        LOCK();
        tBsMacHdl.tx_short_buf_head[port]++;
        if (tBsMacHdl.tx_short_buf_head[port] >= JBSMAC_TX_BUFF_SIZE)
        {
            tBsMacHdl.tx_short_buf_head[port] = 0;
        }
        UNLOCK();
        // DBG(PRINTF("TSIn P%d %d %d\n",  port, tBsMacHdl.tx_short_buf_head[port],  tBsMacHdl.tx_short_buf_tail[port]););
    }
    else if(buf_type == JBSMAC_BUFTYPE_RX)
    {
        LOCK();
        tBsMacHdl.rx_buf_head[port]++;
        if (tBsMacHdl.rx_buf_head[port] >= JBSMAC_RX_BUFF_SIZE)
        {
            tBsMacHdl.rx_buf_head[port] = 0;
        }
        UNLOCK();
        //DBG(PRINTF("RIn P%d %d %d\n",  port, tBsMacHdl.rx_buf_head[port],  tBsMacHdl.rx_buf_tail[port]););
    }
    return;
}
PRIVATE void Jbsmac_EncTail(uint8 port, uint8 buf_type)
{
    if(buf_type == JBSMAC_BUFTYPE_TX)
    {
        LOCK();
        if (tBsMacHdl.tx_buf_tail[port] != tBsMacHdl.tx_buf_head[port])
        {
            tBsMacHdl.tx_buf_tail[port]++;
            if (tBsMacHdl.tx_buf_tail[port] >= JBSMAC_TX_BUFF_SIZE)
            {
                tBsMacHdl.tx_buf_tail[port] = 0;
            }
        }
        UNLOCK();
    }
    else if(buf_type == JBSMAC_BUFTYPE_SHORT_TX)
    {
        LOCK();
        if (tBsMacHdl.tx_short_buf_tail[port] != tBsMacHdl.tx_short_buf_head[port])
        {
            tBsMacHdl.tx_short_buf_tail[port]++;
            if (tBsMacHdl.tx_short_buf_tail[port] >= JBSMAC_TX_BUFF_SIZE)
            {
                tBsMacHdl.tx_short_buf_tail[port] = 0;
            }
        }
        UNLOCK();

    }
    else if(buf_type == JBSMAC_BUFTYPE_RX)
    {
        LOCK();
        if (tBsMacHdl.rx_buf_tail[port] != tBsMacHdl.rx_buf_head[port])
        {
            tBsMacHdl.rx_buf_tail[port]++;
            if (tBsMacHdl.rx_buf_tail[port] >= JBSMAC_RX_BUFF_SIZE)
            {
                tBsMacHdl.rx_buf_tail[port] = 0;
            }
        }
        UNLOCK();
    }
    return;
}

PRIVATE void vTimer0InterruptHandler(uint32 u32DeviceId,uint32 u32ItemBitmap)
{
    if((u32DeviceId == E_AHI_DEVICE_TIMER0) && (u32ItemBitmap &  E_AHI_TIMER_PERIOD_MASK))
    {
    }

}


