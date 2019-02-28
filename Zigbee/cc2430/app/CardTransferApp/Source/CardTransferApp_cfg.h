#ifndef _CARDTRANSFERAPP_CFG_H
#define _CARDTRANSFERAPP_CFG_H

#include "hal_types.h"
#include "app_cfg.h"

#ifndef CARDTRANSFER_ROUTE_EXPIRETIME
#define CARDTRANSFER_ROUTE_EXPIRETIME 	10000
#endif

#ifndef CARDTRANSFER_ROUTE_REBUILDINTERVAL
#define CARDTRANSFER_ROUTE_REBUILDINTERVAL 20000  //unit:ms
#endif

#ifndef CARDTRANSFER_ROUTE_EXPIRECNT
#define CARDTRANSFER_ROUTE_EXPIRECNT 	5
#endif

#ifndef MAXNEIGHBOR
#define MAXNEIGHBOR 			50
#endif

#ifndef CARDTRANSFER_ROUTE_ALIVETIME
#define CARDTRANSFER_ROUTE_ALIVETIME		5000
#endif

#ifndef CARDTRANSFER_ROUTE_TOLERANCECNT
#define CARDTRANSFER_ROUTE_TOLERANCECNT	5
#endif

#ifndef CARDTRANSFER_ROUTE_BUILDCNT
#define CARDTRANSFER_ROUTE_BUILDCNT	5
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
	#define FREQUENCY_MULTIPLICATION 5
#endif

//#define MAX_RADIUS				0xFF

#define CARDTRANSFERAPP_ENDPOINT    MINEAPP_ENDPOINT

#define CARDTRANSFERAPP_PROFID       MINEAPP_PROFID
#define CARDTRANSFERAPP_DEVICEID           MINEAPP_DEVICEID
#define CARDTRANSFERAPP_DEVICE_VERSION     MINEAPP_VERSION
#define CARDTRANSFERAPP_FLAGS             MINEAPP_FLAGS

#if (defined CARDTRANSFER_COLLECTOR) || (defined CARDTRANSFER_DATACENTER)
#define CARDTRANSFERAPP_CLUSTERID     CARD_CLUSTERID
#define CARDTRANSFERAPP_NWK_CLUSTERID   NWK_CLUSTERID
#endif

#define CARDTRANSFERAPP_MAX_DATA_LEN  120
#ifndef DMA_MAX
#define DMA_MAX  128 //can save in nv to dynamic adjust when device wake up.
#endif

#define HAL_LED_POWER	HAL_LED_2
#define HAL_LED_ROUTE	HAL_LED_3
#define HAL_LED_DATA	HAL_LED_1

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

