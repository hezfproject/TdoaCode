#ifndef __BSMAC_H__
#define __BSMAC_H__

/*******************************************************************************
** Macro define
*******************************************************************************/
// init return 
#define BSMAC_INIT_SUCCESS                  (0)
#define BSMAC_INIT_CB_INVALID               (1)
#define BSMAC_INIT_HARDWARE_ERR             (2)
#define BSMAC_INIT_TIMER_ERR                (3)


// write return
#define BSMAC_WRITE_SUCCESS                 (0)
#define BSMAC_WRITE_LENGTH_INVALID          (1)
#define BSMAC_WRITE_DMA_ERROR               (2)
#define BSMAC_WRITE_BUSY                    (3)
#define BSMAC_WRITE_PEER_NOTREADY   (4)


// control return
#define BSMAC_CONTROL_SUCCESS               (0)
#define BSMAC_CONTROL_BUF_INVALID           (1)
#define BSMAC_CONTROL_CTRLID_INVALID        (2)
#define BSMAC_CONTROL_LEN_INVALID           (3)

// ack switch
#define BSMAC_NOTUSE_ACK                    (0)
#define BSMAC_USE_ACK                       (1)

// restransmit switch
#define BSMAC_NOTUSE_RETRANS                (0)
#define BSMAC_USE_RETRANS                   (1)

// flow control switch
#define BSMAC_NOTUSE_FLOWCTL                (0)
#define BSMAC_USE_FLOWCTL                   (1)

// control id
#define BSMAC_GET_LINKSTA                   (0)
#define BSMAC_SET_USEACK                    (1)
#define BSMAC_SET_USERETRANS                (2)
#define BSMAC_SET_USEFLOWCTL                (3)


// link status
#define BSMAC_LINK_DOWN                     (0)
#define BSMAC_LINK_UP                       (1)

// control id
#define BSMAC_GET_LINKSTA                   (0)
#define BSMAC_SET_USEACK                    (1)
#define BSMAC_SET_USERETRANS                (2)
#define BSMAC_SET_USEFLOWCTL                (3)


/*******************************************************************************
** Structure define
*******************************************************************************/
typedef struct _TBsMacGetLinkSta {
    unsigned char sta;
}TBsMacGetLinkSta, *PBSMACGETLINKSTA;

typedef struct _TBsMacSetUseAck {
    unsigned char flag;
}TBsMacSetUseAck, *PBSMACSETUSEACK;

typedef struct _TBsMacSetUseRetrans {
    unsigned char flag;
}TBsMacSetUseRetrans, *PBSMACSETUSERETRANS;

typedef struct _TBsMacSetUseFlowCtl {
    unsigned char flag;
}TBsMacSetUseFlowCtl, *PBSMACSETUSEFLOWCTL;

/*******************************************************************************
** Description:
    call back function to handle received data
** Input param:
    pbuf    : pointer of receiving buf
    len     : received data length
*******************************************************************************/
typedef void (*bsmac_cb)(unsigned char *pbuf, unsigned char len);

/*******************************************************************************
** Description:
    initial mac layer
** Input param:
    bsmac_cb                : callback function pointer
    uart_ch                 : channel number of uart(0 or 1)
    dma_ch_tx               : channel number of tx dma(0 to 4)
    dma_ch_rx               : channel number of rx dma(0 to 4)
** Return value:
    0       : success
    ohters  : fail
*******************************************************************************/
unsigned char bsmac_init(const bsmac_cb cb_hdl, const unsigned char uart_port, const unsigned char dma_ch_tx, const unsigned char dma_ch_rx, 
                                           const unsigned short mac_addr );


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
unsigned char bsmac_write_data(const unsigned char *pbuf, const unsigned short len);

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
unsigned char bsmac_control(const unsigned char ctrl_id, const unsigned char *pbuf, const unsigned char *plen);

#endif  // __UART_DMA__

