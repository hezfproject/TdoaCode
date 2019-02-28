/*
 * File      : bsmac_parser.c
 * Change Logs:
 */

#include "bsmac_parser.h"
//#include "serial.h"
#include "bsmac_header.h"
#include "crc.h"
#include "nwk_protocol.h"
#include "bootcfg.h"
#include "msg_center.h"
#include "ptl_nwk.h"
#include "net_app.h"
#include "3g_watchdog.h"
#include "string.h"
#include "led_indicator.h"
#include <time.h>
#include <stdio.h>
#include <rtthread.h>


#include "../../../../../version.h"
#include "3g_protocol.h"

//#define LOG_DEBUG
#include "3g_log.h"
#include <stm32f2xx.h>


/*****************************************************************************
*
* defines
*
*****************************************************************************/
//#define __STM32__


#define BSMAC_POOL_MAX   2048  			 // size of bsmac message dequeue

#define BSMAC_SEND_LIVE_TIMEOUT  10000   // send live every 10000 ms
#define BSMAC_POLL_PHONE_TIMEOUT  10000    // 10s timeout phone poll
#define BSMAC_LINK_TIMEOUT_COUNT 30     // if 30 no live, the link is down

#define BSMAC_SEND_DATA_TIMEOUT  600    // send data req  600 ms


#define BSMAC_LINK_LED_ON()
#define BSMAC_LINK_LED_OFF()

#define UARTSTATE_PREAMBLE_H 0
#define UARTSTATE_PREAMBLE_L 1
#define UARTSTATE_FRAME_CONTROL 2
#define UARTSTATE_RESERVERD 3
#define UARTSTATE_FRAME_COUNT_H 4
#define UARTSTATE_FRAME_COUNT_L 5
#define UARTSTATE_SRC_ADDR_H 6
#define UARTSTATE_SRC_ADDR_L 7
#define UARTSTATE_DST_ADDR_H 8
#define UARTSTATE_DST_ADDR_L 9
#define UARTSTATE_DATA_LEN_H 10
#define UARTSTATE_DATA_LEN_L 11
#define UARTSTATE_DATA 12

#define BSMAC_RX_LEN      512
#define DEBUG_BSMAC

/* max len */
/* use micros in bsmac_headr.h */
/* the bsmac tx and rx max len is 128 */

//#define BSMAC_MAX_TX_LEN 128
//#define BSMAC_MAX_RX_LEN 128

#define RT_DEBUG_BSMAC  RT_TRUE     // open or close debug log
/*****************************************************************************
*
* typedefs
*
*****************************************************************************/

typedef struct
{
    rt_device_t device;

    /* tx */
    rt_uint8_t tx_buf[BSMAC_TX_LEN_DEFAULT+10];
    rt_uint16_t tx_wantlen;
    rt_uint16_t tx_datalen;

    /* rx */
    rt_uint8_t rx_buf[UWB_RX_LEN_DEFAULT+10];      // point to rx_real_buf + MSG_HEADER_SIZE
    rt_uint16_t rx_wantlen;
    rt_uint16_t rx_datalen;

    rt_bool_t rx_ind;        // indicate the isr has received data

    /* state machine */
    rt_uint8_t state;

    TRANSMIT_ID_EM transmit_id;            // my transmit_id, uart4

    /*control */
    rt_bool_t use_ack;
    rt_uint16_t peer_addr; // mac address of device in uart 0 and 1
    rt_bool_t  peer_rdy;    //for now,  the peer is always ready

    rt_bool_t link_up; 	  				  // indicate link is up or down
    rt_uint16_t    linkStatus_counter;		 // link counter, 0-30

    /* status */
    //rt_bool_t on_tx;
    //  bool on_rx;

    /*frame counts */
    rt_uint16_t tx_frame_cnt;
    rt_uint16_t rx_frame_cnt;

} bsmac_hdl_t;

typedef struct
{
    rt_uint16_t phone_addr;
    rt_uint16_t station_panid;
    rt_tick_t tick;
    rt_bool_t info_ind;
    rt_uint8_t rx_buf[128];
} bsmac_phone_info_t;


/*****************************************************************************
*
* functions defination
*
*****************************************************************************/
void bsmac_init(void);
void bsmac_led_init(void);
void bsmac_reset_gpio_init(void);
void bsmac_set_device(const char* device_name,rt_uint8_t device_port);
rt_err_t bsmac_rx0_ind(rt_device_t dev, rt_size_t size);
rt_err_t bsmac_rx1_ind(rt_device_t dev, rt_size_t size);
rt_err_t bsmac_rx2_ind(rt_device_t dev, rt_size_t size);
void bsmac_tx_ind(rt_uint8_t port);
void bsmac_uart_recv(rt_uint8_t port);
void bsmac_parse_rx(rt_uint8_t* p, rt_uint16_t length, rt_uint8_t port);
rt_size_t bsmac_build_packet( unsigned char * pbuf,const unsigned char * pdata,
                                unsigned short len,const unsigned char frame_type, rt_uint8_t port);

void bsmac_live_poll(void);
void bsmac_data_req_poll(void);

rt_err_t  bsmac_send_packet(rt_uint8_t* p, rt_uint16_t len, rt_uint8_t port);
rt_err_t bsmac_send_live(rt_uint8_t device_port);
rt_err_t bsmac_send_ack(rt_uint8_t port);
rt_bool_t bsmac_parse_local_protocol(rt_uint8_t *p,rt_uint8_t port);

void _bsmac_analyser_callback(void *pvParam);
rt_uint8_t bsmac_search_phone_nwk(rt_uint16_t addr);


/*****************************************************************************
*
* variables
*
*****************************************************************************/
struct rt_messagequeue bsmac_mq;
static  rt_uint8_t bsmac_mq_pool[BSMAC_POOL_MAX];

static  bsmac_hdl_t bsmac_hdl[3];

static bsmac_phone_info_t bsmac_phone_info[10];


#ifdef DEBUG_BSMAC
static char bsmac_err_buf[256];
#endif

static rt_uint8_t bsmac_recv1_data_cnt = 0;
static rt_uint8_t bsmac_recv2_data_cnt = 0;

static rt_tick_t last_data_recv_tick;



/*****************************************************************************
*
* functions
*
*****************************************************************************/
/*bcd code translation */
rt_uint8_t  num_bcd2char(char*pc, rt_uint8_t num)
{
    num &= 0x0F;
    if(num<=0x09)
    {
        *pc = '0'+num;
        return 1;
    }
    else if(num == 0x0A)
    {
        *pc = '+';
        return 1;
    }
    else if(num == 0x0B)
    {
        *pc = '-';
        return 1;
    }
    else if(num == 0x0C)
    {
        *pc = '*';
        return 1;
    }
    else if(num == 0x0D)
    {
        *pc = '#';
        return 1;
    }
    else     if(num == 0x0F)
    {
        *pc = 0;
        return 1;
    }

    return 0;
}


unsigned int num_term_to_str(char* s, const app_termNbr_t *p)
{
    char* ss = s;
    unsigned char i;

    if(!s || !p)  return 0;

    for(i=0; i<APP_NMBRDIGIT;i++)
    {
        char c;
        if(num_bcd2char(&c,p->nbr[i] & 0x0F))
        {
            if(c == 0)
            {
                break;
            }
            else
            {
                *s++ = c;
            }
        }
        else
        {
            break;
        }
        if(num_bcd2char(&c,(p->nbr[i]>>4) & 0x0F))
        {
            if(c == 0)
            {
                break;
            }
            else
            {
                *s++ = c;
            }
        }
        else
        {
            break;
        }
    }

    *s = 0;
    return (unsigned int)(ss - s);
}


void bsmac_thread_entry(void* parameter)
{
    bsmac_init();

    while(1)
    {
        static rt_uint8_t rev_buf[MSG_COM_PKT_SIZE];

         /* 网络线程启动后由网络线程喂狗 */
        if (!iwdg_net_feed_flag) {
            feed_watchdog();
        }

        if(rt_mq_recv(&bsmac_mq, rev_buf, sizeof(rev_buf), RT_WAITING_NO) == RT_EOK)
        {
            //todo:按port分发
            struct nwkhdr *pNwkHdr = (struct nwkhdr *)rev_buf;
            APP_HDR_T *pstAppHdr = (APP_HDR_T *)(pNwkHdr + 1);

            app_mpCmd_t *pMpCmd = (app_mpCmd_t *)(pstAppHdr + 1);

            if(sizeof(struct nwkhdr) + pNwkHdr->len < MSG_COM_PKT_SIZE)
            {              
                    if(pNwkHdr->dst == 0xFFFF)
                    {
                    	//rt_kprintf("AAA\n");
                        bsmac_send_packet(rev_buf, sizeof(struct nwkhdr) + pNwkHdr->len, 1);
                        bsmac_send_packet(rev_buf, sizeof(struct nwkhdr) + pNwkHdr->len, 2);
                        bsmac_send_packet(rev_buf, sizeof(struct nwkhdr) + pNwkHdr->len, 0);
						
                    }
                    else
                    {
			if(APP_TOF_MSG_ALARM_ACK == pstAppHdr->msgtype)
			{
				bsmac_send_packet(rev_buf, sizeof(struct nwkhdr) + pNwkHdr->len, 1);
				bsmac_send_packet(rev_buf, sizeof(struct nwkhdr) + pNwkHdr->len, 0);
			}                        
                        else if(pNwkHdr->dst > 39999)
                        {
                            //pNwkHdr->dst = pNwkHdr->dst - 10000;
                            //rt_kprintf("CCC\n");
                            bsmac_send_packet(rev_buf, sizeof(struct nwkhdr) + pNwkHdr->len, 0);
                        }
                        else
                        {
				bsmac_send_packet(rev_buf, sizeof(struct nwkhdr) + pNwkHdr->len, 1);							
                        }
                    }
            }
            else
            {
                RT_DEBUG_LOG(RT_DEBUG_BSMAC, ("BSMac: bsmac_mq too long %d \n",sizeof(struct nwkhdr) + pNwkHdr->len ));
            }
        }
        else if (bsmac_hdl[0].rx_ind || bsmac_hdl[1].rx_ind || bsmac_hdl[2].rx_ind)
        {
            //bsmac_hdl.rx_ind = RT_FALSE;
            if(bsmac_hdl[0].rx_ind)
            {
		bsmac_uart_recv(0);
                bsmac_hdl[0].rx_ind = RT_FALSE;
		//rt_kprintf("uart0\n");
            }

            if(bsmac_hdl[1].rx_ind)
            {
                bsmac_uart_recv(1);
				
                bsmac_hdl[1].rx_ind = RT_FALSE;
            }

            if(bsmac_hdl[2].rx_ind)
            {
                bsmac_uart_recv(2);
                bsmac_hdl[2].rx_ind = RT_FALSE;
            }
        }
        else
        {
             bsmac_live_poll();
			 bsmac_data_req_poll();
             rt_thread_delay(1);
        }
    }
}

rt_bool_t bsmac_get_link_status(rt_uint8_t port)
{
    return bsmac_hdl[port].link_up;
}

rt_uint16_t bsmac_get_peer_addr(rt_uint8_t port)
{
    return bsmac_hdl[port].peer_addr;
}

void bsmac_init()
{
    //rt_hw_tick_get_microsecond();

    /* message queue */

    rt_mq_init(&bsmac_mq, "BSMACMQ", bsmac_mq_pool, MSG_COM_PKT_SIZE,
               sizeof(bsmac_mq_pool), RT_IPC_FLAG_FIFO);

    /* bsmac hdl */
    memset(bsmac_hdl, 0, sizeof(bsmac_hdl));

    bsmac_hdl[0].use_ack  = RT_TRUE;
    bsmac_hdl[1].use_ack  = RT_TRUE;
    bsmac_hdl[2].use_ack  = RT_TRUE;

    /* uart 4*/
    bsmac_hdl[0].transmit_id = COM4_TRANSMIT_ID;
    bsmac_hdl[1].transmit_id = COM4_TRANSMIT_ID;
    bsmac_hdl[2].transmit_id = COM4_TRANSMIT_ID;


    bsmac_set_device("uart2",0);
    bsmac_set_device("uart4",1);
    bsmac_set_device("uart5",2);


    msg_analyser_register(COM4_TRANSMIT_ID, _bsmac_analyser_callback);

    /* led */
    bsmac_led_init();

    bsmac_reset_gpio_init();
}
void bsmac_reset_gpio_init(void)
{
	//reset stm32l151
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;   
    	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;         
    	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    	GPIO_Init(GPIOE, &GPIO_InitStructure);    
    	//GPIO_ResetBits(GPIOE,GPIO_Pin_5);    
    	GPIO_SetBits(GPIOE,GPIO_Pin_5);
}

void bsmac_led_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;		  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);	  
	GPIO_ResetBits(GPIOE,GPIO_Pin_3);    
	//GPIO_SetBits(GPIOE,GPIO_Pin_5);
}

void bsmac_set_device(const char* device_name,rt_uint8_t device_port)
{
    bsmac_hdl[device_port].device = rt_device_find(device_name);

    if (bsmac_hdl[device_port].device != RT_NULL && rt_device_open(bsmac_hdl[device_port].device, RT_DEVICE_OFLAG_RDWR) == RT_EOK)
    {
        //rt_device_set_rx_indicate(bsmac_hdl[device_port].device, bsmac_rx_ind);
        switch(device_port)
        {
            case 0:
                rt_device_set_rx_indicate(bsmac_hdl[device_port].device, bsmac_rx0_ind);
                break;
            case 1:
                rt_device_set_rx_indicate(bsmac_hdl[device_port].device, bsmac_rx1_ind);
                break;
            case 2:
                rt_device_set_rx_indicate(bsmac_hdl[device_port].device, bsmac_rx2_ind);
                break;
            default:
                RT_DEBUG_LOG(RT_DEBUG_BSMAC,("device port error:%s\n", device_port));
        }
    }
    else
    {
        if (bsmac_hdl[device_port].device != RT_NULL)
        {
            rt_device_close(bsmac_hdl[device_port].device);
        }
        RT_DEBUG_LOG(RT_DEBUG_BSMAC,("can not open device:%s\n", device_name));
    }
}

/* run in isr */
static rt_err_t bsmac_rx0_ind(rt_device_t dev, rt_size_t size)
{
    /* release semaphore to let finsh thread rx data */
    bsmac_hdl[0].rx_ind = RT_TRUE;
    return RT_EOK;
}

/* run in isr */
static rt_err_t bsmac_rx1_ind(rt_device_t dev, rt_size_t size)
{
    /* release semaphore to let finsh thread rx data */
    bsmac_hdl[1].rx_ind = RT_TRUE;
    return RT_EOK;
}

/* run in isr */
static rt_err_t bsmac_rx2_ind(rt_device_t dev, rt_size_t size)
{
    /* release semaphore to let finsh thread rx data */
    bsmac_hdl[2].rx_ind = RT_TRUE;
    return RT_EOK;
}



rt_err_t  bsmac_send_packet(rt_uint8_t* p, rt_uint16_t len, rt_uint8_t port)
{
    rt_size_t size = bsmac_build_packet(bsmac_hdl[port].tx_buf, p, len, BSMAC_FRAME_TYPE_DATA, port);

    if(size >0 && size <= BSMAC_TX_LEN_DEFAULT)
    {
        if(rt_device_write(bsmac_hdl[port].device, 0, bsmac_hdl[port].tx_buf, size) != size)
        {
            RT_DEBUG_LOG(RT_DEBUG_BSMAC,("send pkt error %d\n", size));
        }
        return RT_EOK;
    }
    else
    {
        RT_DEBUG_LOG(RT_DEBUG_BSMAC,("send pkt err %x %d\n", p, len));

        return RT_ERROR;
    }
}

rt_err_t bsmac_send_live(rt_uint8_t device_port)
{
    rt_size_t size = bsmac_build_packet(bsmac_hdl[device_port].tx_buf, RT_NULL, 0, BSMAC_FRAME_TYPE_LIVE, device_port);

    if(size == BSMAC_TX_LEN_DEFAULT)
    {
        if(rt_device_write(bsmac_hdl[device_port].device, 0, bsmac_hdl[device_port].tx_buf, size) != size)
        {
            RT_DEBUG_LOG(RT_DEBUG_BSMAC,("send live error %d\n", size));
        }
    }
    else
    {
        RT_DEBUG_LOG(RT_DEBUG_BSMAC,("send live err %d\n", size));
    }

    if(bsmac_hdl[device_port].linkStatus_counter > 0)
    {
        bsmac_hdl[device_port].linkStatus_counter--;
    }

    if (bsmac_hdl[device_port].linkStatus_counter == 0)
    {
        bsmac_hdl[device_port].link_up = RT_FALSE;
        BSMAC_LINK_LED_OFF();
    }

    return RT_EOK;
}
rt_err_t bsmac_send_ack(rt_uint8_t port)
{

    rt_size_t size = bsmac_build_packet(bsmac_hdl[port].tx_buf, RT_NULL, 0, BSMAC_FRAME_TYPE_ACK, port);

    if(size == sizeof(bsmac_header_t) + BSMAC_FOOTER_LEN)
    {
        if(rt_device_write(bsmac_hdl[port].device, 0, bsmac_hdl[port].tx_buf, size) != size)
        {
            RT_DEBUG_LOG(RT_DEBUG_BSMAC,("send ack error %d\n", size));
        }
        return RT_EOK;
    }
    else
    {
        RT_DEBUG_LOG(RT_DEBUG_BSMAC,("send ack err %d\n", size));
        return RT_ERROR;
    }

}




static  void bsmac_uart_recv(rt_uint8_t port)
{
    rt_uint8_t data;
    rt_size_t len;
    rt_uint16_t i;
    bsmac_hdl_t *pHdl;
    bsmac_header_t *pbsmac;
    rt_uint8_t read_continue = 1;

    static rt_uint8_t uart_buf[BSMAC_RX_LEN];

    pHdl = &bsmac_hdl[port];
    pbsmac =  ( bsmac_header_t *)bsmac_hdl[port].rx_buf;
    while(read_continue--)
    {
    len = rt_device_read(bsmac_hdl[port].device, 0, uart_buf, BSMAC_RX_LEN);
    //rt_kprintf("bsmac_uart_recv port %d, len %d\n",port, len);
    if(len == BSMAC_RX_LEN)
        read_continue = 1;

    //DEBUG_LOG("bamac rec len: %d\n",len);

	//rt_kprintf("uart buf %d,%d,%d,%d\n",uart_buf[0],uart_buf[1],uart_buf[2],uart_buf[3]);
	//rt_kprintf("len%d\n",len);

    if (len > 0)
    {
        for(i=0; i<len; i++)
        {
            data = uart_buf[i];

            switch(pHdl->state)
            {
            case UARTSTATE_PREAMBLE_H:
            {
                if(data == BSMAC_PREAMBLE_H)
                {
                    pbsmac->preamble_H = data;
                    pHdl->state = UARTSTATE_PREAMBLE_L;
                }
                break;
            }
            case UARTSTATE_PREAMBLE_L:
            {
                if(data == BSMAC_PREAMBLE_L)
                {
                    pbsmac->preamble_L  = data;
                    pHdl->state = UARTSTATE_FRAME_CONTROL;
                }
                else
                {
                    pHdl->state = UARTSTATE_PREAMBLE_H;
                }
                break;
            }
            case UARTSTATE_FRAME_CONTROL:
            {
                pbsmac->frame_control= data;
                pHdl->state = UARTSTATE_RESERVERD;
                break;
            }
            case UARTSTATE_RESERVERD:
            {
                pbsmac->reserverd = data;
                pHdl->state = UARTSTATE_FRAME_COUNT_H;
                break;

            }
            case UARTSTATE_FRAME_COUNT_H:
            {
                pbsmac->frame_count_H = data;
                pHdl->state = UARTSTATE_FRAME_COUNT_L;
                break;
            }
            case UARTSTATE_FRAME_COUNT_L:
            {
                pbsmac->frame_count_L = data;
                pHdl->state = UARTSTATE_SRC_ADDR_H;
                break;
            }
            case UARTSTATE_SRC_ADDR_H:
            {
                pbsmac->src_addr_H= data;
                pHdl->state = UARTSTATE_SRC_ADDR_L;
                break;
            }

            case UARTSTATE_SRC_ADDR_L:
            {
                pbsmac->src_addr_L = data;
                pHdl->state = UARTSTATE_DST_ADDR_H;
                break;
            }
            case UARTSTATE_DST_ADDR_H:
            {
                pbsmac->dst_addr_H = data;
                pHdl->state = UARTSTATE_DST_ADDR_L;
                break;
            }

            case UARTSTATE_DST_ADDR_L:
            {
                pbsmac->dst_addr_L = data;
                pHdl->state = UARTSTATE_DATA_LEN_H;
                break;
            }
            case UARTSTATE_DATA_LEN_H:
            {
                pbsmac->data_len_H = data;
                pHdl->state = UARTSTATE_DATA_LEN_L;
                break;
            }
            case UARTSTATE_DATA_LEN_L:
            {
                pbsmac->data_len_L = data;

                pHdl->rx_wantlen = (pbsmac->data_len_H<<8 | pbsmac->data_len_L); // + 2; len is including crc
		//rt_kprintf("wantlen %d\n",pHdl->rx_wantlen);
                if(pHdl->rx_wantlen <= (UWB_RX_LEN_DEFAULT - sizeof(bsmac_header_t)))
                {
                    pHdl->rx_datalen =0;
                    pHdl->state = UARTSTATE_DATA;
                }
                else
                {
                    pHdl->state = UARTSTATE_PREAMBLE_H;
                }
                break;
            }
            case UARTSTATE_DATA:
            {
                if (sizeof(bsmac_header_t) + pHdl->rx_datalen >= UWB_RX_LEN_DEFAULT)
                {
                    pHdl->state = UARTSTATE_PREAMBLE_H;
                    break;
                }
                pHdl->rx_buf[sizeof(bsmac_header_t) + pHdl->rx_datalen] = data;
                if(++pHdl->rx_datalen >= pHdl->rx_wantlen)
                {
					//rt_kprintf("rxlen %d\n",pHdl->rx_datalen);
					bsmac_parse_rx( pHdl->rx_buf, sizeof(bsmac_header_t) + pHdl->rx_datalen,port);

                    pHdl->state = UARTSTATE_PREAMBLE_H;
                }
                else
                {
                    pHdl->state = UARTSTATE_DATA;
                }
                break;
            }
            }
        }
    }
}
}

void bsmac_parse_rx(rt_uint8_t* p, rt_uint16_t length,rt_uint8_t port)
{
    struct nwkhdr *pnwkhdr;
    bsmac_header_t *ph;


    rt_uint8_t frame_type, len;
    //rt_uint8_t device_type;
    rt_uint16_t crc, crc_recv;
    rt_bool_t flag;
    rt_uint16_t rx_fc;
    MSG_CENTER_HEADER_T  * pCenterHeader;
    rt_err_t err = 0;
    static rt_uint8_t rssi_modules_link_state;

	 //struct nwkhdr *pNwkHdr;
    //app_header_t  *pAppHdr;


    if(p == RT_NULL|| length > UWB_RX_LEN_DEFAULT)
    {
        RT_DEBUG_LOG(RT_DEBUG_BSMAC,("BSmac: p or len error  \n"));
        return;
    }

    ph = (bsmac_header_t *) p;
    pnwkhdr = (struct nwkhdr *)(ph+1);

    // check preamble
    flag = (ph->preamble_H != (unsigned char) BSMAC_PREAMBLE_H)
           || (ph->preamble_L != (unsigned char) BSMAC_PREAMBLE_L);
    if (flag)
    {
#ifdef DEBUG_BSMAC
            int report_len = (length < sizeof(bsmac_err_buf)) ?
                length : sizeof(bsmac_err_buf);
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            snprintf(bsmac_err_buf, sizeof(bsmac_err_buf),
                "BSmac: header error\n");
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            memcpy(bsmac_err_buf, p, report_len);
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
#endif
        RT_DEBUG_LOG(RT_DEBUG_BSMAC,("BSmac: header error \n"));
        return;
    }

    // check frame type and device type
    frame_type = BSMAC_GET_FRAMETYPE(ph->frame_control);
    //device_type = BSMAC_GET_DEVICETYPE(ph->frame_control);
    if ((frame_type > BSMAC_FRAME_TYPE_LIVE))
    {
#ifdef DEBUG_BSMAC
            int report_len = (length < sizeof(bsmac_err_buf)) ?
                length : sizeof(bsmac_err_buf);
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            snprintf(bsmac_err_buf, sizeof(bsmac_err_buf),
                "BSmac: frame type error\n");
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            memcpy(bsmac_err_buf, p, report_len);
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
#endif
        RT_DEBUG_LOG(RT_DEBUG_BSMAC,("BSmac: frame type error\n"));
        return;
    }

    //check len
    len = (ph->data_len_H << 8) | ph->data_len_L;
	
    if ((frame_type == BSMAC_FRAME_TYPE_LIVE))
    {
        if(len!=128 - BSMAC_HEADER_LEN)
        {
#ifdef DEBUG_BSMAC
            int report_len = (length < sizeof(bsmac_err_buf)) ?
                length : sizeof(bsmac_err_buf);
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            snprintf(bsmac_err_buf, sizeof(bsmac_err_buf),
                "BSmac: live len %d error\n", len);
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            memcpy(bsmac_err_buf, p, report_len);
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
#endif
            RT_DEBUG_LOG(RT_DEBUG_BSMAC,("BSmac: live len %d error\n", len));
            return;
        }
    }
    else if(frame_type == BSMAC_FRAME_TYPE_ACK)  // fixme: FPGA send a long ACK?
    {
        len = BSMAC_FOOTER_LEN;
    }
    else
    {
        if (len > (UWB_RX_LEN_DEFAULT - BSMAC_HEADER_LEN))
        {
#ifdef DEBUG_BSMAC
            int report_len = (length < sizeof(bsmac_err_buf)) ?
                length : sizeof(bsmac_err_buf);
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            snprintf(bsmac_err_buf, sizeof(bsmac_err_buf),
                "BSmac: phy len %d error\n", len);
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            memcpy(bsmac_err_buf, p, report_len);
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
#endif
            RT_DEBUG_LOG(RT_DEBUG_BSMAC,("BSmac: phy len %d error\n", len));
            return;
        }
    }

    // ack do not check crc
    if (frame_type == BSMAC_FRAME_TYPE_LIVE
            ||frame_type == BSMAC_FRAME_TYPE_DATA)
    {
        crc = CRC16((unsigned char *) (p+ 2), len + BSMAC_HEADER_LEN
                    - BSMAC_FOOTER_LEN - 2, 0xffff); // caculate header and payload
        crc_recv = ((*(p+len + BSMAC_HEADER_LEN - BSMAC_FOOTER_LEN) << 8)
                    | *(p+len + BSMAC_HEADER_LEN - BSMAC_FOOTER_LEN + 1));

        if (crc != crc_recv)
        {
#ifdef DEBUG_BSMAC
            int report_len = (length < sizeof(bsmac_err_buf)) ?
                length : sizeof(bsmac_err_buf);
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            snprintf(bsmac_err_buf, sizeof(bsmac_err_buf),
                "bsmac rec crc error calc crc %d rec crc %d", crc, crc_recv);
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
            memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
            memcpy(bsmac_err_buf, p, report_len);
            net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                sizeof(bsmac_err_buf));
#endif
            RT_DEBUG_LOG(RT_DEBUG_BSMAC,("BSmac: crc error %d,%d\n",crc,crc_recv));

            return;
        }

        if(port == 0)
        {
            bsmac_hdl[port].peer_addr = ((ph->src_addr_H << 8) | ph->src_addr_L);

        }
        else
        {
            bsmac_hdl[port].peer_addr = bsmac_hdl[0].peer_addr + port * 10000;
        }

        //bsmac_hdl[port].peer_addr = ((ph->src_addr_H << 8) | ph->src_addr_L) + port * 10000;
        //pnwkhdr->src = bsmac_hdl[port].peer_addr;

        if(port == 0)
            sys_option.u32BsId = bsmac_hdl[port].peer_addr - 30000;
        DEBUG_LOG("bsmac get bs id %d\n", bsmac_hdl[port].peer_addr);
    }

    // check ready;
    bsmac_hdl[port].peer_rdy = BSMAC_GET_RDY(ph->frame_control);

    // check frame_cnt
    rx_fc = (ph->frame_count_H << 8) | ph->frame_count_L;

    if (frame_type != BSMAC_FRAME_TYPE_ACK)
    {
        bsmac_hdl[port].rx_frame_cnt = rx_fc;
    }

    switch (frame_type)
    {

    case (BSMAC_FRAME_TYPE_DATA):
    {
        /* 首先处理本地需要处理的消息，本地不处理的消息发送给analyser*/
        if(!bsmac_parse_local_protocol(p+sizeof(bsmac_header_t),port))
        {
	    /* if reveive data, send it to  analyser */
            pCenterHeader = (MSG_CENTER_HEADER_T* )(p+sizeof(bsmac_header_t));

            /* 去掉mac头之后，直接前跳一个MSG_CENTER_HEADER_T，省去拷贝*/
            pCenterHeader--;

            pCenterHeader->u32MsgId = bsmac_hdl[port].transmit_id;
            

            err = rt_mq_send(&msg_analyser_mq,(void*)pCenterHeader,MSG_HEADER_SIZE + len - 2);
#ifdef DEBUG_BSMAC
            if (err != RT_EOK)
            {
                memset(bsmac_err_buf, 0, sizeof(bsmac_err_buf));
                snprintf(bsmac_err_buf, sizeof(bsmac_err_buf),
                    "bsmac send to msg_analyser_mq ret %d", err);
                net_report_running_state_msg(ARM_ERROR, bsmac_err_buf,
                    sizeof(bsmac_err_buf));
            }
#endif
            if(err == -RT_EFULL)  // -2 remove crc
            {
                //ERROR_LOG("send to msg_analyser_mq full\n");
            }
            else if(err == -RT_ERROR)
            {
                //ERROR_LOG("send to msg_analyser_mq err\n");
            }
        }
		/* 如果有一个从机就将指示灯点亮 */
		if (rssi_modules_link_state)
		{
			light_up_rssi_modules_data_indicator();
			rssi_modules_link_state=0;
		}
		else
		{
			light_off_rssi_modules_data_indicator();
			rssi_modules_link_state=1;
		}

        break;
    }
    case (BSMAC_FRAME_TYPE_LIVE):
    {
        if(bsmac_hdl[port].use_ack)
        {
            // rt_thread_delay(1);
            bsmac_send_ack(port);
        }
        break;
    }
    case (BSMAC_FRAME_TYPE_ACK):
    {
        bsmac_hdl[port].link_up = RT_TRUE;
        bsmac_hdl[port].linkStatus_counter = BSMAC_LINK_TIMEOUT_COUNT;
        BSMAC_LINK_LED_ON();
        break;
    }
    }

}
rt_size_t bsmac_build_packet( unsigned char * pbuf,
                              const unsigned char * pdata, unsigned short len,
                              const unsigned char frame_type, rt_uint8_t port)
{
    unsigned short tx_len;
    unsigned short crc;
    bsmac_header_t *ph;

    // add mac header
    if (pbuf == NULL || len > BSMAC_MAX_TX_PAYLOAD_LEN )
    {
        RT_DEBUG_LOG(RT_DEBUG_BSMAC,("Build Failed pbuf %X len%d\n", pbuf, len));
        return 0;
    }

    ph = (bsmac_header_t *) pbuf;

    ph->preamble_H = BSMAC_PREAMBLE_H;
    ph->preamble_L = BSMAC_PREAMBLE_L;

    BSMAC_SET_DEVICETYPE(ph->frame_control, BSMAC_DEVICE_TYPE_BS_EP);    // I am FPGA
    BSMAC_SET_RDY(ph->frame_control, 1);           							// always ready
    BSMAC_SET_FRAMETYPE(ph->frame_control, frame_type);
    BSMAC_SET_PRIORITY(ph->frame_control, 1);

    if (frame_type == BSMAC_FRAME_TYPE_ACK) // for ack, use recieved frame_cnt
    {
        ph->frame_count_H = (bsmac_hdl[port].rx_frame_cnt & 0xff00) >> 8;
        ph->frame_count_L = bsmac_hdl[port].rx_frame_cnt  & 0xff;
    }
    else
    {
        ph->frame_count_H = ( bsmac_hdl[port].tx_frame_cnt & 0xff00) >> 8; // framecnt_h
        ph->frame_count_L =  bsmac_hdl[port].tx_frame_cnt& 0xff; // framecnt_l
        bsmac_hdl[port].tx_frame_cnt++;
    }

    ph->src_addr_H = (sys_option.u32BsId >> 8) & 0xff;
    ph->src_addr_L = (sys_option.u32BsId) & 0xff;            // source mac address
    ph->dst_addr_H = 0;                                                     // dst address is useless
    ph->dst_addr_L = 0;
    ph->reserverd = port;

    /* ack do not need payload, Live may have payload */
    if (len != 0 && pdata && frame_type != BSMAC_FRAME_TYPE_ACK)
    {
        memcpy((void*) (pbuf + BSMAC_HEADER_LEN), pdata, len);
    }

    //LIVE packet needs to be a long frame
    if (frame_type == BSMAC_FRAME_TYPE_LIVE)
    {
        len = BSMAC_MAX_TX_PAYLOAD_LEN;
    }
    else if(frame_type == BSMAC_FRAME_TYPE_ACK)
    {
        len = 0;
    }

    tx_len = len + BSMAC_FOOTER_LEN; // length = payload+footer
    ph->data_len_H = (tx_len >> 8) & 0xff; //
    ph->data_len_L = tx_len & 0xff; //

    crc = CRC16((unsigned char *)(pbuf+2), len+BSMAC_HEADER_LEN-2, 0xffff);   // caculate header and payload

    // padding footer
    pbuf[len+BSMAC_HEADER_LEN] = (crc >> 8) & 0xff;
    pbuf[len+BSMAC_HEADER_LEN+1] = crc & 0xff;

    return sizeof(bsmac_header_t) + tx_len;
}

rt_uint8_t bsmac_search_phone_nwk(rt_uint16_t addr)
{
    static rt_uint8_t idx;
    if(addr == 0 || addr == 0xFFFF)
    {
        return (idx + 10);
    }
    for(idx=0;idx<10;idx++)
    {
        if(addr == bsmac_phone_info[idx].phone_addr)
        {
            return idx;
        }
    }
    return (idx + 10);
}

rt_uint8_t bsmac_search_idle_phone_nwk(rt_uint16_t addr)
{
    static rt_uint8_t idx,idle_idx = 20;

    if(addr == 0 || addr == 0xFFFF)
    {
        return (idle_idx);
    }
    for(idx=0;idx<10;idx++)
    {
        if(addr == bsmac_phone_info[idx].phone_addr)
        {
            return idx;
        }
        else if(bsmac_phone_info[idx].phone_addr == 0xFFFF)
        {
            idle_idx = idx;
        }
    }
    //if(idle_idx < idx)
    return idle_idx;

    //return (idx + 10);
}


static void bsmac_live_poll(void)
{
    rt_tick_t tick, diff_tick;
    static rt_tick_t last_tick;
    //static rt_uint8_t cnt;
    //static rt_uint8_t i;
    tick = rt_tick_get();
    diff_tick = tick  - last_tick;

    if(diff_tick > BSMAC_SEND_LIVE_TIMEOUT/(1000/RT_TICK_PER_SECOND))
    {
	
	bsmac_recv1_data_cnt++;
	bsmac_recv2_data_cnt++;
	if((bsmac_recv1_data_cnt > 10) ||( bsmac_recv2_data_cnt > 10))
	{
		//GPIO_ResetBits(GPIOE,GPIO_Pin_5);
		rt_kprintf("bsmac_live_poll %d %d\n",bsmac_recv1_data_cnt, bsmac_recv2_data_cnt);
		bsmac_recv1_data_cnt = 0;
		bsmac_recv2_data_cnt = 0;
	}
	else
	{
		if(GPIO_ReadOutputDataBit(GPIOE,GPIO_Pin_5) == 0)
		{

			//GPIO_SetBits(GPIOE,GPIO_Pin_5);
		}
	}
	last_tick = tick;

	//cnt++;
        /*
        if(cnt % 3 == 0)
            bsmac_send_live(0);

        else if(cnt % 3 == 1)
            bsmac_send_live(1);

        else
            bsmac_send_live(2);

        last_tick = tick;
        for(i=0;i<10;i++)
        {
            if(bsmac_phone_info[i].phone_addr != 0xFFFF)
            {
                if((tick - bsmac_phone_info[i].tick) > (BSMAC_POLL_PHONE_TIMEOUT/(1000/RT_TICK_PER_SECOND)))
                {
                    //rt_kprintf("time %u\n",bsmac_phone_info[i].phone_addr);
                    bsmac_phone_info[i].phone_addr = 0xFFFF;
                    bsmac_phone_info[i].info_ind = RT_FALSE;

                }
            }
        }*/
    }
    return;
}


static void bsmac_data_req_poll(void)
{
    rt_tick_t tick, diff_tick;
    //static rt_tick_t last_tick;
    //static rt_uint8_t cnt;
    //static rt_uint8_t i;
    tick = rt_tick_get();
    diff_tick = tick  - last_data_recv_tick;

    if(diff_tick > BSMAC_SEND_DATA_TIMEOUT/(1000/RT_TICK_PER_SECOND))
    {			
		static rt_uint8_t DataReqBuf[128];

	    struct nwkhdr *pNwkHdr = (struct nwkhdr *)DataReqBuf;
	    app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);
		pNwkHdr->dst = 0xFFFF;
        pNwkHdr->len = sizeof(app_header_t);
        pNwkHdr->src = sys_option.u32BsId;
        pNwkHdr->ttl = 1;
        pNwkHdr->type = NWK_DATA;

        pHeader->len = 0;
        pHeader->msgtype = APP_UWB_MSG_REQ_LOC_DISTANCE;
        pHeader->protocoltype = APP_PROTOCOL_TYPE_UWB_CARD;
	    bsmac_send_packet(DataReqBuf, sizeof(struct nwkhdr) + pNwkHdr->len,0);
		last_data_recv_tick = tick;
    }
    return;
}


rt_bool_t bsmac_parse_local_protocol(rt_uint8_t *p,rt_uint8_t port)
{
    struct nwkhdr *pNwkHdr;
    app_header_t  *pAppHdr;

    rt_bool_t parsed;
    //rt_uint8_t idx;
	//rt_uint8_t i,j;

    static rt_uint8_t pAckBuf[128];

    struct nwkhdr *pAckNwkHdr = (struct nwkhdr *)pAckBuf;
    app_header_t *pAckHeader = (app_header_t *)(pAckNwkHdr + 1);

    parsed = RT_FALSE;

    pNwkHdr = (struct nwkhdr *)p;
    pAppHdr = (app_header_t  *)(pNwkHdr+1);

    switch(pAppHdr->msgtype)
    {
       #if 0
        case APP_UWB_MSG_COORDINATE:
        {
          APP_UWB_TOF_TAG_COORDINATE_S *pstBsTagCoordMsg =  (APP_UWB_TOF_TAG_COORDINATE_S *)(pAppHdr + 1);

         // rt_kprintf("u16BsTagDevId:%d prtype: %d mtype: %d\n",pstBsTagCoordMsg->u16BsTagDevId, pAppHdr->protocoltype, pAppHdr->msgtype);
         // PrintFloat("x_case", pstBsTagCoordMsg->stTagCoord.f32TagCoordinateX);
         // PrintFloat("y_case", pstBsTagCoordMsg->stTagCoord.f32TagCoordinateY);
         // PrintFloat("z_case", pstBsTagCoordMsg->stTagCoord.f32TagCoordinateZ);
        }
        break;
     #endif
     
        case APP_UWB_MSG_REPORT:
        {
            app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pAppHdr + 1);

            if(pStationReport->reporttype == APP_LS_REPORT_LIVE)
            {
                app_LSrfReport_t *pAckStationReport = (app_LSrfReport_t *)(pAckHeader + 1);

                pAckNwkHdr->type = NWK_DATA;
                pAckNwkHdr->ttl = 1;
                pAckNwkHdr->src = sys_option.u32BsId;
                pAckNwkHdr->dst = pNwkHdr->src;

                pAckNwkHdr->len =  sizeof(app_header_t) + sizeof(app_LSrfReport_t);

                pAckHeader->len = sizeof(app_LSrfReport_t);
                pAckHeader->msgtype = APP_TOF_MSG_REPORT_ACK;
                //if(port)
                    pAckHeader->protocoltype = APP_PROTOCOL_TYPE_UWB_CARD;
                //else
                   // pAckHeader->protocoltype = APP_PROTOCOL_TYPE_CARD;

                pAckStationReport->hdr.dstaddr =  pStationReport->hdr.srcaddr;
                pAckStationReport->hdr.srcaddr = sys_option.u32BsId;
                pAckStationReport->len = 0;

                pAckStationReport->reporttype = APP_LS_REPORT_LIVE;
                pAckStationReport->devtype = BSMAC_DEVICE_TYPE_LOC;
                pAckStationReport->seqnum = pStationReport->seqnum;

                bsmac_send_packet(pAckBuf, sizeof(struct nwkhdr) + pAckNwkHdr->len,port);

                //rt_mq_send(&bsmac_mq, pAckBuf,
                           //sizeof(struct nwkhdr)+sizeof(app_header_t)+sizeof(app_LSrfReport_t));

                parsed = RT_TRUE;
            }
            break;
        }

        case APP_UWB_MSG_RSSI:
        {

             if(pAppHdr->protocoltype == APP_PROTOCOL_TYPE_UWB_CARD)
             {
    			bsmac_recv1_data_cnt = 0;
    			bsmac_recv2_data_cnt = 0;
             }
             parsed = RT_FALSE;
             break;
        }

#if 1
        case APP_UWB_MSG_DISTANCE:     //收到主站定位数据时，向副站要求定位数据
        {
    	/*app_uwb_tof_distance_ts * app_uwb_tof_distance;

    	app_uwb_tof_distance = (app_uwb_tof_distance_ts  *)(pNwkHdr+1);

    	 i = app_uwb_tof_distance->app_tof_head.len / (sizeof(uwb_tof_distance_ts));

    	 rt_kprintf("station:%d\n",pNwkHdr->src);

    	 

    	 for(j=0;j<i;j++)
                {
                    
                        rt_kprintf("card :%d,distance %d\n",app_uwb_tof_distance->tof_distance[j].u16ShortAddr,app_uwb_tof_distance->tof_distance[j].u32StationDistance);
                }*/

    	if(port == 1)
    	{
    	    //app_mpPoll_t *pMppoll = (app_mpPoll_t *)(pAppHdr + 1);

    	    //struct nwkhdr *pBufNwkHdr = NULL;

    	    //app_header_t *pAckHeader = (app_header_t *)(pAckNwkHdr + 1);
    	    //app_mpPoll_t *pAckMpPoll = (app_mpPoll_t *)(pAckHeader + 1);
    	    //rt_kprintf("Poll\n");
    	    if(pAppHdr->protocoltype == APP_PROTOCOL_TYPE_UWB_CARD)
    	    {
    	        //rt_kprintf("Poll %u\n",pMppoll->hdr.srcaddr);
    	        last_data_recv_tick = rt_tick_get();			
    	        pAckNwkHdr->dst = pNwkHdr->src+10000;
    	        pAckNwkHdr->len = sizeof(app_header_t);
    	        pAckNwkHdr->src = sys_option.u32BsId;
    	        pAckNwkHdr->ttl = 1;
    	        pAckNwkHdr->type = NWK_DATA;

    	        pAckHeader->len = 0;
    	        pAckHeader->msgtype = APP_UWB_MSG_REQ_LOC_DISTANCE;
    	        pAckHeader->protocoltype = APP_PROTOCOL_TYPE_UWB_CARD;
    	        bsmac_send_packet(pAckBuf, sizeof(struct nwkhdr) + pAckNwkHdr->len,0);
    			bsmac_recv1_data_cnt = 0;
    	    }
    		if(pAppHdr->len == 0)	
    		{
    		    parsed = RT_TRUE;
    		}
    	}
    	else
    	{
    		bsmac_recv2_data_cnt = 0;
    	}
            break;
        }
#endif
        /*case MP_JOIN_NOTIFY:
        {
            if(port != 0)
            {
                app_mpJoinNwk_t *pMpJoinNwk = (app_mpJoinNwk_t *)(pAppHdr + 1);
                app_mpJoinNwk_t *pAckMpJoinNwk = (app_mpJoinNwk_t *)(pAckHeader + 1);
                //rt_kprintf("Join %d\n",pMpJoinNwk->hdr.srcaddr);
                if(pAppHdr->protocoltype == APP_PROTOCOL_TYPE_MOBILE && pMpJoinNwk->joinnwktype == APP_MP_JOINNWK_REQ)
                {
                    idx = bsmac_search_idle_phone_nwk(pMpJoinNwk->hdr.srcaddr);
                    //rt_kprintf("req %u\n",pMpJoinNwk->hdr.srcaddr);
                    if(idx < 10)
                    {
                        //rt_kprintf("req\n");
                        pAckNwkHdr->dst = pNwkHdr->src;
                        pAckNwkHdr->len = sizeof(app_header_t) + sizeof(app_mpJoinNwk_t);
                        pAckNwkHdr->src = sys_option.u32BsId;
                        pAckNwkHdr->ttl = 1;
                        pAckNwkHdr->type = NWK_DATA;

                        pAckHeader->len = sizeof(app_mpJoinNwk_t);
                        pAckHeader->msgtype = MP_JOIN_NOTIFY;
                        pAckHeader->protocoltype = APP_PROTOCOL_TYPE_MOBILE;

                        pAckMpJoinNwk->armid = sys_option.u32BsId;
                        pAckMpJoinNwk->hdr.dstaddr = pMpJoinNwk->hdr.srcaddr;
                        pAckMpJoinNwk->hdr.srcaddr = pMpJoinNwk->hdr.dstaddr;
                        pAckMpJoinNwk->joinnwktype = APP_MP_JOINNWK_SUCCESS;
                        pAckMpJoinNwk->seqnum = pMpJoinNwk->seqnum;
                        bsmac_send_packet(pAckBuf, sizeof(struct nwkhdr) + pAckNwkHdr->len,port);

                        bsmac_phone_info[idx].phone_addr = pMpJoinNwk->hdr.srcaddr;
                        bsmac_phone_info[idx].tick = rt_tick_get();
                        bsmac_phone_info[idx].station_panid = pNwkHdr->src;
                    }
                    parsed = RT_TRUE;
                }
            }
            break;
        }*/
        /*case MP_LEAVE_NOTIFY:
        {
            if(port != 0)
            {
                app_mpLeaveNwk_t *pMpLeaveNwk = (app_mpLeaveNwk_t *)(pAppHdr + 1);
                //app_mpJoinNwk_t *pAckMpJoinNwk = (app_mpJoinNwk_t *)(pAckHeader + 1);
                if(pAppHdr->protocoltype == APP_PROTOCOL_TYPE_MOBILE)
                {
                    idx = bsmac_search_phone_nwk(pMpLeaveNwk->hdr.srcaddr);
                    if(idx < 10)
                    {
                        bsmac_phone_info[idx].phone_addr = 0xFFFF;
                        bsmac_phone_info[idx].info_ind = RT_FALSE;
                    }
                    parsed = RT_TRUE;
                }
            }
            break;
        }*/
    }
    return parsed;
}

/* this function runs in msg_analyser thread !!*/
static void _bsmac_analyser_callback(void *pvParam)
{
    const MSG_CENTER_HEADER_T  *pCenterHeader = (MSG_CENTER_HEADER_T *)pvParam;
    static rt_uint8_t              au8Buf[MSG_ANALYSER_PKT_SIZE];
    rt_err_t err = 0;

    if (pvParam != RT_NULL && bsmac_hdl[0].transmit_id == pCenterHeader->u32MsgId)
    {

        app_SMT32_Data now;
        rt_uint8_t *pu8Buf = au8Buf;
        PACKET_HEADER_T PacketTag;
        APP_HDR_T *pstAppHd = (APP_HDR_T*)PKT_GETAPPHDR((rt_uint8_t*)pvParam + MSG_HEADER_SIZE);
        NWK_HDR_T *pstNwkHd = (NWK_HDR_T*)((rt_uint8_t*)pvParam + MSG_HEADER_SIZE);

        // add PacketTag and app_SMT32_Data
        PacketTag.tag = COMM_3G_DATA;
        PacketTag.len = pstNwkHd->len + sizeof(NWK_HDR_T) + sizeof(app_SMT32_Data);
        
        if (PacketTag.len + sizeof(PACKET_HEADER_T) > MSG_NET_PKT_SIZE)
        {
            RT_DEBUG_LOG(RT_DEBUG_BSMAC, ("COM1 send's packet too long\n"));
            return;
        }
        

        rt_memcpy(pu8Buf, &PacketTag, sizeof(PACKET_HEADER_T));
        pu8Buf += sizeof(PACKET_HEADER_T);

        // copy nwkhdr
        pstNwkHd->len += sizeof(app_SMT32_Data);
        pstNwkHd->dst = 0;
        rt_memcpy(pu8Buf, pstNwkHd, sizeof(NWK_HDR_T));
        pu8Buf += sizeof(NWK_HDR_T);

        // add time stamp (app_SMT32_Data)
        now.panid = sys_option.u32BsId;
        now.moduleid = pstNwkHd->src;
        now.timestamp = time(RT_NULL);
        rt_memcpy(pu8Buf, &now, sizeof(app_SMT32_Data));
        pu8Buf += sizeof(app_SMT32_Data);
        
        // copy apphdr + payload
        rt_memcpy(pu8Buf, pstAppHd, pstAppHd->len + sizeof(APP_HDR_T));
        err = rt_mq_send(&net_mq, au8Buf, PacketTag.len + sizeof(PACKET_HEADER_T));
        if(err == -RT_EFULL)
        {
            ERROR_LOG("send to net_mq full\n");
        }
        else if(err == -RT_ERROR)
        {
            ERROR_LOG("send to net_mq err\n");
        }
    }
}

static rt_thread_t bsmac_thread = RT_NULL;

rt_bool_t start_bsmac_work()
{
    bsmac_thread = rt_thread_create("bsmac", bsmac_thread_entry,
                                    RT_NULL, 2048, 9, 20);

    if (bsmac_thread == RT_NULL)
    {
        ERROR_LOG("create bsmac work thread failed\n");
        return RT_FALSE;
    }

    rt_thread_startup(bsmac_thread);
    DEBUG_LOG("the bsmac thread start up\n");

    return RT_TRUE;
}

void stop_bsmac_work()
{
    rt_enter_critical();

    if (bsmac_thread != RT_NULL && bsmac_thread->stat != RT_THREAD_CLOSE)
    {
        rt_thread_delete(bsmac_thread);
    }

    rt_exit_critical();

    TIME_LOG(LOG_CRITICAL, "stop bsmac thread\n");
}

