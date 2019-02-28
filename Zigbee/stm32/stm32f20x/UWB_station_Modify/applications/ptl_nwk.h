#ifndef _PTL_NWK_H_
#define _PTL_NWK_H_

#include "nwk_protocol.h"   //�͹��ػ�������Э��
#include "app_protocol.h"   //��Jennic������Э��
#include "protocol_types.h" //Э������

#define COMM_3G_DATA    4000

typedef struct nwkhdr NWK_HDR_T;

/* app  header */
typedef app_header_t APP_HDR_T;

// ��һ��packet�л�ȡ����Ӧ��Ŀ��ͷ��ַ
#define PKT_GETAPPHDR(addr)     ((APP_HDR_T*)((char*)(addr) + sizeof(NWK_HDR_T)))

#define PKT_GETNWKHDR(addr)     ((NWK_HDR_T*)(addr))

#define PKT_GETAPPPLD(addr)     ((char*)(addr) + sizeof(NWK_HDR_T) + sizeof(APP_HDR_T))

#endif

