#include "OSAL_Nv.h"
#include "App_cfg.h"
#include "hal_key.h"
#include "key.h"
#include "lcd_serial.h"
#include "TimeUtil.h"
#include "Hal_drivers.h"


#include "Hal_audio.h"
#include "string.h"

#include "MenuLib_tree.h"
#include "MobilePhone_global.h"
#include "MenuLib_global.h"
#include "MobilePhone_MenuLib.h"
#include "MobilePhone_MenuLibChinese.h"
#include "MobilePhone_cfg.h"

#include "MenuAdjustUtil.h"
#include "MobilePhone_Function.h"
#include "MenuChineseInputUtil.h"
#include "WatchDogUtil.h"
#include "OnBoard.h"
#include "MenuLib_Nv.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchdogUtil.h"
#endif
#ifdef MENU_OAD_UPDATE
#include "MobilePhone_global.h"
#include "delay.h"
#include "stringutil.h"
#endif

#define MENU_SMS_PAGE_MAX          5
#define MENU_TREE_DEPTH          4

typedef struct
{
    uint8 ID;
    char* name;
    uint8 ParentID;
    uint8 FirstChildID;
    uint8 ChildNum;
    char* const __code *ItemName_CH;
    MenuOper_t oper;
} Tree_node_t;

typedef struct
{
    uint8 cursor_pos;
    uint8 data_pos;
    uint8 data_line;
    uint8 page_id;
    uint8 page_pos[MENU_SMS_PAGE_MAX];
} data_info_t;

/*static variable*/
static  set_info_t     set_info;
static  data_info_t   data_info;
static  node_info_t   node_info_temp = {1,0,0};
/* setting Volume */
//static uint8             volsetting;
static stack_p_t       tree_stack_p[MENU_TREE_DEPTH];
static stack_t          tree_stack;
/*general functions */
static void menu_list_display(void);
static void menu_list_onkey(uint8 keys, uint8 status);
static void menu_selectlist_display(uint8 *p);
static void menu_selectlist_onkey(uint8 keys, uint8 status);

/*special functions */
static void menu_call_record_display(void);
static void menu_call_record_onkey(uint8 keys, uint8 status);
static void menu_contact_handle_onkey(uint8 keys, uint8 status);
static void menu_contactlist_display(void);
static void menu_contactlist_onkey(uint8 keys, uint8 status);
static void menu_setting_timedate_display(void);
static void menu_setting_timedate_onkey(uint8 keys, uint8 status);
static void menu_ringsetting_display(void);
static void menu_ringsetting_onkey(uint8 keys, uint8 status);
static void menu_functionlist_onkey(uint8 keys, uint8 status);
static void menu_shortmessage_writing_display(void);
static void menu_shortmessage_writing_onkey(uint8, uint8);
#ifdef SMS_TEMPLATE
static void menu_sms_template_display(void);
static void menu_sms_template_onkey(uint8, uint8);

static void menu_sms_template_edit_display(void);
//static void menu_sms_template_edit_onkey(uint8 keys, uint8 status);
static void menu_sms_template_handle_display(void);
static void menu_sms_template_handle_onkey(uint8 keys, uint8 status);
#endif
#ifdef SMS_SENDBOX
static void menu_sms_edit_handle_display(void);
static void menu_sms_edit_handle_onkey(uint8 keys, uint8 status);
static void menu_sms_sendbox_display(void);
static void menu_sms_sendbox_onkey(uint8 keys, uint8 status);
static void menu_sms_sendbox_edit_display(void);
static void menu_sms_sendbox_handle_display(void);
static void menu_sms_sendbox_handle_onkey(uint8 keys, uint8 status);
static void menu_sms_sendbox_delete_display(void);
static void menu_sms_sendbox_delete_onkey(uint8 keys, uint8 status);
#endif
static void menu_sms_handle_display(void);
static void menu_sms_handle_onkey(uint8 keys, uint8 status);
static void menu_sms_reply_display(void);
static void menu_sms_reply_onkey(uint8 keys, uint8 status);
static void menu_sms_delete_display(void);
static void menu_sms_delete_onkey(uint8 keys, uint8 status);
static void menu_sms_deleteall_display(void);
static void menu_sms_deleteall_onkey(uint8 keys, uint8 status);
static void menu_contact_detail_display(void);
static void menu_contact_detail_onkey(uint8 keys, uint8 status);
static void menu_callrecord_delete_onkey(uint8 keys, uint8 status);
static void menu_contact_delete_display(void);
static void menu_contact_delete_onkey(uint8 keys, uint8 status);
static void menu_setting_display(void);
static void menu_setting_onkey(uint8 keys, uint8 status);
static void menu_bellsettingList_onkey(uint8 keys, uint8 status);
static void menu_shakesetting_onkey(uint8 keys, uint8 status);
static void menu_padlocksetting_display(void);
static void menu_padlocksetting_onkey(uint8 keys, uint8 status);
static void menu_shortmessage_read_display(void);
static void menu_shortmessage_read_onkey(uint8 keys, uint8 status);
static void menu_shortmessage_inbox_display(void);
static void menu_shortmessage_inbox_onkey(uint8 keys, uint8 status);
static void menu_backlightsetting_onkey(uint8 keys, uint8 status);
static void menu_tools_display(void);
static void menu_tools_onkey(uint8 keys, uint8 status);
#ifdef MENU_OAD_UPDATE
static void menu_oad_setpanid_display(void);
static void menu_oad_setpanid_onkey(uint8 keys, uint8 status);
static void menu_oad_setshortaddr_display(void);
static void menu_oad_setshortaddr_onkey(uint8 keys, uint8 status);
static void menu_oad_setmsgtype_display(void);
static void menu_oad_setmsgtype_onkey(uint8 keys, uint8 status);
static void menu_oad_setversion_display(void);
static void menu_oad_setversion_onkey(uint8 keys, uint8 status);
static void menu_oad_result_display(void);
static void menu_oad_result_onkey(uint8 keys, uint8 status);
static void meu_oad_inputnumber_onkey(uint8 keys, uint8 status);
#endif
#ifdef MENU_CLOCKFORMAT
static void menu_timeformat_display(void);
static void menu_timeformat_onkey(uint8 keys, uint8 status);
#endif
#ifdef 	MENU_TIMEUPDATE_CTL
static void  menu_timeautoupdate_display(void);//menu_list_display
static void menu_timeautoupdate_onkey(uint8 keys, uint8 status);
#endif
/*
#ifdef MP_INFORMATION
static void menu_mp_information_dispaly(void);
static void menu_mp_information_onkey(uint8 keys, uint8 status);
#endif*/
static void menu_bellvolsetting_display(void);
static void menu_bellvolsetting_onkey(uint8 keys, uint8 status);
#ifdef MENU_RF_DEBUG

static void menu_set_channel_display(void);
static void menu_set_channel_onkey(uint8 keys, uint8 status);

static void menu_set_panid_display(void);
static void menu_set_panid_onkey(uint8 keys, uint8 status);
#endif

static char*  const __code  ItemList_FunctionList[] = {CONTACTLIST_CHINA,SHORTMESSAGE_CHINA,CALLRECORD_CHINA,SETTINGS_CHINA,TOOLS_CHINA};
static char*  const __code  ItemList_Settings[] = {RINGSETTING_CHINA, TIMESETTING_CHINA, BACKLIGHTSETTING_CHINA, PADLOCKSETTING_CHINA, RESTOREDEFAULT_CHINA
#ifdef MENU_RF_DEBUG
        , SET_CHANNEL_CHINA, SET_PANID_CHINA
#endif
#ifdef MENU_OAD_UPDATE
        , OAD_UPDATE_CHINA
#endif
                                                  };

#if (defined SMS_TEMPLATE) && (defined SMS_SENDBOX)
static char*  const __code  ItemList_ShortMessageList[] = {SMS_INCOMINGBOX_CHINA,SMS_WRITINGBOX_CHINA, SMS_SENDBOX_CHINA, SMS_TEMPLATE_CHINA};
#elif (!defined SMS_TEMPLATE) && (defined SMS_SENDBOX)
static char*  const __code  ItemList_ShortMessageList[] = {SMS_INCOMINGBOX_CHINA,SMS_WRITINGBOX_CHINA, SMS_SENDBOX_CHINA};
#elif (defined SMS_TEMPLATE) && (!defined SMS_SENDBOX)
static char*  const __code  ItemList_ShortMessageList[] = {SMS_INCOMINGBOX_CHINA,SMS_WRITINGBOX_CHINA, SMS_TEMPLATE_CHINA};
#elif (!defined SMS_TEMPLATE) && (!defined SMS_SENDBOX)
static char*  const __code  ItemList_ShortMessageList[] = {SMS_INCOMINGBOX_CHINA,SMS_WRITINGBOX_CHINA};
#endif
static char*  const __code  ItemList_Callrecords[] = {MISSED_CALL_CHINA,ANSWERED_CALL_CHINA,DIALED_CALL_CHINA,DELETERECORD_CHINA};
static char*  const __code  ItemList_CallrecordsDelete[] = {MISSED_CALL_CHINA,ANSWERED_CALL_CHINA,DIALED_CALL_CHINA};

static char*  const __code  ItemList_TimeSettings[] = {ClOCK_CHINA
#ifdef MENU_CLOCKFORMAT
        ,CLOCKFORMAT_CHINA
#endif
#ifdef 	MENU_TIMEUPDATE_CTL
        ,TIMEUPDATE_CHINA
#endif
                                                      };
static char*  const __code  ItemList_RingSettings[] = {BELLSETTING_CHINA,SMSBELLSETTING_CHINA,VOLSETTING_CHINA,SHAKESETTING_CHINA};
static char*  const __code  ItemList_BacklightSettings[] = {CLOSE_CHINA,SECONDS_10_CHINA,SECONDS_20_CHINA,SECONDS_30_CHINA};//,NOLIMIT_CHINA};

#ifdef MP_INFORMATION
static char*  const __code  ItemList_tools[] = {DATE_CHINA, MP_NUMBER_CHINA, MP_CHANNEL_CHINA, MP_PANID_CHINA, MP_SW_VERSION_CHINA};
//static char*  const __code  ItemList_MpInfo[] = {MP_NUMBER_CHINA,MP_CHANNEL_CHINA, MP_PANID_CHINA, MP_SW_VERSION_CHINA};
#else
static char*  const __code  ItemList_tools[] = {DATE_CHINA};
#endif
#ifdef MENU_CLOCKFORMAT
static char*  const __code  ItemList_TimeFormat[] = {TIMEFORMAT12_CHINA,TIMEFORMAT24_CHINA};
#endif
static char*  const __code  ItemList_Bools[] = {CLOSE_CHINA, OPEN_CHINA};
static char*  const __code  ItemList_ContactHandl[] = {CHECK_CHINA,DELETE_CHINA,ADD_CHINA};
static char*  const __code  ItemList_SMSHandle[] = {SMS_REPLY_CHINA,DELETE_CHINA,DELETE_ALL_CHINA};
#ifdef SMS_TEMPLATE
static char*  const __code  ItemList_SMSTemplateHandle[] = {SMS_SEND_CHINA,SAVE_CHINA, CANCEL_CHINA};
#endif
#ifdef SMS_SENDBOX
static char*  const __code  ItemList_SMSEditHandle[] = {SMS_SEND_CHINA, SAVE_SEND_CHINA, SAVE_CHINA, CANCEL_CHINA};
static char*  const __code  ItemList_SMSSendboxHandle[] = {SMS_SEND_CHINA, DELETE_CHINA, CANCEL_CHINA};
#endif
#ifdef MENU_OAD_UPDATE
static char*  const __code  ItemList_OAD_SetMsgType[] = {STATION_CHINA, LOCATOR_CHINA, MP_STATION_CHINA};
#endif

static Tree_node_t  const __code Menu_Tree[] =
{
    /* Function List */
    {
        .ID = MENU_ID_FUNCTIONLIST,
        .name = FUNCTIONLIST_CHINA,
        .ParentID = MENU_ID_MAIN,
        .FirstChildID = MENU_ID_CONTACTLIST,
        .ChildNum = sizeof(ItemList_FunctionList)/sizeof(ItemList_FunctionList[0]),
        .ItemName_CH = ItemList_FunctionList,
        .oper.display = menu_list_display,
        .oper.on_key = menu_functionlist_onkey,

    }
    ,
    {
        .ID = MENU_ID_CONTACTLIST,
        .name = CONTACTLIST_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = MENU_ID_CONTACT_HANDLE,
        .ChildNum = NULL,
        .ItemName_CH = NULL,
        .oper.display = menu_contactlist_display,
        .oper.on_key = menu_contactlist_onkey,

    }
    ,
    {
        .ID = MENU_ID_SHORTMESSAGE,
        .name = SHORTMESSAGE_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = MENU_ID_SHORTMESSAGE_INCOMINGBOX,
        .ChildNum= sizeof(ItemList_ShortMessageList)/sizeof(ItemList_ShortMessageList[0]),
        .ItemName_CH = ItemList_ShortMessageList,
        .oper.display = menu_list_display,
        .oper.on_key = menu_list_onkey,

    }
    ,
    {
        .ID = MENU_ID_CALLRECORD,
        .name = CALLRECORD_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = MENU_ID_CALLRECORD_MISSEDCALL,
        .ChildNum = sizeof(ItemList_Callrecords)/sizeof(ItemList_Callrecords[0]),
        .ItemName_CH = ItemList_Callrecords,
        .oper.display = menu_list_display,
        .oper.on_key = menu_list_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS,
        .name = SETTINGS_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = MENU_ID_SETTINGS_RING,
        .ChildNum = sizeof(ItemList_Settings)/sizeof(ItemList_Settings[0]),
        .ItemName_CH = ItemList_Settings,
        .oper.display = menu_setting_display,
        .oper.on_key = menu_setting_onkey,

    }
    ,
    {
        .ID = MENU_ID_TOOLS,
        .name = TOOLS_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_tools)/sizeof(ItemList_tools[0]),
        .ItemName_CH = ItemList_tools,
        .oper.display = menu_tools_display,//menu_list_display,
        .oper.on_key = menu_tools_onkey,

    }
    ,
    /* pages in functionlist */
    {
        .ID = MENU_ID_CONTACT_HANDLE,
        .name = CONTACTLIST_CHINA,
        .ParentID = MENU_ID_CONTACTLIST,
        .FirstChildID = MENU_ID_CONTACT_DETAIL,
        .ChildNum = sizeof(ItemList_ContactHandl)/sizeof(ItemList_ContactHandl[0]),
        .ItemName_CH = ItemList_ContactHandl,
        .oper.display = menu_list_display,
        .oper.on_key = menu_contact_handle_onkey,
    }
    ,
    {
        .ID = MENU_ID_CONTACT_DETAIL,
        .name = CONTACTLIST_CHINA,
        .ParentID = MENU_ID_CONTACT_HANDLE,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_contact_detail_display,
        .oper.on_key = menu_contact_detail_onkey,
    }
    ,
    {
        .ID = MENU_ID_CONTACT_DELETE,
        .name = CONTACTLIST_CHINA,
        .ParentID = MENU_ID_CONTACT_HANDLE,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_contact_delete_display,
        .oper.on_key = menu_contact_delete_onkey,
    }
    ,
    {
        .ID = MENU_ID_CONTACT_ADD,
        .name = CONTACTLIST_CHINA,
        .ParentID = MENU_ID_CONTACT_HANDLE,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = NULL,
        .oper.on_key = NULL,
    }
    ,
    /* pages in shortmessage */
    {
        .ID = MENU_ID_SHORTMESSAGE_INCOMINGBOX,
        .name = SMS_INCOMINGBOX_CHINA,
        .ParentID = MENU_ID_SHORTMESSAGE,
        .FirstChildID = NULL,
        .ChildNum= 0,
        .ItemName_CH = NULL,
        .oper.display = menu_shortmessage_inbox_display,
        .oper.on_key = menu_shortmessage_inbox_onkey,
    }
    ,
    {
        .ID = MENU_ID_SHORTMESSAGE_WRITINGBOX,
        .name = SMS_WRITINGBOX_CHINA,
        .ParentID = MENU_ID_SHORTMESSAGE,
        .FirstChildID = MENU_ID_SHORTMESSAGE_HANDLE,
        .ChildNum= 0,
        .ItemName_CH = NULL,

        .oper.display = menu_shortmessage_writing_display,
        .oper.on_key = menu_shortmessage_writing_onkey,
    }
#ifdef SMS_SENDBOX
    ,
    {
        .ID = MENU_ID_SMS_SENDBOX,
        .name = SMS_SENDBOX_CHINA,
        .ParentID = MENU_ID_SHORTMESSAGE,
        .FirstChildID = NULL,
        .ChildNum= NULL,
        .ItemName_CH = NULL,
        .oper.display = menu_sms_sendbox_display,
        .oper.on_key = menu_sms_sendbox_onkey,
    }
#endif

#ifdef SMS_TEMPLATE
    ,
    {
        .ID = MENU_ID_SMS_TEMPLATE,
        .name = SMS_TEMPLATE_CHINA,
        .ParentID = MENU_ID_SHORTMESSAGE,
        .FirstChildID = MENU_ID_SMS_TEMPLATE_EDIT,
        .ChildNum= MAX_SMS_NUM_TEMPLATE,
        .ItemName_CH = NULL,

        .oper.display = menu_sms_template_display,
        .oper.on_key = menu_sms_template_onkey,
    }
#endif

#ifdef SMS_SENDBOX
    ,
    {
        .ID = MENU_ID_SMS_EDIT_HANDLE,
        .name = SMS_WRITINGBOX_CHINA,
        .ParentID = MENU_ID_SHORTMESSAGE_WRITINGBOX,
        .FirstChildID = MENU_ID_SMS_EDIT_HANDLE,
        .ChildNum= sizeof(ItemList_SMSEditHandle)/sizeof(ItemList_SMSEditHandle[0]),
        .ItemName_CH = ItemList_SMSEditHandle,
        .oper.display = menu_sms_edit_handle_display,
        .oper.on_key = menu_sms_edit_handle_onkey,
    }
#endif

#ifdef SMS_TEMPLATE
    ,
    {
        .ID = MENU_ID_SMS_TEMPLATE_EDIT,
        .name = SMS_TEMPLATE_CHINA,
        .ParentID = MENU_ID_SMS_TEMPLATE,
        .FirstChildID = NULL,
        .ChildNum= 0,
        .ItemName_CH = NULL,

        .oper.display = menu_sms_template_edit_display,
        .oper.on_key = menu_shortmessage_writing_onkey,
    }
    ,
    {
        .ID = MENU_ID_SMS_TEMPLATE_HANDLE,
        .name = SMS_TEMPLATE_CHINA,
        .ParentID = MENU_ID_SMS_TEMPLATE,
        .FirstChildID = MENU_ID_SHORTMESSAGE_READ,
        .ChildNum = sizeof(ItemList_SMSTemplateHandle)/sizeof(ItemList_SMSTemplateHandle[0]),
        .ItemName_CH = ItemList_SMSTemplateHandle,

        .oper.display = menu_sms_template_handle_display,
        .oper.on_key = menu_sms_template_handle_onkey,

    }
#endif

#ifdef SMS_SENDBOX
    ,
    {
        .ID = MENU_ID_SMS_SENDBOX_EDIT,
        .name = SMS_SENDBOX_CHINA,
        .ParentID = MENU_ID_SMS_SENDBOX,
        .FirstChildID = NULL,
        .ChildNum= 0,
        .ItemName_CH = NULL,

        .oper.display = menu_sms_sendbox_edit_display,
        .oper.on_key = menu_shortmessage_writing_onkey,
    }
    ,
    {
        .ID = MENU_ID_SMS_SENDBOX_HANDLE,
        .name = SMS_SENDBOX_CHINA,
        .ParentID = MENU_ID_SMS_SENDBOX_EDIT,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_SMSSendboxHandle)/sizeof(ItemList_SMSSendboxHandle[0]),
        .ItemName_CH = ItemList_SMSSendboxHandle,
        .oper.display = menu_sms_sendbox_handle_display,
        .oper.on_key = menu_sms_sendbox_handle_onkey,

    }
    ,
    {
        .ID = MENU_ID_SMS_SENDBOX_DELETE,
        .name = SMS_SENDBOX_CHINA,
        .ParentID = MENU_ID_SMS_SENDBOX_HANDLE,
        .FirstChildID = NULL,
        .ChildNum = NULL,
        .ItemName_CH = NULL,
        .oper.display = menu_sms_sendbox_delete_display,
        .oper.on_key = menu_sms_sendbox_delete_onkey,

    }
#endif
    ,
    {
        .ID = MENU_ID_SHORTMESSAGE_READ,
        .name = NULL,
        .ParentID = MENU_ID_SHORTMESSAGE_INCOMINGBOX,
        .FirstChildID = NULL,
        .ChildNum= 0,
        .ItemName_CH = NULL,
        .oper.display = menu_shortmessage_read_display,
        .oper.on_key = menu_shortmessage_read_onkey,

    }
    ,
    {
        .ID = MENU_ID_SHORTMESSAGE_HANDLE,
        .name = SHORTMESSAGE_CHINA,
        .ParentID = MENU_ID_SHORTMESSAGE_INCOMINGBOX,
        .FirstChildID = MENU_ID_SHORTMESSAGE_REPLY,
        .ChildNum = sizeof(ItemList_SMSHandle)/sizeof(ItemList_SMSHandle[0]),
        .ItemName_CH = ItemList_SMSHandle,
        .oper.display = menu_sms_handle_display,
        .oper.on_key = menu_sms_handle_onkey,

    }
    ,
    {
        .ID = MENU_ID_SHORTMESSAGE_REPLY,
        .name = NULL,
        .ParentID = MENU_ID_SHORTMESSAGE_INCOMINGBOX,
        .FirstChildID = NULL,
        .ChildNum= 0,
        .ItemName_CH = NULL,
        .oper.display = menu_sms_reply_display,
        .oper.on_key = menu_sms_reply_onkey,

    }
    ,
    {
        .ID = MENU_ID_SHORTMESSAGE_DELETE,
        .name = NULL,
        .ParentID = MENU_ID_SHORTMESSAGE_INCOMINGBOX,
        .FirstChildID = NULL,
        .ChildNum= 0,
        .ItemName_CH = NULL,
        .oper.display = menu_sms_delete_display,
        .oper.on_key = menu_sms_delete_onkey,

    }
    ,
    {
        .ID = MENU_ID_SHORTMESSAGE_DELETEALL,
        .name = NULL,
        .ParentID = MENU_ID_SHORTMESSAGE_INCOMINGBOX,
        .FirstChildID = NULL,
        .ChildNum= 0,
        .ItemName_CH = NULL,
        .oper.display = menu_sms_deleteall_display,
        .oper.on_key = menu_sms_deleteall_onkey,

    }
    ,
    {
        .ID = MENU_ID_CALLRECORD_MISSEDCALL,
        .name = MISSED_CALL_CHINA,
        .ParentID = MENU_ID_CALLRECORD,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_call_record_display,
        .oper.on_key = menu_call_record_onkey,
    }
    ,
    {
        .ID = MENU_ID_CALLRECORD_ANSWEREDCALL,
        .name = ANSWERED_CALL_CHINA,
        .ParentID = MENU_ID_CALLRECORD,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_call_record_display,
        .oper.on_key = menu_call_record_onkey,
    }
    ,
    {
        .ID = MENU_ID_CALLRECORD_DIALEDCALL,
        .name = DIALED_CALL_CHINA,
        .ParentID = MENU_ID_CALLRECORD,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_call_record_display,
        .oper.on_key = menu_call_record_onkey,
    }
    ,
    {
        .ID = MENU_ID_CALLRECORD_DELETE,
        .name = DELETERECORD_CHINA,
        .ParentID = MENU_ID_CALLRECORD,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_CallrecordsDelete)/sizeof(ItemList_CallrecordsDelete[0]),
        .ItemName_CH = ItemList_CallrecordsDelete,
        .oper.display = menu_list_display,
        .oper.on_key = menu_callrecord_delete_onkey,
    }
    ,
    /*Pages in settings */
    {
        .ID = MENU_ID_SETTINGS_RING,
        .name = RINGSETTING_CHINA,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = MENU_ID_SETTINGS_RING_BELLSEL,
        .ChildNum = sizeof(ItemList_RingSettings)/sizeof(ItemList_RingSettings[0]),
        .ItemName_CH = ItemList_RingSettings,
        .oper.display = menu_ringsetting_display,
        .oper.on_key = menu_ringsetting_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS_TIME,
        .name = TIMESETTING_CHINA,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_TimeSettings)/sizeof(ItemList_TimeSettings[0]),
        .ItemName_CH = ItemList_TimeSettings,
        .oper.display = menu_setting_timedate_display,
        .oper.on_key = menu_setting_timedate_onkey,

    }
    ,
    {
        .ID = MENU_ID_SETTINGS_BACKLIGHT,
        .name = BACKLIGHTTIME_CHINA,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_BacklightSettings)/sizeof(ItemList_BacklightSettings[0]),
        .ItemName_CH = ItemList_BacklightSettings,
        .oper.display = menu_list_display,
        .oper.on_key = menu_backlightsetting_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS_PADLOCK,
        .name = PADLOCKSETTING_CHINA,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_Bools)/sizeof(ItemList_Bools[0]),
        .ItemName_CH = ItemList_Bools,
        .oper.display = menu_padlocksetting_display,//menu_list_display,
        .oper.on_key = menu_padlocksetting_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS_RESTORE_DEFAULT,
        .name = NULL,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = NULL,
        .oper.on_key = NULL,

    }
#ifdef MENU_RF_DEBUG
    ,
    {
        .ID = MENU_ID_SETTINGS_SET_CHANNEL,
        .name = NULL,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_set_channel_display,
        .oper.on_key = menu_set_channel_onkey,

    }
    ,
    {
        .ID = MENU_ID_SETTINGS_SET_PANID,
        .name = NULL,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_set_panid_display,
        .oper.on_key = menu_set_panid_onkey,

    }
#endif
#ifdef MENU_OAD_UPDATE
    ,
    {
        .ID = MENU_ID_OAD_UPDATE,
        .name = OAD_SETPANID_CHINA,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_oad_setpanid_display,
        .oper.on_key = menu_oad_setpanid_onkey,
    }
#endif
    ,
    /* Pages in Tools */

    /* Pages in Ring setting */
    {
        .ID = MENU_ID_SETTINGS_RING_BELLSEL,
        .name = BELLSETTING_CHINA,
        .ParentID = MENU_ID_SETTINGS_RING,
        .FirstChildID = NULL,
        .ChildNum = sizeof(BellNameStr_list)/sizeof(BellNameStr_list[0]),
        .ItemName_CH = BellNameStr_list,
        .oper.display = menu_list_display,
        .oper.on_key = menu_bellsettingList_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS_RING_SMSSEL,
        .name = SMSBELLSETTING_CHINA,
        .ParentID = MENU_ID_SETTINGS_RING,
        .FirstChildID = NULL,
        .ChildNum = sizeof(BellNameStr_list)/sizeof(BellNameStr_list[0]),
        .ItemName_CH = BellNameStr_list,
        .oper.display = menu_list_display,
        .oper.on_key = menu_bellsettingList_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS_RING_BELLVOL,
        .name = VOLSETTING_CHINA,
        .ParentID = MENU_ID_SETTINGS_RING,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_bellvolsetting_display,
        .oper.on_key = menu_bellvolsetting_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS_RING_SHAKE,
        .name = SHAKESETTING_CHINA,
        .ParentID = MENU_ID_SETTINGS_RING,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_Bools)/sizeof(ItemList_Bools[0]),
        .ItemName_CH = ItemList_Bools,
        .oper.display = menu_list_display,
        .oper.on_key = menu_shakesetting_onkey,
    }
    ,
    /* Pages in Time setting */
#ifdef MENU_CLOCKFORMAT
    {
        .ID = MENU_ID_SETTINGS_TIME_TIMEFORMAT,
        .name = TIMEFORMAT_CHINA,
        .ParentID = MENU_ID_SETTINGS_TIME,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_TimeFormat)/sizeof(ItemList_TimeFormat[0]),
        .ItemName_CH = ItemList_TimeFormat,
        .oper.display = menu_timeformat_display,//menu_list_display
        .oper.on_key = menu_timeformat_onkey,
    }
#endif
#ifdef 	MENU_TIMEUPDATE_CTL
    ,
    {
        .ID = MENU_ID_SETTINGS_TIME_TIMEAUTOUPDATE,
        .name = TIMEUPDATE_CHINA,
        .ParentID = MENU_ID_SETTINGS_TIME,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_Bools)/sizeof(ItemList_Bools[0]),
        .ItemName_CH = ItemList_Bools,
        .oper.display = menu_timeautoupdate_display,//menu_list_display
        .oper.on_key = menu_timeautoupdate_onkey,
    }
#endif
#ifdef MENU_OAD_UPDATE
    ,
    {
        .ID = MENU_ID_OAD_SET_SHORTADDR,
        .name = OAD_SETSHORTADDR_CHINA,
        .ParentID = MENU_ID_OAD_UPDATE,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_oad_setshortaddr_display,     //menu_list_display
        .oper.on_key = menu_oad_setshortaddr_onkey,
    }
    ,
    {
        .ID = MENU_ID_OAD_SET_MSGTYPE,
        .name = OAD_SETMSGTYPE_CHINA,
        .ParentID = MENU_ID_OAD_UPDATE,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_OAD_SetMsgType)/sizeof(ItemList_OAD_SetMsgType[0]),
        .ItemName_CH = ItemList_OAD_SetMsgType,
        .oper.display = menu_oad_setmsgtype_display,     //menu_list_display
        .oper.on_key = menu_oad_setmsgtype_onkey,
    }

    ,
    {
        .ID = MENU_ID_OAD_SET_VERSION,
        .name = OAD_SETVERSION_CHINA,
        .ParentID = MENU_ID_OAD_UPDATE,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_oad_setversion_display,     //menu_list_display
        .oper.on_key = menu_oad_setversion_onkey,
    }
    ,
    {
        .ID = MENU_ID_OAD_RESULT,
        .name = OAD_SETTING_CHINA,
        .ParentID = MENU_ID_OAD_UPDATE,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_oad_result_display,     //menu_list_display
        .oper.on_key = menu_oad_result_onkey,
    }

#endif

    /*
    #ifdef MP_INFORMATION
    ,
    {
    .ID = MENU_ID_MP_INFORMATION,
    .name = MP_INFORMATION_CHINA,
    .ParentID = NULL,
    .FirstChildID = NULL,
    .ChildNum = sizeof(ItemList_MpInfo)/sizeof(ItemList_MpInfo[0]),
    .ItemName_CH = ItemList_MpInfo,
    .oper.display = menu_mp_information_dispaly,
    .oper.on_key = menu_mp_information_onkey
    }
    #endif*/

};

void  menu_tree_stack_init(void)
{
    tree_stack.stack_depth = MENU_TREE_DEPTH;
    tree_stack.stack_p = tree_stack_p;
    tree_stack.stack_i = 0;
}
void  menu_tree_stack_clear(void)
{
    tree_stack.stack_i = 0;
}
void    menu_tree_nodeID_check(void)
{
    uint8 len = sizeof(Menu_Tree)/sizeof(Menu_Tree[0]);
    for(uint8 i =0; i<len; i++)
    {
        if(Menu_Tree[i].ID != GetIDFromIdx(NODE_TYPE_TREE,i))
        {
            LcdClearDisplay();
            LCD_Str_Print("Menu Node ID Incorrect!", 0, 0, TRUE);
            while(1);
        }
    }
}

void menu_tree_display(void)
{
    if(GetTypeFromID(CurrentNodeID) == NODE_TYPE_TREE)
    {
        uint8 idx = GetIdxFromID(CurrentNodeID);
        if(Menu_Tree[idx].oper.display)
            Menu_Tree[idx].oper.display();
    }
}
void   menu_tree_handle_key(uint8 keys, uint8 status)
{
    if(GetTypeFromID(CurrentNodeID) == NODE_TYPE_TREE)
    {
        uint8 idx = GetIdxFromID(CurrentNodeID);
        if(Menu_Tree[idx].oper.on_key)
            Menu_Tree[idx].oper.on_key(keys,status);
    }
}

void menu_steptochild(uint8 ID, uint8 sel_item)
{
    if(GetTypeFromID(CurrentNodeID) == NODE_TYPE_TREE && GetTypeFromID(ID) == NODE_TYPE_TREE)
    {
        Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
        Tree_node_t node_child = Menu_Tree[GetIdxFromID(ID)];

        if(ID - node.FirstChildID < node.ChildNum && (sel_item==0 || sel_item < node_child.ChildNum))
        {
            //Nodeinfo_Push();
            Stack_Push(&tree_stack, 0, &node_info);
            //MENU_RESET_NODEINFO();
            if(node_child.ChildNum == 0)
                ;
            else if(node_child.ChildNum <= SCREEN_LINES)
            {
                node_info.high_line = sel_item + 1;
                node_info.sel_item = sel_item;
                node_info.show_item = 0;
            }
            else
            {
                if(sel_item <= node_child.ChildNum-SCREEN_LINES)
                {
                    node_info.high_line = 1;
                    node_info.sel_item = sel_item;
                    node_info.show_item = sel_item;
                }
                else
                {
                    node_info.sel_item    = sel_item;
                    node_info.show_item = node_child.ChildNum-SCREEN_LINES;
                    node_info.high_line    = sel_item - node_info.show_item+ 1;

                }
            }
            NearLastNodeID = CurrentNodeID;
            CurrentNodeID = ID;

            if (CurrentNodeID == MENU_ID_CALLRECORD_DIALEDCALL)
            {
                osal_nv_read(MINEAPP_NV_DIALED, 0, 1, &node_info.show_item);
                node_info.sel_item = node_info.show_item;
            }
            else if(CurrentNodeID == MENU_ID_CALLRECORD_MISSEDCALL)
            {
                osal_nv_read(MINEAPP_NV_MISSED, 0, 1, &node_info.show_item);
                node_info.sel_item = node_info.show_item;
            }
            else if(CurrentNodeID == MENU_ID_CALLRECORD_ANSWEREDCALL)
            {
                osal_nv_read(MINEAPP_NV_ANSWERED, 0, 1, &node_info.show_item);
                node_info.sel_item = node_info.show_item;
            }
            menu_display();
        }
    }
}
void menu_steptoparent(void)
{
    if(GetTypeFromID(CurrentNodeID) == NODE_TYPE_TREE)
    {
        Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
        NearLastNodeID = CurrentNodeID;
        CurrentNodeID = node.ParentID;
        Stack_Pop(&tree_stack, NULL, &node_info);
        menu_display();
    }
}
/*
void menu_tree_stackclear()
{
node_info_stack_i = 0;
}
*/
/*general functions */

static void menu_list_display(void)
{

    uint8 i;
    char *name;
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8 *)node.name, (LCD_LINE_WIDTH-osal_strlen(node.name))/2, 0, TRUE);
    }
    else
    {
        for(uint8 i=1; i<SCREEN_LINES+1; i++)
        {
            LCD_ListLine_Clear(i);
        }
    }


    if(node.ChildNum == 0)
        return;
    else if(node.ChildNum <= SCREEN_LINES)
    {
        for(i=0; i<node.ChildNum - node_info.show_item; i++)
        {
            name = node.ItemName_CH[node_info.show_item+i];
            LCD_Str_Print((uint8 *)name, 0, i+1, TRUE);
        }
    }
    else
    {
        for(i=0; i<SCREEN_LINES; i++)
        {
            name = node.ItemName_CH[node_info.show_item+i];
            LCD_Str_Print((uint8 *)name, 0, i+1, TRUE);
        }
    }
    LCD_ListLine_Inv(node_info.high_line);

    if(FIRSTTIME_INTO_NODE())
    {
        LCD_ProgBar_open();
    }
    LCD_ProgBar_update(node_info.sel_item, node.ChildNum);
}

static void menu_list_up_down_onkey(uint8 keys, uint8 list_len, bool list_direction)
{

    switch(keys)
    {
    case HAL_KEY_UP:
    case HAL_KEY_LEFT:
        NearLastNodeID =CurrentNodeID;
        if(list_len > SCREEN_LINES)
        {
            if(list_direction)
            {
                if(node_info.high_line == 1)
                {
                    if(node_info.sel_item == 0)
                    {
                        node_info.sel_item = list_len - 1;
                        node_info.high_line = SCREEN_LINES;
                        node_info.show_item = node_info.sel_item - (SCREEN_LINES - 1);
                    }
                    else
                    {
                        --node_info.sel_item;
                        --node_info.show_item;
                    }
                    menu_display();
                }
                else
                {
                    --node_info.sel_item;
                    --node_info.high_line;
                    LCD_ListLine_Inv(node_info.high_line);
                    LCD_ProgBar_update(node_info.sel_item, list_len);
                }
            }
            else
            {
                if(node_info.high_line==1)
                {
                    if(node_info.sel_item== list_len)
                    {
                        node_info.sel_item = 1;
                        node_info.high_line = SCREEN_LINES;
                        node_info.show_item = node_info.sel_item + SCREEN_LINES-1;
                    }
                    else
                    {
                        ++node_info.sel_item;
                        ++node_info.show_item;
                    }
                    NearLastNodeID = CurrentNodeID;
                    menu_display();
                }
                else
                {
                    ++node_info.sel_item;
                    --node_info.high_line;
                    //LCD_Line_Inv(node_info.high_line);
                    LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);
                }
            }
        }
        else if((list_len <= SCREEN_LINES) && (list_len > 0))
        {
            if(list_direction)
            {
                if(node_info.high_line == 1)
                {

                    node_info.high_line = list_len;
                    node_info.sel_item = list_len - 1;
                }
                else
                {
                    --node_info.sel_item;
                    --node_info.high_line;
                }
                LCD_ListLine_Inv(node_info.high_line);
                LCD_ProgBar_update(node_info.sel_item, list_len);
            }
            else
            {
                if(node_info.high_line == 1)
                    node_info.high_line = list_len;
                else
                    --node_info.high_line;

                if(node_info.sel_item == list_len)
                    node_info.sel_item = 1;
                else
                    ++node_info.sel_item;
                LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);
                //LCD_Line_Inv(node_info.high_line);
            }
        }
        break;
    case HAL_KEY_DOWN:
    case HAL_KEY_RIGHT:
        NearLastNodeID = CurrentNodeID;
        if(list_len > SCREEN_LINES)
        {
            if(list_direction)
            {
                if(node_info.high_line == SCREEN_LINES)
                {
                    if(node_info.sel_item == list_len - 1)
                    {
                        node_info.high_line = 1;
                        node_info.sel_item = 0;
                        node_info.show_item = 0;
                    }
                    else
                    {
                        ++node_info.sel_item;
                        ++node_info.show_item;
                    }
                    menu_display();
                }
                else
                {
                    ++node_info.sel_item;
                    ++node_info.high_line;
                    LCD_ListLine_Inv(node_info.high_line);
                    LCD_ProgBar_update(node_info.sel_item, list_len);
                }
            }
            else
            {
                if(node_info.high_line==SCREEN_LINES)
                {
                    if(node_info.sel_item== 1)
                    {
                        node_info.sel_item = list_len;
                        node_info.high_line = 1;
                        node_info.show_item = list_len;
                    }
                    else
                    {
                        --node_info.sel_item;
                        node_info.show_item = node_info.sel_item + SCREEN_LINES - 1;
                    }
                    NearLastNodeID = CurrentNodeID;
                    menu_display();
                }
                else
                {
                    --node_info.sel_item;
                    ++node_info.high_line;
                    //LCD_Line_Inv(node_info.high_line);
                    LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);
                }
            }
        }
        else if((list_len <= SCREEN_LINES) && (list_len > 0))
        {
            if(list_direction)
            {
                if(node_info.high_line == list_len)
                {
                    node_info.high_line = 1;
                    node_info.sel_item = 0;
                    node_info.show_item = 0;
                }
                else
                {
                    ++node_info.sel_item;
                    ++node_info.high_line;
                }
                LCD_ListLine_Inv(node_info.high_line);
                LCD_ProgBar_update(node_info.sel_item, list_len);
            }
            else
            {
                if(node_info.high_line == list_len)
                    node_info.high_line = 1;
                else
                    ++node_info.high_line;

                if(node_info.sel_item == 1)
                    node_info.sel_item = list_len;
                else
                    --node_info.sel_item;

                //LCD_Line_Inv(node_info.high_line);
                LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);
            }
        }
        break;
    default:
        break;
    }

}

static void menu_list_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    if(CurrentNodeID == MENU_ID_CONTACTLIST)
#ifdef NEW_DOUBLE_NVID_OP
        menu_Contact_ReadContactNum(&node.ChildNum);
#endif

    switch(keys)
    {
    case HAL_KEY_SELECT:
        menu_steptochild(node.FirstChildID + node_info.sel_item, 0); /* select the first one by default*/
        break;
    case HAL_KEY_BACKSPACE:
        if(shortcuts_flag)
        {
            shortcuts_flag = FALSE;
            menu_JumptoMenu(MENU_ID_MAIN);
        }
        else
        {
            menu_steptoparent();
        }
        break;
    default:
        menu_list_up_down_onkey(keys, node.ChildNum, TRUE);
        break;
    }
}

static void    menu_contactlist_display(void)
{
    uint8 i, offset;
    Contact_Node c_node;
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8 *)node.name, (LCD_LINE_WIDTH-osal_strlen(node.name))/2, 0, TRUE);
    }
    else
    {
        for(uint8 i=1; i<SCREEN_LINES+1; i++)
        {
            LCD_ListLine_Clear(i);
        }
    }

#ifdef NEW_DOUBLE_NVID_OP
    menu_Contact_ReadContactNum(&offset);
#endif

    if(NearLastNodeID == MENU_ID_INPUTNAME)
    {
        if(offset <= SCREEN_LINES)
        {
            node_info.high_line = offset;
            node_info.sel_item = offset - 1;
            node_info.show_item = 0;
        }
        else
        {
            node_info.high_line = SCREEN_LINES;
            node_info.sel_item = offset - 1;
            node_info.show_item = offset - 3;
        }
    }
    else if(NearLastNodeID == MENU_ID_CONTACT_DELETE)
    {

        if((offset >= SCREEN_LINES))
        {
            if(node_info.sel_item == offset)
            {
                --node_info.sel_item;
                node_info.show_item = node_info.sel_item - 2;
            }
            else if(node_info.show_item + SCREEN_LINES > offset)
            {
                --node_info.sel_item;
                --node_info.show_item;
            }
        }
        else
        {
            if(node_info.sel_item == offset)
            {
                --node_info.sel_item;
                --node_info.high_line;
                --node_info.show_item;
            }
        }
    }

    node.ChildNum = offset;

    if(node.ChildNum == 0)
        return;
    else if(node.ChildNum <= SCREEN_LINES)
    {
        for(i=0; i<node.ChildNum; i++)
        {
#ifdef NEW_DOUBLE_NVID_OP
            menu_Contact_ReadContact(&c_node, i);
#endif
            LCD_Str_Print((uint8 *)c_node.name, 0, i+1, TRUE);
        }
    }
    else
    {

        for(i=0; i<SCREEN_LINES; i++)
        {
#ifdef NEW_DOUBLE_NVID_OP
            menu_Contact_ReadContact(&c_node, node_info.show_item+i);
#endif
            LCD_Str_Print((uint8 *)c_node.name, 0, i+1, TRUE);
        }
    }

    LCD_ListLine_Inv(node_info.high_line);
    if(FIRSTTIME_INTO_NODE())
    {
        LCD_ProgBar_open();
    }
    LCD_ProgBar_update(node_info.sel_item, node.ChildNum);

}

static void    menu_contactlist_onkey(uint8 keys, uint8 status)
{
    uint8 offset;
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
#ifdef NEW_DOUBLE_NVID_OP
    menu_Contact_ReadContactNum(&offset);
#endif

    if(keys == HAL_KEY_SELECT)
    {
        node_info_temp = node_info;
        NearLastNodeID = CurrentNodeID;
        Stack_Clear(&global_stack);
        Stack_Push(&global_stack, CurrentNodeID, &node_info);
        menu_JumptoMenu(MENU_ID_CONTACT_HANDLE);
        return;
    }
    else if(keys == HAL_KEY_CALL)
    {
        if(offset > 0)
        {
#ifdef NEW_DOUBLE_NVID_OP
            Contact_Node contact_node;
            menu_Contact_ReadContact(&contact_node,node_info.sel_item);
            if(strlen((char*)contact_node.num) < APP_NMBRDIGIT)
            {
                strcpy((char*)dialnum_buf.p,(char*)contact_node.num);
            }
#endif
            dialnum_buf.len = osal_strlen((char*)dialnum_buf.p);
            dialnum_buf.p[dialnum_buf.len] = '\0';
            menu_Dial(dialnum_buf);
        }
        else
        {
            strcpy((char *)g_jump_buf,EMPTY_CONTACTLIST_CHINA);
            menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            MP_StartMenuLibEvt(1000);
        }
        return;
    }
    menu_list_onkey(keys, status);

}

static void    menu_call_record_display(void)
{
    Record record;
    uint16 nv_id;
    uint8 i, j, k, len, pos;

    LcdClearDisplay();
    if (CurrentNodeID == MENU_ID_CALLRECORD_DIALEDCALL)
    {
        nv_id = MINEAPP_NV_DIALED;
        LCD_Str_Print(DIALED_CALL_CHINA, 4, 0, TRUE);
    }
    else if(CurrentNodeID == MENU_ID_CALLRECORD_MISSEDCALL)
    {
        nv_id = MINEAPP_NV_MISSED;
        LCD_Str_Print(MISSED_CALL_CHINA, 4, 0, TRUE);
    }
    else if(CurrentNodeID == MENU_ID_CALLRECORD_ANSWEREDCALL)
    {
        nv_id = MINEAPP_NV_ANSWERED;
        LCD_Str_Print(ANSWERED_CALL_CHINA, 4, 0, TRUE);
    }
    else
    {
        return;
    }

    osal_nv_read(nv_id, 0, 1, &pos);
    if(pos == 0)
    {
        MENU_RESET_NODEINFO();
        return;
    }

    if(FIRSTTIME_INTO_NODE())
    {
        node_info.show_item = pos;
        node_info.sel_item = pos;
        node_info.high_line= 1;
    }

    len = sizeof(Record);

    if((pos > 0) &&(pos <= SCREEN_LINES))
    {
        k = pos;
        for(i=pos, j=1; i>=1; i--, j++)
        {
            osal_nv_read(nv_id, (node_info.show_item-j)*sizeof(Record)+1, sizeof(Record), &record);
            Contact_Node c_node;

            len = LCD_ID_Show(k, 0, j);
            LCD_Str_Print(")", len, j, TRUE);
            if(ZSuccess == menu_Contact_SearchContactByNum(&c_node, NULL, record.num))
            {
                LCD_Str_Print(c_node.name, len+1, j, TRUE);
            }
            else
            {
                LCD_Str_Print(record.num, len+1, j, TRUE);
            }

            --k;
        }
    }
    else if((pos > SCREEN_LINES) &&(pos <= MAX_CALL_NUM))
    {
        k = node_info.show_item;
        for(i = 1, j=node_info.show_item-1; i<=3; i++)
        {
            len = sizeof(Record);
            osal_nv_read(nv_id, j*len+1, len, &record);
            len = LCD_ID_Show(k, 0, i);
            LCD_Str_Print(")", len, i, TRUE);

            Contact_Node c_node;
            if(ZSuccess == menu_Contact_SearchContactByNum(&c_node, NULL, record.num))
            {
                LCD_Str_Print(c_node.name, len+1, i, TRUE);
            }
            else
            {
                LCD_Str_Print(record.num, len+1, i, TRUE);
            }

            if(k=='1')
                k = pos+ '0';
            else
                --k;
            if(j==0)
                j = pos - 1;
            else
                --j;
        }
    }

    if(node_info.high_line!=0)
        LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);
}

static void    menu_call_record_onkey(uint8 keys, uint8 status)
{
    uint8   len;
    uint16 NV_ID;
    Record record;

    if (CurrentNodeID == MENU_ID_CALLRECORD_DIALEDCALL)
    {
        NV_ID = MINEAPP_NV_DIALED;
    }
    else if(CurrentNodeID == MENU_ID_CALLRECORD_MISSEDCALL)
    {
        NV_ID = MINEAPP_NV_MISSED;
    }
    else if(CurrentNodeID == MENU_ID_CALLRECORD_ANSWEREDCALL)
    {
        NV_ID = MINEAPP_NV_ANSWERED;
    }

    osal_nv_read(NV_ID, 0, 1, &len);

    switch(keys)
    {
    case HAL_KEY_CALL:
        if(len == 0)
        {
            strcpy((char *)g_jump_buf,EMPTY_CALLRECORD_CHINA);
            menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            MP_StartMenuLibEvt(1000);
        }
        else
        {
            osal_nv_read(NV_ID, (node_info.sel_item-1)*sizeof(Record)+1, APP_NMBRDIGIT, dialnum_buf.p);
            dialnum_buf.len = osal_strlen((char*)dialnum_buf.p);
            dialnum_buf.p[dialnum_buf.len] = '\0';
            menu_Dial(dialnum_buf);
        }
        break;
    case HAL_KEY_SELECT:
        if(len == 0)
        {
            strcpy((char *)g_jump_buf,EMPTY_CALLRECORD_CHINA);
            menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            MP_StartMenuLibEvt(1000);
        }
        else
        {
            osal_nv_read(NV_ID, (node_info.sel_item-1)*sizeof(Record)+1, sizeof(Record), &record);
            num_buf.len = osal_strlen((char *)record.num);
            num_buf.p[num_buf.len] = '\0';
            osal_memcpy(num_buf.p, record.num, num_buf.len);
            LCD_Clear(0, 1, LCD_LINE_WIDTH, 3);
            LCD_Str_Print(record.num, 0, 1, TRUE);
            LCD_Str_Print(record.time, 0, 2, TRUE);
            LCD_Str_Print(SAVE_CHINA, 0, 3, TRUE);
            LCD_Str_Print(BACK_CHINA, 12, 3, TRUE);
            menu_JumpandMark(MENU_ID_CALLRECORD_DETAIL);
        }
        break;
    case HAL_KEY_BACKSPACE:
        if(shortcuts_flag)
        {
            menu_JumptoMenu(MENU_ID_MAIN);
            shortcuts_flag = FALSE;
        }
        else
        {
            menu_steptoparent();
        }
        break;
    default:
        menu_list_up_down_onkey(keys, len, FALSE);
        break;

    }
}

static void menu_callrecord_delete_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_SELECT)
    {
        /* put the sel_item into first byte of g_jump_buf, and question into others */
        g_jump_buf[0] = node_info.sel_item;
        menu_JumpandMark(MENU_ID_SHOWQUESTION);
    }
    else
    {
        menu_list_onkey( keys,  status);
    }
}
static void     menu_functionlist_onkey(uint8 keys, uint8 status)
{
    if(keys  == HAL_KEY_STAR && FIRSTTIME_INTO_NODE())
    {
        //Menu_handle_msg(MSG_PAD_LOCK, NULL, 0);
        HalSetPadLockStat(PADLOCK_LOCKED);
        menu_JumptoMenu(MENU_ID_MAIN);
        return;
    }
    menu_list_onkey( keys,  status);
}
static void     menu_setting_display(void)
{
    osal_nv_read(MINEAPP_NV_SET_INFORMATION, 0, sizeof(set_info_t), &set_info);
    menu_list_display();
}

static void     menu_setting_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];


    if(keys == HAL_KEY_SELECT)
    {
        uint8 sel_item=0;

        if(node_info.sel_item==MENU_ID_SETTINGS_BACKLIGHT-MENU_ID_SETTINGS_RING)
        {
            //sel_item = LCDGetBackLightCtl();
            sel_item = set_info.backlight_ctl;
        }
        else if(node_info.sel_item==MENU_ID_SETTINGS_PADLOCK-MENU_ID_SETTINGS_RING)
        {
            //sel_item = HalGetPadLockEnable() ? 0 :1 ;
            sel_item = set_info.padlock_ctl;
        }

        if(node_info.sel_item==MENU_ID_SETTINGS_RESTORE_DEFAULT-MENU_ID_SETTINGS_RING)
        {
            strcpy((char *)g_jump_buf,IS_RESTORE_DEFAULT_CHINA);
            menu_JumpandMark(MENU_ID_SHOWQUESTION);
            return;
        }
        menu_steptochild(node.FirstChildID + node_info.sel_item, sel_item);
    }
    else
    {
        menu_list_onkey( keys,  status);
    }
}

static void    menu_setting_timedate_display()
{

    if(node_info.sel_item == 0)  /*show time and date*/
    {
        uint8 p[8];
        GetDateChar(p);
        menu_selectlist_display(p);

        uint8 p_time[TIME_LEN];
        GetFullTimeChar(p_time);
        LCD_Memory_Print(p_time, osal_strlen((char*)p_time), LCD_LINE_WIDTH-osal_strlen((char*)p_time)-2, 2);
    }
#ifdef MENU_CLOCKFORMAT
    else if(node_info.sel_item == 1)  /* time format */
    {
        if(set_info.timeformat_t == TIME_FORMAT_24)
        {
            menu_selectlist_display(TIMEFORMAT24_CHINA);
        }
        else
        {
            menu_selectlist_display(TIMEFORMAT12_CHINA);
        }
    }
#endif
#ifdef 	MENU_TIMEUPDATE_CTL
    else if(node_info.sel_item == 2)  /* automatic time update */
    {
        bool time_autoupdate = GetTimeAutoUpdate();
        if(time_autoupdate)
        {
            menu_selectlist_display(OPEN_CHINA);
        }
        else
        {
            menu_selectlist_display(CLOSE_CHINA);
        }
    }
#endif
}

static void    menu_setting_timedate_onkey(uint8 keys, uint8 status)
{
    if(HAL_KEY_SELECT == keys)
    {
        if(node_info.sel_item == 0) /*set time*/
        {
            menu_setadjusttime();
            menu_JumpandMark(MENU_ID_ADJUSTTIME);
        }
#ifdef MENU_CLOCKFORMAT
        else if(node_info.sel_item == 1) /* set time format */
        {
            menu_JumpandMark(MENU_ID_SETTINGS_TIME_TIMEFORMAT);
        }
#endif
#ifdef 	MENU_TIMEUPDATE_CTL
        else if(node_info.sel_item == 2) /* set time auto update */
        {
            menu_JumpandMark(MENU_ID_SETTINGS_TIME_TIMEAUTOUPDATE);
        }
#endif
    }
    else
    {
        menu_selectlist_onkey(keys, status);
    }
}

static void    menu_ringsetting_display(void)
{

    if(node_info.sel_item == 0)  /* bell select*/
    {
        uint8 p[RING_MAX_BELLNAMELEN];

        HalRingGetBellNameStr(set_info.bell_ring_t, p);
        menu_selectlist_display(p);
    }
    else if(node_info.sel_item == 1) /* sms bell select */
    {
        uint8 p[RING_MAX_BELLNAMELEN];

        HalRingGetSMSNameStr(set_info.sms_ring_t, p);
        menu_selectlist_display(p);
    }
    else if(node_info.sel_item == 2)  /* bell volume */
    {
        uint8 str[8];
        strcpy((char *)str, VOLUME_CHINA);
        str[sizeof(VOLUME_CHINA)-1] = '0'+set_info.bell_gain;
        str[sizeof(VOLUME_CHINA)] = '\0';

        menu_selectlist_display(str);
    }
    else if(node_info.sel_item == 3)  /*shake setting */
    {
        //bool p= HalRingGetShake();
        if(set_info.shake_ctl)
        {
            menu_selectlist_display((uint8 *)ItemList_Bools[1]);
        }
        else
        {
            menu_selectlist_display((uint8 *)ItemList_Bools[0]);
        }
    }
}

static void menu_ringsetting_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
    if(keys == HAL_KEY_SELECT)
    {
        uint8 sel;


        if(node_info.sel_item == 0) /*set bell*/
        {
            sel = set_info.bell_ring_t;
        }
        else if(node_info.sel_item == 1) /*set sms bell */
        {
            sel = set_info.sms_ring_t;
        }
        else if(node_info.sel_item == 2) /* set bell volume */
        {
            sel = 0;
        }
        else if(node_info.sel_item == 3) /* shake */
        {
            sel = set_info.shake_ctl;
        }
        menu_steptochild(node.FirstChildID + node_info.sel_item,sel);
    }
    else
    {
        menu_selectlist_onkey(keys, status);
    }
}

static void menu_bellsettingList_onkey(uint8 keys, uint8 status)
{
    RingName name = (RingName)(node_info.sel_item + (uint8)RING_BELL_1);

    if(HAL_KEY_SELECT == keys)
    {
        if(CurrentNodeID == MENU_ID_SETTINGS_RING_BELLSEL)
        {
            set_info.bell_ring_t = node_info.sel_item;
        }
        else if(CurrentNodeID == MENU_ID_SETTINGS_RING_SMSSEL)
        {
            set_info.sms_ring_t = node_info.sel_item;
        }
        MP_SettingInformation_Handout(&set_info);
        MP_SettingInformation_WriteFlash(&set_info);

        HalRingClose();
        osal_stop_timerEx(Hal_TaskID, HAL_RING_EVENT);
        osal_unset_event(Hal_TaskID, HAL_RING_EVENT);
        //menu_steptoparent();
        strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
        return;
    }
    if(keys== HAL_KEY_BACKSPACE || keys == HAL_KEY_POWER)
    {
        HalRingClose();
        osal_stop_timerEx(Hal_TaskID, HAL_RING_EVENT);
        osal_unset_event(Hal_TaskID,  HAL_RING_EVENT);
    }
    menu_list_onkey(keys, status);
    name = (RingName)(node_info.sel_item + (uint8)RING_BELL_1);
    if(keys == HAL_KEY_UP ||
            keys == HAL_KEY_LEFT  ||
            keys == HAL_KEY_DOWN  ||
            keys == HAL_KEY_RIGHT )
    {
        HalRingOpen(name,OPENFLAG_ASLISTEN);
        osal_start_timerEx(Hal_TaskID, HAL_RING_EVENT, 15);
    }
}

static void menu_shakesetting_onkey(uint8 keys, uint8 status)
{
    if(HAL_KEY_SELECT == keys)
    {
        if(node_info.sel_item == 0)  /* shake on */
        {
            set_info.shake_ctl = FALSE;
            //HalRingSetShake(TRUE);
        }
        else if(node_info.sel_item == 1) /* shake off */
        {
            set_info.shake_ctl = TRUE;
            //HalRingSetShake(FALSE);
        }

        MP_SettingInformation_Handout(&set_info);
        MP_SettingInformation_WriteFlash(&set_info);

        strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
    }
    else
    {
        menu_list_onkey(keys, status);
    }
}

static void menu_padlocksetting_display(void)
{
    //bool padlock_status = FALSE;

    //osal_nv_read(MINEAPP_NV_SET_INFORMATION, PADLOCK_OFFSET, sizeof(uint8), &padlock_status);
    node_info.sel_item = (uint8)set_info.padlock_ctl;
    node_info.high_line = node_info.sel_item + 1;
    menu_list_display();
}

static void  menu_padlocksetting_onkey(uint8 keys, uint8 status)
{
    if(HAL_KEY_SELECT == keys)
    {
        if(node_info.sel_item == 0)  /* padlock on */
        {
            set_info.padlock_ctl = FALSE;
            //HalSetPadLockEnable(FALSE);
        }
        else if(node_info.sel_item == 1) /* padlock off */
        {
            set_info.padlock_ctl = TRUE;
            //HalSetPadLockEnable(TRUE);
        }

        MP_SettingInformation_Handout(&set_info);
        MP_SettingInformation_WriteFlash(&set_info);
        strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
    }
    else
    {
        menu_list_onkey(keys, status);
    }

}
static void    menu_shortmessage_inbox_display(void)
{
    uint8 offset;
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
    Stack_Clear(&global_stack);

#ifdef NEW_DOUBLE_NVID_OP
    osal_nv_read(MINEAPP_NV_SMS1, 0, 1, &offset);
#endif

    if((NearLastNodeID == MENU_ID_SHORTMESSAGE) || (NearLastNodeID == MENU_ID_INCOMINGSMS) ||(NearLastNodeID == MENU_ID_SHORTMESSAGE_DELETE))
    {
        node_info.show_item = offset;
        node_info.sel_item = offset;
        node_info.high_line= 1;
    }
    node.ChildNum = offset;

    LcdClearDisplay();
    LCD_Str_Print(SMS_INCOMINGBOX_CHINA, (LCD_LINE_WIDTH-osal_strlen(SMS_INCOMINGBOX_CHINA))/2, 0, TRUE);

    if(node.ChildNum == 0)
        return;
    else if(node.ChildNum <= SCREEN_LINES)
    {
        uint8 i, j, xpos;

        for(i=node.ChildNum, j=1; i>0; i--, j++)
        {
            uint16 tmp;
            bool SMS_flag;

            tmp = (i-1)*SMS_NV_LEN+SMS_LEN_TYPE_SIZE+1;
#ifdef NEW_DOUBLE_NVID_OP
            menu_doubleNVID_read(SMS_NVID, tmp, APP_NMBRDIGIT,  num_buf.p);
#endif

            num_buf.len = osal_strlen((char*)num_buf.p);
            num_buf.p[num_buf.len] = '\0';
            xpos = LCD_ID_Show(i, 0, j);
            LCD_Str_Print(")", xpos, j, TRUE);

            Contact_Node c_node;
            if(ZSuccess==menu_Contact_SearchContactByNum(&c_node, NULL, num_buf.p))
            {
                LCD_Str_Print(c_node.name, xpos+1, j, TRUE);
            }
            else
            {
                LCD_Str_Print(num_buf.p, xpos+1, j, TRUE);
            }

            tmp = i*SMS_NV_LEN;
#ifdef NEW_DOUBLE_NVID_OP
            menu_doubleNVID_read(SMS_NVID, tmp, 1, &SMS_flag);
#endif

            if(SMS_flag)
                LCD_BigAscii_Print(0x93, LCD_LINE_WIDTH-2, j);
        }
    }
    else
    {
        uint8 i, xpos;

        for(i=0; i<SCREEN_LINES; i++)
        {
            uint16 tmp;
            bool SMS_flag;

            tmp = (node_info.show_item-i-1)*SMS_NV_LEN+SMS_LEN_TYPE_SIZE+1;
#ifdef NEW_DOUBLE_NVID_OP
            menu_doubleNVID_read(SMS_NVID, tmp, APP_NMBRDIGIT, num_buf.p);
#endif

            num_buf.len = osal_strlen((char*)num_buf.p);
            num_buf.p[num_buf.len] = '\0';

            xpos = LCD_ID_Show(node_info.show_item-i, 0, i+1);
            LCD_Str_Print(")", xpos, i+1, TRUE);

            Contact_Node c_node;
            if(ZSuccess==menu_Contact_SearchContactByNum(&c_node, NULL, num_buf.p))
            {
                LCD_Str_Print(c_node.name, xpos+1, i+1, TRUE);
            }
            else
            {
                LCD_Memory_Print(num_buf.p, num_buf.len, xpos+1, i+1);
            }


            tmp = (node_info.show_item-i)*SMS_NV_LEN;
#ifdef NEW_DOUBLE_NVID_OP
            menu_doubleNVID_read(SMS_NVID, tmp, 1, &SMS_flag);
#endif

            if(SMS_flag)
                LCD_BigAscii_Print(0x93, LCD_LINE_WIDTH-2, i+1);
        }
    }
    LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);

}

static void menu_shortmessage_inbox_onkey(uint8 keys, uint8 status)
{
    uint8 len;

#ifdef NEW_DOUBLE_NVID_OP
    osal_nv_read(MINEAPP_NV_SMS1, 0, 1, &len);
#endif

    switch(keys)
    {
    case HAL_KEY_SELECT:
    {
        if(len > 0)
        {
            node_info_temp = node_info;
            Stack_Push(&global_stack, CurrentNodeID, &node_info);
            menu_JumptoMenu(MENU_ID_SHORTMESSAGE_READ);
        }
        else
        {
            strcpy((char *)g_jump_buf,EMPTY_SMSINBOX_CHINA);
            menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            MP_StartMenuLibEvt(1000);
        }
        break;
    }
    case HAL_KEY_BACKSPACE:
    {
        if(shortcuts_flag)
        {
            shortcuts_flag = FALSE;
            menu_JumptoMenu(MENU_ID_MAIN);
        }
        else
        {
            menu_steptoparent();
        }
        break;
    }
    default:
        menu_list_up_down_onkey(keys, len, FALSE);
        break;
    }
}

static void menu_data_line_pos_update(buf_t* data)
{
    uint8 j, finish_len;
    uint8 len = data->len;

    data_info.data_line = 0;
    data_info.page_pos[data_info.data_line++] = 0;
    finish_len = 0;
    while(len >= LCD_LINE_WIDTH)
    {
        j = ascii_scan(data->p + finish_len, LCD_LINE_WIDTH);
        if(j%2 == 1)
        {
            data_info.page_pos[data_info.data_line] = data_info.page_pos[data_info.data_line-1] + LCD_LINE_WIDTH -1;
            finish_len += LCD_LINE_WIDTH -1;
        }
        else
        {
            data_info.page_pos[data_info.data_line] = data_info.page_pos[data_info.data_line -1] + LCD_LINE_WIDTH;
            finish_len += LCD_LINE_WIDTH;
        }
        len = data->len - finish_len;
        //i++;
        data_info.data_line++;
    }
    //data_info.data_line--;
}

static void menu_sms_data_curentline_show(uint8 data_line, uint8 show_line)
{
    uint8 len;
    uint8 * mem = NULL;

    LCD_Line_Clear(show_line);
    len = (data_buf.len > (data_info.page_pos[data_line-1]+LCD_LINE_WIDTH)) ?
          (data_info.page_pos[data_line] - data_info.page_pos[data_line-1]) :
          (data_buf.len - data_info.page_pos[data_line-1]);
    mem = osal_mem_alloc(len+1);
    if (mem)
    {
        osal_memcpy(mem, data_buf.p + data_info.page_pos[data_line-1], len);
        mem[len] = '\0';
        LCD_Memory_Print(mem, len, 0, show_line);
        osal_mem_free(mem);
    }

}
static void    menu_shortmessage_writing_display(void)
{
    if(NULL == data_buf.p)
    {
        data_info.data_pos = 0;
        data_info.cursor_pos = 0;
        data_info.page_id = 0;
        data_info.page_pos [data_info.page_id++]= 0;
        if(NULL == Buffer_Init(&data_buf, APP_SMS_MAX_LEN+1))
            return;
    }
    menu_inputchinese_display();
    LCD_Str_Print(SHORTMESSAGE_CHINA, 5, 0, TRUE);
    LCD_ShowCursor(0, 1);
    if(data_buf.len > 0)
    {
        menu_data_line_pos_update(&data_buf);
        menu_sms_data_curentline_show(data_info.data_line, 1);
        data_info.page_id = data_info.data_line;
        data_info.data_pos = data_buf.len;
        data_info.cursor_pos = data_buf.len - data_info.page_pos[data_info.data_line-1];
        LCD_ShowCursor(data_info.cursor_pos, 1);
    }
}

static bool menu_sms_output_handle(uint8 keys, uint8 input_status)
{

    if(input_status == OUTPUT_STATUS)
    {

        if(keys == HAL_KEY_BACKSPACE)
        {
            if(data_buf.len== 0)
            {
                LCD_CloseCursor();
                Buffer_Free(&data_buf);
                if(CurrentNodeID == MENU_ID_SHORTMESSAGE_REPLY)
                {
                    Stack_Pop(&global_stack, &CurrentNodeID, NULL);
                    menu_display();
                }
#ifdef SMS_TEMPLATE
                else if(CurrentNodeID == MENU_ID_SMS_TEMPLATE_EDIT)
                {
                    menu_JumpBackWithMark();
                }
#endif
#ifdef SMS_SENDBOX
                else if(CurrentNodeID == MENU_ID_SMS_SENDBOX_EDIT)
                {
                    Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
                    menu_display();
                }
#endif
                else
                {
                    menu_steptoparent();
                }
            }
            else if(data_info.data_pos > 0)
            {
                uint8 len =0;
                uint8* mem = NULL;

                len = data_buf.len - data_info.data_pos;
                mem = (uint8*)osal_mem_alloc(len);
                if (mem == NULL)
                    return false;
                if(data_buf.p[data_info.data_pos-1] > 0x80)
                {
                    osal_memcpy(mem, data_buf.p+data_info.data_pos, len);
                    osal_memcpy(data_buf.p+data_info.data_pos-2, mem, len);
                    data_buf.len -= 2;
                    data_buf.p[data_buf.len] = '\0';
                    menu_data_line_pos_update(&data_buf);
                    if(data_info.data_pos == data_info.page_pos[data_info.page_id-1])
                    {
                        if(data_buf.len > 0)
                        {
                            data_info.data_pos -= 2;
                            data_info.cursor_pos = data_info.page_pos[data_info.page_id-1] - data_info.page_pos[data_info.page_id-2] - 2;
                            data_info.page_id--;
                        }
                    }
                    else
                    {
                        data_info.data_pos -= 2;
                        data_info.cursor_pos -= 2;
                    }
                }
                else
                {
                    osal_memcpy(mem, data_buf.p+data_info.data_pos, len);
                    osal_memcpy(data_buf.p+data_info.data_pos-1, mem, len);
                    data_buf.len -= 1;
                    data_buf.p[data_buf.len] = '\0';
                    menu_data_line_pos_update(&data_buf);
                    if(data_info.data_pos == data_info.page_pos[data_info.page_id-1])
                    {
                        if(data_buf.len > 0)
                        {
                            data_info.data_pos -= 1;
                            data_info.cursor_pos =data_info.page_pos[data_info.page_id-1] - data_info.page_pos[data_info.page_id-2] - 1;
                            data_info.page_id--;
                        }
                    }
                    else
                    {
                        data_info.data_pos -= 1;
                        data_info.cursor_pos -= 1;
                    }
                }
                menu_sms_data_curentline_show(data_info.page_id, 1);
                osal_mem_free(mem);
                LCD_ShowCursor(data_info.cursor_pos, 1);
            }
            return TRUE;
        }
        else if(keys == HAL_KEY_SELECT)
        {
            if(data_buf.len > 0)
            {

                LCD_CloseCursor();
                if(CurrentNodeID == MENU_ID_SHORTMESSAGE_WRITINGBOX)
                {
#ifdef SMS_SENDBOX
                    Stack_Push(&global_stack, CurrentNodeID, &node_info);
                    Buffer_Clear(&num_buf);
                    menu_JumptoMenu(MENU_ID_SMS_EDIT_HANDLE);
#else
                    Stack_Push(&global_stack, CurrentNodeID, &node_info);
                    Buffer_Clear(&num_buf);
                    menu_JumptoMenu(MENU_ID_INPUTNUMBER_SMS);
#endif
                }
                else if(CurrentNodeID == MENU_ID_SHORTMESSAGE_REPLY)
                {
                    uint16 offset = (node_info_temp.sel_item-1)*SMS_NV_LEN+SMS_LEN_TYPE_SIZE+1 ;
#ifdef NEW_DOUBLE_NVID_OP
                    menu_doubleNVID_read(SMS_NVID, offset, APP_NMBRDIGIT, num_buf.p);
#endif
                    SET_ON_SM_SENDING();
                    menu_JumptoMenu(MENU_ID_SM_SENDING);
                }
#ifdef SMS_TEMPLATE
                else if(CurrentNodeID == MENU_ID_SMS_TEMPLATE_EDIT)
                {
                    Stack_Push(&global_stack, CurrentNodeID, &node_info_temp);
                    menu_JumptoMenu(MENU_ID_SMS_TEMPLATE_HANDLE);
                }
#endif
#ifdef SMS_SENDBOX
                else if(CurrentNodeID == MENU_ID_SMS_SENDBOX_EDIT)
                {
                    Stack_Push(&global_stack, CurrentNodeID, &node_info_temp);
                    menu_JumptoMenu(MENU_ID_SMS_SENDBOX_HANDLE);
                }
#endif
            }
            return TRUE;
        }
        else if(keys == HAL_KEY_UP)
        {

            if(data_info.page_id> 1)
            {
                //uint8 len;
                uint8 j = ascii_scan(data_buf.p+data_info.page_pos[data_info.page_id-2], data_info.cursor_pos);
                if(j%2 == 1)
                {
                    data_info.cursor_pos --;
                }
                data_info.data_pos = data_info.page_pos[data_info.page_id-2] + data_info.cursor_pos;
                data_info.page_id--;
                if(data_info.cursor_pos == (data_info.page_pos[data_info.page_id] - data_info.page_pos[data_info.page_id-1]))
                {
                    if(data_buf.p[data_info.page_pos[data_info.page_id-1]+data_info.cursor_pos-1] > 0x80)
                    {
                        data_info.cursor_pos -= 2;
                        data_info.data_pos -= 2;
                    }
                    else
                    {
                        data_info.cursor_pos --;
                        data_info.data_pos --;
                    }
                }
                menu_sms_data_curentline_show(data_info.page_id, 1);
                LCD_ShowCursor(data_info.cursor_pos, 1);
            }

            return TRUE;
        }
        else if(keys == HAL_KEY_DOWN)
        {
            if((data_info.page_pos[data_info.page_id-1] + LCD_LINE_WIDTH) <= data_buf.len)
            {
                data_info.page_id++;
                if((data_buf.len - data_info.page_pos[data_info.page_id-1]) < data_info.cursor_pos)
                {
                    data_info.cursor_pos = data_buf.len - data_info.page_pos[data_info.page_id-1];
                    data_info.data_pos = data_info.cursor_pos;
                }
                else
                {
                    uint8 j = ascii_scan(data_buf.p+data_info.page_pos[data_info.page_id-1], data_info.cursor_pos);
                    if(j%2 == 1)
                    {
                        data_info.cursor_pos --;
                    }
                    data_info.data_pos = data_info.page_pos[data_info.page_id-1] + data_info.cursor_pos;
                }
                menu_sms_data_curentline_show(data_info.page_id, 1);
                LCD_ShowCursor(data_info.cursor_pos, 1);
            }
            return TRUE;
        }
        else if(keys == HAL_KEY_RIGHT)
        {
            if(data_info.data_pos < data_buf.len)
            {
                if(data_buf.p[data_info.data_pos] > 0x80)
                {
                    data_info.data_pos += 2;
                    if(data_info.data_pos == data_info.page_pos[data_info.page_id-1] + LCD_LINE_WIDTH - 1) //&& \
                        //(data_info.data_pos < data_buf.len))
                    {
                        if((data_info.data_pos < data_buf.len) && (data_buf.p[data_info.data_pos] > 0x80))
                        {
                            data_info.cursor_pos = 0;
                            data_info.page_id++;
                            menu_sms_data_curentline_show(data_info.page_id, 1);
                        }
                        else if((data_info.data_pos < data_buf.len) && (data_buf.p[data_info.data_pos] < 0x80))
                        {
                            data_info.cursor_pos += 2;
                        }
                        else
                        {
                            data_info.cursor_pos += 2;
                        }
                    }
                    else if(data_info.data_pos == data_info.page_pos[data_info.page_id-1] + LCD_LINE_WIDTH)
                    {
                        data_info.cursor_pos = 0;
                        data_info.page_id++;
                        menu_sms_data_curentline_show(data_info.page_id, 1);
                    }
                    else
                    {
                        data_info.cursor_pos += 2;
                    }

                }
                else
                {
                    data_info.data_pos += 1;
                    if(data_info.data_pos == data_info.page_pos[data_info.page_id-1] + LCD_LINE_WIDTH - 1)
                    {
                        if((data_info.data_pos < data_buf.len) && (data_buf.p[data_info.data_pos] > 0x80))
                        {
                            data_info.cursor_pos = 0;
                            data_info.page_id++;
                            menu_sms_data_curentline_show(data_info.page_id, 1);
                        }
                        else if((data_info.data_pos < data_buf.len) && (data_buf.p[data_info.data_pos] < 0x80))
                        {
                            data_info.cursor_pos += 1;

                        }
                        else
                        {
                            data_info.cursor_pos += 1;
                        }
                    }
                    else if(data_info.data_pos == data_info.page_pos[data_info.page_id-1] + LCD_LINE_WIDTH)
                    {
                        data_info.cursor_pos = 0;
                        data_info.page_id++;
                        menu_sms_data_curentline_show(data_info.page_id, 1);
                    }
                    else
                    {
                        data_info.cursor_pos += 1;
                    }
                }
                LCD_ShowCursor(data_info.cursor_pos, 1);
            }
            return TRUE;
        }
        else if(keys == HAL_KEY_LEFT)
        {
            if(data_info.data_pos > 0)
            {

                if(data_buf.p[data_info.data_pos-1] > 0x80)
                {
                    if(data_info.data_pos == data_info.page_pos[data_info.page_id-1])
                    {
                        data_info.cursor_pos = data_info.page_pos[data_info.page_id-1] - data_info.page_pos[data_info.page_id-2] - 2;
                        data_info.page_id--;
                        menu_sms_data_curentline_show(data_info.page_id, 1);
                    }
                    else
                    {
                        data_info.cursor_pos -= 2;
                    }
                    data_info.data_pos -= 2;

                }
                else
                {
                    if(data_info.data_pos == data_info.page_pos[data_info.page_id-1])
                    {
                        data_info.cursor_pos = data_info.page_pos[data_info.page_id-1] - data_info.page_pos[data_info.page_id-2]- 1;
                        data_info.page_id--;
                        menu_sms_data_curentline_show(data_info.page_id, 1);
                    }
                    else
                    {
                        data_info.cursor_pos -= 1;
                    }
                    data_info.data_pos -= 1;
                }
                LCD_ShowCursor(data_info.cursor_pos, 1);
            }
            return TRUE;
        }
        else if(keys == HAL_KEY_CANCEL)
        {
            LcdClearDisplay();
            return TRUE;
        }
    }

    return FALSE;
}


static void menu_shortmessage_writing_onkey(uint8 keys, uint8 status)
{
    static uint8 input_status = OUTPUT_STATUS;
    uint8 *output_p = NULL;

    //it will return after handle the character of sms
    if(menu_sms_output_handle(keys, input_status))
        return;

    //input new character
    input_status = menu_inputchinese_onkey(keys, status);

    //if the status of input function is OUTPUT_STATUS, the new character
    //should be print on the LCD
    if(input_status == OUTPUT_STATUS)
    {
        output_p = menu_ChineseOutput();
        if(menu_ChineseOutput_Length() > 0)
        {
            uint8 len, len_output = 0;

            if((data_buf.len >= APP_SMS_MAX_LEN-1) || ((data_buf.len >= APP_SMS_MAX_LEN-2)&&(output_p[0]>0x80)))
            {
                strcpy((char *)g_jump_buf, SMS_WRITING_FULL_CHINA);
                menu_JumpandMark(MENU_ID_SHOWMESSAGE);
                return;
            }

            len_output = osal_strlen((char*)output_p);
            if(data_info.data_pos < data_buf.len)
            {
                uint8* mem_temp;

                len = data_buf.len - data_info.data_pos;
                mem_temp = (uint8*)osal_mem_alloc(len);
                if (mem_temp)
                {
                    osal_memcpy(mem_temp, &data_buf.p[data_info.data_pos], len);
                    osal_memcpy(&data_buf.p[data_info.data_pos+len_output], mem_temp, len);
                    osal_mem_free(mem_temp);
                }
            }
            osal_memcpy(&data_buf.p[data_info.data_pos], output_p, len_output);

            data_buf.len += len_output;
            menu_data_line_pos_update(&data_buf);

            data_info.data_pos += len_output;

            if(data_info.data_pos == (data_info.page_pos[data_info.page_id-1] + LCD_LINE_WIDTH))
            {
                data_info.page_id++;
                data_info.cursor_pos = 0;
            }
            else if(data_info.data_pos > (data_info.page_pos[data_info.page_id-1] + LCD_LINE_WIDTH))
            {
                data_info.page_id++;
                data_info.cursor_pos = len_output;
            }
            else if(data_info.data_pos == (data_info.page_pos[data_info.page_id-1] + LCD_LINE_WIDTH - 1))
            {
                if((data_buf.len > data_info.data_pos) && (data_buf.p[data_info.data_pos] > 0x80))
                {
                    data_info.page_id++;
                    data_info.cursor_pos = 0;
                }
                else
                {
                    data_info.cursor_pos += len_output;
                }
            }
            else
            {
                data_info.cursor_pos += len_output;
            }
            menu_sms_data_curentline_show(data_info.page_id, 1);
            menu_ChineseOutputClear();

        }
        LCD_ShowCursor(data_info.cursor_pos, 1);
    }
}
#ifdef SMS_SENDBOX
void menu_sms_edit_handle_display(void)
{
    menu_list_display();
}
void menu_sms_edit_handle_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_SELECT)
    {
        if(node_info.sel_item == 0)//only send
        {
            Stack_Push(&global_stack, CurrentNodeID, &node_info);
            Buffer_Clear(&num_buf);
            menu_JumptoMenu(MENU_ID_INPUTNUMBER_SMS);
        }
        else if(node_info.sel_item == 1)//save and send
        {
            uint8 offset;

            osal_nv_read(MINEAPP_NV_SMS_SENDBOX, 0, sizeof(uint8), &offset);
            if(offset > MAX_SMS_NUM_SENDBOX-1)
            {
                strcpy((char *)g_jump_buf,FULL_SENDBOX_CHINA);
                menu_JumpandMark(MENU_ID_SHOWMESSAGE);
                MP_StartMenuLibEvt(1000);

            }
            else
            {
                osal_nv_write(MINEAPP_NV_SMS_SENDBOX, 1+APP_SMS_MAX_LEN*offset, 1, &data_buf.len);
                osal_nv_write(MINEAPP_NV_SMS_SENDBOX, 1+APP_SMS_MAX_LEN*offset+1, data_buf.len, data_buf.p);
                offset++;
                osal_nv_write(MINEAPP_NV_SMS_SENDBOX, 0, sizeof(uint8), &offset);
                Stack_Push(&global_stack, CurrentNodeID, NULL);
                Buffer_Clear(&num_buf);
                menu_JumptoMenu(MENU_ID_INPUTNUMBER_SMS);
            }

        }
        else if(node_info.sel_item == 2)//only save
        {
            uint8 offset;

            osal_nv_read(MINEAPP_NV_SMS_SENDBOX, 0, sizeof(uint8), &offset);
            if(offset > MAX_SMS_NUM_SENDBOX-1)
            {
                strcpy((char *)g_jump_buf,FULL_SENDBOX_CHINA);
                menu_JumpandMark(MENU_ID_SHOWMESSAGE);
                MP_StartMenuLibEvt(1000);

            }
            else
            {
                osal_nv_write(MINEAPP_NV_SMS_SENDBOX, 1+APP_SMS_MAX_LEN*offset, 1, &data_buf.len);
                osal_nv_write(MINEAPP_NV_SMS_SENDBOX, 1+APP_SMS_MAX_LEN*offset+1, data_buf.len, data_buf.p);
                offset++;
                osal_nv_write(MINEAPP_NV_SMS_SENDBOX, 0, sizeof(uint8), &offset);
                Buffer_Free(&data_buf);
                NearLastNodeID = CurrentNodeID;
                Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
                menu_steptoparent();
            }
        }
        else if(node_info.sel_item == 3)//drop
        {
            Buffer_Free(&data_buf);
            NearLastNodeID = CurrentNodeID;
            Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
            menu_steptoparent();
        }
        return;
    }
    else if(keys == HAL_KEY_BACKSPACE)
    {
        //Buffer_Free(&data_buf);
        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        //menu_steptoparent();
        return;
    }

    menu_list_onkey(keys, status);
}
#endif

#ifdef SMS_TEMPLATE
void menu_sms_template_display(void)
{
    uint8 offset, len;
    //uint8* p;

    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    Stack_Clear(&global_stack);
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8 *)node.name, (LCD_LINE_WIDTH-osal_strlen(node.name))/2, 0, TRUE);
    }
    else
    {
        for(uint8 i=1; i<SCREEN_LINES+1; i++)
        {
            LCD_ListLine_Clear(i);
        }
    }

    if(NULL == data_buf.p)
    {
        if(NULL == Buffer_Init(&data_buf, APP_SMS_MAX_LEN+1))
            return;
    }

    osal_nv_read(MINEAPP_NV_SMS_TEMPLATE, 0, 1, &offset);

    node.ChildNum = offset;
    //template_name_len = 8;
    // p = (uint8*)osal_mem_alloc(template_name_len+1);//malloc a memory for template name display

    if(node.ChildNum == 0)
        return;
    else if(node.ChildNum <= SCREEN_LINES)
    {
        for(uint8 i=1; i<=node.ChildNum; i++)
        {
            uint8 tmp;

            osal_nv_read(MINEAPP_NV_SMS_TEMPLATE, 1+APP_SMS_MAX_LEN*(i-1), sizeof(uint8), &data_buf.len);
            osal_nv_read(MINEAPP_NV_SMS_TEMPLATE, 1+APP_SMS_MAX_LEN*(i-1)+sizeof(uint8), data_buf.len, data_buf.p);

            len = (data_buf.len > 10) ? 10 : data_buf.len;
            tmp = ascii_scan(data_buf.p, len);
            if((tmp % 2) == 1)
            {
                LCD_Memory_Print(data_buf.p, len-1, 0, i);
                LCD_Memory_Print("...", 3, len-1, i);
            }
            else
            {
                LCD_Memory_Print(data_buf.p, len, 0, i);
                LCD_Memory_Print("...", 3, len, i);
            }

            //LCD_Memory_Print(p, template_name_len, 0, i);
            //LCD_Memory_Print("...", 3, template_name_len, i);
        }
    }
    else
    {
        for(uint8 i=1; i<=SCREEN_LINES; i++)
        {
            uint8 tmp;

            osal_nv_read(MINEAPP_NV_SMS_TEMPLATE, 1+APP_SMS_MAX_LEN*(node_info.show_item+i-1), sizeof(uint8), &data_buf.len);
            osal_nv_read(MINEAPP_NV_SMS_TEMPLATE, 1+APP_SMS_MAX_LEN*(node_info.show_item+i-1)+sizeof(uint8), data_buf.len, data_buf.p);

            len = (data_buf.len > 10) ? 10 : data_buf.len;
            tmp = ascii_scan(data_buf.p, len);
            if((tmp % 2) == 1)
            {
                LCD_Memory_Print(data_buf.p, len-1, 0, i);
                LCD_Memory_Print("...", 3, len-1, i);
            }
            else
            {
                LCD_Memory_Print(data_buf.p, len, 0, i);
                LCD_Memory_Print("...", 3, len, i);
            }

            //osal_nv_read(MINEAPP_NV_SMS_TEMPLATE, 1+SMS_NV_LEN*(node_info.show_item+i-1)+1, template_name_len, p);
            //LCD_Memory_Print(p, template_name_len, 0, i);
            //LCD_Memory_Print("...", 3, template_name_len, i);
        }
    }
    //osal_mem_free(p);
    //LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);

    LCD_ListLine_Inv(node_info.high_line);

    if(FIRSTTIME_INTO_NODE())
    {
        LCD_ProgBar_open();
    }
    LCD_ProgBar_update(node_info.sel_item, node.ChildNum);

}

static void menu_sms_template_onkey(uint8 keys, uint8 status)
{
    uint8 len;

    osal_nv_read(MINEAPP_NV_SMS_TEMPLATE, 0, 1, &len);

    switch(keys)
    {
    case HAL_KEY_SELECT:
    {
        node_info_temp = node_info;
        menu_JumpandMark(MENU_ID_SMS_TEMPLATE_EDIT);
        break;
    }
    case HAL_KEY_BACKSPACE:
    {
        Buffer_Free(&data_buf);
        menu_steptoparent();
        break;
    }
    default:
        menu_list_up_down_onkey(keys, len, TRUE);
        break;
    }
}


static void menu_sms_template_edit_display(void)
{
    if(NULL == data_buf.p)
    {
        data_info.data_pos = 0;
        data_info.cursor_pos = 0;
        data_info.page_id = 0;
        data_info.page_pos [data_info.page_id++]= 0;
        if(NULL == Buffer_Init(&data_buf, APP_SMS_MAX_LEN+1))
            return;
    }
    osal_nv_read(MINEAPP_NV_SMS_TEMPLATE, 1+APP_SMS_MAX_LEN*(node_info_temp.sel_item), 1, &data_buf.len);
    osal_nv_read(MINEAPP_NV_SMS_TEMPLATE, 1+APP_SMS_MAX_LEN*(node_info_temp.sel_item)+1, data_buf.len, data_buf.p);
    menu_shortmessage_writing_display();
}

/*
static void menu_sms_template_edit_onkey(uint8 keys, uint8 status)
{
menu_shortmessage_writing_onkey(keys, status);
}

*/
void menu_sms_template_handle_display(void)
{
    menu_list_display();
}
void menu_sms_template_handle_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_SELECT)
    {
        if(node_info.sel_item == 0)
        {
            Stack_Push(&global_stack, CurrentNodeID, NULL);
            Buffer_Clear(&num_buf);
            menu_JumptoMenu(MENU_ID_INPUTNUMBER_SMS);
        }
        else if(node_info.sel_item == 1)
        {
            osal_nv_write(MINEAPP_NV_SMS_TEMPLATE, 1+APP_SMS_MAX_LEN*node_info_temp.sel_item, 1, &data_buf.len);
            osal_nv_write(MINEAPP_NV_SMS_TEMPLATE, 1+APP_SMS_MAX_LEN*node_info_temp.sel_item+1, data_buf.len, data_buf.p);
            Buffer_Free(&data_buf);
            menu_JumpBackWithMark();
        }
        else if(node_info.sel_item == 2)
        {
            //menu_JumptoMenu(MENU_ID_SMS_TEMPLATE_EDIT);
            Buffer_Free(&data_buf);
            menu_JumpBackWithMark();
        }
        return;
    }
    else if(keys == HAL_KEY_BACKSPACE)
    {
        //Buffer_Free(&data_buf);
        /*
        NearLastNodeID = CurrentNodeID;
        CurrentNodeID = MENU_ID_SMS_TEMPLATE;
        node_info = node_info_temp;*/
        Buffer_Free(&data_buf);
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        //menu_JumpBackWithMark();
        return;
    }

    menu_list_onkey(keys, status);
}
#endif
#ifdef SMS_SENDBOX
void menu_sms_sendbox_display(void)
{
    uint8 offset;
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
    //Stack_Clear(&global_stack);
    osal_nv_read(MINEAPP_NV_SMS_SENDBOX, 0, sizeof(uint8), &offset);
    if((NearLastNodeID == MENU_ID_SHORTMESSAGE) || (NearLastNodeID == MENU_ID_SMS_SENDBOX_DELETE))// ||(NearLastNodeID == MENU_ID_SHORTMESSAGE_DELETE))
    {
        node_info.show_item = offset;
        node_info.sel_item = offset;
        node_info.high_line= 1;
    }

    if(NULL == data_buf.p)
    {
        if(NULL == Buffer_Init(&data_buf, APP_SMS_MAX_LEN+1))
            return;
    }

    LcdClearDisplay();
    LCD_Str_Print(SMS_SENDBOX_CHINA, (LCD_LINE_WIDTH-osal_strlen(SMS_SENDBOX_CHINA))/2, 0, TRUE);

    node.ChildNum = offset;
    if(node.ChildNum == 0)
        return;
    else if(node.ChildNum <= SCREEN_LINES)
    {
        uint8 i, j, xpos, len;
        uint16 tmp;

        for(i=node.ChildNum, j=1; i>0; i--, j++)
        {
            tmp = (i-1)*APP_SMS_MAX_LEN+SMS_LEN_TYPE_SIZE+1;
            osal_nv_read(MINEAPP_NV_SMS_SENDBOX, tmp-1, 1, &data_buf.len);
            osal_nv_read(MINEAPP_NV_SMS_SENDBOX, tmp, data_buf.len, data_buf.p);
            data_buf.p[data_buf.len] = '\0';
            xpos = LCD_ID_Show(i, 0, j);
            LCD_Str_Print(")", xpos, j, TRUE);
            len = (data_buf.len > (LCD_LINE_WIDTH-(xpos+1))) ? (LCD_LINE_WIDTH-(xpos+1)) : data_buf.len;
            tmp = ascii_scan(data_buf.p, len);
            if((tmp % 2) == 1)
                LCD_Memory_Print(data_buf.p, len-1, xpos+1, j);
            else
                LCD_Memory_Print(data_buf.p, len, xpos+1, j);
        }
    }
    else
    {
        uint8 i, xpos, len;
        uint16 tmp;

        for(i=0; i<SCREEN_LINES; i++)
        {

            tmp = (node_info.show_item-i-1)*APP_SMS_MAX_LEN+SMS_LEN_TYPE_SIZE+1;
            osal_nv_read(MINEAPP_NV_SMS_SENDBOX, tmp-1, 1, &data_buf.len);
            osal_nv_read(MINEAPP_NV_SMS_SENDBOX, tmp, data_buf.len, data_buf.p);
            data_buf.p[data_buf.len] = '\0';
            xpos = LCD_ID_Show(node_info.show_item-i, 0, i+1);
            LCD_Str_Print(")", xpos, i+1, TRUE);
            len = (data_buf.len > (LCD_LINE_WIDTH-(xpos+1))) ? (LCD_LINE_WIDTH-(xpos+1)) : data_buf.len;
            tmp = ascii_scan(data_buf.p, len);
            if((tmp % 2) == 1)
                LCD_Memory_Print(data_buf.p, len-1, xpos+1, i+1);
            else
                LCD_Memory_Print(data_buf.p, len, xpos+1, i+1);
        }
    }
    LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);

}

static void menu_sms_sendbox_onkey(uint8 keys, uint8 status)
{
    uint8 len;

    osal_nv_read(MINEAPP_NV_SMS_SENDBOX, 0, 1, &len);

    switch(keys)
    {
    case HAL_KEY_SELECT:
    {
        if(len > 0)
        {
            node_info_temp = node_info;
            Stack_Push(&global_stack, CurrentNodeID, &node_info);
            menu_JumptoMenu(MENU_ID_SMS_SENDBOX_EDIT);
        }
        else
        {
            strcpy((char *)g_jump_buf,EMPTY_SENDBOX_CHINA);
            menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            MP_StartMenuLibEvt(1000);
        }
        break;
    }
    case HAL_KEY_BACKSPACE:
    {
        if(shortcuts_flag)
        {
            shortcuts_flag = FALSE;
            menu_JumptoMenu(MENU_ID_MAIN);
        }
        else
        {
            Buffer_Free(&data_buf);
            menu_steptoparent();
        }
        break;
    }
    default:
        menu_list_up_down_onkey(keys, len, FALSE);
        break;
    }
}

static void menu_sms_sendbox_edit_display(void)
{
    if(NULL == data_buf.p)
    {
        data_info.data_pos = 0;
        data_info.cursor_pos = 0;
        data_info.page_id = 0;
        data_info.page_pos [data_info.page_id++]= 0;
        if(NULL == Buffer_Init(&data_buf, APP_SMS_MAX_LEN+1))
            return;
    }
    osal_nv_read(MINEAPP_NV_SMS_SENDBOX, 1+APP_SMS_MAX_LEN*(node_info_temp.sel_item-1), 1, &data_buf.len);
    osal_nv_read(MINEAPP_NV_SMS_SENDBOX, 1+APP_SMS_MAX_LEN*(node_info_temp.sel_item-1)+1, data_buf.len, data_buf.p);
    data_buf.p[data_buf.len] = '\0';
    menu_shortmessage_writing_display();
}

void menu_sms_sendbox_handle_display(void)
{
    menu_list_display();
}
void menu_sms_sendbox_handle_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_SELECT)
    {
        if(node_info.sel_item == 0)//send
        {
            Stack_Push(&global_stack, CurrentNodeID, NULL);
            NearLastNodeID = CurrentNodeID;
            Buffer_Clear(&num_buf);
            menu_JumptoMenu(MENU_ID_INPUTNUMBER_SMS);
        }
        else if(node_info.sel_item == 1)//delete
        {
            Stack_Push(&global_stack, CurrentNodeID, NULL);
            menu_JumptoMenu(MENU_ID_SMS_SENDBOX_DELETE);
            //menu_steptochild(uint8 ID, uint8 sel_item);
            /*
            uint8 data[SMS_NV_LEN];
            uint8 tmp;

            osal_nv_read(MINEAPP_NV_SMS_SENDBOX, 0, sizeof(uint8), &tmp);
            for(uint8 i=node_info_temp.sel_item; i<tmp; i++)
            {
            osal_nv_read(MINEAPP_NV_SMS_SENDBOX, i*SMS_NV_LEN+1, SMS_NV_LEN, data);
            osal_nv_write(MINEAPP_NV_SMS_SENDBOX, (i-1)*SMS_NV_LEN+1, SMS_NV_LEN, data);
            }
            tmp--;
            osal_nv_write(MINEAPP_NV_SMS_SENDBOX, 0, sizeof(uint8), &tmp);
            Buffer_Free(&data_buf);
            NearLastNodeID = CurrentNodeID;
            Stack_Pop(&global_stack, NULL, NULL);
            Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
            menu_display();
            */
        }
        else if(node_info.sel_item == 2)//cancel
        {
            Buffer_Free(&data_buf);
            NearLastNodeID = CurrentNodeID;
            Stack_Pop(&global_stack, NULL, NULL);
            Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
            menu_display();
        }
        return;
    }
    else if(keys == HAL_KEY_BACKSPACE)
    {
        //Buffer_Free(&data_buf);
        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        return;
    }

    menu_list_onkey(keys, status);
}

void menu_sms_sendbox_delete_display(void)
{
    LcdClearDisplay();
    LCD_Str_Print(SMS_DELETE_CHINA, 3, 1, TRUE);
    LCD_Str_Print(CONFIRM_CHINA, 0, 3, TRUE);
    LCD_Str_Print(CANCEL_CHINA, 12, 3, TRUE);
}

void menu_sms_sendbox_delete_onkey(uint8 keys, uint8 status)
{
    uint8 offset;
    uint8 data[SMS_NV_LEN];

    switch(keys)
    {
    case HAL_KEY_SELECT:
        LcdClearDisplay();
        LCD_Str_Print_Pixel(DELETEING_CHINA, (LCD_LINE_WIDTH-osal_strlen(LOG_CHINA))/2, 1.5*LCD_LINE_HIGH);
        osal_nv_read(MINEAPP_NV_SMS_SENDBOX, 0, 1, &offset);
        for(uint8 i=node_info_temp.sel_item; i<offset; i++)
        {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
            FeedWatchDog();
#endif
            osal_nv_read(MINEAPP_NV_SMS_SENDBOX, i*APP_SMS_MAX_LEN+1, APP_SMS_MAX_LEN, data);
            osal_nv_write(MINEAPP_NV_SMS_SENDBOX, (i-1)*APP_SMS_MAX_LEN+1, APP_SMS_MAX_LEN, data);

        }
        --offset;
        osal_nv_write(MINEAPP_NV_SMS_SENDBOX, 0, sizeof(uint8), &offset);
        Buffer_Free(&data_buf);
        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, NULL, NULL);
        Stack_Pop(&global_stack, NULL, NULL);
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        break;
    case HAL_KEY_BACKSPACE:
        Stack_Pop(&global_stack, NULL, NULL);
        Stack_Pop(&global_stack, &CurrentNodeID, NULL);
        menu_display();
        break;
    default:
        break;
    }
}
#endif

void menu_sms_handle_display(void)
{
    menu_list_display();
}
void menu_sms_handle_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_SELECT)
    {
        Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
#ifdef NEW_DOUBLE_NVID_OP
        osal_nv_read(MINEAPP_NV_SMS1, 0, 1, &node.ChildNum);
#endif

        CurrentNodeID = node.FirstChildID + node_info.sel_item;
        if(CurrentNodeID == MENU_ID_SHORTMESSAGE_REPLY)
            Buffer_Free(&data_buf);
        menu_display();
        return;
    }
    else if(keys == HAL_KEY_BACKSPACE)
    {
        Stack_Pop(&global_stack, &CurrentNodeID, NULL);
        menu_display();
        //menu_JumpBackWithMark();
        return;
    }

    menu_list_onkey(keys, status);
}

static uint8  menu_sms_data_show(uint8 data_line, uint8 lcd_line)
{
    while((lcd_line <= SCREEN_LINES) && (data_line <= data_info.data_line))
    {
        menu_sms_data_curentline_show(data_line++, lcd_line++);
    }
    return (data_line-1);
}

static void    menu_shortmessage_read_display(void)
{
    bool SMS_flag = TRUE;
    uint16 len_temp = 0;

    //new_sms_flag = FALSE;
    //osal_nv_write(MINEAPP_NV_SMS, sizeof(uint8)+ MAX_SMS_NUM*SMS_NV_LEN, sizeof(uint8), &new_sms_flag);
    node_info = node_info_temp;

    if(NULL == data_buf.p)
    {
        if(NULL == Buffer_Init(&data_buf, APP_SMS_MAX_LEN+1))
            return;
        data_info.page_id = 1;
    }

    len_temp = (node_info.sel_item-1)*SMS_NV_LEN + 1;
#ifdef NEW_DOUBLE_NVID_OP
    menu_doubleNVID_read(SMS_NVID, len_temp, 1, &data_buf.len);
    menu_doubleNVID_read(SMS_NVID, len_temp+SMS_LEN_TYPE_SIZE+APP_NMBRDIGIT, data_buf.len, data_buf.p);
    menu_doubleNVID_read(SMS_NVID, len_temp+SMS_LEN_TYPE_SIZE, APP_NMBRDIGIT, num_buf.p);
#endif

    len_temp = node_info.sel_item*SMS_NV_LEN + sizeof(uint8);
#ifdef NEW_DOUBLE_NVID_OP
    menu_doubleNVID_read(SMS_NVID, len_temp-sizeof(uint8), 1, &SMS_flag);
#endif

    if(SMS_flag)
    {
        SMS_flag = FALSE;
#ifdef NEW_DOUBLE_NVID_OP
        menu_doubleNVID_write(SMS_NVID, len_temp-sizeof(uint8), 1, &SMS_flag);
#endif

        len_temp = sizeof(uint8)+ MAX_SMS_NUM*SMS_NV_LEN+ sizeof(uint8);
#ifdef NEW_DOUBLE_NVID_OP
        menu_doubleNVID_read(SMS_NVID, len_temp -sizeof(uint8), sizeof(uint8), &unread_sms);
#endif
        --unread_sms;
#ifdef NEW_DOUBLE_NVID_OP
        menu_doubleNVID_write(SMS_NVID, len_temp-sizeof(uint8), 1, &unread_sms);
#endif
    }

    num_buf.len = osal_strlen((char*)num_buf.p);
    num_buf.p[num_buf.len] = '\0';

    LcdClearDisplay();
    Contact_Node c_node;
    if(ZSuccess==menu_Contact_SearchContactByNum(&c_node, NULL, num_buf.p))
    {
        LCD_Str_Print(c_node.name,(LCD_LINE_WIDTH-num_buf.len)/2, 0,TRUE);
    }
    else
    {
        LCD_Str_Print(num_buf.p,(LCD_LINE_WIDTH-num_buf.len)/2, 0,TRUE);
        //LCD_Memory_Print(num_buf.p, num_buf.len, (LCD_LINE_WIDTH-num_buf.len)/2, 0);
    }

    if(data_buf.len > 0)
    {
        menu_data_line_pos_update(&data_buf);
        data_info.page_id = menu_sms_data_show(1, 1);
        len_temp = 5;
    }

}

static void menu_shortmessage_read_onkey(uint8 keys, uint8 status)
{

    switch(keys)
    {
    case HAL_KEY_SELECT:
        Stack_Push(&global_stack, CurrentNodeID, NULL);
        menu_JumptoMenu(MENU_ID_SHORTMESSAGE_HANDLE);
        break;
    case HAL_KEY_BACKSPACE:
        Buffer_Free(&data_buf);
        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        //menu_JumpBackWithMark();
        break;
    case HAL_KEY_UP:
        if(data_info.page_id > SCREEN_LINES)
        {
            LCD_Clear(0, 1, LCD_LINE_WIDTH, 3);
            data_info.page_id--;
            menu_sms_data_show(data_info.page_id-(SCREEN_LINES-1), 1);
        }
        break;
    case HAL_KEY_DOWN:
        if(data_info.page_id < data_info.data_line)
        {
            LCD_Clear(0, 1, LCD_LINE_WIDTH, 3);
            data_info.page_id ++;
            menu_sms_data_show(data_info.page_id-(SCREEN_LINES-1), 1);
        }
        break;
    default:
        break;
    }
}

void menu_sms_reply_display(void)
{
    menu_shortmessage_writing_display();
}

void menu_sms_reply_onkey(uint8 keys, uint8 status)
{
    menu_shortmessage_writing_onkey(keys, status);
}

void menu_sms_delete_display(void)
{
    LcdClearDisplay();
    LCD_Str_Print(SMS_DELETE_CHINA, 3, 1, TRUE);
    LCD_Str_Print(CONFIRM_CHINA, 0, 3, TRUE);
    LCD_Str_Print(CANCEL_CHINA, 12, 3, TRUE);
}

void menu_sms_delete_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_SELECT:
        LcdClearDisplay();
        LCD_Str_Print_Pixel(DELETEING_CHINA, (LCD_LINE_WIDTH-osal_strlen(LOG_CHINA))/2, 1.5*LCD_LINE_HIGH);
#ifdef NEW_DOUBLE_NVID_OP
        {
            menu_doubleNVID_delete(SMS_NVID, node_info_temp.sel_item-1);
        }
#endif
        Buffer_Free(&data_buf);
        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, &CurrentNodeID, NULL);
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        break;
    case HAL_KEY_BACKSPACE:
        Stack_Pop(&global_stack, &CurrentNodeID, NULL);
        menu_display();
        break;
    default:
        break;
    }
}
void menu_sms_deleteall_display(void)
{
    LcdClearDisplay();
    LCD_Str_Print(SMS_DELETEALL_CHINA, 2, 1, TRUE);
    LCD_Str_Print(CONFIRM_CHINA, 0, 3, TRUE);
    LCD_Str_Print(CANCEL_CHINA, 12, 3, TRUE);
}

void menu_sms_deleteall_onkey(uint8 keys, uint8 status)
{

    switch(keys)
    {
    case HAL_KEY_SELECT:
        LcdClearDisplay();
        LCD_Str_Print_Pixel(DELETEING_CHINA, (LCD_LINE_WIDTH-osal_strlen(LOG_CHINA))/2, 1.5*LCD_LINE_HIGH);
#ifdef NEW_DOUBLE_NVID_OP
        {
            uint8 len = 0;
            menu_doubleNVID_write(SMS_NVID, 0, 1, &len);
            unread_sms = 0;
            uint16 len_temp = sizeof(uint8)+ MAX_SMS_NUM*SMS_NV_LEN+ sizeof(uint8);
            menu_doubleNVID_write(SMS_NVID, len_temp -sizeof(uint8), sizeof(uint8), &unread_sms);
        }
#endif
        Buffer_Free(&data_buf);
        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, &CurrentNodeID, NULL);
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        break;
    case HAL_KEY_BACKSPACE:
        Stack_Pop(&global_stack, &CurrentNodeID, NULL);
        menu_display();
        break;
    default:
        break;
    }
}

static void    menu_contact_handle_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    if(keys == HAL_KEY_BACKSPACE)
    {

        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        //menu_JumpBackWithMark();
        return;
    }
    else if(keys == HAL_KEY_SELECT)
    {
        uint8 contact_num = 0;

#ifdef NEW_DOUBLE_NVID_OP
        menu_Contact_ReadContactNum(&contact_num);
#endif

        if(node.FirstChildID + node_info.sel_item == MENU_ID_CONTACT_ADD)
        {
            if(contact_num >= MAX_CONTACT_NUM)
            {
                strcpy((char *)g_jump_buf,FULL_CONTACTLIST_CHINA);
                menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
                MP_StartMenuLibEvt(1000);
            }
            else
            {
                Buffer_Clear(&num_buf);
                Stack_Push(&global_stack, CurrentNodeID, &node_info);
                menu_JumptoMenu(MENU_ID_INPUTNUMBER_CONTACT);
            }
            return;
        }
        else if(node.FirstChildID + node_info.sel_item == MENU_ID_CONTACT_DELETE)
        {
            if(contact_num == 0)
            {
                strcpy((char *)g_jump_buf,EMPTY_CONTACTLIST_CHINA);
                menu_JumpandMark(MENU_ID_SHOWMESSAGE);
                MP_StartMenuLibEvt(1000);
            }
            else
            {
                menu_JumptoMenu(MENU_ID_CONTACT_DELETE);
            }
            return;
        }
        else if(node.FirstChildID + node_info.sel_item == MENU_ID_CONTACT_DETAIL)
        {
            if(contact_num == 0)
            {
                strcpy((char *)g_jump_buf,EMPTY_CONTACTLIST_CHINA);
                menu_JumpandMark(MENU_ID_SHOWMESSAGE);
                MP_StartMenuLibEvt(1000);
            }
            else
            {
                menu_JumptoMenu(MENU_ID_CONTACT_DETAIL);
            }
            return;
        }
    }
    else
    {
        menu_list_onkey(keys, status);
    }
}

static void    menu_contact_detail_display(void)
{
    Contact_Node c_node;

    LCD_Clear(13, 0, LCD_LINE_WIDTH, 3);
#ifdef NEW_DOUBLE_NVID_OP
    menu_Contact_ReadContact(&c_node, node_info_temp.sel_item);
#endif
    LCD_Str_Print(c_node.name, 0, 1, TRUE);
    LCD_Str_Print(c_node.num, 0, 2, TRUE);
    num_buf.len = osal_strlen((char*)c_node.num);
    osal_memcpy(num_buf.p, c_node.num, num_buf.len);
    num_buf.p[num_buf.len] = '\0';
}

static void    menu_contact_detail_onkey(uint8 keys, uint8 status)
{

    switch(keys)
    {
    case HAL_KEY_CALL:
        Buffer_Copy(&dialnum_buf, &num_buf);
        menu_Dial(dialnum_buf);
        break;
    case HAL_KEY_BACKSPACE:
        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        //menu_JumpBackWithMark();
        break;
    default:
        break;
    }
}


static void    menu_contact_delete_display(void)
{
    Contact_Node c_node;

    LcdClearDisplay();
#ifdef NEW_DOUBLE_NVID_OP
    menu_Contact_ReadContact(&c_node,node_info_temp.sel_item);
#endif
    LCD_Str_Print(DELETE_CHINA, 6, 0, TRUE);
    LCD_Str_Print(c_node.name, (LCD_LINE_WIDTH-osal_strlen((char *)c_node.name))/2, 2, TRUE);
    LCD_Str_Print("?", (LCD_LINE_WIDTH+osal_strlen((char *)c_node.name))/2, 2, TRUE);
    LCD_Str_Print(CONFIRM_CHINA, 0, 3, TRUE);
    LCD_Str_Print(CANCEL_CHINA, 12, 3, TRUE);
}

static void    menu_contact_delete_onkey(uint8 keys, uint8 status)
{

    switch(keys)
    {
    case HAL_KEY_SELECT:
        LcdClearDisplay();
        LCD_Str_Print_Pixel(DELETEING_CHINA, (LCD_LINE_WIDTH-osal_strlen(LOG_CHINA))/2, 1.5*LCD_LINE_HIGH);

#ifdef NEW_DOUBLE_NVID_OP
        {
            menu_Contact_DeleteContact(node_info_temp.sel_item);
        }
#endif
        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        break;
    case HAL_KEY_BACKSPACE:
        NearLastNodeID = CurrentNodeID;
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        break;
    default:
        break;
    }
}
void menu_backlightsetting_onkey(uint8 keys, uint8 status)
{
    if(HAL_KEY_SELECT == keys)
    {
        //LCDSetBackLightCtl(node_info.sel_item);
        set_info.backlight_ctl = node_info.sel_item;
        MP_SettingInformation_Handout(&set_info);
        MP_SettingInformation_WriteFlash(&set_info);
        strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
    }
    else
    {
        menu_list_onkey(keys, status);
    }

}
void menu_bellvolsetting_display(void)
{
    //uint8 volsetting;

    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    //osal_nv_read(MINEAPP_NV_SET_INFORMATION, VOLUME_OFFSET, sizeof(uint8), &volsetting);

    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8 *)node.name, (LCD_LINE_WIDTH-osal_strlen(node.name))/2, 0, TRUE);
        //HalRingGetGain(RING_TYPE_BELL, &volsetting);
    }

    LCD_BigAscii_Print(0x90, 2, 2);
    LCD_BigAscii_Print(0x8B, 3, 2);
    for(uint8 i=0; i<set_info.bell_gain; i++)
    {
        LCD_BigAscii_Print(0x0D, 5+2*i, 2);
        LCD_BigAscii_Print(0x0D, 5+2*i+1, 2);
    }
    for(uint8 i=set_info.bell_gain; i<4; i++)
    {
        LCD_BigAscii_Print(0x00, 5+2*i, 2);
        LCD_BigAscii_Print(0x00, 5+2*i+1, 2);
    }
}
void menu_bellvolsetting_onkey(uint8 keys, uint8 status)
{
    // uint8 volsetting;

    //osal_nv_read(MINEAPP_NV_SET_INFORMATION, VOLUME_OFFSET, sizeof(uint8), &volsetting);

    switch(keys)
    {
    case HAL_KEY_SELECT:
        //HalRingSetGain(RING_TYPE_BELL, volsetting);
        MP_SettingInformation_Handout(&set_info);
        MP_SettingInformation_WriteFlash(&set_info);

        HalRingClose();
        osal_stop_timerEx(Hal_TaskID, HAL_RING_EVENT);
        osal_unset_event(Hal_TaskID,  HAL_RING_EVENT);
        strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
        break;
    case HAL_KEY_BACKSPACE:
        HalRingClose();
        osal_stop_timerEx(Hal_TaskID,  HAL_RING_EVENT);
        osal_unset_event(Hal_TaskID,  HAL_RING_EVENT);
        menu_steptoparent();
        break;
    case HAL_KEY_POWER:
        HalRingClose();
        osal_stop_timerEx(Hal_TaskID,  HAL_RING_EVENT);
        osal_unset_event(Hal_TaskID,  HAL_RING_EVENT);
        break;
    case HAL_KEY_RIGHT:
    case HAL_KEY_DOWN:
        if(set_info.bell_gain < MAX_GAIN_LEVEL-1)
        {
            set_info.bell_gain++;
            MP_SettingInformation_Handout(&set_info);
            MP_SettingInformation_WriteFlash(&set_info);

        }
        NearLastNodeID = CurrentNodeID;
        HalRingOpen((RingName) HalRingGetBellName(),OPENFLAG_ASLISTEN);
        osal_start_timerEx(Hal_TaskID, HAL_RING_EVENT, 15);
        menu_display();
        break;
    case HAL_KEY_LEFT:
    case HAL_KEY_UP:
        if(set_info.bell_gain > 0)
        {
            set_info.bell_gain--;
            MP_SettingInformation_Handout(&set_info);
            MP_SettingInformation_WriteFlash(&set_info);

        }
        NearLastNodeID = CurrentNodeID;
        HalRingOpen((RingName) HalRingGetBellName(),OPENFLAG_ASLISTEN);
        osal_start_timerEx(Hal_TaskID, HAL_RING_EVENT, 15);
        menu_display();
        break;
    default:
        break;
    }
}
static void menu_tools_display(void)
{
    uint8 p[12];

    if(node_info.sel_item == 0)  /*show date*/
    {
        GetDateChar(p);
    }
#ifdef MP_INFORMATION
    else if(node_info.sel_item == 1)  /*show mp number*/
    {
        uint16 number;
        number = BUILD_UINT16( MP_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE], MP_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);
        _ltoa(number , p, 10);
    }
    else if(node_info.sel_item == 2)  /* show channel */
    {
        if(MP_IsNwkOn())
        {
            _ltoa(MP_DevInfo.Channel, p, 10);
        }
        else
        {
#ifdef MENU_RF_DEBUG
            _ltoa( MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL], p, 10);
#else
            _ltoa( MP_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL], p, 10);
#endif
        }
    }
    else if(node_info.sel_item == 3)  /* show panid */
    {

        if(MP_IsNwkOn())
        {
            _ltoa(MP_DevInfo.CoordPanID, p, 10);
        }
        else
        {
            LCD_Clear(0, 3, LCD_LINE_WIDTH-2, 3);
            osal_memset(p, '\0', 7);
            osal_memcpy(p, MP_NO_PANID_CHINA, osal_strlen((char*)MP_NO_PANID_CHINA));
        }

    }
    else if(node_info.sel_item == 4)  /* show software version */
    {
        strcpy((char *)p, MP_SW_VERSION);
    }
#endif
    menu_selectlist_display(p);
    if(node_info.sel_item == 0)  /*show date*/
    {
        uint8 week_str[7];
        GetWeekChar(week_str);
        LCD_Memory_Print(week_str, osal_strlen((char*)week_str), LCD_LINE_WIDTH-osal_strlen((char*)week_str)-2, 2);
    }
}

static void menu_tools_onkey(uint8 keys, uint8 status)
{

    if( (keys == HAL_KEY_RIGHT)  ||
            (keys == HAL_KEY_UP)       ||
            (keys == HAL_KEY_DOWN)  ||
            (keys == HAL_KEY_LEFT))
    {
        menu_selectlist_onkey(keys, status);
    }
    else if(keys == HAL_KEY_BACKSPACE)
    {
        menu_steptoparent();
    }
}
#ifdef MENU_OAD_UPDATE
static void menu_oad_setpanid_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        Buffer_Clear(&num_buf);
        LCD_Str_Print(OAD_SETPANID_CHINA, 0, 0, TRUE);
    }
    LCD_Line_Clear(2);
    LCD_Str_Print(num_buf.p , 6 , 1, TRUE);
    LCD_ShowCursor(6 + num_buf.len, 1);
}

static void menu_oad_setpanid_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_SELECT:
    {
        if(num_buf.p)
        {
            MP_OadUpdate.panid = atoul(num_buf.p);
        }
        menu_JumpandMark(MENU_ID_OAD_SET_MSGTYPE);
        break;
    }
    case HAL_KEY_BACKSPACE:
    {
        Buffer_Clear(&num_buf);
        menu_steptoparent();
        break;
    }
    default:
    {
        meu_oad_inputnumber_onkey(keys,  status);
        break;
    }
    }
}
static void menu_oad_setshortaddr_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        Buffer_Clear(&num_buf);
        LCD_Str_Print(OAD_SETSHORTADDR_CHINA, 0, 0, TRUE);
    }
    LCD_Line_Clear(2);
    LCD_Str_Print(num_buf.p , 6 , 1, TRUE);
    LCD_ShowCursor(6 + num_buf.len, 1);
}
static void menu_oad_setshortaddr_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_SELECT:
    {
        MP_OadUpdate.shortAddr = atoul(num_buf.p);
        menu_JumptoMenu(MENU_ID_OAD_SET_MSGTYPE);
        break;
    }
    case HAL_KEY_BACKSPACE:
    {
        Buffer_Clear(&num_buf);
        menu_JumpBackMarkParent();
        break;
    }
    default:
    {
        meu_oad_inputnumber_onkey(keys,  status);
        break;
    }
    }
}
static void menu_oad_setmsgtype_display(void)
{
    menu_list_display();
}
static void menu_oad_setmsgtype_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_SELECT:
    {
        MP_OadUpdate.msgidx = node_info.sel_item;
        menu_JumptoMenu(MENU_ID_OAD_SET_VERSION);
        break;
    }
    case HAL_KEY_BACKSPACE:
    {
        Buffer_Clear(&num_buf);
        menu_JumpBackMarkParent();
        break;
    }
    default:
    {
        menu_list_onkey(keys, status);
        break;
    }
    }
}
static void menu_oad_setversion_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        Buffer_Clear(&num_buf);
        LCD_Str_Print(OAD_SETVERSION_CHINA, 0, 0, TRUE);
    }
    LCD_Line_Clear(2);
    LCD_Str_Print(num_buf.p , 6 , 1, TRUE);
    LCD_ShowCursor(6 + num_buf.len, 1);

}
static void menu_oad_setversion_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_SELECT:
    {
        MP_OadUpdate.version = atoul(num_buf.p);
        menu_JumptoMenu(MENU_ID_OAD_RESULT);
        break;
    }
    case HAL_KEY_BACKSPACE:
    {
        Buffer_Clear(&num_buf);
        menu_JumpBackMarkParent();
        break;
    }
    default:
    {
        meu_oad_inputnumber_onkey(keys,  status);
        break;
    }
    }

}

static void menu_oad_result_display(void)
{
    char buf[8];
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        _itoa(MP_OadUpdate.panid, buf, 10);
        LCD_Str_Print("号码:", 0, 0, TRUE);
        LCD_Str_Print(buf, sizeof("号码:"), 0, TRUE);

        LCD_Str_Print("类型:", 0, 2, TRUE);
        LCD_Str_Print(ItemList_OAD_SetMsgType[MP_OadUpdate.msgidx], sizeof("类型:"), 2, TRUE);

        _itoa(MP_OadUpdate.version, buf, 10);
        LCD_Str_Print("版本:", 0, 3, TRUE);
        LCD_Str_Print(buf,sizeof("版本:"), 3, TRUE);

        /* start OAD */
        uint16 oad_devid = MP_OadUpdate.panid;
        switch(MP_OadUpdate.msgidx)
        {
        case 0:   //定位主站
        {
            MP_OadUpdate.panid = oad_devid;
            MP_OadUpdate.shortAddr = 0;
            MP_OadUpdate.msgtype = TOF_LOC_STATION_OAD;
            break;
        }
        case 1:   //定位辅站
        {
            MP_OadUpdate.panid = 0xFFF2;
            MP_OadUpdate.shortAddr = oad_devid;
            MP_OadUpdate.msgtype = TOF_LOCATOR_OAD;
            break;
        }
        case 2:   //通信基站
        {
            MP_OadUpdate.panid = oad_devid;
            MP_OadUpdate.shortAddr = 0;
            MP_OadUpdate.msgtype = TOF_COM_STATION_OAD;
            break;
        }
        default:
            MP_OadUpdate.msgtype = 0;

        }

        if(MP_OadUpdate.panid!=0xFFFF && MP_OadUpdate.shortAddr!=0xFFFF
                && MP_OadUpdate.msgtype>=TOF_CARD_OAD && MP_OadUpdate.msgtype<=TOF_COM_STATION_OAD)
        {
            MP_OadUpdate.sendcnt = 0;
            if(osal_start_timerEx(MP_Function_TaskID, MP_FUNC_OADUPDATE_EVENT, 5)!=ZSuccess)
            {
                SystemReset();
            }
        }
        else
        {
            LcdClearDisplay();
            LCD_Str_Print(INVALID_PARAM, 0, 0, TRUE);

            for(uint8 i=0; i<6; i++)
            {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
                FeedWatchDog();
#endif
                DelayMs(500);
            }
            LcdClearDisplay();
            menu_JumpBackMarkParent();
            return;
        }
    }
    LCD_Line_Clear(3);

    _itoa(MP_OadUpdate.version, buf, 10);
    LCD_Str_Print("VER:", 0, 3, TRUE);
    LCD_Str_Print(buf,sizeof("VER:"), 3, TRUE);

    uint8 x = 13;
    _itoa(MP_OadUpdate.sendcnt, buf, 10);
    LCD_Str_Print(buf, x, 3, TRUE);
}
static void menu_oad_result_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_BACKSPACE:
    {
        Buffer_Clear(&num_buf);
        osal_stop_timerEx(MP_Function_TaskID, MP_FUNC_OADUPDATE_EVENT);
        osal_unset_event(MP_Function_TaskID, MP_FUNC_OADUPDATE_EVENT);
        LcdClearDisplay();
        menu_JumpBackMarkParent();
        break;
    }
    }
}
static void meu_oad_inputnumber_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_0:
    case HAL_KEY_1:
    case HAL_KEY_2:
    case HAL_KEY_3:
    case HAL_KEY_4:
    case HAL_KEY_5:
    case HAL_KEY_6:
    case HAL_KEY_7:
    case HAL_KEY_8:
    case HAL_KEY_9:
        if(num_buf.len < APP_NMBRDIGIT - 1)
        {
            num_buf.p[num_buf.len++] = MP_Key2ASCII(keys);
            num_buf.p[num_buf.len] = '\0';
            NearLastNodeID = CurrentNodeID;
            menu_display();
        }
        break;
    default:
        break;
    }
}
#endif

#ifdef MENU_CLOCKFORMAT
static void menu_timeformat_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        if(GetTimeFormat() == TIME_FORMAT_12)
        {
            node_info.high_line = 1;
            node_info.sel_item = 0;
            node_info.show_item = 0;
        }
        else
        {
            node_info.high_line = 2;
            node_info.sel_item = 1;
            node_info.show_item = 0;
        }
    }
    menu_list_display();
}

static void menu_timeformat_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_SELECT)
    {
        if(node_info.sel_item == 0) /* 12 clock format */
        {
            set_info.timeformat_t = TIME_FORMAT_12;
            //SetTimeFormat(TIME_FORMAT_12);
        }
        else if(node_info.sel_item == 1) /* 24 clock format */
        {
            set_info.timeformat_t = TIME_FORMAT_24;
            //SetTimeFormat(TIME_FORMAT_24);
        }
        MP_SettingInformation_Handout(&set_info);
        MP_SettingInformation_WriteFlash(&set_info);

        //menu_JumpBackWithMark();
        strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
    }
    else if(keys == HAL_KEY_BACKSPACE)
    {
        menu_JumpBackWithMark();
    }
    else
    {
        menu_list_onkey(keys, status);
    }
}
#endif
#ifdef 	MENU_TIMEUPDATE_CTL
static void  menu_timeautoupdate_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        if(GetTimeAutoUpdate())
        {
            node_info.high_line = 2;
            node_info.sel_item = 1;
            node_info.show_item = 0;
        }
        else
        {
            node_info.high_line = 1;
            node_info.sel_item = 0;
            node_info.show_item = 0;
        }
    }
    menu_list_display();
}
static void menu_timeautoupdate_onkey(uint8 keys, uint8 status)
{
    if(HAL_KEY_SELECT == keys)
    {
        if(node_info.sel_item == 0)  /* auto udpate off*/
        {
            set_info.time_autoupdate = FALSE;
            //HalSetPadLockEnable(FALSE);
        }
        else if(node_info.sel_item == 1) /* auto update on */
        {
            set_info.time_autoupdate = TRUE;
            //HalSetPadLockEnable(TRUE);
        }

        MP_SettingInformation_Handout(&set_info);
        MP_SettingInformation_WriteFlash(&set_info);

        strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
    }
    else if(keys == HAL_KEY_BACKSPACE)
    {
        menu_JumpBackWithMark();
    }
    else
    {
        menu_list_onkey(keys, status);
    }
}
#endif
static void    menu_selectlist_display(uint8* pVal)
{
    char *name;
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8 *)node.name, (LCD_LINE_WIDTH-osal_strlen(node.name))/2, 0, TRUE);
    }
    else
    {
        for(uint8 i=1; i<SCREEN_LINES+1; i++)
        {
            LCD_ListLine_Clear(i);
        }
    }
    //LCD_Str_Print((uint8 *)node.name, (LCD_LINE_WIDTH-osal_strlen(node.name))/2, 0, TRUE);

    name = node.ItemName_CH[node_info.sel_item];
    LCD_Str_Print((uint8 *)name, 0, 1, TRUE);

    LCD_Str_Print(pVal, LCD_LINE_WIDTH - osal_strlen((char *)pVal)-2, 3, TRUE);

    if(FIRSTTIME_INTO_NODE())
    {
        LCD_ProgBar_open();
    }
    LCD_ProgBar_update(node_info.sel_item, node.ChildNum);

}

#ifdef MENU_RF_DEBUG

static void menu_set_channel_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        Buffer_Clear(&num_buf);
        LCD_Str_Print(INPUT_CHANNEL_CHINA, 0, 0, TRUE);
    }
    LCD_Line_Clear(2);
    LCD_Str_Print(num_buf.p , 6 , 1, TRUE);
    LCD_ShowCursor(6+num_buf.len, 1);
}

static void menu_set_channel_onkey(uint8 keys, uint8 status)
{

    switch(keys)
    {
    case HAL_KEY_SELECT:
    {
        uint32 channel = 0;
        for(uint8 i=0; i<num_buf.len; i++)
            channel = 10*channel + (num_buf.p[i] - '0');
        if((channel > 26) || (channel < 11))
        {
            strcpy((char *)g_jump_buf,INVALID_CHANNEL_CHINA);
            menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            MP_StartMenuLibEvt(1000);
            return;
        }
        uint8 u8channel = (uint8)(channel);
        if(ZSUCCESS !=  osal_nv_write( ZCD_NV_SET_CHANLIST, 0, sizeof(uint8), &u8channel))
        {
            Buffer_Clear(&num_buf);
            menu_steptoparent();
            break;
        }
        else
        {
            SystemReset();
        }
        break;
    }
    case HAL_KEY_BACKSPACE:
        Buffer_Clear(&num_buf);
        menu_steptoparent();
        break;
    case HAL_KEY_0:
    case HAL_KEY_1:
    case HAL_KEY_2:
    case HAL_KEY_3:
    case HAL_KEY_4:
    case HAL_KEY_5:
    case HAL_KEY_6:
    case HAL_KEY_7:
    case HAL_KEY_8:
    case HAL_KEY_9:
        if(num_buf.len < APP_NMBRDIGIT - 1)
        {
            num_buf.p[num_buf.len++] = MP_Key2ASCII(keys);
            num_buf.p[num_buf.len] = '\0';
            NearLastNodeID = CurrentNodeID;
            menu_display();
        }
        break;
    default:
        break;
    }
}

static void menu_set_panid_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        Buffer_Clear(&num_buf);
        LCD_Str_Print(INPUT_PANID_CHINA, 0, 0, TRUE);
    }
    LCD_Line_Clear(2);
    LCD_Str_Print(num_buf.p , 5 , 1, TRUE);
    LCD_ShowCursor(5+num_buf.len, 1);
}

static void menu_set_panid_onkey(uint8 keys, uint8 status)
{

    switch(keys)
    {
    case HAL_KEY_SELECT:
    {
        uint16 pan_id = 0;

        for(uint8 i=0; i<num_buf.len; i++)
            pan_id = 10*pan_id + (num_buf.p[i] - '0');

        if((pan_id > 0X3FFF && pan_id != 0xFFFF))
        {
            strcpy((char *)g_jump_buf,INVALID_PANID_CHINA);
            menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            MP_StartMenuLibEvt(1000);
            return;
        }

        if(ZSUCCESS !=  osal_nv_write( ZCD_NV_SET_PANID, 0, sizeof(uint16), &pan_id))
        {
            Buffer_Clear(&num_buf);
            menu_steptoparent();
            break;
        }
        else
        {
            SystemReset();
        }
    }
    case HAL_KEY_BACKSPACE:
        Buffer_Clear(&num_buf);
        menu_steptoparent();
        break;
    case HAL_KEY_0:
    case HAL_KEY_1:
    case HAL_KEY_2:
    case HAL_KEY_3:
    case HAL_KEY_4:
    case HAL_KEY_5:
    case HAL_KEY_6:
    case HAL_KEY_7:
    case HAL_KEY_8:
    case HAL_KEY_9:
        if(num_buf.len < APP_NMBRDIGIT - 1)
        {
            num_buf.p[num_buf.len++] = MP_Key2ASCII(keys);
            num_buf.p[num_buf.len] = '\0';
            NearLastNodeID = CurrentNodeID;
            menu_display();
        }
        break;
    default:
        break;
    }
}
#endif

static void    menu_selectlist_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    switch(keys)
    {
    case HAL_KEY_SELECT:
        menu_steptochild(node.FirstChildID + node_info.sel_item,0);  /* select the first one by default*/
        break;
    case HAL_KEY_BACKSPACE:
        menu_steptoparent();
        break;
    case HAL_KEY_UP:
    case HAL_KEY_LEFT:
        NearLastNodeID = CurrentNodeID;
        if(node_info.sel_item >0)
        {
            node_info.sel_item--;
        }
        else
        {
            node_info.sel_item  = node.ChildNum-1;
        }
        menu_display();
        break;
    case HAL_KEY_DOWN:
    case HAL_KEY_RIGHT:

        NearLastNodeID = CurrentNodeID;
        if(node_info.sel_item < node.ChildNum-1)
        {
            node_info.sel_item++;
        }
        else
        {
            node_info.sel_item = 0;
        }
        menu_display();
        break;
    default:
        break;
    }

}
/*
void Nodeinfo_Push(void)
{
if(node_info_stack_i < MENU_TREE_DEPTH)
{
node_info_stack[node_info_stack_i++] = node_info;
}
}

void Nodeinfo_Pop(void)
{
if(node_info_stack_i>0)
{
node_info =  node_info_stack[--node_info_stack_i];
}
else
{
MENU_RESET_NODEINFO();
}
}
*/


