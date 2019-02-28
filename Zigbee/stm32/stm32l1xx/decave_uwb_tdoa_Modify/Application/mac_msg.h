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
* ��ȡһ����Ϣ���ⲿӦ�ó���
*/
MacMsg_t *mac_msg_recv(uint_8 msg);


/*
* �ͷ�һ����Ϣ��¼λ�õ�
*/
void mac_msg_remove(MacMsg_t *pMsg);


#endif

