#ifndef _MAC_MSG_H_
#define _MAC_MSG_H_

typedef struct
{
    uint_8                   msgType;
    uint_8                   msgStatus;
    uint_8                   msgBufIdx;
    uint_8                   rssi;
    uint_32                  timestamp;
    uint_16             srcAddr;
    uint_16             dstAddr;

    uint_8                   len;
    uint_8                   *payload;
} MacMsg_t;

#define MAC_MSG_BUF_MAX                 8
#define MAC_MSG_DATA                    1


/*
* 收取一条消息给外部应用程序
*/
MacMsg_t *mac_msg_recv(uint_8 msg);


/*
* 释放一个消息记录位置点
*/
void mac_msg_remove(MacMsg_t *pMsg);


#endif

