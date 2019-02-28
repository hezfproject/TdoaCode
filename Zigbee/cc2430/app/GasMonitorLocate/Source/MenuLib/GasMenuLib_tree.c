#include "App_cfg.h"
#include "hal_key.h"
#include "TimeUtil.h"
#include "drivers.h"
#include "string.h"
#include "ch4.h"
#include "OSAL_Nv.h"

#include "GasMonitor_MenuLib.h"
#include "GasMonitor.h"
#include "GasMenuLib_tree.h"
#include "GasMenuLib_global.h"
#include "GasMonitor_sms.h"
#include "GasMonitor_MenuLibChinese.h"
#include "MenuAdjustUtil.h"
#include "WatchDogUtil.h"
#include "OnBoard.h"
#include "ZGlobals.h"
#include "Nwk.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchdogUtil.h"
#endif
#include "lcd_interface.h"
#include "temperature.h"
#include "stdio.h"
#include "app_cfg.h"
#include "osal.h"
#include "Delay.h"
#include "Beeper.h"

#include "Locate_Uart.h"

#define MAXIDNUM          99
#define MENU_SMS_PAGE_MAX          5
#define MENU_TREE_DEPTH         	 4
#define IsNumberEqual(termNbr_t1, termNbr_t2) (osal_strcmp(termNbr_t1, termNbr_t2) == 0)

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
    char* p;
    uint8 len;
} charbuf_t;

typedef struct
{
    uint8 smsSelNum;
    uint8 smsStartLine;
    uint8 smsDelFlag;
} gassms_info_t;

typedef struct
{
    uint16*  CardID;
    uint8*  CardSiglevel;
    uint8     IDnum;
} card_buf_t;

/* gas sms info */
charbuf_t 		     gassms_buf = {0,NULL};

static gassms_info_t gassms_info;

/*static variable*/
static  set_info_t     set_info;
//static  node_info_t   node_info_temp = {1,0,0};
/* setting Volume */

//static uint8             volsetting;
static stack_p_t       tree_stack_p[MENU_TREE_DEPTH];
static stack_t          tree_stack;
static bool 		zeroadjust_flag;
static GasAlarmType Alarmtype;
static card_buf_t CardInfo;
static gassms_info_t  gassms_info;
/* for card check */
#ifdef CFG_GAS_CARDCHECK
static uint16       gas_cardcheck_num;
#endif
/*general functions */
static void  menu_tree_stack_init(void);
static void menu_list_display(void);
static void menu_list_onkey(uint8 keys, uint8 status);
#ifdef CFG_GAS_INFORMATION
static void    menu_selectlist_display(uint8* pVal);
static void    menu_selectlist_onkey(uint8 keys, uint8 status);
#endif
static uint8 menu_ShowItem2StorageNum(uint8 totalCnt, uint8 show_item);
static bool menu_FillCardInfo(uint16 shortAddr, uint8 CardSiglevel);

/*identical functions */
static void menu_functionlist_onkey(uint8 keys, uint8 status);
static void menu_zeroadjust_display(void);
static void menu_zeroadjust_onkey(uint8 keys, uint8 status);
static void menu_zeroadjust_result_display(void);
static void menu_zeroadjust_result_onkey(uint8 keys, uint8 status);
static void menu_calibration_display(void);
static void menu_calibration_onkey(uint8 keys, uint8 status);
static void menu_calibration_result_display(void);
static void menu_calibration_result_onkey(uint8 keys, uint8 status);

#ifdef CFG_GAS_SHORTMESSAGE
static void menu_shortmessage_numjudge_display(void);
static void menu_shortmessage_display(void);
static void menu_shortmessage_onkey(uint8 keys, uint8 status);
static void menu_shortmessage_read_display(void);
static void menu_shortmessage_read_onkey(uint8 keys, uint8 status);
static void menu_shortmessage_handle_onkey(uint8 keys, uint8 status);
static void menu_shortmessage_delresult_display(void);
#endif

static void menu_temperature_display(void);
static void menu_temperature_onkey(uint8 keys, uint8 status);
static void menu_settings_display(void);
static void menu_settings_onkey(uint8 keys, uint8 status);
static void menu_settings_backlight_display(void);
static void menu_settings_backlight_onkey(uint8 keys, uint8 status);
static void menu_setting_timedate_display(void);
static void menu_setting_timedate_onkey(uint8 keys, uint8 status);
static void menu_timeformat_display(void);
static void menu_timeformat_onkey(uint8 keys, uint8 status);
#ifdef CFG_GAS_INFORMATION
static void menu_tools_display(void);
static void menu_tools_onkey(uint8 keys, uint8 status);
#endif


static void menu_SOSAlarm_display(void);
static void menu_SOSAlarm_onkey(uint8 keys, uint8 status);
static void menu_SOSAlarm_alert_display(void);
static void menu_SOSAlarm_alert_onkey(uint8 keys, uint8 status);
static void menu_CardSearch_display(void);
#ifdef CFG_GAS_CARDCHECK
static void menu_CardCheck_display(void);
static void menu_CardCheck_onkey(uint8 keys, uint8 status);
#endif
static void menu_CardSearch_result_display(void);
static void menu_CardSearch_result_onkey(uint8 keys, uint8 status);

static void    menu_locate_display(void);

static char*  const __code  ItemList_FunctionList[] =
{
#ifdef CFG_GAS_SHORTMESSAGE
    SHORTMESSAGE_CHINA,
#endif
    ZEROADJUST_CHINA,CALIBRATION_CHINA,/*ACCIDENTALARM_CHINA,*/

    LOCATEINFORMATION_CHINA,

    TEMPERATURE_CHINA,SOSALARM_CHINA,
    CARDSEARCH_CHINA,
#ifdef CFG_GAS_CARDCHECK
    CARDCHECK_CHINA,
#endif
    SETTINGS_CHINA,
#ifdef CFG_GAS_INFORMATION
    TOOLS_CHINA,
#endif
};
static char*  const __code  ItemList_BacklightSettings[] = {CLOSE_CHINA,SECONDS_10_CHINA,SECONDS_20_CHINA,SECONDS_30_CHINA};//,NOLIMIT_CHINA};

static char*  const __code  ItemList_SettingsList[] = {OVERDENSITY_CHINA,BACKLIGHTTIME_CHINA,TEMPERATURE_CALIB_CHINA,TIMESETTING_CHINA};

static char*  const __code  ItemList_TimeSetting[]= {TIMEDATE_CHINA,TIMEFORMAT_CHINA};

static char*  const __code  ItemList_TimeFormat[] = {TIMEFORMAT12_CHINA,TIMEFORMAT24_CHINA};

static char*  const __code  ItemList_SOSALARM[]= {FIRE_CHINA,WATER_CHINA,ROOF_CHINA,OTHERS_CHINA};

#ifdef CFG_GAS_SHORTMESSAGE
static char*  const __code  ItemList_ShortMsgHandle[] = {DELETE_CHINA,DELETEALL_CHINA};
#endif
#ifdef CFG_GAS_INFORMATION
static char*  const __code  ItemList_tools[] = {GAS_NUMBER_CHINA, GAS_CHANNEL_CHINA, GAS_SW_VERSION_CHINA};
#endif
//static char*  const __code  ItemList_Bools[] = {CLOSE_CHINA, OPEN_CHINA};

static Tree_node_t  const __code Menu_Tree[] =
{
    /* Function List */
    {
        .ID = MENU_ID_FUNCTIONLIST,
        .name = FUNCTIONLIST_CHINA,
        .ParentID = MENU_ID_MAIN,
#ifdef CFG_GAS_SHORTMESSAGE
        .FirstChildID = MENU_ID_SHORTMESSAGE_NUMJUDGE,
#else
        .FirstChildID = MENU_ID_ZEROADJUST,
#endif
        .ChildNum = sizeof(ItemList_FunctionList)/sizeof(ItemList_FunctionList[0]),
        .ItemName_CH = ItemList_FunctionList,
        .oper.display = menu_list_display,
        .oper.on_key = menu_functionlist_onkey,
    }
    ,
#ifdef CFG_GAS_SHORTMESSAGE
    {
        .ID = MENU_ID_SHORTMESSAGE_NUMJUDGE,
        .name = NULL,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_shortmessage_numjudge_display,
        .oper.on_key = NULL
    }
    ,
#endif

    {
        .ID = MENU_ID_ZEROADJUST,
        .name = ZEROADJUST_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_zeroadjust_display,
        .oper.on_key = menu_zeroadjust_onkey,
    }
    ,
    {
        .ID = MENU_ID_CALIBRATION,
        .name = CALIBRATION_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_calibration_display,
        .oper.on_key = menu_calibration_onkey,
    }
    ,
    /*
    {
    .ID = MENU_ID_ACCIDENTALARM,
    .name = ACCIDENTALARM_CHINA,
    .ParentID = MENU_ID_FUNCTIONLIST,
    .FirstChildID = NULL,
    .ChildNum = 0,
    .ItemName_CH = NULL,
    .oper.display = NULL,
    .oper.on_key = NULL,
    }
    ,
    */
#if 1
    {
        .ID = MENU_ID_LOCATE,
        .name = LOCATEINFORMATION_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_locate_display,
        .oper.on_key = menu_functionlist_onkey,
    }
    ,
#endif

    {
        .ID = MENU_ID_TEMPERATURE,
        .name = TEMPERATURE_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_temperature_display,
        .oper.on_key = menu_temperature_onkey,
    }
    ,
    {
        .ID = MENU_ID_SOSALARM,
        .name = SOSALARM_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = MENU_ID_SOSALARM_ALERT,
        .ChildNum = sizeof(ItemList_SOSALARM)/sizeof(ItemList_SOSALARM[0]),
        .ItemName_CH =ItemList_SOSALARM,
        .oper.display = menu_SOSAlarm_display,
        .oper.on_key = menu_SOSAlarm_onkey,
    }
    ,
    {
        .ID = MENU_ID_CARDSEARCH,
        .name = CARDSEARCH_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID =NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_CardSearch_display,
        .oper.on_key = NULL,
    }
    ,
#ifdef CFG_GAS_CARDCHECK
    {
        .ID = MENU_ID_CARDCHECK,
        .name = CARDCHECK_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID =NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_CardCheck_display,
        .oper.on_key = menu_CardCheck_onkey,
    }
    ,
#endif

    {
        .ID = MENU_ID_SETTINGS,
        .name = SETTINGS_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = MENU_ID_SETTINGS_OVERDENSITY,
        .ChildNum = sizeof(ItemList_SettingsList)/sizeof(ItemList_SettingsList[0]),
        .ItemName_CH = ItemList_SettingsList,
        .oper.display = menu_settings_display,
        .oper.on_key = menu_settings_onkey,
    }
    ,
#ifdef CFG_GAS_INFORMATION
    {
        .ID = MENU_ID_TOOLS,
        .name = TOOLS_CHINA,
        .ParentID = MENU_ID_FUNCTIONLIST,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_tools)/sizeof(ItemList_tools[0]),
        .ItemName_CH = ItemList_tools,
        .oper.display = menu_tools_display,
        .oper.on_key = menu_tools_onkey,

    }
    ,
#endif

#ifdef CFG_GAS_SHORTMESSAGE
    {
        .ID = MENU_ID_SHORTMESSAGE,
        .name = SHORTMESSAGE_CHINA,
        .ParentID = MENU_ID_SHORTMESSAGE_NUMJUDGE,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_shortmessage_display,
        .oper.on_key = menu_shortmessage_onkey,
    }
    ,
#endif
    {
        .ID = MENU_ID_ZEROADJUST_RESULT,
        .name = NULL,
        .ParentID = MENU_ID_ZEROADJUST,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_zeroadjust_result_display,
        .oper.on_key = menu_zeroadjust_result_onkey,
    }
    ,
    {
        .ID = MENU_ID_CALIBRATION_SETDENSITY,
        .name = NULL,
        .ParentID = MENU_ID_CALIBRATION,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_calibration_setdensity_display,
        .oper.on_key = menu_calibration_setdensity_onkey,
    }
    ,
    {
        .ID = MENU_ID_CALIBRATION_RESULT,
        .name = NULL,
        .ParentID = MENU_ID_ZEROADJUST,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_calibration_result_display,
        .oper.on_key = menu_calibration_result_onkey,
    }
    ,
#ifdef CFG_GAS_SHORTMESSAGE
    {
        .ID = MENU_ID_SHORTMESSAGE_READ,
        .name = NULL,
        .ParentID = MENU_ID_SHORTMESSAGE,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_shortmessage_read_display,
        .oper.on_key = menu_shortmessage_read_onkey,
    }
    ,
    {
        .ID = MENU_ID_SHORTMESSAGE_HANDLE,
        .name = SHORTMESSAGE_CHINA,
        .ParentID = NULL,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_ShortMsgHandle)/sizeof(ItemList_ShortMsgHandle[0]),
        .ItemName_CH = ItemList_ShortMsgHandle,
        .oper.display = menu_list_display,
        .oper.on_key = menu_shortmessage_handle_onkey,
    }
    ,
    {
        .ID = MENU_ID_SHORTMESSAGE_DELETE_RESULT,
        .name = NULL,
        .ParentID = NULL,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_shortmessage_delresult_display,
        .oper.on_key = NULL,
    }
    ,
#endif
    {
        .ID = MENU_ID_SETTINGS_OVERDENSITY,
        .name = OVERDENSITY_CHINA,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_settings_overdensity_display,
        .oper.on_key = menu_settings_overdensity_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS_BACKLIGHT,
        .name = BACKLIGHTTIME_CHINA,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_BacklightSettings)/sizeof(ItemList_BacklightSettings[0]),
        .ItemName_CH = ItemList_BacklightSettings,
        .oper.display = menu_settings_backlight_display,
        .oper.on_key = menu_settings_backlight_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS_TEMPERATURE,
        .name = TEMPERATURE_CALIB_CHINA,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_settings_temperature_display,
        .oper.on_key = menu_settings_temperature_onkey,
    }
    ,
    {
        .ID = MENU_ID_SETTINGS_TIME,
        .name = TIMESETTING_CHINA,
        .ParentID = MENU_ID_SETTINGS,
        .FirstChildID = NULL,
        .ChildNum = sizeof(ItemList_TimeSetting)/sizeof(ItemList_TimeSetting[0]),
        .ItemName_CH = ItemList_TimeSetting,
        .oper.display = menu_setting_timedate_display,
        .oper.on_key = menu_setting_timedate_onkey,
    }
    ,
    {
        .ID = MENU_ID_SOSALARM_ALERT,
        .name =NULL,
        .ParentID = MENU_ID_SOSALARM,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_SOSAlarm_alert_display,
        .oper.on_key = menu_SOSAlarm_alert_onkey,

    }
    ,
    {
        .ID = MENU_ID_CARDSEARCH_RESULT,
        .name =NULL,
        .ParentID = MENU_ID_CARDSEARCH,
        .FirstChildID = NULL,
        .ChildNum = 0,
        .ItemName_CH = NULL,
        .oper.display = menu_CardSearch_result_display,
        .oper.on_key = menu_CardSearch_result_onkey
    }
    ,
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

};

void menu_tree_init(void)
{
    gassms_buf.p = (char*)osal_mem_alloc(GASSMS_MAX_LEN);
    if(gassms_buf.p == NULL)
    {
        SystemReset();
    }
    gassms_buf.len = 0;

    menu_tree_stack_init();
}
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

static uint8 menu_ShowItem2StorageNum(uint8 totalCnt, uint8 show_item)
{
    return totalCnt - show_item - 1;
}

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
#ifdef CFG_GAS_INFORMATION
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
static void    menu_selectlist_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    switch(keys)
    {
    case HAL_KEY_OK:
        menu_steptochild(node.FirstChildID + node_info.sel_item,0);  /* select the first one by default*/
        break;
    case HAL_KEY_MODE:
        menu_steptoparent();
        break;
    case HAL_KEY_UP:
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
#endif

static void menu_list_up_down_onkey(uint8 keys, uint8 list_len, bool isListLine)
{

    switch(keys)
    {
    case HAL_KEY_UP:
        //case HAL_KEY_LEFT:
        NearLastNodeID =CurrentNodeID;
        if(list_len > SCREEN_LINES)
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
                if(isListLine)
                {
                    LCD_ListLine_Inv(node_info.high_line);
                    LCD_ProgBar_update(node_info.sel_item, list_len);
                }
                else
                {
                    LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);
                }
            }

        }
        else if((list_len <= SCREEN_LINES) && (list_len > 0))
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
            if(isListLine)
            {
                LCD_ListLine_Inv(node_info.high_line);
                LCD_ProgBar_update(node_info.sel_item, list_len);
            }
            else
            {
                LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);
            }
        }
        break;
    case HAL_KEY_DOWN:
        NearLastNodeID = CurrentNodeID;
        if(list_len > SCREEN_LINES)
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
                if(isListLine)
                {
                    LCD_ListLine_Inv(node_info.high_line);
                    LCD_ProgBar_update(node_info.sel_item, list_len);
                }
                else
                {
                    LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);
                }
            }
        }
        else if((list_len <= SCREEN_LINES) && (list_len > 0))
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

            if(isListLine)
            {
                LCD_ListLine_Inv(node_info.high_line);
                LCD_ProgBar_update(node_info.sel_item, list_len);
            }
            else
            {
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

    switch(keys)
    {
    case HAL_KEY_OK:
        menu_steptochild(node.FirstChildID + node_info.sel_item, 0); /* select the first one by default*/
        break;
    case HAL_KEY_MODE:
        menu_steptoparent();
        break;
    default:
        menu_list_up_down_onkey(keys, node.ChildNum, TRUE);
        break;
    }

}

/*
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

static void    menu_selectlist_onkey(uint8 keys, uint8 status)
{
Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

switch(keys)
{
case HAL_KEY_SELECT:
menu_steptochild(node.FirstChildID + node_info.sel_item,0);
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
*/
static void     menu_functionlist_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_MODE)
    {
        menu_JumptoMenu(MENU_ID_MAIN);
    }
    else
    {
        menu_list_onkey( keys,  status);
    }
}
static void menu_zeroadjust_display(void)
{
    LcdClearDisplay();

    if(ch4_GetLdoStatus())
    {
        LCD_Str_Print(ZEROADJUST_ALERT_CHINA, 0, 1, TRUE);
    }
    else
    {
        strcpy((char *)g_jump_buf,LDOCLOSED_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
    }
}
static void menu_zeroadjust_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_OK:
    {
        if(ch4_GetLdoStatus())
        {
            if(ch4_zero_adjust() == 0)
            {
                zeroadjust_flag  =  true;
            }
            else
            {
                zeroadjust_flag = false;
            }
            strcpy((char *)g_jump_buf,ZEROADJUSTING_CHINA);
            menu_JumpandMark(MENU_ID_SHOWMESSAGE);
        }
        break;
    }
    case HAL_KEY_MODE:
    {
        menu_steptoparent();
        break;
    }
    }
}
static void menu_zeroadjust_result_display(void)
{
    LcdClearDisplay();
    uint8 *p;
    if(zeroadjust_flag)
    {
        p = (uint8*) ZEROADJUST_SUCCESS_CH;
    }
    else
    {
        p = (uint8*) ZEROADJUST_FAILED_CH;
    }
    strcpy((char *)g_jump_buf,(char *)p);
    menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
}
static void menu_zeroadjust_result_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_OK || keys== HAL_KEY_MODE)
    {
        menu_JumpBackMarkParent();
    }
}
static void menu_calibration_display(void)
{
    LcdClearDisplay();
    if(ch4_GetLdoStatus())
    {
        LCD_Str_Print(CLBRATION_ALERT_CHINA, 0, 1, TRUE);
    }
    else
    {
        strcpy((char *)g_jump_buf,LDOCLOSED_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
    }


}
static void menu_calibration_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_OK:
    {
        if(ch4_GetLdoStatus())
        {
            menu_JumpandMark(MENU_ID_CALIBRATION_SETDENSITY);
        }
        break;
    }
    case HAL_KEY_MODE:
    {
        menu_steptoparent();
    }
    }
}
static void menu_calibration_result_display(void)
{
    LcdClearDisplay();
    uint8 *p;
    if(menu_get_calibration_result())
    {
        p = (uint8*) CALIBRATION_SUCCESS_CH;
    }
    else
    {
        p = (uint8*) CALIBRATION_FAILED_CH;
    }
    strcpy((char *)g_jump_buf,(char *)p);
    menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
}
static void menu_calibration_result_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_OK || keys== HAL_KEY_MODE)
    {
        menu_JumpBackMarkParent();
    }
}

#ifdef CFG_GAS_SHORTMESSAGE
static void menu_shortmessage_numjudge_display(void)
{
    uint8* p;
    Tree_node_t node = Menu_Tree[GetIdxFromID(MENU_ID_SHORTMESSAGE)];
    node.ChildNum =  GasSms_GetSMSCnt();

    if(node.ChildNum>=GASSMS_MAX_NUM-2 && NearLastNodeID == MENU_ID_FUNCTIONLIST)
    {
        if(node.ChildNum<GASSMS_MAX_NUM)
        {
            p = (uint8*) BEFULL_SMSINBOX_CHINA;
        }
        else
        {
            p = (uint8*) ISFULL_SMSINBOX_CHINA;
        }

        strcpy((char *)g_jump_buf,(char *)p);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
    }

    else
    {
        menu_JumpandMark(MENU_ID_SHORTMESSAGE);
    }

}


static void menu_shortmessage_display(void)
{
    uint8 i;
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8*)node.name, (LCD_LINE_WIDTH-osal_strlen(node.name))/2, 0, TRUE);
    }
    else
    {
        for(uint8 i=1; i<SCREEN_LINES+1; i++)
        {
            LCD_Line_Clear(i);
        }
    }

    termNbr_t nmbr;
    uint8 xpos;
    node.ChildNum =  GasSms_GetSMSCnt();

    if(node.ChildNum == 0)
        return;

    if(NearLastNodeID == MENU_ID_SHORTMESSAGE_NUMJUDGE
            ||NearLastNodeID == MENU_ID_SHOWMESSAGE)
    {
        node_info.high_line = 1;
        node_info.sel_item = 0;
        node_info.show_item = 0;
    }

    if(node.ChildNum <= SCREEN_LINES)
    {
        for(i=0; i<node.ChildNum - node_info.show_item; i++)
        {
            uint8 storageCnt;
            storageCnt = menu_ShowItem2StorageNum(node.ChildNum, node_info.show_item+i);

            if(GasSms_GetSmsNmbr(&nmbr, storageCnt)!=ZSuccess)
            {
                return;
            }
            xpos = LCD_ID_Show(i+1, 0, i+1);
            LCD_Str_Print(")", xpos, i+1, TRUE);
            LCD_Str_Print((uint8 *)nmbr.Nmbr, xpos+1, i+1, TRUE);

            bool isReaded;
            if(GasSms_GetSmsReadFlag(&isReaded,storageCnt)!=ZSuccess)
            {
                return;
            }
            if(!isReaded)
            {
                LCD_BigAscii_Print(0x93, LCD_LINE_WIDTH-2, i+1);
            }
        }
    }
    else
    {
        for(i=0; i<SCREEN_LINES; i++)
        {
            uint8 storageCnt;
            storageCnt = menu_ShowItem2StorageNum(node.ChildNum, node_info.show_item+i);

            if(GasSms_GetSmsNmbr(&nmbr, storageCnt)!=ZSuccess)
            {
                return;
            }
            xpos = LCD_ID_Show(node_info.show_item+i+1, 0, i+1);
            LCD_Str_Print(")", xpos, i+1, TRUE);
            LCD_Str_Print((uint8 *)nmbr.Nmbr, xpos+1, i+1, TRUE);

            bool isReaded;
            if(GasSms_GetSmsReadFlag(&isReaded,storageCnt)!=ZSuccess)
            {
                return;
            }
            if(!isReaded)
            {
                LCD_BigAscii_Print(0x93, LCD_LINE_WIDTH-2, i+1);
            }
        }
    }
    LCD_Char_Inv(0, node_info.high_line, LCD_LINE_WIDTH);
}
static void menu_shortmessage_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
    node.ChildNum =  GasSms_GetSMSCnt();

    switch(keys)
    {
    case HAL_KEY_OK:
        if(node.ChildNum> 0)
        {
            gassms_info.smsSelNum = node.ChildNum - node_info.sel_item -1;
            gassms_info.smsStartLine = 0;
            Stack_Clear(&global_stack);
            menu_Mark();
            menu_JumpandPush(MENU_ID_SHORTMESSAGE_READ);
        }
        else
        {
            strcpy((char *)g_jump_buf,EMPTY_SMSINBOX_CHINA);
            menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            GasMonitor_StartMenuLibEvt(1000);
        }
        break;
    case HAL_KEY_MODE:
        menu_JumptoMenu(MENU_ID_FUNCTIONLIST);
        break;
    default:
        menu_list_up_down_onkey(keys, node.ChildNum, false);
        break;
    };
}
static void    menu_shortmessage_read_display(void)
{
    termNbr_t nmbr;
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        GasSms_GetSmsNmbr(&nmbr, gassms_info.smsSelNum);
        LCD_Str_Print((uint8*)nmbr.Nmbr, (LCD_LINE_WIDTH-osal_strlen(nmbr.Nmbr))/2, 0, TRUE);
        GasSms_ReadSms((char*)gassms_buf.p, &gassms_buf.len, gassms_info.smsSelNum);
        gassms_info.smsStartLine = 0;
    }
    else
    {
        for(uint8 i=1; i<SCREEN_LINES+1; i++)
        {
            LCD_Line_Clear(i);
        }
    }

    GasSms_LCDPrint(gassms_buf.p, gassms_buf.len, gassms_info.smsStartLine, 1, SCREEN_LINES+1, LCD_LINE_WIDTH);
}

static void menu_shortmessage_read_onkey(uint8 keys, uint8 status)
{
    uint8 lineCnt = GasSms_GetLineCnt(gassms_buf.p, gassms_buf.len, LCD_LINE_WIDTH);
    switch(keys)
    {
    case HAL_KEY_OK:
        menu_JumpandPush(MENU_ID_SHORTMESSAGE_HANDLE);
        break;
    case HAL_KEY_MODE:
        menu_JumpandPop();
        break;
    case HAL_KEY_UP:
        if(gassms_info.smsStartLine > 0)
        {
            LCD_Clear(0, 1, LCD_LINE_WIDTH, 3);
            gassms_info.smsStartLine--;
            GasSms_LCDPrint(gassms_buf.p, gassms_buf.len, gassms_info.smsStartLine, 1, SCREEN_LINES+1, LCD_LINE_WIDTH);
        }
        break;
    case HAL_KEY_DOWN:
        if(gassms_info.smsStartLine < lineCnt-1)
        {
            LCD_Clear(0, 1, LCD_LINE_WIDTH, 3);
            gassms_info.smsStartLine++;
            GasSms_LCDPrint(gassms_buf.p, gassms_buf.len, gassms_info.smsStartLine, 1, SCREEN_LINES+1, LCD_LINE_WIDTH);
        }
        break;
    default:
        break;
    }
}

static void menu_shortmessage_handle_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
    switch(keys)
    {
    case HAL_KEY_OK:
        if(node_info.sel_item == 0)  // delete
        {
            strcpy((char *)g_jump_buf,DELETEING_CHINA);
            menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
            //menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            GasMonitor_StartMenuLibEvt(1000);
            gassms_info.smsDelFlag =  GasSms_DelSms(gassms_info.smsSelNum);
        }
        else if(node_info.sel_item == 1)  // delete all
        {
            strcpy((char *)g_jump_buf,DELETEING_CHINA);
            menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
            //menu_JumpandMark(MENU_ID_SHOWMESSAGE);
            GasMonitor_StartMenuLibEvt(1000);
            gassms_info.smsDelFlag =  GasSms_ClearSms();
        }
        break;
    case HAL_KEY_MODE:
        menu_JumpandPop();
        break;
    default:
        menu_list_up_down_onkey(keys, node.ChildNum, false);
        break;
    };

}
static void menu_shortmessage_delresult_display(void)
{
    if(gassms_info.smsDelFlag == ZSuccess)
    {
        strcpy((char *)g_jump_buf,DELETED_CHINA);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
        //menu_JumpandMark(MENU_ID_SHOWMESSAGE);
    }
    else
    {
        strcpy((char *)g_jump_buf,DELETE_FAILED_CHINA);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
        //menu_JumpandMark(MENU_ID_SHOWMESSAGE);
    }
    GasMonitor_StartMenuLibEvt(500);
}

#endif

static void menu_temperature_display(void)
{
    int16 temper;
    temper = tem_get_temperature();

    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8 *)TEMPERATURE_CHINA, (LCD_LINE_WIDTH-osal_strlen((char *)TEMPERATURE_CHINA))/2, 0, TRUE);
        LCD_BigCharPrint('.',TEMPERDISP_START_X+6, TEMPERDISP_START_Y*LCD_LINE_HIGH);
        LCD_BigCharPrint('C', TEMPERDISP_START_X+9, TEMPERDISP_START_Y*LCD_LINE_HIGH);
        menu_TemperPrint(temper, true);
    }
    else
    {
        menu_TemperPrint(temper, false);
    }

    GasMonitor_StartMenuLibEvt(10000);
}
static void menu_temperature_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_OK || keys == HAL_KEY_MODE)
    {
        GasMonitor_StopMenuLibEvt();
        menu_steptoparent();
    }
}
static void menu_settings_display(void)
{
    menu_list_display();
}
static void menu_settings_onkey(uint8 keys, uint8 status)
{
    menu_list_onkey(keys,status);
}
static void menu_settings_backlight_display(void)
{
    menu_list_display();
}
static void menu_settings_backlight_onkey(uint8 keys, uint8 status)
{
    if(HAL_KEY_OK == keys)
    {
        osal_nv_read(GASMONITOR_NV_SETTINGS, 0, sizeof(set_info), &set_info);
        set_info.backlight_ctl = node_info.sel_item;
        osal_nv_write(GASMONITOR_NV_SETTINGS, 0, sizeof(set_info), &set_info);
        LCDSetBackLightCtl(node_info.sel_item);
        strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
    }
    else
    {
        menu_list_onkey(keys, status);
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
    else if(node_info.sel_item == 1)  /* time format */
    {
        if(set_info.timeformat_t== TIME_FORMAT_24)
        {
            menu_selectlist_display(TIMEFORMAT24_CHINA);
        }
        else
        {
            menu_selectlist_display(TIMEFORMAT12_CHINA);
        }
    }
}

static void    menu_setting_timedate_onkey(uint8 keys, uint8 status)
{
    if(HAL_KEY_OK == keys)
    {
        if(node_info.sel_item == 0) /*set time*/
        {
            menu_setadjusttime();
            menu_JumpandMark(MENU_ID_ADJUSTTIME);
        }
        else if(node_info.sel_item == 1) /* set time format */
        {
            menu_JumpandMark(MENU_ID_SETTINGS_TIME_TIMEFORMAT);
        }
    }

    else
    {
        menu_selectlist_onkey(keys, status);
    }
}

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
    if(keys == HAL_KEY_OK)
    {
        osal_nv_read(GASMONITOR_NV_SETTINGS, 0, sizeof(set_info), &set_info);
        if(node_info.sel_item == 0) /* 12 clock format */
        {
            set_info.timeformat_t = TIME_FORMAT_12;
            SetTimeFormat(TIME_FORMAT_12);
        }
        else if(node_info.sel_item == 1) /* 24 clock format */
        {
            set_info.timeformat_t = TIME_FORMAT_24;
            SetTimeFormat(TIME_FORMAT_24);
        }
        osal_nv_write(GASMONITOR_NV_SETTINGS, 0, sizeof(set_info), &set_info);

        //menu_JumpBackWithMark();
        strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
    }
    else if(keys == HAL_KEY_MODE)
    {
        menu_JumpBackWithMark();
    }
    else
    {
        menu_list_onkey(keys, status);
    }
}
#ifdef CFG_GAS_INFORMATION
static void menu_tools_display(void)
{
    uint8 p[12];

    if(node_info.sel_item == 0)  /*show mp number*/
    {
        uint16 number;
        number = BUILD_UINT16( aExtendedAddress[EXT_MACADDR_DEVID_LBYTE], aExtendedAddress[EXT_MACADDR_DEVID_HBYTE]);
        //_ltoa(number , p, 10);
        menu_itoa(number , (char*)p);
    }
    else if(node_info.sel_item == 1)  /* show channel */
    {
        //_ltoa( aExtendedAddress[EXT_MACADDR_CHANNEL], p, 10);
        menu_itoa( aExtendedAddress[EXT_MACADDR_CHANNEL], (char*)p);
    }
    else if(node_info.sel_item == 2)  /* show software version */
    {
        strcpy((char *)p, GASMONITOR_SW_VERSION);
    }

    menu_selectlist_display(p);

}

static void menu_tools_onkey(uint8 keys, uint8 status)
{

    if((keys == HAL_KEY_UP)       ||
            (keys == HAL_KEY_DOWN))
    {
        menu_selectlist_onkey(keys, status);
    }
    else if(keys == HAL_KEY_MODE)
    {
        menu_steptoparent();
    }
}
#endif

void menu_TemperPrint(int16 temper, bool forcePrint)
{
    static char str_bak[8];
    char str[8];
    uint8 x,y;
    tem_getnum_fromtemper(str, temper);

    if(forcePrint)
    {
        osal_memcpy(str_bak, str, 8);

        x = TEMPERDISP_START_X;
        y = TEMPERDISP_START_Y;
        LCD_Clear(x,  y, x+2, y);
        LCD_BigCharPrint(str[0], x, y*LCD_LINE_HIGH);

        x = TEMPERDISP_START_X+2;
        y = TEMPERDISP_START_Y;
        LCD_Clear(x,  y, x+2, y);
        LCD_BigCharPrint(str[1], x, y*LCD_LINE_HIGH);

        x = TEMPERDISP_START_X+4;
        y = TEMPERDISP_START_Y;
        LCD_Clear(x, y, x+2, y);
        LCD_BigCharPrint(str[2], x, y*LCD_LINE_HIGH);

        x = TEMPERDISP_START_X+7;
        y = TEMPERDISP_START_Y;
        LCD_Clear(x, y, x+2, y);
        LCD_BigCharPrint(str[3], x, y*LCD_LINE_HIGH);

    }
    else
    {
        if(str[0]==' ')
        {
            str_bak[0]=str[0];
            x = TEMPERDISP_START_X;
            y = TEMPERDISP_START_Y;
            LCD_Clear(x,  y, x+2, y);
        }
        else
        {
            if(str_bak[0]!=str[0])
            {
                str_bak[0]=str[0];
                x = TEMPERDISP_START_X;
                y = TEMPERDISP_START_Y;
                LCD_Clear(x,  y, x+2, y);
                LCD_BigCharPrint(str[0], x, y*LCD_LINE_HIGH);
            }
        }
        if(str[1] == ' ')
        {
            str_bak[1]=str[1];
            x = TEMPERDISP_START_X+2;
            y = TEMPERDISP_START_Y;
            LCD_Clear(x,  y, x+2, y);
        }
        else
        {
            if(str_bak[1]!=str[1])
            {
                str_bak[1]=str[1];
                x = TEMPERDISP_START_X+2;
                y = TEMPERDISP_START_Y;
                LCD_Clear(x,  y, x+2, y);
                LCD_BigCharPrint(str[1], x, y*LCD_LINE_HIGH);
            }
        }

        if(str_bak[2]!=str[2])
        {
            str_bak[2]=str[2];
            x = TEMPERDISP_START_X+4;
            y = TEMPERDISP_START_Y;
            LCD_Clear(x, y, x+2, y);
            LCD_BigCharPrint(str[2], x, y*LCD_LINE_HIGH);
        }
        if(str_bak[3]!=str[3])
        {
            str_bak[3]=str[3];
            x = TEMPERDISP_START_X+7;
            y = TEMPERDISP_START_Y;
            LCD_Clear(x, y, x+2, y);
            LCD_BigCharPrint(str[3], x, y*LCD_LINE_HIGH);
        }
    }
}

static void menu_SOSAlarm_display(void)
{
    menu_list_display();
}

static void menu_SOSAlarm_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
    switch(keys)
    {
    case HAL_KEY_OK:

        switch(node_info.sel_item)
        {
        case 0:
            Alarmtype=GASNODE_ALARM_FIRE;
            break;

        case 1:
            Alarmtype=GASNODE_ALARM_WATER;
            break;

        case 2:
            Alarmtype=GASNODE_ALARM_TOPBOARD;
            break;

        case 3:
            Alarmtype=GASNODE_ALARM_OTHERS;
            break;

        }

        menu_JumpandMark(MENU_ID_SOSALARM_ALERT);
        break;




    case HAL_KEY_MODE:
        menu_steptoparent();
        break;
    default:
        menu_list_up_down_onkey(keys,node.ChildNum, TRUE);
        break;
    }




}

static void  menu_SOSAlarm_alert_display(void)
{
    LcdClearDisplay();
    LCD_Str_Print(SOSALARM_ALERT_CHINA, 0, 1, TRUE);

}
static void menu_SOSAlarm_alert_onkey(uint8 keys, uint8 status)
{


    switch(keys)
    {

    case HAL_KEY_OK:
    {
        GasMonitor_SOSAlarm_startsend(Alarmtype);
        break;
    }

    case HAL_KEY_MODE:
    {
        menu_JumpBackWithMark();
        break;
    }
    }
}

static void menu_CardSearch_display(void)
{
    menu_CardSearch_start();
    strcpy((char *)g_jump_buf,CARDSEARCHING_CHINA);
    menu_JumpandMark(MENU_ID_SHOWMESSAGE);
}
#ifdef CFG_GAS_CARDCHECK
static void menu_CardCheck_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print(CARDCHECKING_CHINA,1, 0, TRUE);

        gas_cardcheck_num = 0;  //initial the synced card number
    }

    if(gas_cardcheck_num != 0)
    {
        menu_CardCheckPrint(gas_cardcheck_num);
    }
}
static void menu_CardCheck_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_MODE:
        menu_steptoparent();
        break;
    }
}
#endif

static void menu_CardSearch_result_display(void)
{
    uint8 i;

    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8 *)CARDSEARCHED_CHINA, 2, 0, TRUE);
    }
    else
    {
        for(uint8 i=1; i<SCREEN_LINES+1; i++)
        {
            LCD_ListLine_Clear(i);
        }
    }

    uint8 IDprintbuf[8];
    uint8 Siglevelprintbuf[4];
    node.ChildNum = CardInfo.IDnum;

    if(CardInfo.IDnum < 100)
    {
        sprintf((char*)IDprintbuf, "%02u",CardInfo.IDnum);
        LCD_Clear(8, 0, 9 , 0);
        LCD_Str_Print(IDprintbuf,8,0,TRUE);
    }

    memset(IDprintbuf, 0, sizeof(IDprintbuf));
    memset(Siglevelprintbuf, 0, sizeof(Siglevelprintbuf));

    if(node.ChildNum <= SCREEN_LINES)
    {
        for(i=0; i<node.ChildNum - node_info.show_item; i++)
        {
            sprintf((char*)Siglevelprintbuf, "%u",CardInfo.CardSiglevel[node_info.show_item+i]);
            sprintf((char*)IDprintbuf, "%u",CardInfo.CardID[node_info.show_item+i]);

            if(IDprintbuf!=0)
            {
                LCD_Str_Print(IDprintbuf, 0, i+1, TRUE);
                LCD_Str_Print(SIGNAL_CHINA,8,i+1, TRUE);
                LCD_Clear(13, i+1, 15, i+1);
                LCD_Str_Print(Siglevelprintbuf,13,i+1,TRUE);
            }
        }
    }
    else
    {
        for(i=0; i<SCREEN_LINES; i++)
        {
            sprintf((char*)Siglevelprintbuf, "%u",CardInfo.CardSiglevel[node_info.show_item+i]);
            sprintf((char*)IDprintbuf, "%u",CardInfo.CardID[node_info.show_item+i]);

            if(IDprintbuf!=0)
            {
                LCD_Str_Print(IDprintbuf, 0, i+1, TRUE);
                LCD_Str_Print(SIGNAL_CHINA,8,i+1, TRUE);
                LCD_Clear(13, i+1, 15, i+1);
                LCD_Str_Print(Siglevelprintbuf,13,i+1,TRUE);
            }
        }
    }

    if(node.ChildNum !=0 )
    {
        LCD_ListLine_Inv(node_info.high_line);
    }


    if(FIRSTTIME_INTO_NODE())
    {
        LCD_ProgBar_open();
    }
    LCD_ProgBar_update(node_info.sel_item, node.ChildNum);


    GasMonitor_StartItselfMenuLibEvt(1000);
}

void menu_CardSearch_result_onkey(uint8 keys, uint8 status)
{
    Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
    switch(keys)
    {
    case HAL_KEY_MODE:
        menu_CardSearch_end();
        menu_JumpBackMarkParent();
        break;

    default:

        menu_list_up_down_onkey(keys, CardInfo.IDnum, TRUE);
        break;
    }
}

void menu_CardInfo_receive(uint16 CardID, int8 rssi)
{
    if((CurrentNodeID==MENU_ID_CARDSEARCH_RESULT || CurrentNodeID==MENU_ID_CARDSEARCH))
    {
        uint8 CardSiglevel  = GasMonitor_RSSI2Level(rssi);
        menu_FillCardInfo(CardID,CardSiglevel);
    }
#ifdef CFG_GAS_CARDCHECK
    else if(CurrentNodeID == MENU_ID_CARDCHECK  && rssi > -7)  // for card check
    {
        //if(CardID != gas_cardcheck_num)  // a new card  comes
        {
            gas_cardcheck_num = CardID;
            menu_JumptoMenu(MENU_ID_CARDCHECK);
            HalResetBackLightEvent();
            beep_begin();
#if (defined WATCHDOG) &&(WATCHDOG == TRUE)
            FeedWatchDog();
#endif
            DelayMs(100);
            beep_stop();
        }

        /* for debug, print the current rssi */
        /*
        LCD_Line_Clear(0);
        uint8 buf[8];
        if(rssi < 0)
        {
            buf[0] = '-';
            rssi = -rssi;
        }
        else
        {
            buf[0] = '+';
        }
        _itoa((uint16)rssi, buf+1, 10);
        LCD_Str_Print(buf, 0, 0, true);
        */
    }
#endif
}

bool menu_FillCardInfo(uint16 shortAddr, uint8 CardSiglevel)
{

    if(CardInfo.IDnum< MAXIDNUM)
    {
        for(uint8 i=0; i<CardInfo.IDnum; i++)
        {
            if(CardInfo.CardID[i] == shortAddr)
            {
                if((uint8)(CardInfo.CardSiglevel[i]-CardSiglevel)>=1)
                {
                    CardInfo.CardSiglevel[i]=CardSiglevel;
                }
                return false;
            }
        }
        CardInfo.CardSiglevel[CardInfo.IDnum]= CardSiglevel;
        CardInfo.CardID[CardInfo.IDnum] = shortAddr;
        CardInfo.IDnum++;

        return true;
    }
    else
    {
        return false;
    }
}

/*
void menu_CardID_check(uint16 CardID)
{
int i;
if(CardID!=0&&CardIDbuf[MAXIDNUM-1]==0)
{
for(i=0;i<MAXIDNUM;i++)
{
if(CardIDbuf[i]==CardID)
{
bufcanwrite=FALSE;
return;
}
}
}
else
{
bufcanwrite=FALSE;
return;
}
bufcanwrite=TRUE;
}
*/
void menu_CardSearch_start(void)
{
    CardInfo.CardID = (uint16*)osal_mem_alloc(sizeof(uint16)*MAXIDNUM);
    CardInfo.CardSiglevel= (uint8*)osal_mem_alloc(sizeof(uint8)*MAXIDNUM);

    if(CardInfo.CardID == NULL
            ||CardInfo.CardSiglevel ==NULL)
    {
        SystemReset();
    }
    CardInfo.IDnum = 0;

}
void menu_CardSearch_end(void)
{
    GasMonitor_StopItselfMenuLibEvt();
    osal_mem_free(CardInfo.CardID);
    osal_mem_free(CardInfo.CardSiglevel);
    CardInfo.IDnum=0;
}
void menu_locate_display()
{

    uint8 n1,n2,n3,n4,n5;
    uint8 a[6];
    LcdClearDisplay();

    LCD_Str_Print((uint8 *)LOCATEINFORMATION_CHINA, (LCD_LINE_WIDTH-osal_strlen((char *)OVERDENSITYED_CHINA))/2, 0,TRUE);
    if(Locate_isvalid&&(GasMonitor_GetLocateDistance()<1000))
    {
        LCD_Str_Print((uint8 *)DISTANCE_CHINA, 0, 1,TRUE);
        GasMonitor_stationID_fromdensity(&n1,&n2,&n3,&n4, &n5, GasMonitor_GetLocateTargetName());
        a[0]=n1+'0';
        a[1]=n2+'0';
        a[2]=n3+'0';
        a[3]=n4+'0';
        a[4]=n5+'0';
        a[5]='\0';
        LCD_Str_Print((uint8 *)a, osal_strlen((char *)DISTANCE_CHINA), 1,TRUE);
        LCD_Str_Print((uint8 *)":", osal_strlen((char *)DISTANCE_CHINA)+osal_strlen((char *)a), 1,TRUE);
        GasMonitor_stationID_fromdensity(&n1,&n2,&n3,&n4, &n5, GasMonitor_GetLocateDistance());	
        menu_LocatePrint(n3,n4,n5,true);
        LCD_Str_Print((uint8 *)"m", 9, 2,TRUE);
		//uint16 GasMonitor_Get_SAVE_LocateDistance(void)
        GasMonitor_stationID_fromdensity(&n1,&n2,&n3,&n4, &n5, GasMonitor_Get_SAVE_LocateDistance());	
        //menu_LocatePrint(n3,n4,n5,true);
        a[0]=n1+'0';
        a[1]=n2+'0';
        a[2]=n3+'0';
        a[3]=n4+'0';
        a[4]=n5+'0';
        a[5]='\0';
        LCD_Str_Print((uint8 *)a, 1, 3,TRUE);        
    }
    else
    {
        LCD_Str_Print((uint8 *)LOCATEFAILURE_CHINA, (LCD_LINE_WIDTH-osal_strlen((char *)OVERDENSITYED_CHINA))/2, 2,TRUE);
    }
}
