/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#define __MAP_APP_VARS__

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "KeyScan.h"
#include "App.h"
#include "CPU.h"
#include "Console.h"
#include "NtrxDrv.h"
#include "OLED.h"
#include "LED.h"
#include "BoardConfig.h"
#include "Utility.h"
#include "HAL.h"
#include "Protocol.h"
#include "hal_uart.h"

#include "OSAL.h"
#include "OSAL_PwrMgr.h"

//#ifdef NT_STATION
//	#define STATION 1
//#else
//	#define STATION 0
//#endif

//#ifdef NT_ANCHOR
	#define ANCHOR 1
//#else
//	#define ANCHOR 0
//#endif

//#ifdef NT_TAG
//	#define TAG 1
//#else
	#define TAG 0
//#endif

//#ifdef NT_SNIFFER
//	#define SNIFFER 1
//#else
	#define SNIFFER 0
//#endif

//#ifdef NT_REFTAG
//	#define REFTAG 1
//#else
//	#define REFTAG 0
//#endif

//#if (STATION + ANCHOR + TAG + SNIFFER + REFTAG != 1)
//	//#error "device macro define error"
//#endif

//#if SNIFFER
//uint8 sniffer_output_mode = 0;
//#endif

#define ENABLE_DEBUG_PRINT	1

#ifdef ENABLE_DEBUG_SLEEP
uint32 sleeptime = 0;
#endif

//#if (!SNIFFER)
static uint8 app_dsn = 0;
//#endif

//#if ANCHOR
static uint8 anchor_bsn = 0;
//#endif

#define MSG_EVENT_MAC_DATA				1
typedef struct
{
	osal_event_hdr_t		hdr;
	uint32					timestamp;
	NtrxDevAddr				srcAddr;
	NtrxDevAddr				dstAddr;
	uint8					msgStatus;
	uint8					dstState;
	uint8					rssi;
	uint8					len;
	uint8					payload[];
} MacMsg_t;

static uint8 App_TaskId;
static uint8 uartbuf[128];


#define APP_EVENT_TAG_RANGING						0x0001
#define APP_EVENT_ANCHOR_RANGING					0x0002
#define APP_EVENT_SEARCH_NEIGHBOR					0x0004
#define APP_EVENT_UPDATE_DEPTH						0x0008
#define APP_EVENT_TICK								0x0010
#define APP_EVENT_PROCESS_TIMEOUT					0x0020
#define APP_EVENT_TAG_CFG_POLL						0x0040
#define APP_EVENT_CONFIG							0x0080

#define APP_EVENT_DEBUG								0x4000

enum
{
	DEV_TAG,
	DEV_ANCHOR,
	DEV_STATION,
};

typedef struct
{
	uint8		dsn;
	uint8		occupied;
	uint16		recvtime;
} BTT_Entry_t;
#define MAX_BTT_ENTRY_NUM	8
#define MAX_BTT_ENTRY_OCCUPIED_TIME			1000

typedef struct
{
	uint16			tagaddr;
	uint16			recvtime;
	cfg_info_t		info;
} tag_cfg_info_t;
#define TAG_CFG_ENTRY_TIMEOUT	1000
#define MAX_TAG_CFG_ENTRY_NUM	8
#define TAG_WAIT_CFG_TIMEOUT	40
#define TAG_SEARCH_NEIGHBOR_TIMEOUT		40

#define MAX_RANGING_FAIL_COUNT	5
typedef struct
{
	uint16			address;
	uint8			rssi:6;
	uint8			pathok:1;
	uint8			dummy:1;
	uint8			failcount;
}neighbor_info_t;

#define ANCHOR_NUM			8
#define ANCHOR_ADDR_MIN		1


#define MIN_NEIGHBORS_NUM	1 //3
#define MAX_NEIGHBORS_NUM	8
typedef struct  //处理动态网络，组网的地址信息
{
	uint8			type;
	uint16			address;
	uint16			parent;
	uint8			cost;
	uint16			accucost;
	uint8			depth;
	uint16			path2root[NT_MAX_DEPTH];

//	#if (ANCHOR || TAG)
	neighbor_info_t neighbors[MAX_NEIGHBORS_NUM];
//	BTT_Entry_t		btt[MAX_BTT_ENTRY_NUM];
//	#endif

	uint8			ticks;
	uint16			speriod;
	uint16			dperiod;

//	#if ANCHOR
	tag_cfg_info_t	tagcfginfos[MAX_TAG_CFG_ENTRY_NUM];
//	#endif

//	#if TAG
//	uint8			cfgpollaction;
//	uint8			ChkCfgPoll;
//	uint16			CfgPollStart;
//
//	uint8			ChkSrchNbr;
//	uint16			SrchNbrStart;
//	#endif

//	#if TAG && NT_DEMO_2
//	uint8			anchorfail[ANCHOR_NUM];
//	#endif

//	#if ((STATION || TAG) && NT_DEMO_1)
//	uint16			anchorlist[ANCHOR_NUM];
//	uint8			anchornum;
//	uint8			tagrate;			//2^tagrate ms
//	#endif
} NT_dev_info_t;
static NT_dev_info_t devInfo;

#define EEPROM_ADDRESS_L		1
#define EEPROM_ADDRESS_H		2

#define RANGING_PERIOD          200				// 测距时间间隔
#define RANGING_DELAY           1000			// 设为Tag模式后延时启动
#define SEARCH_NEIGHBOR_DURATION	80

//#if TAG
//	#define APP_TICK_INTERVAL			5000u
//#else
	#define APP_TICK_INTERVAL			1000u
//#endif

#define UPDATE_DEPTH_PERIOD				60   //60 ticks
#define SEND_BEACON_PERIOD              2    //2 ticks

#define STATION_CONFIG_PERIOD			(1000u * 5)	//5 seconds
#define TAG_CONFIG_PERIOD				(1000u * 60)	//60 seconds
#define TAG_CONFIG_DURATION				(50u)	//50 ms
#define TAG_CONFIG_FAIL_MAX				5	//5 TAG_CONFIG_PERIODs

#define RXID_ALWAYS_ON			0
#define RXID_SEARCH_NEIGHBOR	1
#define RXID_CFG_POLL			2
#define RXID_CONFIG				3

//=============================================================================
#define ADDRESS_L 0
#define ADDRESS_H 1

#define CON_INPUT_LINE_SIZE     32				// 串口输入读取缓冲区大小
//#define RANGING_PERIOD          100				// 测距时间间隔
#define WAIT_ACK_TIME           330				// 发出广播信息包后等待应答的时间

NtrxDevAddr ThisDev = { 0,0,0,0,0,0 } ;			// 本地地址
NtrxDevAddr DestDev = { 0,0,0,0,0,0 } ;			// 互连目标的地址
NtrxBufferType AppKeyword[] = "WNanoDrvDemo" ;	// 建立连接的口令

//static NtrxBufferType ConInputBuffer[CON_INPUT_LINE_SIZE] ;

//static uint8 EchoPrompt ;						// 监控端口需要输出提示符的标志变量
static uint8 LedStatus ;						// 发送信息包的结果,用于LED显示
	#define LED_WAIT_CMD     0
	#define LED_SEND_MSG_OK  1
	#define LED_MSG_NO_ACK   2

//static uint8 AppMode ;
	#define APP_IDLE_MODE    0
	#define APP_RANGING_MODE 1
	#define APP_SEARCH_MODE  0x80

//static uint8 RangingErrorCount = 0 ;

void NtrxWriteReg( uint8 Addr, uint8 Value ) ;
uint8 NtrxGetShadowReg( uint8 Addr ) ;

// 距离数据保存,用于平均值计算
#define RECORD_VALID_TIME	1000				// 最长时间1秒平均值
#define RECORD_BUFFER_SIZE	10					// 最多10次平均值
//static float RecordDist[RECORD_BUFFER_SIZE];
//static uint16 RecordTime[RECORD_BUFFER_SIZE] ;
//static uint8 RecordIndex = 0 ;

//static uint8 FactoryMode = 0 ;					// 模块生产测试模式

//=============================================================================
// OLED 显示

#ifdef ENABLE_OLED

//	static uint8 OLED_SearchCount = 0 ;
	static uint8 OLED_RTC_Timer = 0 ;

//	static void OLED_StartSearch( void )
//	{
//		OLED_PutString( 0, 0, CSTR( "-       " ) ) ;
//		OLED_SearchCount = 0 ;
//	}

//	static void OLED_SearchProcess( void )
//	{
//		uint8 c ;
//		OLED_SearchCount = ( OLED_SearchCount + 1 ) & 3 ;
//		if( OLED_SearchCount == 0 )
//			c = '-' ;
//		else if( OLED_SearchCount == 1 )
//			c = '\\' ;
//		else if( OLED_SearchCount == 2 )
//			c = '|' ;
//		else
//			c = '/' ;
//		OLED_PutChar( 0, 0, c ) ;
//	}
//
//	static void OLED_DisplayHelpMsg( void )
//	{
//		OLED_PutString( 0, 0, CSTR( "        " ) ) ;
//		OLED_PutString( 0, 1, CSTR( "K1 TimeSync\nK2 PowerSave\nK3 Ranging\n" ) ) ;
//	}
//
//	static void OLED_DisplayStopMsg( void )
//	{
//		OLED_PutString( 0, 1, CSTR( "\n\nAny Key To Stop\n" ) ) ;
//	}
//
//	static void OLED_DisplayRTC( void )
//	{
//		NtrxBufferType Rtc[6] ;
//		uint16 t ;
//		uint8 p ;
//
//		if( FactoryMode )
//		{
//			OLED_PutChar( 11, 0, 'F' ) ;
//			OLED_PutChar( 12, 0, 'M' ) ;
//			OLED_PutChar( 13, 0, ':' ) ;
//			p = NtrxGetShadowReg( 0x44 ) & 0x3F ;   //寄存器低六位
//			OLED_PutChar( 14, 0, p / 10 + 0x30 ) ;
//			p %= 10 ;
//			OLED_PutChar( 15, 0, p + 0x30 ) ;
//			OLED_RTC_Timer = 0xFF ;
//			return ;
//		}
//
//		NtrxGetRTC( Rtc ) ;
//
//		// 计算下次显示要经过的时间
//		t = 128 - ( Rtc[1] & 0x7F ) ;
//		OLED_RTC_Timer = t * 125 / 160 + 2 ;
//
//		// 显示时间值:分与秒
//		t = ((uint16)Rtc[3]<<8) | Rtc[2] ;
//		t <<= 1 ;
//		if( Rtc[1] & 0x80 )
//			t ++ ;
//		t %= 3600 ;
//		OLED_PutChar( 11, 0, t / 600 + 0x30 ) ;
//		t %= 600 ;
//		OLED_PutChar( 12, 0, t / 60 + 0x30 ) ;
//		OLED_PutChar( 13, 0, ':' ) ;
//		t %= 60 ;
//		OLED_PutChar( 14, 0, t / 10 + 0x30 ) ;
//		t %= 10 ;
//		OLED_PutChar( 15, 0, t + 0x30 ) ;
//	}
//
#endif

//=============================================================================
// 输出所有支持的监控命令

//static void EchoHelpMessage( void )
//{
//	con_PutString( CSTR( "F - Search destination\n" ) ) ;
//	con_PutString( CSTR( "C - Send message   : C Message\n" ) ) ;
//	con_PutString( CSTR( "S - Enter power save mode\n" ) ) ;
//	con_PutString( CSTR( "T - Time synchronous\n" ) ) ;
//	con_PutString( CSTR( "D - Start Ranging\n" ) ) ;
//	con_PutString( CSTR( "L - List all registers\n" ) ) ;
//	con_PutString( CSTR( "R - Read register  : R Address\n" ) ) ;
//	con_PutString( CSTR( "W - Write register : W Address Data\n" ) ) ;
//	EchoPrompt = 1 ;
//}

//=============================================================================
// 输出地址信息

//static void EchoDeviceAddress( NtrxDevPtr DevPtr )
//{
//	uint8 i ;
//	for( i = 0 ; i < NTRX_DEV_ADDR_SIZE ; i ++ )
//	{
//		con_PutHexNum( DevPtr[i] ) ;
//		if( i != NTRX_DEV_ADDR_SIZE - 1 )
//			con_putchar( '-' ) ;
//	}
//}

//=============================================================================
// 给本身设置一个随机的地址

//static void SetMyAddress( void )	
//{
//	while( ThisDev[0] == 0 )
//		NtrxGetRTC( ThisDev ) ;
//	con_PutString( CSTR("Set my address : ") ) ;
//	EchoDeviceAddress( ThisDev ) ;
//	con_PutReturn() ;
//	NtrxSetStaAddress( ThisDev ) ;		// Write the source address to the TRX chip
//}

//=============================================================================
// 输出当前芯片的RTC时间信息用于对比

//static void EchoRtcTime( void )
//{
//	uint8 i ;
//	NtrxBufferType Rtc[6] ;
//	NtrxGetRTC( Rtc ) ;
//	con_PutString( CSTR( "RTC=" ) ) ;
//	for( i = 0 ; i < 6 ; i ++ )
//	{
//		con_PutSpace() ;
//		con_PutHexNum( Rtc[5-i] ) ;
//	}
//	con_PutReturn() ;
//}

//=============================================================================
// 发出时间同步信息包,
//
//static void TimeSynchronous( void )
//{
//	NtrxSetRxMode( NtrxGetRxMode() | ModeRxTimeBAuto ) ;
//	NtrxSendMessage( PacketTypeTimeB, DestDev, AppKeyword, sizeof(AppKeyword) ) ;
//	EchoRtcTime() ;
//	LedStatus = LED_MSG_NO_ACK ;
//}

//function declaration
Bool AppSendMsg(NT_frm_hdr_t *hdr);
void AppProcessComingData(MacMsg_t *pMsg);
void AnchorRelayData(MacMsg_t *pMsg);
void AppProcessUpdateDepth(MacMsg_t *pMsg);
void StationProcessToaResult(MacMsg_t *pMsg);
void StationProcessAnchorCfgPoll(MacMsg_t *pMsg);
void StationProcessTagEvent(MacMsg_t *pMsg);
void AppPrintFrm(NT_frm_hdr_t * frm);
//void AppUpdateDepth(uint16 dstAddr);
void AppProcessUartFrm(uint8* buf, uint8 len);
void AppSearchNeighbor(void);
void TagConfigPoll(void);
void AnchorConfigPoll(NT_tag_cfg_poll_t *frm);
void AnchorProcessAnchorConfig(MacMsg_t *pMsg);
void AnchorProcessTagConfigPoll(MacMsg_t *pMsg);
void TagProcessTagConfig(MacMsg_t *pMsg);
void AppProcessTimeout(void);
void AppRxOn(uint8 rxid);
void AppRxOff(uint8 rxid);
void AnchorSendBeacon(void);
void AppProcessBeacon(MacMsg_t *pMsg);
inline void AppBuildFrmHdr(NT_frm_hdr_t *hdr, NT_frm_type type, uint8 srcrtg, uint8 recordpath, uint8 dsn, uint16 srcaddr, uint16 dstaddr, uint8 hops, uint8 len);
void TagUpdateParent(void);
void SnifferProcessData(MacMsg_t *pMsg);
void StationConfig(NT_config_info_t *cfginfo);
void TagConfig(NT_config_info_t *cfginfo);
void StationProcessTagConfig(MacMsg_t *pMsg);
void TagProcessStationConfig(MacMsg_t *pMsg);
void TagSendTDOABeacon(void);

#ifdef ENABLE_OLED
	static void OLED_PutAddress( void )
	{
		uint16 addr;
		uint8 start, ch;

//		#if STATION
//			OLED_PutString( 0, 0, CSTR( "Station:0x" ) ) ;
//		#elif ANCHOR
			OLED_PutString( 0, 0, CSTR( "Anchor :0x" ) ) ;
//		#elif TAG
//			OLED_PutString( 0, 0, CSTR( "Tag    :0x" ) ) ;
//		#endif
		addr = devInfo.address;
		start = 10;
		ch = (uint8)(addr >> 12);
		ch &= 0x0F;
		ch = ch < 10 ? ch + '0' : ch - 10 + 'A';
		OLED_PutChar( start + 0, 0, ch ) ;
		ch = (uint8)(addr >> 8);
		ch &= 0x0F;
		ch = ch < 10 ? ch + '0' : ch - 10 + 'A';
		OLED_PutChar( start + 1, 0, ch ) ;
		ch = (uint8)(addr >> 4);
		ch &= 0x0F;
		ch = ch < 10 ? ch + '0' : ch - 10 + 'A';
		OLED_PutChar( start + 2, 0, ch ) ;
		ch = (uint8)addr;
		ch &= 0x0F;
		ch = ch < 10 ? ch + '0' : ch - 10 + 'A';
		OLED_PutChar( start + 3, 0, ch ) ;
	}

#endif

// set device address
static void SetAddress( uint16 address )
{
	uint8 lo = EepromRead( EEPROM_ADDRESS_L );
	uint8 hi = EepromRead( EEPROM_ADDRESS_H );

//#if STATION
//	address = NT_STATION_ADDRESS;
//#elif ANCHOR
	if(address < NT_ANCHOR_ADDRESS_MIN) address = NT_ANCHOR_ADDRESS_MIN;
	if(address > NT_ANCHOR_ADDRESS_MAX) address = NT_ANCHOR_ADDRESS_MAX;
//#elif TAG
//	if(address < NT_TAG_ADDRESS_MIN) address = NT_TAG_ADDRESS_MIN;
//	if(address > NT_TAG_ADDRESS_MAX) address = NT_TAG_ADDRESS_MAX;
//#elif REFTAG
//	if(address < NT_REFTAG_ADDRESS_MIN) address = NT_REFTAG_ADDRESS_MIN;
//	if(address > NT_REFTAG_ADDRESS_MAX) address = NT_REFTAG_ADDRESS_MAX;
//#endif

	if(address != (((uint16)hi << 8) | lo)){
		lo = (uint8)address;
		hi = (uint8)(address >> 8);
		EepromWrite(EEPROM_ADDRESS_L, lo);
		EepromWrite(EEPROM_ADDRESS_H, hi);
	}

	devInfo.address = address;

	ThisDev[ADDRESS_L] = lo;
	ThisDev[ADDRESS_H] = hi;
	NtrxSetStaAddress( ThisDev ) ;	// Write the source address to the TRX chip

	//#ifdef ENABLE_DEBUG_PRINT
	con_PutString( CSTR( "Set Address 0x" ) ) ;
	con_PutHexNum( hi ) ;
	con_PutHexNum( lo ) ;
	con_PutReturn() ;
	//#endif

	#ifdef ENABLE_OLED
		OLED_PutAddress() ;
	#endif
}

void ResetAddress(void)
{
//#if STATION
//	SetAddress(NT_STATION_ADDRESS);
//#elif ANCHOR
	SetAddress(NT_ANCHOR_ADDRESS_MIN);
//#elif TAG
//	SetAddress(NT_TAG_ADDRESS_MIN);
//#endif
}

void IncAddress(void)
{
	SetAddress(devInfo.address + 1);
}

void DecAddress(void)
{
	SetAddress(devInfo.address - 1);
}

//=============================================================================
// 输出所有支持的监控命令

static void EchoHelpMessage( void )
{
//	#ifdef ENABLE_DEBUG_PRINT
	con_PutString( CSTR( "R - Reset Address\n" ) ) ;
	con_PutString( CSTR( "I - Increase Address\n" ) ) ;
	con_PutString( CSTR( "D - Decrease Address\n" ) ) ;
//	#endif
}

//#if (ANCHOR || TAG)
//void AppSearchNeighbor(void)
//{
//	NT_search_neighbor_t frm;
//	Bool ret;
//	AppBuildFrmHdr(&frm.hdr, FT_BEACON_REQ, 0, 0, app_dsn++, devInfo.address, NT_BROADCAST_ADDRESS, 1, sizeof(NT_search_neighbor_t) - sizeof(NT_frm_hdr_t));
//	
//	ret = AppSendMsg(&frm.hdr);
//
//	#if TAG
//	if(ret == True){
//		AppRxOn(RXID_SEARCH_NEIGHBOR);
//		devInfo.ChkSrchNbr = 1;
//		devInfo.SrchNbrStart = (uint16)GetSysClock();
//		osal_set_event(App_TaskId, APP_EVENT_PROCESS_TIMEOUT);
//	}
//	#else
//	(void)ret;
//	#endif
//}
//#endif

//#if TAG
//void TagRangingProcess(void)
//{
//	NtrxRangeRawDataType RawBuf[MAX_NEIGHBORS_NUM] ;
//	Bool retval[MAX_NEIGHBORS_NUM] ;
//	uint8 i ;
//	uint8 cnt;
//	NT_toa_result_t *frm;
//
//	cnt = 0; 
//	for( i = 0 ; i < MAX_NEIGHBORS_NUM ; i ++ ){
//		if(devInfo.neighbors[i].address != NT_INVALID_ADDRESS) cnt++;
//	}
//
//	#if TAG_POWER_SAVING
//	if(cnt < MIN_NEIGHBORS_NUM){
//		osal_set_event(App_TaskId, APP_EVENT_SEARCH_NEIGHBOR);
//		return;
//	}
//	#else
//	if(cnt == 0) return;
//	#endif
//
//	cnt = 0;
//	for( i = 0 ; i < MAX_NEIGHBORS_NUM ; i ++ ){
//		if(devInfo.neighbors[i].address == NT_INVALID_ADDRESS){
//			retval[i] = False ;
//		}else{
//			DestDev[ADDRESS_L] = (uint8)devInfo.neighbors[i].address;
//			DestDev[ADDRESS_H] = (uint8)(devInfo.neighbors[i].address >> 8) ;
//			retval[i] = NtrxRangeRaw( DestDev, &RawBuf[i] ) ;
//			if(retval[i]){
//				cnt++;
//			}else if(++devInfo.neighbors[i].failcount > MAX_RANGING_FAIL_COUNT){
//				devInfo.neighbors[i].address = NT_INVALID_ADDRESS;
//			}
//		}
//	}
//
//	if(cnt == 0) return;
//
//	frm = (NT_toa_result_t*)malloc(sizeof(NT_toa_result_t) + MAX_NEIGHBORS_NUM * sizeof(NT_ranging_record_t));
//	if(!frm) return;
//	
//	frm->timestamp = GetSysClock();
//
//	cnt = 0;
//	for( i = 0 ; i < MAX_NEIGHBORS_NUM ; i ++ ){
//		if(retval[i]){
//			NT_ranging_record_t* rec = (NT_ranging_record_t*)&frm->records[cnt++];
//			GetTxData(RawBuf[i].set1, (uint8*)rec->rawdata.set1);
//			GetTxData(RawBuf[i].set2, (uint8*)rec->rawdata.set2);
//			rec->rawdata.rssi = RawBuf[i].rssi;
//			rec->anchoraddr = devInfo.neighbors[i].address;
//		} else {
//			//devInfo.neighbors[i].address = NT_INVALID_ADDRESS ;
//		}
//	}
//
//	TagUpdateParent();
//
//	frm->numrec = cnt;
//
//    AppBuildFrmHdr(&frm->hdr, FT_TOA_RESULT, 0, 0, app_dsn++, devInfo.address, NT_STATION_ADDRESS, 0xFF, sizeof(NT_toa_result_t) + cnt * sizeof(NT_ranging_record_t));
//
//	AppSendMsg(&frm->hdr);  //发送信息
//
//	LED1_Toggle();
//
//#ifdef ENABLE_DEBUG_PRINT
//	AppPrintFrm(&frm->hdr);
//#endif
//
//	free(frm);
//}
//#endif //TAG

//和周围的基站一一测距
//#if (TAG && NT_DEMO_2)
//#define MIN_FAIL_COUNT	3
//#define MAX_FAIL_COUNT	(MIN_FAIL_COUNT + 6)
//#define MAX_PERIOD		(1u << 6)	//64 TICKS
//#define MAX_PERIOD_MASK	(MAX_PERIOD - 1)
//#define TICK_BITS		10	//1024MS
//#define TICK_PERIOD		(1u << TICK_BITS)
//#define HALF_TICK_PERIOD	(TICK_PERIOD >> 1)
//void TagRangingProcess2(void)
//{
//	NtrxRangeRawDataType RawBuf[ANCHOR_NUM] ;
//	Bool retval[ANCHOR_NUM] ;
//	uint8 i ;
//	uint8 itick;
//	uint8 itickmask;
//	uint8 iremain;
//	uint32 next;
//	uint32 timeout;
//	NT_toa_result_t *frm;
//
//	uint32 current = GetSysClock();
//	uint8 tick = ((uint8)((current + HALF_TICK_PERIOD) >> TICK_BITS) & MAX_PERIOD_MASK);		//?
//
//	uint8 remain = MAX_PERIOD;
//	uint8 cnt = 0;
//	for( i = 0 ; i < ANCHOR_NUM ; i ++ ){
//		uint8 fail = devInfo.anchorfail[i];
//		if(fail < MIN_FAIL_COUNT) fail = MIN_FAIL_COUNT;
//		if(fail > MAX_FAIL_COUNT) fail = MAX_FAIL_COUNT;
//		itick = (1 << (fail - MIN_FAIL_COUNT));
//		itickmask = itick - 1;
//		iremain = itick - (tick & itickmask);
//		if(iremain != 0){
//			if(iremain < remain) remain = iremain;
//		}else{
//			uint16 address = ANCHOR_ADDR_MIN + i;
//			DestDev[ADDRESS_L] = (uint8)(address);
//			DestDev[ADDRESS_H] = (uint8)(address >> 8) ;
//			retval[i] = NtrxRangeRaw( DestDev, &RawBuf[i] ) ;
//			if(retval[i]){
//				devInfo.anchorfail[i] = 0;
//				remain = 1;
//				cnt++;
//			}else{
//				if(devInfo.anchorfail[i] < MAX_FAIL_COUNT) devInfo.anchorfail[i]++;
//				if(remain > 1){
//					fail = devInfo.anchorfail[i];
//					if(fail < MIN_FAIL_COUNT) fail = MIN_FAIL_COUNT;
//					if(fail > MAX_FAIL_COUNT) fail = MAX_FAIL_COUNT;
//					itick = (1 << (fail - MIN_FAIL_COUNT));
//					itickmask = itick - 1;
//					iremain = itick - (tick & itickmask);
//					if(iremain < remain) remain = iremain;
//				}
//			}
//		}
//	}
//
//	//下一次测距的时间？
//	next = (((current + HALF_TICK_PERIOD) >> TICK_BITS) << TICK_BITS) + TICK_PERIOD * remain;
//	timeout = next - GetSysClock();
//	if(timeout > (1UL << 16)) timeout = ~0U;
//	osal_start_timerEx(App_TaskId, APP_EVENT_TAG_RANGING, (uint16)timeout);
//
//	if(cnt == 0) return;
//
//	frm = (NT_toa_result_t*)malloc(sizeof(NT_toa_result_t) + cnt * sizeof(NT_ranging_record_t));
//	if(!frm) return;
//	
//	frm->timestamp = GetSysClock();
//
//	//记录所有的测距结果
//	cnt = 0;
//	for( i = 0 ; i < ANCHOR_NUM ; i ++ ){
//		if(retval[i]){
//			NT_ranging_record_t* rec = (NT_ranging_record_t*)&frm->records[cnt++];
//			GetTxData(RawBuf[i].set1, (uint8*)rec->rawdata.set1);
//			GetTxData(RawBuf[i].set2, (uint8*)rec->rawdata.set2);
//			rec->rawdata.rssi = RawBuf[i].rssi;
//			rec->anchoraddr = ANCHOR_ADDR_MIN + i;
//		}
//	}
//
//	frm->numrec = cnt;
//
//	devInfo.parent = NT_STATION_ADDRESS;
//
//	//组建测距包
// 	AppBuildFrmHdr(&frm->hdr, FT_TOA_RESULT, 0, 0, app_dsn++, devInfo.address, NT_STATION_ADDRESS, 1, sizeof(NT_toa_result_t) + cnt * sizeof(NT_ranging_record_t));
//
//	LED1_Toggle();
//
//	free(frm);
//}
//#endif

//#if (TAG && NT_DEMO_1)
//void TagRangingProcess3(void)
//{
//	NtrxRangeRawDataType* RawBuf = (NtrxRangeRawDataType*)malloc(devInfo.anchornum * sizeof(NtrxRangeRawDataType)) ;
//	if(!RawBuf) return;
//	Bool* retval = (Bool*)malloc(devInfo.anchornum * sizeof(Bool));
//	if(!retval){
//		free(RawBuf);
//		return;
//	}
//
//	uint8 cnt = 0;
//	for(uint8 i = 0 ; i < devInfo.anchornum ; i ++ ){
//		uint16 address = devInfo.anchorlist[i];
//		DestDev[ADDRESS_L] = (uint8)(address);
//		DestDev[ADDRESS_H] = (uint8)(address >> 8) ;
//		retval[i] = NtrxRangeRaw( DestDev, &RawBuf[i] ) ;
//		if(retval[i]){
//			cnt++;
//		}
//	}
//
//	if(cnt > 0){
//		NT_toa_result_t *frm = (NT_toa_result_t*)malloc(sizeof(NT_toa_result_t) + cnt * sizeof(NT_ranging_record_t));
//		if(frm){
//			frm->timestamp = GetSysClock();
//
//			cnt = 0;
//			for(uint8 i = 0 ; i < devInfo.anchornum ; i ++ ){
//				if(retval[i]){
//					NT_ranging_record_t* rec = (NT_ranging_record_t*)&frm->records[cnt++];
//					GetTxData(RawBuf[i].set1, (uint8*)rec->rawdata.set1);
//					GetTxData(RawBuf[i].set2, (uint8*)rec->rawdata.set2);
//					rec->rawdata.rssi = RawBuf[i].rssi;
//					rec->anchoraddr = devInfo.anchorlist[i];
//				}
//			}
//
//			frm->numrec = cnt;
//
//			devInfo.parent = NT_STATION_ADDRESS;
//
//			AppBuildFrmHdr(&frm->hdr, FT_TOA_RESULT, 0, 0, app_dsn++, devInfo.address, NT_STATION_ADDRESS, 1, sizeof(NT_toa_result_t) + cnt * sizeof(NT_ranging_record_t));
//
//			if(AppSendMsg(&frm->hdr)){
//				LED1_Toggle();
//			}
//			free(frm);
//		}
//	}
//	free(RawBuf);
//	free(retval);
//}
//#endif


//#if ANCHOR
static void AnchorRangingProcess( void )
{
	NtrxRangeRawDataType RawBuf[MAX_NEIGHBORS_NUM] ;
	Bool retval[MAX_NEIGHBORS_NUM] ;
	uint8 i ;
	uint8 cnt;
	NT_toa_result_t *frm;

	cnt = 0; 
	for( i = 0 ; i < MAX_NEIGHBORS_NUM ; i ++ )
	{
		if(devInfo.neighbors[i].address == NT_INVALID_ADDRESS){
			retval[i] = False ;
		}
        else
        {
			DestDev[ADDRESS_L] = (uint8)devInfo.neighbors[i].address;
			DestDev[ADDRESS_H] = (uint8)(devInfo.neighbors[i].address >> 8) ;
			retval[i] = NtrxRangeRaw( DestDev, &RawBuf[i] ) ;   //一次完整的测距，获取两次握手的时间集
			if(retval[i]) cnt++;
		}
	}

	if(cnt == 0) return;

	frm = (NT_toa_result_t*)malloc(sizeof(NT_toa_result_t) + MAX_NEIGHBORS_NUM * sizeof(NT_ranging_record_t));
	if(!frm) return;
	
	frm->timestamp = GetSysClock();

	cnt = 0;
	for( i = 0 ; i < MAX_NEIGHBORS_NUM ; i ++ )
	{
		if(retval[i])
        {
			NT_ranging_record_t* rec = (NT_ranging_record_t*)&frm->records[cnt++];
			GetTxData(RawBuf[i].set1, (uint8*)rec->rawdata.set1);
			GetTxData(RawBuf[i].set2, (uint8*)rec->rawdata.set2);
			rec->rawdata.rssi = RawBuf[i].rssi;
			rec->anchoraddr = devInfo.neighbors[i].address;
		} 
        else 
		{
			devInfo.neighbors[i].address = NT_INVALID_ADDRESS ;
		}
	}

	frm->numrec = cnt;

	AppBuildFrmHdr(&frm->hdr, FT_TOA_RESULT, 0, 0, app_dsn++, devInfo.address, NT_STATION_ADDRESS, 0xFF, sizeof(NT_toa_result_t) + cnt * sizeof(NT_ranging_record_t));

	AppSendMsg(&frm->hdr);

//#ifdef ENABLE_DEBUG_PRINT
	AppPrintFrm(&frm->hdr);
//#endif

	free(frm);
}
//#endif

//=============================================================================
// 初始化应用程序模块

void InitApplication( void )
{
	uint8 lo = EepromRead( EEPROM_ADDRESS_L );
	uint8 hi = EepromRead( EEPROM_ADDRESS_H );
	uint16 address = ((uint16)hi << 8) | lo;

	InitApplicationIO() ;           // 初始化IO口
	InitKeyScan() ;                 // 键盘扫描初始化
	EchoHelpMessage() ;             // 输出监控命令帮助

	#ifdef ENABLE_OLED
		OLED_Init() ;
		OLED_PutString( 0, 2, CSTR( "K1: inc addr" ) ) ;
		OLED_PutString( 0, 3, CSTR( "K2: dec addr" ) ) ;
	#endif

//	#if (!SNIFFER)
	    SetAddress( address ) ;
//	#else
//	    (void)address;
//	#endif
}

//=============================================================================
// 应用程序的信息包接收回调函数

void AppCallback( NtrxDevPtr SrcAddr, NtrxDevPtr DstAddr, uint8 MsgStatus, NtrxBufferPtr Payload, uint8 Len , uint8 Rssi, uint8 dstState)
{
	uint8 size;
	MacMsg_t *pMsg;

//	#if (!SNIFFER)
    	if( ( MsgStatus & NtrxRxPacketTypeMask ) == PacketTypeRanging )
    	{
//    		#if ANCHOR
    			LED2_Toggle();
//    		#endif //ANCHOR
    		return ;
    	}

    	// 特殊信息包处理
    	MsgStatus &= NtrxRxPacketTypeMask ;
//	#else
//    	static uint8 seqnum = 0;
//    	seqnum++;
//	#endif

//	#if SNIFFER
//	size = sizeof(MacMsg_t) + 1;    //payload字段长度不同
//	#else
	size = sizeof(MacMsg_t) + Len;
//	#endif

	pMsg = (MacMsg_t*)osal_msg_allocate(size);
	if(pMsg){
		pMsg->hdr.event = MSG_EVENT_MAC_DATA;
		pMsg->timestamp = GetSysClock();
		memcpy(pMsg->srcAddr, SrcAddr, NTRX_DEV_ADDR_SIZE);
		memcpy(pMsg->dstAddr, DstAddr, NTRX_DEV_ADDR_SIZE);
		pMsg->msgStatus = MsgStatus;
		pMsg->dstState = dstState;
		pMsg->rssi = Rssi;
		pMsg->len = Len;
//		#if SNIFFER
//		    pMsg->payload[0] = seqnum;
//		#else
		    memcpy(pMsg->payload, Payload, Len);
//		#endif
		
		osal_msg_send(App_TaskId, (uint8*)pMsg);    //send message to another task_
	}else{

	}
}

//=============================================================================

void App_Init(uint8 taskid) //idx
{
	uint8 i;

	App_TaskId = taskid;

	
	InitApplication();

//	#if SNIFFER || REFTAG
//	(void)i;
//	osal_pwrmgr_device(PWRMGR_ALWAYS_ON);
//	return;
//	#else

//	#if ANCHOR
//	for(i = 0; i < MAX_BTT_ENTRY_NUM; i++){
//		devInfo.btt[i].occupied = 0;
//	}
//	#endif

//	#if !(NT_DEMO_1 || NT_DEMO_2)
	osal_start_reload_timer(App_TaskId, APP_EVENT_TICK, APP_TICK_INTERVAL);
//	#endif

	devInfo.speriod = RANGING_PERIOD;
	devInfo.dperiod = RANGING_PERIOD;

//	#if (TAG)
//	osal_start_timerEx(App_TaskId, APP_EVENT_TAG_RANGING, RANGING_DELAY);
//	devInfo.ChkCfgPoll = 0;
//	devInfo.ChkSrchNbr = 0;
//	#endif

//	#if ((STATION || TAG) && NT_DEMO_1)
//	devInfo.anchornum = 0;
//	osal_start_timerEx(App_TaskId, APP_EVENT_CONFIG, 100);
//	#endif
//
//	#if (TAG || ANCHOR)
	for(i = 0; i < MAX_NEIGHBORS_NUM; i++){
		devInfo.neighbors[i].address = NT_INVALID_ADDRESS;
	}
	devInfo.parent = NT_INVALID_ADDRESS;
//	#endif

//	#if ANCHOR
	devInfo.parent = NT_INVALID_ADDRESS;
	devInfo.depth = NT_INVALID_DEPTH;
	for(i = 0; i < MAX_TAG_CFG_ENTRY_NUM; i++){
		devInfo.tagcfginfos[i].tagaddr = NT_INVALID_ADDRESS;
	}
//	#endif

//	#if (TAG_POWER_SAVING) && (TAG)
//	osal_pwrmgr_device(PWRMGR_BATTERY); //允许进入休眠_
//	#else
	osal_pwrmgr_device(PWRMGR_ALWAYS_ON);   //一直开启，不进入休眠_
//	#endif
	
	osal_pwrmgr_task_state(App_TaskId, PWRMGR_CONSERVE);    //申明该任务是否要节能
	
	//osal_start_timerEx(App_TaskId, APP_EVENT_DEBUG, 5000);
	(void)i;
//	#endif
}

uint16 App_ProcessEvent(uint8 taskid, uint16 events)    //(idx, event)
{
	if(events & SYS_EVENT_MSG) {    //a message is waiting_
		uint8* pMsg;

		while ((pMsg = osal_msg_receive(App_TaskId)) != NULL) {
			switch ( *pMsg ) {
				case MSG_EVENT_MAC_DATA:    
				//处理收到的信息. ->特殊帧处理
					AppProcessComingData((MacMsg_t*)pMsg);  //组网，更新网络等
					break;
				default:
					break;
			}
			osal_msg_deallocate(pMsg);
		}
		return events ^ SYS_EVENT_MSG;  //clear
	}

	
//	#if TAG
//	if(events & APP_EVENT_TAG_RANGING) {
//		#if NT_DEMO_1
//		if(devInfo.anchornum > 0){
//			uint16 next = (1U << devInfo.tagrate);
//			osal_start_timerEx(App_TaskId, APP_EVENT_TAG_RANGING, next);
//			TagRangingProcess3();
//		}
//		#elif NT_DEMO_2
//		TagRangingProcess2() ;
//		#else
//		uint16 next = devInfo.dperiod;
//		if(next == 0) next = devInfo.speriod;
//		osal_start_timerEx(App_TaskId, APP_EVENT_TAG_RANGING, next) ;
//		#if TDOA_MODE
//		TagSendTDOABeacon();
//		LED1_Toggle();
//		#else
//		TagRangingProcess() ;
//		#endif
//		#endif
//		return events ^ APP_EVENT_TAG_RANGING;
//	}
//	#endif //TAG

	//DEMO1中，STATION和TAG对收到设置帧的处理
//	#if ((STATION || TAG) && NT_DEMO_1)
//	if(events & APP_EVENT_CONFIG){  //发送设置帧
//		#if TAG
//		static uint8 configwait = 1;
//		if(configwait){
//			AppRxOff(RXID_CONFIG);
//			osal_start_timerEx(App_TaskId, APP_EVENT_CONFIG, TAG_CONFIG_PERIOD);
//		}else{
//			osal_start_timerEx(App_TaskId, APP_EVENT_CONFIG, TAG_CONFIG_DURATION);
//			AppRxOn(RXID_CONFIG);
//			TagConfig(NULL);
//		}
//		#endif
//
//		#if STATION
//		osal_start_timerEx(App_TaskId, APP_EVENT_CONFIG, STATION_CONFIG_PERIOD);
//		StationConfig(NULL);
//		#endif
//		return events ^ APP_EVENT_CONFIG;
//	}
//	#endif

	//ANCHOR主动发起测距
//	#if ANCHOR
	if(events & APP_EVENT_ANCHOR_RANGING) {
		AnchorRangingProcess() ;
		return events ^ APP_EVENT_ANCHOR_RANGING;
	}
//	#endif

//	#if (STATION || ANCHOR)
//	if(events & APP_EVENT_UPDATE_DEPTH) {   //更新网络链路
//		AppUpdateDepth(NT_BROADCAST_ADDRESS);
//		return events ^ APP_EVENT_UPDATE_DEPTH;
//	}
//	#endif

#if 0
	if(events & APP_EVENT_TICK) {   //定时时间，每秒产生一次
		devInfo.ticks++;
//		#if STATION
//		if((devInfo.ticks % UPDATE_DEPTH_PERIOD) == 0){
//			osal_set_event(App_TaskId, APP_EVENT_UPDATE_DEPTH);
//		}
//		#endif
//
//		#if ANCHOR
//		if(devInfo.parent == NT_INVALID_ADDRESS){
//			osal_start_timerEx(App_TaskId, APP_EVENT_UPDATE_DEPTH, 50);
//		}
//		#endif

//		#if ANCHOR
		if(devInfo.ticks % SEND_BEACON_PERIOD == 0){
			LED1_Toggle();
			AnchorSendBeacon();		 //每两个TICK广播一次BEACON，用于TAG建立邻居表
		}
//		#endif

//		#if TAG
//		osal_set_event(App_TaskId, APP_EVENT_TAG_CFG_POLL);
//		#endif
		
		return events ^ APP_EVENT_TICK;
	}
#endif

//	#if !(SNIFFER)
	if(events & APP_EVENT_PROCESS_TIMEOUT){ //处理各种超时事件
		AppProcessTimeout();
		return events ^ APP_EVENT_PROCESS_TIMEOUT;
	}
//	#endif

	if(events & APP_EVENT_DEBUG) {
		static uint8 rxon = 0;
		osal_start_timerEx(App_TaskId, APP_EVENT_DEBUG, 1000);
		if(rxon){
			LED1_Off();
			rxon = 0;
			AppRxOff(RXID_ALWAYS_ON);
		}else{
			uint32 ms1;
			LED1_On();
			rxon = 1;
			AppRxOn(RXID_ALWAYS_ON);
			ms1 = GetSysClock();
			con_PutHexNum((uint8)(ms1 >> 24));
			con_PutHexNum((uint8)(ms1 >> 16));
			con_PutHexNum((uint8)(ms1 >> 8));
			con_PutHexNum((uint8)(ms1 >> 0));
			con_putchar(',');
	#ifdef ENABLE_DEBUG_SLEEP
			ms1 = sleeptime;
	#endif
			//ms1 = sleeptime;
			con_PutHexNum((uint8)(ms1 >> 24));
			con_PutHexNum((uint8)(ms1 >> 16));
			con_PutHexNum((uint8)(ms1 >> 8));
			con_PutHexNum((uint8)(ms1 >> 0));			
			con_PutReturn();
		}
		return events ^ APP_EVENT_DEBUG;
	}
	return 0;
}

void App_UartCB(uint8 port, uint8 event)
{
	uint16 len = 0;
	uint8* buf = uartbuf;
	NT_Uart_hdr_t *ufrm;
	NT_Uart_tail_t *utail;
	Bool badfrm = False;

	if(event == HAL_UART_RX_FULL || event == HAL_UART_RX_TIMEOUT){
		len = HalUARTRead(port, buf, 128);
	}else{
		return;
	}
	if(len == 0) return;

//#ifdef ENABLE_DEBUG_PRINT
	HalUARTWrite(port, buf, len);
	printf("print by uart write.\n");
//#endif

	ufrm = (NT_Uart_hdr_t*)buf;
	if(ufrm->sof != NT_SOF){
		badfrm = True;
	}
	if(len < ufrm->len + sizeof(NT_Uart_hdr_t) + sizeof(NT_Uart_tail_t)){
		badfrm = True;
	}
	if(badfrm){
		ISP_Service(buf, len);
		return;
	}
	
	utail = (NT_Uart_tail_t*)&ufrm->frm[ufrm->len];
	len = ufrm->len + sizeof(NT_Uart_hdr_t) + sizeof(NT_Uart_tail_t);
//	#if 0
//	if(utail->chk != calcFCS(ufrm->frm, ufrm->len)){
//		return;
//	}
//	#endif
	if(utail->eof != NT_EOF){
		return;
	}

	switch( ufrm->cmd )
	{
		case NT_DISTANCES_REPORT:	  //收到TAG上报的测距结果
			//计算定位，打印结果？
			break;

		case NT_UCMD_SET_ADDR:
			SetAddress(*(uint16*)ufrm->frm);
			break ;

		case NT_UCMD_DEBUG_PRINT:
			break;

		case NT_UCMD_NWK_FRM:
			AppProcessUartFrm(ufrm->frm, ufrm->len);
			break;
			
		case NT_UCMD_SLEEP:
			Hal_Sleep(10000);
			break;

		case NT_UCMD_CONFIG:
//			#if (STATION && NT_DEMO_1)
//			StationConfig((NT_config_info_t*)ufrm->frm);
//			#endif
			break;
	}
}

void AppProcessUartFrm(uint8* buf, uint8 len)
{
	NT_frm_hdr_t *frm;

	if(len < NT_FRM_HDR_SIZE) return;
	frm = (NT_frm_hdr_t*)buf;
	if(frm->len + NT_FRM_HDR_SIZE != len) return; 

	AppSendMsg(frm);

//#ifdef ENABLE_DEBUG_PRINT
	AppPrintFrm(frm);
//#endif
}

void App_KeyCB(uint8 keys, uint8 state)
{
//	#if SNIFFER
//	if(keys & (KEY_SW1 | KEY_SW2)){
//		sniffer_output_mode = !sniffer_output_mode;
//	}
//	return;
//	#else
	
	switch( keys )
	{
		case KEY_SW4:
			//debugprint = !debugprint ;
			Hal_Sleep(1000);
			break ;
		case KEY_SW5:
			break ;

		case KEY_SW1 :
			IncAddress();
			break;

		case KEY_SW2 :
			DecAddress();
			break;
			
		case KEY_SW1|KEY_SW2 :
			ResetAddress();
			break ;
	}
//	#endif
}

Bool AppSendMsg(NT_frm_hdr_t *hdr)
{
	uint16 dstAddr;
	NtrxPacketType ptype;
	Bool retval;

	if(hdr->srcrtg)
    {
		NT_frm_ext_hdr_t *ext = (NT_frm_ext_hdr_t*)(hdr + 1);
		dstAddr = ext->path[ext->idx];
	}
    else if(hdr->dstaddr == NT_STATION_ADDRESS)
	{
//		#if TAG
//		if(devInfo.parent == NT_INVALID_ADDRESS)
//        {
//			TagUpdateParent();
//		}
//		#endif
		if(devInfo.parent == NT_INVALID_ADDRESS)
        {
			return False;
		}
		if(hdr->recordpath)
        {
			NT_frm_ext_hdr_t *ext = (NT_frm_ext_hdr_t*)(hdr + 1);
			ext->path[ext->cnt] = devInfo.address;
			ext->cnt++;
		}
		dstAddr = devInfo.parent;
	}
    else if(hdr->dstaddr >= NT_GROUP_ADDRESS)
    {
		dstAddr = NT_BROADCAST_ADDRESS;
	}
    else
    {
		dstAddr = hdr->dstaddr;
	}
	
	DestDev[ADDRESS_L] = (uint8)dstAddr;
	DestDev[ADDRESS_H] = (uint8)(dstAddr >> 8);

	ptype = dstAddr == NT_BROADCAST_ADDRESS ? PacketTypeBrdcast : PacketTypeData;
	
	retval = NtrxSendMessage(ptype, DestDev, (NtrxBufferPtr)hdr, NT_FRM_HDR_SIZE + hdr->len);
	if(!retval)
    {
//		#if TAG
//		uint8 i;
//		for(i = 0; i < MAX_NEIGHBORS_NUM; i++)
//        {
//			if(devInfo.neighbors[i].address == devInfo.parent)
//            {
//				devInfo.neighbors[i].address = NT_INVALID_ADDRESS;
//				break;
//			}
//		}
//		devInfo.parent = NT_INVALID_ADDRESS;
//		#endif
	}
	return retval;
}

//#if ANCHOR
void AnchorRelayData(MacMsg_t *pMsg)
{
	NT_frm_hdr_t* hdr = (NT_frm_hdr_t*)(pMsg->payload);
	if(hdr->srcrtg){    //
		NT_frm_ext_hdr_t* ext = (NT_frm_ext_hdr_t*)(hdr + 1);   //跳过帧头
		if(ext->path[ext->idx] == devInfo.address){
			ext->idx--;
			AppSendMsg((NT_frm_hdr_t*)(pMsg->payload));
		}
	}else{
		if(hdr->hops > 1){
			hdr->hops--;
			AppSendMsg((NT_frm_hdr_t*)(pMsg->payload));
		}
	}
}
//#endif

//#if (ANCHOR || STATION)
//void AppUpdateDepth(uint16 dstAddr)
//{
//	uint8 size;
//	NT_update_depth_t *frm;
//
//	if(dstAddr != NT_BROADCAST_ADDRESS && devInfo.depth == NT_INVALID_DEPTH)
//		return;     //无父节点
//
//	size = 0;
//	if(devInfo.depth != 0 && devInfo.depth != NT_INVALID_DEPTH){
//		size = devInfo.depth * sizeof(uint16);
//	}
//	
//	frm = (NT_update_depth_t*)malloc(sizeof(NT_update_depth_t) + size);
//	if(frm){
//		AppBuildFrmHdr(&frm->hdr, FT_UPDATE_DEPTH, 0, 0, app_dsn++, devInfo.address, dstAddr, 1, sizeof(NT_update_depth_t) - sizeof(NT_frm_hdr_t) + size);
//
//		frm->depth = devInfo.depth;
//		frm->accucost = devInfo.accucost;
//		memcpy(frm->path2root, devInfo.path2root, size);
//		
//		AppSendMsg(&frm->hdr);
//		free(frm);
//	}
//}
//#endif

//#if (ANCHOR || STATION)
const NtrxFlashCode rssi2cost[] = {
	1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  ,
	1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  ,
	1  , 1  , 1  , 2  , 2  , 2  , 2  , 2  ,
	3  , 3  , 3  , 4  , 4  , 5  , 5  , 6  ,
	7  , 8  , 9  , 10 , 11 , 12 , 14 , 15 ,
	17 , 19 , 21 , 24 , 27 , 30 , 34 , 38 ,
	42 , 47 , 53 , 59 , 66 , 74 , 83 , 93 ,
	104, 117, 131, 146, 164, 184, 206, 230,
};
//#endif

//#if (ANCHOR || STATION)
//void AppProcessUpdateDepth(MacMsg_t *pMsg)
//{
//	NT_update_depth_t* frm = (NT_update_depth_t*)(pMsg->payload);
//	uint8 cost = NtrxReadFlash(&rssi2cost[pMsg->rssi]);
//	uint8 i;
//
//	if(devInfo.parent == NT_INVALID_ADDRESS){//lost connection
//		if(frm->depth == NT_INVALID_DEPTH){ //无父节点
//			return;
//		}
//        else{
//			for(i = frm->depth; i > 0; --i){
//				if(devInfo.address == frm->path2root[i - 1]){//from descendant
//					return;
//				}
//			}
//			
//			//candidate parent
//			devInfo.parent = frm->hdr.srcaddr;
//			devInfo.depth = frm->depth + 1;
//			devInfo.cost = cost;
//			devInfo.accucost = frm->accucost + cost;    //这里可加入判断cost，防止频繁切换
//			if(frm->depth != 0) memcpy(devInfo.path2root, frm->path2root, frm->depth * sizeof(uint16));
//			devInfo.path2root[frm->depth] = devInfo.parent;
//			osal_set_event(App_TaskId, APP_EVENT_UPDATE_DEPTH);
//		}
//	}
//    else{//connected
//		if(frm->depth == NT_INVALID_DEPTH){//
//			for(i = devInfo.depth; i > 0 ; --i){    //断掉所有子节点的连接
//				if(frm->hdr.srcaddr == devInfo.path2root[i - 1]){//from ancestor
//					devInfo.depth = NT_INVALID_DEPTH;
//					devInfo.parent = NT_INVALID_ADDRESS;
//					osal_set_event(App_TaskId, APP_EVENT_UPDATE_DEPTH);
//					return;
//				}
//			}
//
//			//this is a scanning for active node, make response
//			AppUpdateDepth(frm->hdr.srcaddr);   //更新列表，重新建立连接
//			return;
//		}
//        else{//update path to root
//			#if ANCHOR
//			if(frm->hdr.srcaddr == devInfo.parent){//path to root update
//				devInfo.depth = frm->depth + 1;
//				devInfo.cost = cost;
//				devInfo.accucost = frm->accucost + cost;
//				if(frm->depth != 0) memcpy(devInfo.path2root, frm->path2root, frm->depth * sizeof(uint16));
//				devInfo.path2root[frm->depth] = devInfo.parent;
//				
//				osal_start_timerEx(App_TaskId, APP_EVENT_UPDATE_DEPTH, 1 + (osal_rand() & 0x1F));
//				return;
//			}
//            else{
//				for(i = frm->depth; i > 0; --i){
//					if(devInfo.address == frm->path2root[i - 1]){//from descendant
//						return;
//					}
//				}
//                //判断是否加入子节点序列中
//				if(frm->accucost + cost < devInfo.accucost || (frm->accucost + cost == devInfo.accucost && frm->depth + 1 > devInfo.depth)){
//					devInfo.parent = frm->hdr.srcaddr;
//					devInfo.depth = frm->depth + 1;
//					devInfo.cost = cost;
//					devInfo.accucost = frm->accucost + cost;
//					if(frm->depth != 0) memcpy(devInfo.path2root, frm->path2root, frm->depth * sizeof(uint16));
//					devInfo.path2root[frm->depth] = devInfo.parent;
//					osal_start_timerEx(App_TaskId, APP_EVENT_UPDATE_DEPTH, 1 + (osal_rand() & 0x1F));
//					return;
//				}
//				return;
//			}
//			#endif
//		}
//	}
//}
//#endif

void AppProcessComingData(MacMsg_t *pMsg)
{
	uint16 i;
	NT_frm_hdr_t* hdr;
//#if SNIFFER
//	(void)hdr;
//	SnifferProcessData(pMsg);
//	return;
//#else
	
	hdr = (NT_frm_hdr_t*)(pMsg->payload);

	//（加入其它处理）打印收到的信息》》》
	printf("Received a message, length %d, time %s:\n   ", pMsg->len, CSTR( __TIME__ ));

	for(i = 0; i < pMsg->len; i++)
	{
		printf("%02X ", pMsg->payload[i]);
	}
	printf("\n");

//	#if ANCHOR
	if(hdr->dstaddr >= NT_GROUP_ADDRESS){//broadcast，在上层才区分组播_
//		uint8 freeidx = MAX_BTT_ENTRY_NUM;  //广播表，保留一秒?
//		uint8 i;
//		for(i = 0; i < MAX_BTT_ENTRY_NUM; i++){
//			if(devInfo.btt[i].occupied){
//				if(devInfo.btt[i].dsn == hdr->dsn){ //如果dsn已存在
//					return;
//				}
//			}else{
//				freeidx = i;
//			}
//		}
//		if(freeidx == MAX_BTT_ENTRY_NUM) return;
//		devInfo.btt[freeidx].occupied = 1;
//		devInfo.btt[freeidx].dsn = hdr->dsn;
//		devInfo.btt[freeidx].recvtime = (uint16)GetSysClock();
//		osal_set_event(App_TaskId, APP_EVENT_PROCESS_TIMEOUT);  //set event flag fot a task
//		AnchorRelayData(pMsg);  //转播
//		if(hdr->dstaddr == NT_ALLTAG_ADDRESS){
//			return;
//		}
		
        
	}
    else if(hdr->dstaddr != devInfo.address ){
//		AnchorRelayData(pMsg);  //转播
		return;
	}
//	#endif

//	#if TAG
////	if(hdr->dstaddr == NT_ALLANCHOR_ADDRESS){
////		return;
////	}
////	else 
//	if(hdr->dstaddr >= NT_GROUP_ADDRESS){//broadcast，组播
////		uint8 freeidx = MAX_BTT_ENTRY_NUM;
////		uint8 i;
////		for(i = 0; i < MAX_BTT_ENTRY_NUM; i++){
////			if(devInfo.btt[i].occupied){
////				if(devInfo.btt[i].dsn == hdr->dsn){
////					return;
////				}
////			}else{
////				freeidx = i;
////			}
////		}
////		if(freeidx == MAX_BTT_ENTRY_NUM) return;
////		devInfo.btt[freeidx].occupied = 1;
////		devInfo.btt[freeidx].dsn = hdr->dsn;
////		devInfo.btt[freeidx].recvtime = (uint16)GetSysClock();
////		osal_set_event(App_TaskId, APP_EVENT_PROCESS_TIMEOUT);
//	}
//	else if(hdr->dstaddr != devInfo.address){
//		return;
//	}
//	#endif

	switch(hdr->type){
//		#if STATION
//		case FT_TOA_RESULT:
//		case FT_TDOA_RESULT:
//			LED1_Toggle();
//			StationProcessToaResult(pMsg);
//			break;
//		case FT_ANCHOR_CFG_POLL:
//			LED2_Toggle();
//			StationProcessAnchorCfgPoll(pMsg);
//			break;
//		case FT_TAG_EVENT:
//			StationProcessTagEvent(pMsg);
//			break;			
//		case FT_CONFIG:
//			#if NT_DEMO_1
//			StationProcessTagConfig(pMsg);
//			#endif
//			break;
//		#endif

//		#if (STATION || ANCHOR)
//		case FT_UPDATE_DEPTH:
//			AppProcessUpdateDepth(pMsg);    //处理收到的更新路径的信息
//			break;
//		#endif

//		#if ANCHOR
		case FT_BEACON:
			AnchorSendBeacon();
			break;

//		case FT_ANCHOR_CFG:
//			LED2_Toggle();
//			AnchorProcessAnchorConfig(pMsg);    //参数设置帧?_
//			break;
//		case FT_TAG_CFG_POLL:
//			AnchorProcessTagConfigPoll(pMsg);   //休眠后主动发送参数设置请求帧?_
//			break;
//		#endif

//		#if (ANCHOR || TAG)

		//case FT_BEACON:
		//	AppProcessBeacon(pMsg);
		//	break;

//		#endif

		//处理设置指令信息
//		#if TAG
//		case FT_TAG_CFG:
//			//LED2_Toggle();
//			TagProcessTagConfig(pMsg);
//			break;
//		case FT_CONFIG:
//			#if NT_DEMO_1
//			TagProcessStationConfig(pMsg);
//			#endif
//			break;
//		#endif
		
		default:
			break;
	}
//#endif
}

//#if STATION
//void StationProcessToaResult(MacMsg_t *pMsg)
//{
//	AppPrintFrm((NT_frm_hdr_t*)pMsg->payload);
//}
//#endif
//
//#if STATION
//void StationProcessAnchorCfgPoll(MacMsg_t * pMsg)
//{
//	AppPrintFrm((NT_frm_hdr_t*)pMsg->payload);
//}
//#endif
//
//#if STATION
//void StationProcessTagEvent(MacMsg_t *pMsg)
//{
//	AppPrintFrm((NT_frm_hdr_t*)pMsg->payload);
//}
//#endif

void AppPrintFrm(NT_frm_hdr_t *frm)
{
	NT_Uart_hdr_t hdr;
	NT_Uart_tail_t tail;
	hdr.sof = NT_SOF;
	hdr.len = NT_FRM_HDR_SIZE + frm->len;
	hdr.cmd = NT_UCMD_NWK_FRM;
	//tail.chk = calcFCS((uint8*)frm, hdr.len);
	tail.chk = 0;
	tail.eof = NT_EOF;
	
	HalUARTWrite(0, (uint8*)&hdr, sizeof(NT_Uart_hdr_t));
	HalUARTWrite(0, (uint8*)frm, hdr.len);
	HalUARTWrite(0, (uint8*)&tail, sizeof(NT_Uart_tail_t));
}

//#if ANCHOR
void AnchorProcessTagConfigPoll(MacMsg_t *pMsg)
{   //回应config请求?
	NT_tag_cfg_poll_t *frm = (NT_tag_cfg_poll_t*)pMsg->payload;
	if(frm->action == TAG_CFG_POLL_FIRST || frm->action == TAG_CFG_POLL_TUNNEL){
		AnchorConfigPoll(frm);  //转发?
	}else{
		uint8 i = 0;
		Bool ret;
		NT_tag_cfg_t cfg;
		for(; i < MAX_TAG_CFG_ENTRY_NUM; i++){
			if(devInfo.tagcfginfos[i].tagaddr == frm->hdr.srcaddr) break;
		}

		AppBuildFrmHdr(&cfg.hdr, FT_TAG_CFG, 0, 0, app_dsn++, devInfo.address, frm->hdr.srcaddr, 0xFF, sizeof(NT_tag_cfg_t) - sizeof(NT_frm_hdr_t));
		
		if(i != MAX_TAG_CFG_ENTRY_NUM){
			cfg.cfginfo = devInfo.tagcfginfos[i].info;
		}else{
			cfg.cfginfo.mask = 0;
		}

		ret = AppSendMsg(&cfg.hdr);

		if(ret == True && i != MAX_TAG_CFG_ENTRY_NUM){//free the entry
			devInfo.tagcfginfos[i].tagaddr = NT_INVALID_ADDRESS;
		}
	}
}
//#endif

//#if TAG
//void TagConfigPoll(void)
//{
//	NT_tag_cfg_poll_t poll;
//	Bool ret;
//	if(devInfo.parent == NT_INVALID_ADDRESS){
//		osal_set_event(App_TaskId, APP_EVENT_SEARCH_NEIGHBOR);
//		osal_start_timerEx(App_TaskId, APP_EVENT_TAG_CFG_POLL, TAG_CFG_ENTRY_TIMEOUT);
//		return;
//	}
//	
//	AppBuildFrmHdr(&poll.hdr, FT_TAG_CFG_POLL, 0, 0, app_dsn++, devInfo.address, devInfo.parent, 1, sizeof(NT_tag_cfg_poll_t) - sizeof(NT_frm_hdr_t));
//
//	poll.action = devInfo.cfgpollaction;
//
//	ret = AppSendMsg(&poll.hdr);
//
//	if(devInfo.cfgpollaction == TAG_CFG_POLL_FIRST){
//		if(ret == True){
//			devInfo.cfgpollaction = TAG_CFG_POLL_SECOND;
//			osal_start_timerEx(App_TaskId, APP_EVENT_TAG_CFG_POLL, TAG_CFG_ENTRY_TIMEOUT/2);
//		}
//	}else{
//		devInfo.cfgpollaction = TAG_CFG_POLL_FIRST;
//		if(ret == True){
//			AppRxOn(RXID_CFG_POLL);
//			devInfo.ChkCfgPoll = 1;
//			devInfo.CfgPollStart = (uint16)GetSysClock();
//			osal_set_event(App_TaskId, APP_EVENT_PROCESS_TIMEOUT);
//		}
//	}
//
//#ifdef ENABLE_DEBUG_PRINT
//	AppPrintFrm(&poll.hdr);
//#endif
//}
//#endif

//#if ANCHOR
void AnchorConfigPoll(NT_tag_cfg_poll_t *frm)
{
	NT_anchor_cfg_poll_t poll;
	AppBuildFrmHdr(&poll.hdr, FT_ANCHOR_CFG_POLL, 0, 1, frm ? frm->hdr.dsn : app_dsn++, devInfo.address, NT_STATION_ADDRESS, 0xFF, sizeof(NT_anchor_cfg_poll_t) - sizeof(NT_frm_hdr_t));

	poll.ext.cnt = 0;

	poll.tagaddr = frm ? frm->hdr.srcaddr : devInfo.address;
	poll.action = frm ? frm->action : TAG_CFG_POLL_NONE;

	AppSendMsg(&poll.hdr);

//#ifdef ENABLE_DEBUG_PRINT
	AppPrintFrm(&poll.hdr);
//#endif
}
//#endif

//#if TAG
//void TagProcessTagConfig(MacMsg_t * pMsg)
//{
//	NT_tag_cfg_t *frm;
//	
//	AppRxOff(RXID_CFG_POLL);
//	devInfo.ChkCfgPoll = 0;
//
//	frm = (NT_tag_cfg_t*)pMsg->payload;
//	if(frm->cfginfo.mask & CFG_MASK_STATIC_RATE){
//		devInfo.speriod = frm->cfginfo.speriod;
//	}
//	if(frm->cfginfo.mask & CFG_MASK_DYNAMIC_RATE){
//		devInfo.dperiod = frm->cfginfo.dperiod;
//	}
//	if(frm->cfginfo.mask & CFG_MASK_SLEEP){
//		uint32 ms = 6ul * 60 * 1000 * frm->cfginfo.sleep;
//		Hal_Sleep(ms);
//	}
//
//#ifdef ENABLE_DEBUG_PRINT
//	AppPrintFrm(&frm->hdr);
//#endif
//}
//#endif

//#if !(SNIFFER)
void AppProcessTimeout(void)
{
	uint16 current = (uint16)GetSysClock();
	uint16 next = 0xFFFF;
	uint8 i;

	#if 0 //ANCHOR
	for(i = 0; i < MAX_TAG_CFG_ENTRY_NUM; i++){
		uint16 lifetime;
		if(devInfo.tagcfginfos[i].tagaddr == NT_INVALID_ADDRESS) continue;
		
		lifetime = current - devInfo.tagcfginfos[i].recvtime;
		if(lifetime >= TAG_CFG_ENTRY_TIMEOUT){
			devInfo.tagcfginfos[i].tagaddr = NT_INVALID_ADDRESS;
		}else if(next > TAG_CFG_ENTRY_TIMEOUT - lifetime){
			next = TAG_CFG_ENTRY_TIMEOUT - lifetime;
		}
	}
	#endif
	
	//#if (ANCHOR || TAG)
//	for(i = 0; i < MAX_BTT_ENTRY_NUM; i++){
//		uint16 lifetime;
//		if(devInfo.btt[i].occupied == 0) continue;
//
//		lifetime = current - devInfo.btt[i].recvtime;
//		if(lifetime >= MAX_BTT_ENTRY_OCCUPIED_TIME){
//			devInfo.btt[i].occupied = 0;
//		}else if(next > MAX_BTT_ENTRY_OCCUPIED_TIME - lifetime){
//			next = MAX_BTT_ENTRY_OCCUPIED_TIME - lifetime;
//		}
//	}
	//#endif

//	#if TAG
//	if(devInfo.ChkCfgPoll){
//		uint16 lifetime = current - devInfo.CfgPollStart;
//		if(lifetime >= TAG_WAIT_CFG_TIMEOUT){
//			devInfo.ChkCfgPoll = 0;
//			AppRxOff(RXID_CFG_POLL);
//		}else if(next > TAG_WAIT_CFG_TIMEOUT - lifetime){
//			next = TAG_WAIT_CFG_TIMEOUT - lifetime;
//		}
//	}
//	if(devInfo.ChkSrchNbr){
//		uint16 lifetime = current - devInfo.SrchNbrStart;
//		if(lifetime >= TAG_SEARCH_NEIGHBOR_TIMEOUT){
//			devInfo.ChkSrchNbr = 0;
//			AppRxOff(RXID_SEARCH_NEIGHBOR);
//			TagUpdateParent();
//		}else if(next > TAG_SEARCH_NEIGHBOR_TIMEOUT - lifetime){
//			next = TAG_SEARCH_NEIGHBOR_TIMEOUT - lifetime;
//		}
//	}
//	#endif

	(void)current;
	(void)i;
	
	if(next == 0xFFFF) return;

	osal_start_timerEx(App_TaskId, APP_EVENT_PROCESS_TIMEOUT, next);
}
//#endif

static uint16 RxOn_bitmap = 0;
void AppRxOn(uint8 rxid)
{
	RxOn_bitmap |= (uint16)1 << rxid;
	osal_pwrmgr_task_state(App_TaskId, PWRMGR_HOLD);
}

void AppRxOff(uint8 rxid)
{
	RxOn_bitmap &= ~((uint16)1 << rxid);
	if(RxOn_bitmap == 0){
		osal_pwrmgr_task_state(App_TaskId, PWRMGR_CONSERVE);
	}
}

//#if ANCHOR
void AnchorProcessAnchorConfig(MacMsg_t *pMsg)
{
	NT_anchor_cfg_t *frm = (NT_anchor_cfg_t*)pMsg->payload;
	if(frm->tagaddr == devInfo.address){
		//config the anchor
	}else if(frm->action == TAG_CFG_POLL_TUNNEL){
		NT_tag_cfg_t cfg;
		AppBuildFrmHdr(&cfg.hdr, FT_TAG_CFG, 0, 0, app_dsn++, devInfo.address, frm->tagaddr, 1, sizeof(NT_tag_cfg_t) - sizeof(NT_frm_hdr_t));
		cfg.cfginfo = frm->cfginfo;
		AppSendMsg(&cfg.hdr);
	}else{
		uint8 freeidx = MAX_TAG_CFG_ENTRY_NUM;
		uint8 i;
		for(i = 0; i < MAX_TAG_CFG_ENTRY_NUM; i++){
			if(devInfo.tagcfginfos[i].tagaddr == NT_INVALID_ADDRESS){
				freeidx = i;
			}else if(devInfo.tagcfginfos[i].tagaddr == frm->tagaddr){
				freeidx = i;
				break;
			}
		}
		if(freeidx == MAX_TAG_CFG_ENTRY_NUM) return;

		devInfo.tagcfginfos[freeidx].tagaddr = frm->tagaddr;
		devInfo.tagcfginfos[freeidx].recvtime = (uint16)GetSysClock();
		devInfo.tagcfginfos[freeidx].info = frm->cfginfo;

		osal_set_event(App_TaskId, APP_EVENT_PROCESS_TIMEOUT);
	}
}
//#endif

//#if ANCHOR
void AnchorSendBeacon(void)
{
	NT_anchor_beacon_t bcn;
	AppBuildFrmHdr(&bcn.hdr, FT_BEACON, 0, 0, anchor_bsn++, devInfo.address, NT_BROADCAST_ADDRESS, 1, sizeof(NT_anchor_beacon_t) - sizeof(NT_frm_hdr_t));
	bcn.pathok = devInfo.parent != NT_INVALID_ADDRESS ? 1 : 0;
	AppSendMsg(&bcn.hdr);
}
//#endif

//#if (ANCHOR || TAG)
void AppProcessBeacon(MacMsg_t * pMsg)
{
	NT_anchor_beacon_t *frm = (NT_anchor_beacon_t*)pMsg->payload;
	uint8 freeidx = MAX_NEIGHBORS_NUM;
	uint8 i;
	uint8 worstrssi = 0;
	uint8 worstidx = MAX_NEIGHBORS_NUM;
	for(i = 0; i < MAX_NEIGHBORS_NUM; i++){
		if(devInfo.neighbors[i].address == NT_INVALID_ADDRESS){
			freeidx = i;
		}else if(devInfo.neighbors[i].address == frm->hdr.srcaddr){
			devInfo.neighbors[i].rssi = pMsg->rssi;
			devInfo.neighbors[i].pathok = frm->pathok ? 1 : 0;
			devInfo.neighbors[i].failcount = 0;
			return;
		}else if(devInfo.neighbors[i].rssi > worstrssi){
			worstrssi = devInfo.neighbors[i].rssi;
			worstidx = i;
		}
	}
	if(freeidx != MAX_NEIGHBORS_NUM){
		devInfo.neighbors[freeidx].address = frm->hdr.srcaddr;
		devInfo.neighbors[freeidx].rssi = pMsg->rssi;
		devInfo.neighbors[freeidx].pathok = frm->pathok ? 1 : 0;
		devInfo.neighbors[freeidx].failcount = 0;
	}else if(worstrssi > 0){
		devInfo.neighbors[worstidx].address = frm->hdr.srcaddr;
		devInfo.neighbors[worstidx].rssi = pMsg->rssi;
		devInfo.neighbors[worstidx].pathok = frm->pathok ? 1 : 0;
		devInfo.neighbors[worstidx].failcount = 0;
	}
}
//#endif

inline void AppBuildFrmHdr(NT_frm_hdr_t *hdr, NT_frm_type type, uint8 srcrtg, uint8 recordpath, uint8 dsn, uint16 srcaddr, uint16 dstaddr, uint8 hops, uint8 len)
{
	hdr->type = type;
	hdr->srcrtg = srcrtg;
	hdr->recordpath = recordpath;
	hdr->dsn = dsn;
	hdr->srcaddr = srcaddr;
	hdr->dstaddr = dstaddr;
	hdr->hops = hops;
	hdr->len = len;
}

//#if TAG
//void TagUpdateParent(void)  //找到rssi最强的邻居节点作为父节点_
//{
//	#if (NT_DEMO_1 || NT_DEMO_2)
//
//	devInfo.parent = NT_STATION_ADDRESS;
//	return;
//
//	#else
//
//	uint8 bestrssi;
//	uint8 bestidx;
//	uint8 i;
//
//	bestrssi = 0xFF;
//	bestidx = MAX_NEIGHBORS_NUM;
//	for(i = 0; i < MAX_NEIGHBORS_NUM; ++i){
//		if(devInfo.neighbors[i].pathok && devInfo.neighbors[i].rssi < bestrssi){
//			bestrssi = devInfo.neighbors[i].rssi;
//			bestidx = i;
//		}
//	}
//	if(bestidx != MAX_NEIGHBORS_NUM){
//		devInfo.parent = devInfo.neighbors[bestidx].address;
//	}
//	#endif
//}
//#endif


//
//#if SNIFFER
//inline void SnifferProcessData(MacMsg_t * pMsg)
//{
//	if(sniffer_output_mode == 0){
//		NT_Uart_hdr_t hdr;
//		NT_Uart_tail_t tail;
//		hdr.sof = NT_SOF;
//		hdr.len = sizeof(pMsg->timestamp) + 2 * 2 + sizeof(pMsg->msgStatus) + sizeof(pMsg->dstState) + sizeof(pMsg->rssi) + sizeof(pMsg->len) + 1;// + pMsg->len;
//		hdr.cmd = NT_UCMD_SNIFFER_FRM;
//		//tail.chk = calcFCS((uint8*)frm, hdr.len);
//		tail.chk = 0;
//		tail.eof = NT_EOF;
//		HalUARTWrite(0, (uint8*)&hdr, sizeof(NT_Uart_hdr_t));
//		HalUARTWrite(0, (uint8*)&pMsg->timestamp, sizeof(pMsg->timestamp));
//		HalUARTWrite(0, (uint8*)pMsg->srcAddr, 2);
//		HalUARTWrite(0, (uint8*)pMsg->dstAddr, 2);
//		HalUARTWrite(0, (uint8*)&pMsg->msgStatus, sizeof(pMsg->msgStatus));
//		HalUARTWrite(0, (uint8*)&pMsg->dstState, sizeof(pMsg->dstState));
//		HalUARTWrite(0, (uint8*)&pMsg->rssi, sizeof(pMsg->rssi));
//		HalUARTWrite(0, (uint8*)&pMsg->len, sizeof(pMsg->len));
//		//HalUARTWrite(0, (uint8*)pMsg->payload, pMsg->len);
//		HalUARTWrite(0, (uint8*)pMsg->payload, 1);
//		HalUARTWrite(0, (uint8*)&tail, sizeof(NT_Uart_tail_t));
//	}else{
//		uint8 i;
//		con_PutReturn();
//		con_PutReturn();
//
//		con_PutString(CSTR("timestamp:"));
//		con_PutHexNum((uint8)(pMsg->timestamp >> 24));
//		con_PutHexNum((uint8)(pMsg->timestamp >> 16));
//		con_PutHexNum((uint8)(pMsg->timestamp >> 8));
//		con_PutHexNum((uint8)(pMsg->timestamp));
//		con_PutReturn();
//		
//		con_PutString(CSTR("SrcAddr:"));
//		for(i = 0; i < NTRX_DEV_ADDR_SIZE; ++i)
//			con_PutHexNum(pMsg->srcAddr[i]);
//		con_PutReturn();
//		
//		con_PutString(CSTR("DstAddr:"));
//		for(i = 0; i < NTRX_DEV_ADDR_SIZE; ++i)
//			con_PutHexNum(pMsg->dstAddr[i]);
//		con_PutReturn();
//		
//		con_PutString(CSTR("status:"));
//		con_PutHexNum(pMsg->msgStatus);
//		con_PutReturn();
//
//		con_PutString(CSTR("dststate:"));
//		con_PutHexNum(pMsg->dstState);
//		con_PutReturn();
//
//		con_PutString(CSTR("rssi:"));
//		con_PutDecNum(pMsg->rssi);
//		con_PutReturn();
//
//		con_PutString(CSTR("len:"));
//		con_PutDecNum(pMsg->len);
//		con_PutReturn();
//
//		con_PutString(CSTR("payload:"));
//		for(i = 0; i < pMsg->len; ++i){
//			con_PutHexNum(pMsg->payload[i]);
//			con_PutSpace();
//		}
//		con_PutReturn();
//	}
//}
//#endif

//#if (STATION && NT_DEMO_1)
//void StationConfig(NT_config_info_t * cfginfo)
//{
//	if(cfginfo){
//		devInfo.tagrate = cfginfo->tagrate;
//		devInfo.anchornum = cfginfo->anchornum;
//		memcpy(devInfo.anchorlist, cfginfo->anchorlist, cfginfo->anchornum * sizeof(uint16));
//	}else{
//		NT_Uart_hdr_t hdr;
//		NT_Uart_tail_t tail;
//		hdr.sof = NT_SOF;
//		hdr.len = sizeof(devInfo.tagrate) + sizeof(devInfo.anchornum) + sizeof(uint16) * devInfo.anchornum;
//		hdr.cmd = NT_UCMD_CONFIG;
//		//tail.chk = calcFCS((uint8*)frm, hdr.len);
//		tail.chk = 0;
//		tail.eof = NT_EOF;
//		
//		HalUARTWrite(0, (uint8*)&hdr, sizeof(NT_Uart_hdr_t));
//		HalUARTWrite(0, (uint8*)&devInfo.tagrate, sizeof(devInfo.tagrate));
//		HalUARTWrite(0, (uint8*)&devInfo.anchornum, sizeof(devInfo.anchornum));
//		HalUARTWrite(0, (uint8*)devInfo.anchorlist, devInfo.anchornum * sizeof(uint16));
//		HalUARTWrite(0, (uint8*)&tail, sizeof(NT_Uart_tail_t));
//	}
//}
//#endif

//#if (STATION && NT_DEMO_1)
//void StationProcessTagConfig(MacMsg_t * pMsg)
//{
//	NT_frm_hdr_t* hdr = (NT_frm_hdr_t*)pMsg->payload;
//	uint8 size = sizeof(NT_config_t) + devInfo.anchornum * sizeof(uint16);
//	NT_config_t* frm = (NT_config_t*)malloc(size);
//	if(frm){
//		NT_config_info_t* info = (NT_config_info_t*)frm->info;
//		AppBuildFrmHdr(&frm->hdr, FT_CONFIG, 0, 0, app_dsn++, devInfo.address, hdr->srcaddr, 1, size - sizeof(NT_frm_hdr_t));
//		info->tagrate = devInfo.tagrate;
//		info->anchornum = devInfo.anchornum;
//		memcpy(info->anchorlist, devInfo.anchorlist, devInfo.anchornum * sizeof(uint16));
//		AppSendMsg(&frm->hdr);
//		free(frm);
//	}
//}
//#endif

//#if (TAG && NT_DEMO_1)
//void TagConfig(NT_config_info_t* cfginfo)
//{
//	static uint8 configfail = 0;
//	if(cfginfo){
//		configfail = 0;
//		
//		devInfo.tagrate = cfginfo->tagrate;
//		uint8 oldnum = devInfo.anchornum;
//		devInfo.anchornum = cfginfo->anchornum;
//		memcpy(devInfo.anchorlist, cfginfo->anchorlist, cfginfo->anchornum * sizeof(uint16));
//		if(oldnum == 0) {
//			osal_start_timerEx(App_TaskId, APP_EVENT_TAG_RANGING, 5);
//		}
//	}else{
//		NT_config_t cfg;
//		NT_config_info_t* info = (NT_config_info_t*)cfg.info;
//		AppBuildFrmHdr(&cfg.hdr, FT_CONFIG, 0, 0, app_dsn++, devInfo.address, NT_STATION_ADDRESS, 1, sizeof(NT_config_t) - sizeof(NT_frm_hdr_t));
//		info->tagrate = devInfo.tagrate;
//		info->anchornum = 0;
//		AppSendMsg(&cfg.hdr);
//
//		if(configfail > TAG_CONFIG_FAIL_MAX){
//			devInfo.anchornum = 0;
//		}else{
//			configfail++;
//		}
//	}
//}
//#endif
//
//#if (TAG && NT_DEMO_1)
//void TagProcessStationConfig(MacMsg_t * pMsg)
//{
//	NT_config_t* cfg = (NT_config_t*)pMsg->payload;
//	TagConfig((NT_config_info_t*)cfg->info);
//}
//#endif


volatile static unsigned long timemstmp = 0;
volatile static unsigned int timecounttmp = 0;
volatile static unsigned long timems = 0;
volatile static unsigned int timecount = 0;
void AppPCInt3(void)
{
//#ifdef __AVR__
//	timecounttmp = TCNT1;
//#endif

//#ifdef __TARGET_CPU_CORTEX_M3
//	//TODO
//	//timecounttmp = xxx;
//#endif
	timemstmp = GetSysClock();
}

void AppRecordTime(void)
{
	timems = timemstmp;
	timecount = timecounttmp;
}

//#if ANCHOR
static NT_TDOA_result_t tdoa_result;
//#endif

Bool TOACB(NtrxBufferPtr payload, uint8 len)
{
//#if (ANCHOR || REFTAG)
//	#if ANCHOR
	static uint8 step = 0;
	Bool ret;
	uint8 idx;
//	#endif


	NT_TDOA_beacon_t* bcn = (NT_TDOA_beacon_t*)payload;
	if(bcn->hdr.type != FT_TDOA_BEACON || len != sizeof(NT_TDOA_beacon_t)){
		return False;
	}


//	#if TAG
//	return True;
//	#endif
//
//	#if REFTAG
//	if(bcn->step == TDOA_STEP_1)    //TDOA中tag和reftag发出的三帧信号的顺序
//    {   
//		bcn->step = TDOA_STEP_2;
//		bcn->hdr.srcaddr = devInfo.address;
//		AppSendMsg(&bcn->hdr);
//		Delay_ms(1);
//		bcn->step = TDOA_STEP_3;
//		AppSendMsg(&bcn->hdr);
//	}
//	return True;
//	#endif

//	#if ANCHOR
	idx = bcn->step;
    //获取收到信息的NTRX时间
	ret = TOAGET(&tdoa_result.info[idx].toaoff, &tdoa_result.info[idx].phaseoff);
	if(ret == False) 
        return True;
    //接收中断到达的MCU时间
	tdoa_result.info[idx].ms = timems;
	tdoa_result.info[idx].count = timecount;
	if(bcn->step == TDOA_STEP_1)
    {
		tdoa_result.tagid = bcn->tagid;
		tdoa_result.tagdsn = bcn->hdr.dsn;
		step = 1;
	}
    else if(bcn->step == TDOA_STEP_2)
	{
		tdoa_result.reftagid = bcn->hdr.srcaddr;
		if(step != 1 || tdoa_result.tagid != bcn->tagid || tdoa_result.tagdsn != bcn->hdr.dsn)
        {
			return True;
		}
		step = 2;
	}
    else
	{
		if(step != 2 || tdoa_result.tagid != bcn->tagid || tdoa_result.tagdsn != bcn->hdr.dsn || tdoa_result.reftagid != bcn->hdr.srcaddr)
        {
			return True;
		}
		step = 0;
		
		AppBuildFrmHdr(&tdoa_result.hdr, FT_TDOA_RESULT, 0, 0, app_dsn++, devInfo.address, NT_STATION_ADDRESS, 0xFF, sizeof(NT_TDOA_result_t) - sizeof(NT_frm_hdr_t));
		AppSendMsg(&tdoa_result.hdr);
	}
//	#endif

	return True;
//#else
//	return False;
//#endif
}

//#if TAG
//void TagSendTDOABeacon(void)
//{
//	NT_TDOA_beacon_t bcn;
//	bcn.step = TDOA_STEP_1;
//	bcn.tagid = devInfo.address;
//	AppBuildFrmHdr(&bcn.hdr, FT_TDOA_BEACON, 0, 0, app_dsn++, devInfo.address, NT_BROADCAST_ADDRESS, 1, sizeof(NT_TDOA_beacon_t) - sizeof(NT_frm_hdr_t));
//	AppSendMsg(&bcn.hdr);
//}
//#endif


//原Demo1中的文件
void AppTimerTickLoop( void )
{
	#ifdef ENABLE_KEY
		// 键盘扫描
		KeyScanProcess();
	#endif

	// LED处理
	if( LedStatus == LED_SEND_MSG_OK )
	{
		LED1_On() ;
		LED2_On() ;
		LedStatus = 255 - 100/BASE_TIME_TICK ;		// LED显示100ms
	}
	else if( LedStatus == LED_MSG_NO_ACK )
	{
		LED1_On() ;
		LED2_Off() ;
		LedStatus = 255 - 100/BASE_TIME_TICK ;		// LED显示100ms
	}
	else if( LedStatus != LED_WAIT_CMD )
	{
		if( ++LedStatus == LED_WAIT_CMD )
		{
			LED1_Off() ;
			LED2_Off() ;
		}
	}

	// OLED显示时间的定时
	#ifdef ENABLE_OLED
		if( OLED_RTC_Timer )
			OLED_RTC_Timer -- ;
	#endif	
}



