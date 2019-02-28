/**************************************************************************************************
  Filename:       MineApp_MP_Chinese.c
  Revised:        $Date: 2009/07/29 18:10:34 $
  Revision:       $Revision: 1.2 $

  Description:    Some chinese menu item
  **************************************************************************************************/
#ifndef MINEAPP_MP_CHINESE_H
#define MINEAPP_MP_CHINESE_H
/*********************************************************************
 * define
 */
#define LOG                      "���տƼ�"
#define FUNCTION_CHINA           "�� ��"
#define CONTACT_CHINA            "ͨ��¼"
#define CALL_RECORD_CHINA        "ͨ����¼"
#define MESSAGE_CHINA            "����Ϣ"
#define MESSAGE_RECEIVE_CHINA    "1)�ռ���(0)"
#define MESSAGE_WRITE_CHINA      "2)д��Ϣ"
#define UNREAD_MESSAGE_CHINA     "����Ϣ"
#define CALENDAR_CHINA           "����"
#define MISSED_CALL_CHINA        "δ�ӵ绰"
#define DIALED_CALL_CHINA        "�Ѳ��绰"
#define ANSWERED_CALL_CHINA      "�ѽӵ绰"
#define INCOMING_CHINA           "����"
#define VOICING_CHINA            "����ͨ��"
#define CALLING_CHINA            "���ں���..."
#define NWK_INIT_CHINA           "��������..."
#define MISSED_CALL_MENU_CHINA   "��δ�ӵ绰"
#define NO_POWER_CHINA           "��ص�����"
/*********************************************************************
 * contacts
 */
#define MAX_NAME_LEN 8
#define NUM_LEN 20
 
typedef struct
{
  uint8              name[MAX_NAME_LEN];
  uint8              num[NUM_LEN];
}C_node;

C_node contactor[] = {
  {"����", "7000"},
  {"����", "7001"},
  {"����", "7002"},
  /*{"��ǿ", "7003"},
  {"����", "7004"},
  {"����", "7005"},
};
  {"С��", "7006"},
  {"С��", "7007"},
  {"С��", "7008"},
  {"С��", "7009"},
//};
  {"С��", "7010"},
  {"С��", "7011"},
  {"С��", "7012"},
  {"2С��", "7006"},
  {"2С��", "7007"},
  {"2С��", "7008"},
  {"2С��", "7009"},
//};
  {"2С��", "7010"},
  {"2С��", "7011"},
  {"2С��", "7012"},
  {"3С��", "7012"},*/
};
#endif