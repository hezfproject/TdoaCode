#include <string.h>
#include "bsmac.h"
#include "hal_mcu.h"
#include "hal_uart_dma.h"
#include "bsmac_timer.h"
#include "bsmac_header.h"
#include "crc.h"
/*******************************************************************************
** local macro define
*******************************************************************************/
// mac length
#define MAC_MAX_TX_PAYLOAD_LEN         (128-MAC_HEADER_LEN - MAC_FOOTER_LEN)
#define MAC_RX_LEN_DEFAULT          (128)
#define MAC_HEADER_LEN              (12)
#define MAC_FOOTER_LEN              (2)
#define MAC_TX_BUFF_SIZE           (5)


// wait send
#define WAIT_SEND_SUCCESS           (0)
#define WAIT_SEND_FAIL              (1)
#define WAIT_SEND_TIMEOUT_CNT       (32768)         // (255)

// timer cnt
#define MAC_TIMER_CNT_LIVE          (120)


/*******************************************************************************
** local struct define
*******************************************************************************/
typedef struct _TBsMacHdl {
    unsigned short mac_addr;            // mac address of itself
    unsigned short peer_addr;           // mac address of peer
    unsigned short tx_frame_cnt;        // tx frame count inside mac header
    unsigned short last_tx_frame_cnt;   // frame cnt of last data frame
    unsigned short rx_frame_cnt;        // received frame cnt
    unsigned short last_rx_frame_cnt;   // frame cnt of previous received frame
    unsigned short rx_frame_cnt_tmbk;   // timer-backup rx frame cnt for link status checking
    unsigned char use_ack;              // switch for ack
    unsigned char use_retrans;          // switch for retransmit
    unsigned char use_flowctl;          // flow control switch    
    unsigned char wait_ack;             // status indicating receive ack or not
    unsigned char link_up;              // indicate link is up or down
    unsigned short timer_cnt;           // timer int count
    unsigned short rx_frame_total_cnt;  // total received frame count
    unsigned short peer_busy_cnt;  // total received frame count
    unsigned char volatile on_send;              // lock for sending
    unsigned char volatile peer_rdy;             // peer ready to do flow-control    
    unsigned char volatile ready;         // peer ready to do flow-control        
//    unsigned char rate_switch;          // first running at 100K, then switch to 1M
    bsmac_cb cb_hdl;                    // call back function handler
    unsigned char tx_sending_frame_type; //count of free
    unsigned char volatile tx_buf_empty; //count of free
    unsigned char tx_buf_head;
    unsigned char tx_buf_tail;
    unsigned char tx_buf_frame_len[MAC_TX_BUFF_SIZE];
    unsigned char tx_buf_frame_type[MAC_TX_BUFF_SIZE];
    unsigned char tx_buf[MAC_TX_BUFF_SIZE][MAC_MAX_TX_PAYLOAD_LEN+MAC_HEADER_LEN+MAC_FOOTER_LEN]; // buf for tx, or can use up-layer provided buf? if can, 

    unsigned char volatile tx_short_buf_empty; //count of free
    unsigned char tx_short_buf_head;
    unsigned char tx_short_buf_tail;
    unsigned char tx_short_buf_type[MAC_TX_BUFF_SIZE];    
    unsigned char tx_short_buf[MAC_TX_BUFF_SIZE][MAC_HEADER_LEN+MAC_FOOTER_LEN];
                                                                     
    unsigned char rx_buf[MAC_MAX_TX_PAYLOAD_LEN+MAC_HEADER_LEN+MAC_FOOTER_LEN]; // buf for rx
    
}TBsMacHdl, *PBSMACHDL;

static TBsMacHdl tBsMacHdl;

//#define DEBUG_BSMAC

#ifdef DEBUG_BSMAC
#define DBG_EXCUTE(x) BSMAC_EXCUTE(x)
#else
#define DBG_EXCUTE(x) 
#endif

#ifdef DEBUG_BSMAC
typedef struct _TBsMacDebug {
    unsigned short frame_lost_cnt;      // count of frame lost
    unsigned short frame_header_err;    //  header error count
    unsigned short crc_err_cnt;         // count of crc error   
    unsigned short frame_dupl_err_cnt;  // error count for received duplicate frame
    unsigned short peer_busy_cnt;  // total received frame count
    unsigned char old_peer_rdy;  // total received frame count        
    
    unsigned char intProcesshistory[100];
    unsigned short intstart[100];
    unsigned short intend[100];    
    unsigned char intIndex;

    unsigned short timerx;
    unsigned short timetx;    
    unsigned short timelive;   
    
}TBsMacDebug;

static TBsMacDebug tBsMacDebug;
#endif


// for debugging    
//unsigned short live_cnt = 0;
//unsigned short ack_cnt = 0;
//unsigned short pam_err_cnt = 0;
//unsigned short type_err_cnt = 0;
//unsigned short lost_err_cnt = 0;

/*******************************************************************************
** Local subroutine define
*******************************************************************************/
static char mac_send_data(const unsigned char *pbuf, const unsigned short len ,const unsigned char frame_type);
static void mac_rx_handler(void);
static void mac_tx_handler(void);

static void mac_timer_handler(unsigned char int_ch);
static unsigned char wait_send_free(void);
/*******************************************************************************
** Description:
    initial mac layer
** Input param:
    bsmac_cb                : callback function pointer
    uart_port                 : uart port(0 or 1)
    dma_ch_tx               : channel number of tx dma(0 to 4)
    dma_ch_rx               : channel number of rx dma(0 to 4)
** Return value:
    0       : success
    ohters  : fail
*******************************************************************************/
unsigned char bsmac_init(const bsmac_cb cb_hdl, const unsigned char uart_port, const unsigned char dma_ch_tx, const unsigned char dma_ch_rx, 
                                           const unsigned short mac_addr )
{
    char rs = 0;
    
    // clear handle
    memset(&tBsMacHdl, 0, sizeof(TBsMacHdl));
    
    // initial variable
    tBsMacHdl.use_ack = 1;
    tBsMacHdl.use_retrans = 0;
    tBsMacHdl.tx_buf_empty = MAC_TX_BUFF_SIZE;
    tBsMacHdl.tx_buf_head = 0;
    tBsMacHdl.tx_buf_tail = 0;
    
    tBsMacHdl.tx_short_buf_empty = MAC_TX_BUFF_SIZE;
    tBsMacHdl.tx_short_buf_head = 0;
    tBsMacHdl.tx_short_buf_tail = 0;

    tBsMacHdl.peer_rdy = 1;

    DBG_EXCUTE(
        tBsMacDebug.old_peer_rdy= 1;
    );

    tBsMacHdl.ready = 1;
    tBsMacHdl.mac_addr = mac_addr;
    
    // check input parameter
    if (cb_hdl == NULL) {
        return BSMAC_INIT_CB_INVALID;
    } else {
        tBsMacHdl.cb_hdl = cb_hdl;
    }   
    
    // init uart-dma driver
    rs = hal_uartdma_init(mac_rx_handler, mac_tx_handler, uart_port, dma_ch_tx, dma_ch_rx, (const unsigned char *)tBsMacHdl.tx_buf[0], (const unsigned char*)tBsMacHdl.rx_buf);
    if (rs != 0) {
        return BSMAC_INIT_HARDWARE_ERR;
    }
    
    // init mac timer
    rs = bsmac_timer_init(mac_timer_handler);
    if (rs != 0) {
        return BSMAC_INIT_TIMER_ERR;
    }
    
    return BSMAC_INIT_SUCCESS;
}

/*******************************************************************************
** Description:
    Build bsmac header
** Input param:
    pbuf     pointer to header
    pdata   pointer to data
    len       data len
    frame_type
** Return value
    0         scucess
    other   fail
*******************************************************************************/
static char mac_buildPacket(unsigned char * pbuf, const unsigned char * pdata, const unsigned short len, const unsigned char frame_type)
{
    // add mac header
    if(pbuf == NULL ||  len > MAC_MAX_TX_PAYLOAD_LEN) 
    {
        return (char)-1;
    }
    
    bsmac_header_t *ph = (bsmac_header_t *)pbuf;
    ph->preamble_H = BSMAC_PREAMBLE_H;
    ph->preamble_L = BSMAC_PREAMBLE_L;

    BSMAC_SET_DEVICETYPE(ph->frame_control, BSMAC_DEVICE_TYPE_COM);
    BSMAC_SET_RDY(ph->frame_control, 1);
    BSMAC_SET_FRAMETYPE(ph->frame_control, frame_type);
    BSMAC_SET_PRIORITY(ph->frame_control, 1);
    
    if (frame_type == BSMAC_FRAME_TYPE_ACK) {     // for ack, use recieved frame_cnt
        ph->frame_count_H = (tBsMacHdl.rx_frame_cnt & 0xff00) >> 8;
        ph->frame_count_L = tBsMacHdl.rx_frame_cnt & 0xff;
    } else {
        if (frame_type == BSMAC_FRAME_TYPE_DATA) {
            tBsMacHdl.last_tx_frame_cnt = tBsMacHdl.tx_frame_cnt;   // bkup tx_frame_cnt for data frame
        }
        ph->frame_count_H = (tBsMacHdl.tx_frame_cnt & 0xff00) >> 8;  // framecnt_h
        ph->frame_count_L = (tBsMacHdl.tx_frame_cnt)++ & 0xff;       // framecnt_l
    }
    ph->src_addr_H = (tBsMacHdl.mac_addr >> 8) & 0xff;
    ph->src_addr_L = (tBsMacHdl.mac_addr) & 0xff;  // source mac address
    ph->dst_addr_H = 0;
    ph->dst_addr_L = 0;
    
    unsigned short tx_len = len + MAC_FOOTER_LEN;                  // length = payload+footer
    ph->data_len_H = (tx_len >> 8) & 0xff;   // 
    ph->data_len_L = tx_len & 0xff;          //     

    
    if (len != 0 && frame_type != BSMAC_FRAME_TYPE_LIVE) {
        memcpy((void*)(pbuf+MAC_HEADER_LEN), pdata, len);
    }

    unsigned short crc = CRC16((unsigned char *)(pbuf+2), len+MAC_HEADER_LEN-2, 0xffff);   // caculate header and payload

    // padding footer
    pbuf[len+MAC_HEADER_LEN] = (crc >> 8) & 0xff;
    pbuf[len+MAC_HEADER_LEN+1] = crc & 0xff;    

    return 0;
}


/*******************************************************************************
** Description:
    send data, constructing mac layer frame
** Input param:
    pbuf    : poniter of sending buf
    len     : sending length, including header and footer
    type    : sending frame type
** Return value:
    0       :   success
    others  : fail
*******************************************************************************/
static char mac_send_data(const unsigned char *pbuf, const unsigned short len , const unsigned char frame_type)
{
    unsigned char rs = 0;    

    // set lock
    tBsMacHdl.on_send = 1;
    
    // check tx dma is free or not
    if (hal_uartdma_TxBusy() != HAL_UARTDMA_FREE) {
        // release lock
        //tBsMacHdl.on_send = 0;           
        return (char)-1;
    }

    // check flowctl status
    if (frame_type == BSMAC_FRAME_TYPE_DATA && tBsMacHdl.use_flowctl) {
        if (tBsMacHdl.peer_rdy == 0) {
            return 0;
        }
    }        
    
    // start dma
    hal_uartdma_config_tx(pbuf, len);
    rs = hal_uartdma_start_tx();

    //if (frame_type == BSMAC_FRAME_TYPE_DATA && tBsMacHdl.use_flowctl) tBsMacHdl.peer_rdy = 0;
    tBsMacHdl.tx_sending_frame_type = frame_type;
    bsmac_timer_reset_live();
    
    if (rs != 0) {
        // release lock
        //tBsMacHdl.on_send = 0;        
        return (char)-1;
    }
    
    if ((frame_type == BSMAC_FRAME_TYPE_DATA) && (tBsMacHdl.use_retrans)) {
        tBsMacHdl.wait_ack = 1;
    }

    // release lock???
    //tBsMacHdl.on_send = 0;    
    
    return 0;
}

static void mac_queue_transfer()
{
    unsigned char *p; 
    unsigned short len; 
    unsigned char frame_type; 

    if(tBsMacHdl.tx_short_buf_empty != MAC_TX_BUFF_SIZE)
    {
        p = (unsigned char*)(&tBsMacHdl.tx_short_buf[tBsMacHdl.tx_short_buf_tail]);
        len = MAC_HEADER_LEN+MAC_FOOTER_LEN;
        frame_type = tBsMacHdl.tx_short_buf_type[tBsMacHdl.tx_short_buf_tail];
        mac_send_data(p, len, frame_type);
    }
    else if(tBsMacHdl.tx_buf_empty != MAC_TX_BUFF_SIZE)
    {
        p = (unsigned char*)(&tBsMacHdl.tx_buf[tBsMacHdl.tx_buf_tail]);
        len = tBsMacHdl.tx_buf_frame_len[tBsMacHdl.tx_buf_tail];
        frame_type = tBsMacHdl.tx_buf_frame_type[tBsMacHdl.tx_buf_tail];
        mac_send_data(p, len, frame_type);
    }
}

/*******************************************************************************
** Description:
    Enqueue the pending data
    do NOT call me in interrupt handling
** Input param:
    pbuf    : poniter of sending buf
    len     : sending length, NOT including header and footer
** Return value:
    0       :   success
    others  : fail
*******************************************************************************/
static unsigned char mac_tx_enqueue(const unsigned char *pbuf, const unsigned short len, const unsigned char frame_type)
{
    unsigned char rv = BSMAC_WRITE_SUCCESS;
    unsigned char onSending = 0;
    char rs = 0;
    unsigned char *p;

    if (tBsMacHdl.tx_buf_empty == 0) {
        return BSMAC_WRITE_BUSY;
    }
    
    if(frame_type == BSMAC_FRAME_TYPE_ACK)
    {
        p = (unsigned char*)(&tBsMacHdl.tx_short_buf[tBsMacHdl.tx_short_buf_head]);
        mac_buildPacket(p, pbuf, len, frame_type);
        
        tBsMacHdl.tx_short_buf_type[tBsMacHdl.tx_short_buf_head] = frame_type;
        
        tBsMacHdl.tx_short_buf_head ++;
        if(tBsMacHdl.tx_short_buf_head == MAC_TX_BUFF_SIZE) 
            tBsMacHdl.tx_short_buf_head = 0;

halIntState_t is;
HAL_ENTER_CRITICAL_SECTION(is);
        
        tBsMacHdl.tx_short_buf_empty--;

        onSending = tBsMacHdl.on_send;

        //send immediately if no data is sending; actually there should be only one data in queue;
        if(!onSending && ((!tBsMacHdl.use_flowctl) || tBsMacHdl.peer_rdy )){
            mac_queue_transfer();
        }
HAL_EXIT_CRITICAL_SECTION(is);
        
    }
    else
    {
        
        p = (unsigned char*)(&tBsMacHdl.tx_buf[tBsMacHdl.tx_buf_head]);
        mac_buildPacket(p, pbuf, len, frame_type);

        
        tBsMacHdl.tx_buf_frame_len[tBsMacHdl.tx_buf_head] = len + MAC_HEADER_LEN + MAC_FOOTER_LEN;
        tBsMacHdl.tx_buf_frame_type[tBsMacHdl.tx_buf_head] = frame_type;
        
        tBsMacHdl.tx_buf_head ++;
        if(tBsMacHdl.tx_buf_head == MAC_TX_BUFF_SIZE) 
            tBsMacHdl.tx_buf_head = 0;

halIntState_t is;
HAL_ENTER_CRITICAL_SECTION(is);
        
        tBsMacHdl.tx_buf_empty--;

        onSending = tBsMacHdl.on_send;

        //send immediately if no data is sending; actually there should be only one data in queue;
        if(!onSending && ((!tBsMacHdl.use_flowctl) || tBsMacHdl.peer_rdy )){
            mac_queue_transfer();
        }
HAL_EXIT_CRITICAL_SECTION(is);
    }

    if(rs != 0) rv = BSMAC_WRITE_DMA_ERROR;

    return rv;

}

/*******************************************************************************
** Description:
    write data to mac layer
** Input param:
    pbuf    : pointer of writing buf
    len     : writing length
** Return value:
    0       : success
    others  : fail
*******************************************************************************/
unsigned char bsmac_write_data(const unsigned char *pbuf, const unsigned short len)
{
    unsigned char rs = 0;

    if (tBsMacHdl.tx_buf_empty == 0) {
        return BSMAC_WRITE_BUSY;
    }
    // check input parameter
    if (len > MAC_MAX_TX_PAYLOAD_LEN) {
        return BSMAC_WRITE_LENGTH_INVALID;
    }

    rs = mac_tx_enqueue(pbuf, len, BSMAC_FRAME_TYPE_DATA);
    
    return rs;
}


/*******************************************************************************
** Description:
    mac layer call back function to handle driver-received data
*******************************************************************************/
static void mac_rx_handler(void)
{
    unsigned char frame_type = 0;
    unsigned char device_type = 0;
    unsigned short crc = 0;
    unsigned short crc_recv = 0;
    unsigned short len = 0;
    unsigned char oldready;

#ifdef DEBUG_BSMAC    
    unsigned short times = 0, timee = 0;
#endif

    DBG_EXCUTE(
        times |= T1CNTL;
        times |= T1CNTH << 8;

        tBsMacDebug.intstart[tBsMacDebug.intIndex] = times;
        tBsMacDebug.intProcesshistory[tBsMacDebug.intIndex] = 1;
    );

    // stop rx_dma timer
    //bsmac_timer_stop_listen_rxdma();

    bsmac_header_t *ph = (bsmac_header_t *)tBsMacHdl.rx_buf;
    // check preamble
    if ((ph->preamble_H != BSMAC_PREAMBLE_H) || (ph->preamble_L != BSMAC_PREAMBLE_L)) {

    DBG_EXCUTE(        
        if (tBsMacHdl.link_up == 1) {
            tBsMacDebug.frame_header_err++;
        }
    );        
        hal_uartdma_reset_rx();
        return;
    }
    
    // check frame type and device type
    frame_type = BSMAC_GET_FRAMETYPE(ph->frame_control);
    device_type = BSMAC_GET_DEVICETYPE(ph->frame_control);

    DBG_EXCUTE(
        if ((frame_type > BSMAC_FRAME_TYPE_LIVE) || (device_type != BSMAC_DEVICE_TYPE_BS_EP)) {
            tBsMacDebug.frame_header_err++;       
            return;
        }
    );

    // get peer address
    tBsMacHdl.peer_addr = (ph->src_addr_H << 8) | ph->src_addr_L;    
    
    // check length
    if ((frame_type == BSMAC_FRAME_TYPE_LIVE) || (frame_type == BSMAC_FRAME_TYPE_ACK)) {
        len = MAC_RX_LEN_DEFAULT - MAC_HEADER_LEN;
    } else {
        len = (ph->data_len_H << 8) | ph->data_len_L;
        DBG_EXCUTE(
            if (len != (MAC_RX_LEN_DEFAULT - MAC_HEADER_LEN)) {
                tBsMacDebug.frame_header_err++;
                return;
            }
        );
    }
    
    // check crc
    if(frame_type != BSMAC_FRAME_TYPE_ACK)
    {
	    crc = CRC16((unsigned char *)(tBsMacHdl.rx_buf+2), len+MAC_HEADER_LEN-MAC_FOOTER_LEN-2, 0xffff);       // caculate header and payload
	    crc_recv = (tBsMacHdl.rx_buf[len+MAC_HEADER_LEN-MAC_FOOTER_LEN] << 8) | tBsMacHdl.rx_buf[len+MAC_HEADER_LEN-MAC_FOOTER_LEN+1];
        if (crc != crc_recv) {
            DBG_EXCUTE(
            
                tBsMacDebug.crc_err_cnt++;
            );
            return;
        }
    }
    
    if(tBsMacHdl.use_flowctl)
    {
        oldready = tBsMacHdl.peer_rdy;
        tBsMacHdl.peer_rdy = BSMAC_GET_RDY(ph->frame_control);

        DBG_EXCUTE(
        static uint8 ii;
            if(tBsMacDebug.old_peer_rdy == 0 && tBsMacHdl.peer_rdy == 0)
            {
                ii++;
            }        
            tBsMacDebug.old_peer_rdy = tBsMacHdl.peer_rdy;
        ); 
        
        if(!tBsMacHdl.peer_rdy) tBsMacHdl.peer_busy_cnt++;        
        if(tBsMacHdl.peer_rdy == 1 && oldready == 0 && !tBsMacHdl.on_send)
        {            
            mac_queue_transfer();            
        }
    }

    // check frame_cnt
    if (frame_type != BSMAC_FRAME_TYPE_ACK) {
        tBsMacHdl.last_rx_frame_cnt = tBsMacHdl.rx_frame_cnt;   // bkup last frame_cnt
        tBsMacHdl.rx_frame_cnt = (ph->frame_count_H << 8) | ph->frame_count_L;

        DBG_EXCUTE(        
            if ((tBsMacHdl.link_up == 1) && (tBsMacHdl.rx_frame_cnt != (tBsMacHdl.last_rx_frame_cnt + 1))) 
            {
                tBsMacDebug.frame_lost_cnt++;   // no return, need further processing
            }
        );
    }

   
    switch (frame_type) {
    case (BSMAC_FRAME_TYPE_DATA):
        tBsMacHdl.rx_frame_total_cnt++;
            static uint8 ii;
        if(tBsMacHdl.rx_frame_total_cnt == 2)
        {
            ii =1;
        }

        DBG_EXCUTE(
            if (tBsMacHdl.rx_frame_cnt == tBsMacHdl.last_rx_frame_cnt) {    // frame duplicated, may caused by re-transmit with ack lost, so ignore it
                tBsMacDebug.frame_dupl_err_cnt++;
    //            return;
            }
        );
        
        tBsMacHdl.cb_hdl((unsigned char*)(&(tBsMacHdl.rx_buf[MAC_HEADER_LEN])), len-MAC_FOOTER_LEN);      // call back

        if (tBsMacHdl.use_ack) {
                mac_tx_enqueue(NULL, 0, BSMAC_FRAME_TYPE_ACK);
        } 
        break;
        
    case (BSMAC_FRAME_TYPE_LIVE):
//        live_cnt++;
        if (tBsMacHdl.use_ack) {           

            mac_tx_enqueue(NULL, 0, BSMAC_FRAME_TYPE_ACK);
            
        } 
        tBsMacHdl.link_up = 1;
        
        hal_uartdma_set_linksta(1); // for debugging
        break;
    case (BSMAC_FRAME_TYPE_ACK):
//        ack_cnt++;
        if (tBsMacHdl.use_retrans && (tBsMacHdl.rx_frame_cnt == tBsMacHdl.last_tx_frame_cnt)) {
            tBsMacHdl.wait_ack = 0;
        }

        break;
    default:
        return;
    }          

    DBG_EXCUTE(

        timee |= T1CNTL;
        timee |= T1CNTH << 8;

        tBsMacDebug.intend[tBsMacDebug.intIndex] = timee;
        tBsMacDebug.intIndex++;
        if(tBsMacDebug.intIndex==100) tBsMacDebug.intIndex = 0;
        unsigned short time = timee >= times ? (timee - times):(8193 + timee - times);
        if(time > tBsMacDebug.timerx) tBsMacDebug.timerx = time;
    );    
}

/*******************************************************************************
** Description:
    mac layer call back function to handle driver-sending-finish
*******************************************************************************/
static void mac_tx_handler(void)
{
#ifdef DEBUG_BSMAC    
    unsigned short times = 0, timee = 0;
#endif

    DBG_EXCUTE(
        times |= T1CNTL;
        times |= T1CNTH << 8;

        tBsMacDebug.intstart[tBsMacDebug.intIndex] = times;
        tBsMacDebug.intProcesshistory[tBsMacDebug.intIndex] = 2;
    );
    
    if(tBsMacHdl.tx_sending_frame_type == BSMAC_FRAME_TYPE_DATA || tBsMacHdl.tx_sending_frame_type == BSMAC_FRAME_TYPE_LIVE)
    {
        tBsMacHdl.tx_buf_empty++;
        tBsMacHdl.tx_buf_tail++;
        if(tBsMacHdl.tx_buf_tail == MAC_TX_BUFF_SIZE)  tBsMacHdl.tx_buf_tail = 0;
    }
    else
    {
        tBsMacHdl.tx_short_buf_empty++;
        tBsMacHdl.tx_short_buf_tail++;
        if(tBsMacHdl.tx_short_buf_tail == MAC_TX_BUFF_SIZE)  tBsMacHdl.tx_short_buf_tail = 0;
    }

    //still have data in queue
    if(tBsMacHdl.tx_short_buf_empty != MAC_TX_BUFF_SIZE ||
       (tBsMacHdl.tx_buf_empty != MAC_TX_BUFF_SIZE && ((!tBsMacHdl.use_flowctl) || tBsMacHdl.peer_rdy ))
      ) 
    {
        mac_queue_transfer();
    }
    else
    {        
        tBsMacHdl.on_send = 0;
    }

    DBG_EXCUTE(
        timee |= T1CNTL;
        timee |= T1CNTH << 8;

        tBsMacDebug.intend[tBsMacDebug.intIndex] = timee;
        tBsMacDebug.intIndex++;
        if(tBsMacDebug.intIndex==100) tBsMacDebug.intIndex = 0;
        unsigned short time = timee >= times ? (timee - times):(8193 + timee - times);
        if(time > tBsMacDebug.timetx) tBsMacDebug.timetx = time;    
    );
    
}

/*******************************************************************************
** Description:
    timer callback function, to check link status and do further handling
*******************************************************************************/
static void mac_timer_handler(unsigned char int_chan)
{
#ifdef DEBUG_BSMAC    
    unsigned short times = 0, timee = 0;
#endif

    DBG_EXCUTE(
        times |= T1CNTL;
        times |= T1CNTH << 8;

        tBsMacDebug.intstart[tBsMacDebug.intIndex] = times;
        tBsMacDebug.intProcesshistory[tBsMacDebug.intIndex] = 3;
    );
    
    if (int_chan == BSMAC_TIMER_CH0_INT) {  // link handling
        tBsMacHdl.timer_cnt++;
        // check link status
        if (tBsMacHdl.timer_cnt  == MAC_TIMER_CNT_LIVE) {    // 60s
            tBsMacHdl.timer_cnt = 0;
            if (tBsMacHdl.rx_frame_cnt_tmbk == tBsMacHdl.rx_frame_cnt) {
                tBsMacHdl.link_up = 0;
                
                hal_uartdma_set_linksta(0); // for debugging
            } else {
                tBsMacHdl.rx_frame_cnt_tmbk = tBsMacHdl.rx_frame_cnt;
            }
        }

        // send live packet
        mac_tx_enqueue((unsigned char*)(&(tBsMacHdl.rx_buf[MAC_HEADER_LEN])), MAC_MAX_TX_PAYLOAD_LEN, BSMAC_FRAME_TYPE_LIVE);
    } else if (int_chan == BSMAC_TIMER_CH1_INT) {   // rx dma hang handling
        hal_uartdma_reset_rx();
    }

    DBG_EXCUTE(
        timee |= T1CNTL;
        timee |= T1CNTH << 8;

        tBsMacDebug.intend[tBsMacDebug.intIndex] = timee;
        tBsMacDebug.intIndex++;
        if(tBsMacDebug.intIndex==100) tBsMacDebug.intIndex = 0;
        unsigned short time = timee >= times ? (timee - times):(8193 + timee - times);
        if(time > tBsMacDebug.timelive) tBsMacDebug.timelive = time;    
    );
    
}


/*******************************************************************************
** Description:
    control bsmac
** Input param:
    ctrl_id : control id
    pbuf    : pointer to control buf
    plen    : pointer to buf length variable
** Return value:
    0       : success
    0thers  : fail
*******************************************************************************/
unsigned char bsmac_control(const unsigned char ctrl_id, const unsigned char *pbuf, const unsigned char *plen)
{
    TBsMacGetLinkSta *pBsMacGetLinkSta;
    TBsMacSetUseAck *pBsMacSetUseAck;
    TBsMacSetUseRetrans *pBsMacSetUseRetrans;
    TBsMacSetUseFlowCtl *pBsMacSetUseFlowCtl;
    
    if (pbuf == 0) {
        return BSMAC_CONTROL_BUF_INVALID;
    }
    
    switch (ctrl_id) {
    case (BSMAC_GET_LINKSTA):
        // check length
        if (*plen != sizeof(TBsMacGetLinkSta)) {
            return BSMAC_CONTROL_LEN_INVALID;
        }
        pBsMacGetLinkSta = (TBsMacGetLinkSta *)pbuf;
        pBsMacGetLinkSta->sta = (tBsMacHdl.link_up == 0) ? BSMAC_LINK_DOWN : BSMAC_LINK_UP;
        
        break;
    case (BSMAC_SET_USEACK):
        if (*plen != sizeof(TBsMacSetUseAck)) {
            return BSMAC_CONTROL_LEN_INVALID;
        }
        pBsMacSetUseAck = (TBsMacSetUseAck *)pbuf;
        tBsMacHdl.use_ack = pBsMacSetUseAck->flag;
        
        break;
    case (BSMAC_SET_USERETRANS):
        if (*plen != sizeof(TBsMacSetUseRetrans)) {
            return BSMAC_CONTROL_LEN_INVALID;
        }
        pBsMacSetUseRetrans = (TBsMacSetUseRetrans *)pbuf;
        tBsMacHdl.use_retrans = pBsMacSetUseRetrans->flag;
        
        break;
    case (BSMAC_SET_USEFLOWCTL):
        if (*plen != sizeof(TBsMacSetUseFlowCtl)) {
            return BSMAC_CONTROL_LEN_INVALID;
        }
        pBsMacSetUseFlowCtl = (TBsMacSetUseFlowCtl *)pbuf;
        tBsMacHdl.use_flowctl = pBsMacSetUseFlowCtl->flag;
        
        break;
        
        
    default:
        return BSMAC_CONTROL_CTRLID_INVALID;
    }
    
    return BSMAC_CONTROL_SUCCESS;
}

/*******************************************************************************
** Description:
    wait send free, before call mac_send_data() function
** Return value:
    0:      success
    1:      timeout
*******************************************************************************/
static unsigned char wait_send_free(void)
{   
    unsigned short rs = 0;
    
    while (((hal_uartdma_TxBusy() != HAL_UARTDMA_FREE) || (tBsMacHdl.on_send == 1)) && (rs < WAIT_SEND_TIMEOUT_CNT)) {
        rs++;
    }
    if ((tBsMacHdl.on_send == 0) && (hal_uartdma_TxBusy() == HAL_UARTDMA_FREE)) {
        return WAIT_SEND_SUCCESS;
    } else {
        return WAIT_SEND_FAIL;
    }
}
