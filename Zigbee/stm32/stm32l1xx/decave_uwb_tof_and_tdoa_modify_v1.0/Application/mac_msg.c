#include <stdio.h>
#include <string.h>
#include "cc_def.h"
#include "CommonTypes.h"
#include "mac_msg.h"
#include "mem.h"
#include "debug.h"
//#include "ntrxdrv.h"
#include "timer_event.h"

static MacMsg_t   *mac_msg_list[MAC_MSG_BUF_MAX];
static uint_8       free_node = 0;

/*
* 收取一条消息给外部应用程序
*/
MacMsg_t *mac_msg_recv(uint_8 msg)
{
    uint_8 idx;

    for (idx = 0; idx < MAC_MSG_BUF_MAX; idx++)
    {
        if (mac_msg_list[idx] != NULL && msg == mac_msg_list[idx]->msgType)
        {
            return mac_msg_list[idx];
        }
    }

    return NULL;
}

/*
* 释放一个消息记录位置点
*/
void mac_msg_remove(MacMsg_t *pMsg)
{
    if (pMsg != NULL && pMsg->msgBufIdx < MAC_MSG_BUF_MAX)
    {
        mac_msg_list[pMsg->msgBufIdx] = NULL;
        rt_free(pMsg);
    }
}

/*
* 插入一个消息，返回记录位置点
*/
uint_8 mac_msg_insert(MacMsg_t *pMsg)
{
    free_node &= MAC_MSG_BUF_MAX - 1;   // 0 - (MAC_MSG_BUF_MAX-1)

    if (mac_msg_list[free_node] != NULL)
    {
        mac_msg_remove(mac_msg_list[free_node]);
        MAC_DBG("lose msg\n");
    }

    mac_msg_list[free_node] = pMsg;

    return free_node++;
}

//=============================================================================
// 应用程序的信息包接收回调函数
/*                 NtrxBufferPtr Payload, uint_8 Len, uint_8 Rssi)
{
    MacMsg_t *pMsg;

    if ((MsgStatus & NtrxRxPacketTypeMask) == PacketTypeRanging)
    {
        return;
    }
    // 特殊信息包处理
    MsgStatus &= NtrxRxPacketTypeMask;

    pMsg = (MacMsg_t *)rt_malloc(sizeof(MacMsg_t) + Len);

    if (pMsg != NULL)
    {
        pMsg->msgType = MAC_MSG_DATA;
        pMsg->msgStatus = MsgStatus;
        pMsg->msgBufIdx = mac_msg_insert(pMsg);
        pMsg->rssi = Rssi;
        pMsg->timestamp = GetSysClock();
        memcpy(pMsg->srcAddr, SrcAddr, NTRX_DEV_ADDR_SIZE);
        memcpy(pMsg->dstAddr, DstAddr, NTRX_DEV_ADDR_SIZE);
        pMsg->len = Len;
        pMsg->payload = (uint_8 *)(pMsg + 1);
        memcpy(pMsg->payload, Payload, Len);
        event_timer_set(EVENT_RECV_DATA);
    }
    else
    {
        MAC_DBG("msg recv out of memory %s:%d\n", __FILE__, __LINE__);
    }
}*/

