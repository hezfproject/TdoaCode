#ifndef _PTL_NWK_H_
#define _PTL_NWK_H_

#include "nwk_protocol.h"   //和工控机交互的协议
#include "app_protocol.h"   //和Jennic交互的协议
#include "protocol_types.h" //协议类型

#define COMM_3G_DATA    4000

typedef struct nwkhdr NWK_HDR_T;

/* app  header */
typedef app_header_t APP_HDR_T;

// 从一个packet中获取到对应的目标头地址
#define PKT_GETAPPHDR(addr)     ((APP_HDR_T*)((char*)(addr) + sizeof(NWK_HDR_T)))

#define PKT_GETNWKHDR(addr)     ((NWK_HDR_T*)(addr))

#define PKT_GETAPPPLD(addr)     ((char*)(addr) + sizeof(NWK_HDR_T) + sizeof(APP_HDR_T))

#endif

