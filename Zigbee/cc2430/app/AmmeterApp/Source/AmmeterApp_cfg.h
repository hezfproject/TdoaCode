#ifndef _AMMETERAPP_CFG_H
#define _AMMETERAPP_CFG_H

#include "hal_types.h"

#ifndef AMMETER_ROUTE_EXPIRETIME
#define AMMETER_ROUTE_EXPIRETIME 	10000
#endif

#ifndef AMMETER_ROUTE_REBUILDINTERVAL
#define AMMETER_ROUTE_REBUILDINTERVAL 20000  //unit:ms
#endif

#ifndef AMMETER_ROUTE_EXPIRECNT
#define AMMETER_ROUTE_EXPIRECNT 	5
#endif

#ifndef MAXNEIGHBOR
#define MAXNEIGHBOR 			50
#endif

#ifndef AMMETER_ROUTE_ALIVETIME
#define AMMETER_ROUTE_ALIVETIME		5000
#endif

#ifndef AMMETER_ROUTE_TOLERANCECNT
#define AMMETER_ROUTE_TOLERANCECNT	5
#endif

#ifndef AMMETER_ROUTE_BUILDCNT
#define AMMETER_ROUTE_BUILDCNT	5
#endif

#define INVALIDLINKQUAL			-127

#ifndef	TOLERANCERSSI 
#define TOLERANCERSSI			-80
#endif

#if (defined DEBUG_REPORT_NW)  || (defined CFG_AMMETER_IEC) 
	#define FREQUENCY_MULTIPLICATION 5
#elif(defined CFG_AMMETER_MODBUS)
	#define FREQUENCY_MULTIPLICATION 2
#else
	#define FREQUENCY_MULTIPLICATION 1
#endif

//#define MAX_RADIUS				0xFF

#define AMMETERAPP_ENDPOINT           11

#define AMMETERAPP_PROFID             0x0F05
#define AMMETERAPP_DEVICEID           0x0001
#define AMMETERAPP_DEVICE_VERSION     0
#define AMMETERAPP_FLAGS              0

#define AMMETERAPP_CLUSTERID         		  1
#define AMMETERAPP_NWK_CLUSTERID         2

#define AMMETERAPP_MAX_DATA_LEN  120
#define DMA_MAX  128 //can save in nv to dynamic adjust when device wake up.
#define AMMETER_MAX_PKT_LEN          249 //From ammeter protocol.
#define AMMETER_MAX_UART_READCNT	33
#define AMMETER_UART_READINTERVAL	30
//Ammeter data protocol.
//idx of data.
#define AMMETER_ADDR_IDX               	  0
#define AMMETER_FUNCCODE_IDX               1
#define AMMETER_STARTADDR_IDX             2
#define AMMETER_REGCNT_IDX             	  4
#define AMMETER_CRC_IDX             	  	  6

#define AMMETER_BYTECNT_IDX             	  2

#define AMMETER_READREG 			0x03
#define AMMETER_WRITEREG 			0x10

#define HAL_LED_POWER	HAL_LED_2
#define HAL_LED_ROUTE	HAL_LED_3
#define HAL_LED_DATA	HAL_LED_1

typedef struct
{
	uint8 ammeterAddr;
	uint8 funcCode;
	uint16 startAddr;
	uint16 regCnt;
	uint16 CRCCheck;
} AmmeterQuery_t;

typedef enum
{
	ROUTEREQ = 1,
	ROUTERSP,
	ROUTEALIVE,
	ROUTECLEAN,
	ROUTEREPORT,
} PktType;

typedef struct
{
	uint8   nodeDepth;
	uint16 parentID;
	uint16 nodeID;
	uint16 sinkID;
	int8 parentQuality;
	bool needRepair;
	uint8 cleanDepth;
} NodeInfo;

typedef struct
{
	uint16  nextHopAddress;
	uint16  dstAddress;
	byte    expiryTime;
	//byte    status;
} RouteEntry_t;

typedef struct 
{
	PktType msgtype;
	uint8	nodeDepth;
	uint16	nodeID;
	uint16	seq;
	uint16 	srcAddress;
	uint16 	dstAddress;
//	uint8	radius;
} Nwk_RouteReq_t;

typedef struct 
{
	PktType msgtype;
	uint8	nodeDepth;
	uint16	nodeID;
	uint16	seq;
	uint16  srcAddress;
	uint16  dstAddress;
//	uint8	radius;
} Nwk_RouteRsp_t;

typedef  Nwk_RouteReq_t Nwk_RouteAlive_t;
typedef Nwk_RouteReq_t Nwk_RouteClean_t;

#endif

