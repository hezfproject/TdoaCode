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
* ��ȡһ����Ϣ���ⲿӦ�ó���
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
* �ͷ�һ����Ϣ��¼λ�õ�
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
* ����һ����Ϣ�����ؼ�¼λ�õ�
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
// Ӧ�ó������Ϣ�����ջص�����
