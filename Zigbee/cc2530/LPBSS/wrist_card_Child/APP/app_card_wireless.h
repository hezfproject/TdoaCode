/*******************************************************************************
  Filename:     card_wireless.h
  Revised:      $Date: 16:41 2012年11月26日
  Revision:     $Revision: 1.0 $

  Description:  card wireless header file

*******************************************************************************/
#ifndef _CARD_WIRELESS_PROTOCOL_H
#define _CARD_WIRELESS_PROTOCOL_H


/*******************************************************************************
* INCLUDES
*/
#include <types.h>
#include "lpbssnwk.h"

/*******************************************************************************
* TYPEDEFS
*/
typedef struct
{
    int frameSum;
    int frameSeq;

    //Following is used for retransmission
    uint8 retransNum;   //重传次数
    BOOL isRetrans;     //重传标志

    //BOOL waitACKorNext; //传输状态,或可去掉
}LF_TRANSLATION_INFO_T;

typedef struct
{
    uint8 pg;
    uint16 offset;
}INFO_ITEM_POS_S;


typedef enum
{
    CARD_STATE_FREE = 1,    //sleep_on
    CARD_STATE_READ,        //sleep_off
    CARD_STATE_WAIT,        //sleep_on
    CARD_STATE_WRITE        //sleep_off
}LF_CARD_STATE_E;
//LF_CARD_STATE_E cardState;    //card state, 4 kinds: free, read, wait, write.

typedef struct
{
    uint8 headPage;
    uint8 tailPage;

    uint16 headOffset;
    uint16 tailOffset;

    uint8 pading;
    uint8 itemFrameSum;
    uint16 infoItemLen; //内容实际长度
    //uint8 frameSum;
}CARD_INFO_UNKNOWN_ITEM_POSITION;

typedef enum
{
    ZONE_1 = 0,
    ZONE_2
}USING_ZONE;

typedef struct
{
    USING_ZONE usingZone;    //两个FLASH区域，一个是实际使用的，另一个用于缓存。
    CARD_INFO_UNKNOWN_ITEM_POSITION position[2];
}CARD_INFO_UNKNOWN_ITEM_POSITION_HANDLE;


typedef struct
{
    UINT32  u32EventHold;           // 事件保持，以免休眠
    BOOL    bIsSearch;              // 是否工作在被搜索状态
    UINT16  u16SearchCnt;           // 被搜索后持续的时间
    UINT8   u8DisplayCnt;           // 显示状态的次数
    UINT16  u16DisInterval;         // 显示状态的间隔
    UINT8   u8IsInfochange;         // 信息是否被改变
    BOOL    bReportOn;              // 上报配置信息
    UINT8   u8ReportInfoCnt;        // 上报的次数
    UINT16  u16DescLen;             // 配置信息的长度
    UINT16  u16RecvSeqNum;
}APP_WORK_STATE_T;

/*******************************************************************************
* CONSTANTS AND DEFINES
*/
#define EXCITATION_TIMEOUT  1600    //ms
#define RETRANS_TIMEOUT     400     //ms


#ifdef _DEBUG_TICK 
#define DEBUG_PIN_SEL   P1SEL
#define DEBUG_PIN_DIR   P1DIR
#define DEBUG_PIN_H     {P1_0=1;}
#define DEBUG_PIN_L     {P1_0=0;}
#endif


//激励修改后，屏蔽无线修改的时间
#define INFO_MASK_TIME              (60UL * 1000)

bool app_set_workstate(UINT8 *pu8Buf, UINT8 u8Len);

bool app_set_worktype(UINT8 *pu8Buf, UINT8 u8Len);

bool app_set_baseinfo(UINT8 *pu8Buf, UINT16 u16Len);

UINT8 app_get_workstate(UINT8 *pu8Buf, UINT8 u8Len);

UINT8 app_get_worktype(UINT8 *pu8Buf, UINT8 u8Len);

UINT16 app_get_baseinfo(UINT8 *pu8Buf, UINT16 u16Len);

VOID app_StopReport(VOID);

VOID app_StartReport(VOID);

VOID app_SleepOff(UINT32);

VOID app_SleepOn(UINT32);

/*
* 清空FLASH中卡信息写入的页面
*/
void clearCardInfoPage();

//获取卡的存储信息
BOOL getCardInfoHandle();

/*
* 卡信息上报
*/
void app_ReadProc(void);

void app_ReadRetransProc(void);

/*
* 卡收到卡座的信息上报回复后的处理
*/
void app_ReadACKProc(uint8* buf);

/*
* 卡信息写入
*/
void app_WriteProc(uint8* buf);

void app_WriteACKRetransProc(void);

/*
* 改变卡状态
*/
void jumpIntoCardState(LF_CARD_STATE_E state);

bool saveCardInfoHandle(void);

void cardInfoInit(void);

BOOL isNewCard(void);
//初始化卡
void app_CardStateInit(void);

void resetTransInfo(void);
/*
* 组帧函数
*/
BOOL groupFrame(LPBSS_LF_WR_MSGType frameType, int8 frameSum, int8 frameSeq);

/*
 * LF数据处理
 */
void app_ResponseLFProc(void);

/*
 *  LF信息接收处理
 */
void app_recv_card_proc(uint8 *buf);

/*
 *  卡的重传和超时信息更新
 */
void cardTransInfoUpdata(void);

void app_RemovInfoMask(void);
#ifdef _DEBUG_TICK 
void Report_tick(uint8 cmd,uint32 tick);
#endif//_DEBUG_TICK
#endif














