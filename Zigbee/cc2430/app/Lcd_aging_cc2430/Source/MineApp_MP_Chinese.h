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
#define LOG                      "翌日科技"
#define FUNCTION_CHINA           "功 能"
#define CONTACT_CHINA            "通信录"
#define CALL_RECORD_CHINA        "通话记录"
#define MESSAGE_CHINA            "短信息"
#define MESSAGE_RECEIVE_CHINA    "1)收件箱(0)"
#define MESSAGE_WRITE_CHINA      "2)写信息"
#define UNREAD_MESSAGE_CHINA     "新消息"
#define CALENDAR_CHINA           "日期"
#define MISSED_CALL_CHINA        "未接电话"
#define DIALED_CALL_CHINA        "已拨电话"
#define ANSWERED_CALL_CHINA      "已接电话"
#define INCOMING_CHINA           "来电"
#define VOICING_CHINA            "正在通话"
#define CALLING_CHINA            "正在呼叫..."
#define NWK_INIT_CHINA           "搜索网络..."
#define MISSED_CALL_MENU_CHINA   "个未接电话"
#define NO_POWER_CHINA           "电池电量低"
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
  {"张三", "7000"},
  {"李四", "7001"},
  {"王五", "7002"},
  /*{"阿强", "7003"},
  {"阿龙", "7004"},
  {"阿宽", "7005"},
};
  {"小赵", "7006"},
  {"小高", "7007"},
  {"小张", "7008"},
  {"小王", "7009"},
//};
  {"小李", "7010"},
  {"小刘", "7011"},
  {"小陈", "7012"},
  {"2小赵", "7006"},
  {"2小高", "7007"},
  {"2小张", "7008"},
  {"2小王", "7009"},
//};
  {"2小李", "7010"},
  {"2小刘", "7011"},
  {"2小陈", "7012"},
  {"3小陈", "7012"},*/
};
#endif