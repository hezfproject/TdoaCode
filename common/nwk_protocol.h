#ifndef _NWK_PROTOCOL_H_
#define _NWK_PROTOCOL_H_

#include "CommonTypes.h"
#include "protocol_types.h"
#define NWK_TTL_INIT		255

#define MAX_PACKET_SIZE		1024

enum {
	NWK_ROUTE_REQ = 0,
	NWK_ROUTE_ACK,
	NWK_ROUTE_DEL,
	NWK_BCAST_DATA,
	NWK_BCAST_ACK,
	NWK_DATA,
	NWK_HELLO,
};

struct nwkhdr
{
	uint_8		type;
	uint_8		ttl;
	pan_addr_t	src;
	pan_addr_t 	dst;
	uint_16		len;
	/* The options start here */
};

struct tnwk_depth_weight
{
	uint_16		depth;
	uint_16		weight;
	uint_32		seqnum;
	/* npath should be an even number, followed by path */
};

struct tnwk_route_del
{
	pan_addr_t	target;
	uint_16		padding;
};

/***************************************************************************************/
/****                        STM32 add
****/

// IPC and STM32 transmit packet:
// PACKET_HEADER_T + nwkhdr + app_SMT32_Data + app_header_t + paylaod
typedef struct
{
	int_32	tag;                
	int_32	len;                
} PACKET_HEADER_T;

#define NET2STM32_PACKET_SIZE   128

#endif
