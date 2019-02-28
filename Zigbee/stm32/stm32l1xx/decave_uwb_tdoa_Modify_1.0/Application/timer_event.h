#ifndef _TIMER_EVENT_H_
#define _TIMER_EVENT_H_

/*
* ������¼�
*/
#define EVENT_TIMER_MAX 32

#define EVENT_RECV_DATA             (0x00000001)
#define EVENT_RAGING_REPORT         (0x00000002)
#define EVENT_CHECK_IDLE            (0x00000008)
#define EVENT_END_RCV_BEACON        (0x00000004)

#define EVENT_BUTTON_MSG            (0x00000010)
#define EVENT_EXCIT_EVENT           (0x00000020)
#define EVENT_LED_HUNGER            (0x00000040)
#define EVENT_LED_WORK              (0x00000080)

#define EVENT_HELP_MSG              (0x00000100)
#define EVENT_URGENT_RETREAT        (0x00000200)
#define EVENT_URGENT_RESET          (0x00000400)
#define EVENT_DEVTYPE_RESET         (0x00000800)


#define EVENT_UART_SEND             (0x00001000)
#define EVENT_GREEN_LED_FLASH       (0x00002000)
#define EVENT_RED_LED_FLASH         (0x00004000)

#define EVENT_MMA8452Q_EVENT        (0x00008000)
#define EVENT_NEWSLOT_EVENT         (0x00010000)
#define EVENT_SLEEP_EVENT           (0x00020000)
#define EVENT_REPORT_CARDVERSION    (0x00040000)
#define EVENT_CARD_VER_BATTERY_EVENT   (0x00080000)
#define EVENT_CHECKTDOA_REVMSG_EVENT   (0x00100000)     //��������У��

/*
* ����һ���¼��Ƿ񱻷���
*/
Bool event_timer_take(uint_32 event);

/*
* ���������¼��Ƿ񱻷���
*/
uint_32 event_timer_take_all(void);

/*
* ��ʱ���¼�ģ��ĳ�ʼ��
*/
void event_timer_init(void);

Bool event_timer_set(uint_32 event);

Bool event_timer_unset(uint_32 event);

/*
* ���һ���������еĶ�ʱ��
* tick: 0���������¼���������Ϊ��ʱ�¼�
*/
Bool event_timer_add(uint_32 event, uint_32 tick);

/*
* ���һ���������еĶ�ʱ��,�Զ����¼���
*/
Bool event_timer_add_reload(uint_32 event, uint_32 tick);

/*
* ȡ��һ���������еĶ�ʱ��
*/
Bool event_timer_del(uint_32 event);

/*
* ��ʱ��ģ�����ѭ��
*/
void event_timer_update(void);

uint_32 event_timer_next_tick(void);

#endif

