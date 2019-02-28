/*******************************************************************************
  Filename:     card_wireless.h
  Revised:      $Date: 16:41 2012��11��26��
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
    uint8 retransNum;   //�ش�����
    BOOL isRetrans;     //�ش���־

    //BOOL waitACKorNext; //����״̬,���ȥ��
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
    uint16 infoItemLen; //����ʵ�ʳ���
    //uint8 frameSum;
}CARD_INFO_UNKNOWN_ITEM_POSITION;

typedef enum
{
    ZONE_1 = 0,
    ZONE_2
}USING_ZONE;

typedef struct
{
    USING_ZONE usingZone;    //����FLASH����һ����ʵ��ʹ�õģ���һ�����ڻ��档
    CARD_INFO_UNKNOWN_ITEM_POSITION position[2];
}CARD_INFO_UNKNOWN_ITEM_POSITION_HANDLE;


typedef struct
{
    UINT32  u32EventHold;           // �¼����֣���������
    BOOL    bIsSearch;              // �Ƿ����ڱ�����״̬
    UINT16  u16SearchCnt;           // �������������ʱ��
    UINT8   u8DisplayCnt;           // ��ʾ״̬�Ĵ���
    UINT16  u16DisInterval;         // ��ʾ״̬�ļ��
    UINT8   u8IsInfochange;         // ��Ϣ�Ƿ񱻸ı�
    BOOL    bReportOn;              // �ϱ�������Ϣ
    UINT8   u8ReportInfoCnt;        // �ϱ��Ĵ���
    UINT16  u16DescLen;             // ������Ϣ�ĳ���
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


//�����޸ĺ����������޸ĵ�ʱ��
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
* ���FLASH�п���Ϣд���ҳ��
*/
void clearCardInfoPage();

//��ȡ���Ĵ洢��Ϣ
BOOL getCardInfoHandle();

/*
* ����Ϣ�ϱ�
*/
void app_ReadProc(void);

void app_ReadRetransProc(void);

/*
* ���յ���������Ϣ�ϱ��ظ���Ĵ���
*/
void app_ReadACKProc(uint8* buf);

/*
* ����Ϣд��
*/
void app_WriteProc(uint8* buf);

void app_WriteACKRetransProc(void);

/*
* �ı俨״̬
*/
void jumpIntoCardState(LF_CARD_STATE_E state);

bool saveCardInfoHandle(void);

void cardInfoInit(void);

BOOL isNewCard(void);
//��ʼ����
void app_CardStateInit(void);

void resetTransInfo(void);
/*
* ��֡����
*/
BOOL groupFrame(LPBSS_LF_WR_MSGType frameType, int8 frameSum, int8 frameSeq);

/*
 * LF���ݴ���
 */
void app_ResponseLFProc(void);

/*
 *  LF��Ϣ���մ���
 */
void app_recv_card_proc(uint8 *buf);

/*
 *  �����ش��ͳ�ʱ��Ϣ����
 */
void cardTransInfoUpdata(void);

void app_RemovInfoMask(void);
#ifdef _DEBUG_TICK 
void Report_tick(uint8 cmd,uint32 tick);
#endif//_DEBUG_TICK
#endif














