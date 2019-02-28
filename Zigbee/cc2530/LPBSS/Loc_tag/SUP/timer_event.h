#ifndef _TIMER_EVENT_H_
#define _TIMER_EVENT_H_

#include "bsp.h"

enum
{
    _5ms = 1,
    _10ms = 10,
    _50ms = 50,
    _100ms = 100,
    _500ms = 500,
    _1s = 1000,
    _1p5s = 1500,
    _2s = 2000
};

#ifdef STAFF_CARD
#define EVENT_TIMER_MAX         (12)

#define EVENT_MAC_MSG           (0x00000001)
#define EVENT_KEY_MSG           (0x00000002)
#define EVENT_ADC_MSG           (0x00000004)
#define EVENT_LOCATE_MSG        (0x00000008)

#define EVENT_RESPONCELF_MSG    (0x00000010)
#define EVENT_WAKE_TIMEOUT_MSG  (0x00000020)

#define EVENT_READ_MSG      	(0x00000040)
#define EVENT_WRITE_MSG     	(0x00000080)
#define EVENT_READ_ACK_MSG      (0x00000100)
#define EVENT_READ_RETRANS_MSG  (0x00000200)
#define EVENT_WRITE_ACK_RETRANS_MSG      (0x00000400)

#define EVENT_REPORT_MSG        (0x00000800)
#endif

#ifdef DEV_CARD_PROJ
#define EVENT_TIMER_MAX         (16)

#define EVENT_MAC_MSG           (0x00000001)
#define EVENT_KEY_MSG           (0x00000002)
#define EVENT_ADC_MSG           (0x00000004)
#define EVENT_REPORT_MSG        (0x00000008)
#define EVENT_ON_SLEEP          (0x00000010)
#define EVENT_DISPLAY_MSG       (0x00000020)
#define EVENT_LOCATE_MSG        (0x00000040)
#define EVENT_SEARCH_MSG        (0x00000080)

#define EVENT_RESPONCELF_MSG    (0x00000100)
#define EVENT_WAKE_TIMEOUT_MSG  (0x00000200)

#define EVENT_READ_MSG      	(0x00000400)
#define EVENT_WRITE_MSG     	(0x00000800)
#define EVENT_READ_ACK_MSG      (0x00001000)
#define EVENT_READ_RETRANS_MSG  (0x00002000)
#define EVENT_WRITE_ACK_RETRANS_MSG      (0x00004000)
#define EVENT_INFO_MASK_MSG     (0x00008000)
#endif

/*
* ����һ���¼��Ƿ񱻷���
*/
BOOL event_timer_take(UINT32 event);

/*
* ���������¼��Ƿ񱻷���
*/
UINT32 event_timer_take_all(void);

/*
* ��ʱ���¼�ģ��ĳ�ʼ��
*/
void event_timer_init(void);

BOOL event_timer_set(UINT32 event);

BOOL event_timer_unset(UINT32 event);

/*
* ���һ���������еĶ�ʱ��
* tick: 0���������¼���������Ϊ��ʱ�¼�
*/
BOOL event_timer_add(UINT32 event, UINT32 tick);
/*
* ���һ���������еĶ�ʱ��,�Զ����¼���
*/
BOOL event_timer_add_reload(UINT32 event, UINT32 tick);

/*
* ȡ��һ���������еĶ�ʱ��
*/
BOOL event_timer_del(UINT32 event);

/*
* ��ʱ��ģ�����ѭ��
*/
void event_timer_update(void);

UINT32 event_timer_next_tick(void);

#endif
