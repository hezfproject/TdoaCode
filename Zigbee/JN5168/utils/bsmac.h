#ifndef BSMAC_H
#define BSMAC_H

#include <jendefs.h>
#include "bsmac_header.h"

/*******************************************************************************
* typedef
*******************************************************************************/
#define BSMAC_UART_BAUD_500k 1
#define BSMAC_UART_BAUD_100k 2
#define BSMAC_UART_BAUD_115200 3
#define BSMAC_UART_BAUD_460800 4


typedef enum
{
    JBSMAC_INIT_SUCCESS,
    JBSMAC_INIT_CB_INVALID,
    JBSMAC_INIT_PORT_INVALID,
    JBSMAC_INIT_DEVICE_INVALID,
    JBSMAC_WRITE_SUCCESS,
    JBSMAC_WRITE_LENGTH_INVALID,
    JBSMAC_WRITE_DMA_ERROR,
    JBSMAC_WRITE_BUSY,

    JBSMAC_SEND_PENDDING,
    JBSMAC_SEND_FIN,

    JBSMAC_QUEUE_SENDING,
    JBSMAC_QUEUE_NO_SENDING,

}eJbsmacRetVal;

typedef void (*PR_BSMAC_RX_CALLBACK)(unsigned char *pbuf, unsigned char len);


/*******************************************************************************
* function declaration
*******************************************************************************/
PUBLIC eJbsmacRetVal Jbsmac_u8Init(const PR_BSMAC_RX_CALLBACK cb_hdl, uint8 port, const uint8 uart_baud, const unsigned short mac_addr, const uint8 device_type);
PUBLIC eJbsmacRetVal Jbsmac_u8WarmStartInit(const PR_BSMAC_RX_CALLBACK cb_hdl, uint8 port, const uint8 uart_baud, const unsigned short mac_addr, const uint8 device_type);
PUBLIC eJbsmacRetVal Jbsmac_eWriteData(uint8 *pbuf, uint8 len);
PUBLIC int8 Jbsmac_i8GetUpPort(void);
PUBLIC uint8 Jbsmac_u8GetLinkStatus(uint8 port);
PUBLIC uint16 Jbsmac_u16GetPeerAddr(uint8 port);
PUBLIC void Jbsmac_vGetErrCnt(uint8 port, uint16* ptotalcnt, uint16* perrcnt);
PUBLIC uint16 Jbsmac_u16GetArmid(void);
PUBLIC void Jbsmac_vPoll();
PUBLIC bool Jbsmac_bIsArmNeighbor();


#endif
