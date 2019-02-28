#ifndef MENULIB_GLOBAL_H
#define MENULIB_GLOBAL_H
#include "hal_types.h"
#include "MobilePhone_MenuLib.h"
#include "AppProtocol.h"

#include "MenuLib_Nv.h"

#define DATE_LEN                11
#define MAX_DATA_BUF      32
#define SCREEN_LINES         3
#define PIPELINE_DEPTH         5
#define STACK_DEPTH         4
#define TOP_POS_TIME        5
#define SMS_LEN_TYPE_SIZE 1
//#define SMS_NV_LEN                    (SMS_MAX_LEN+NMBRDIGIT+SMS_LEN_TYPE_SIZE)//82

#define MENU_RESET_NODEINFO()    st(node_info.high_line =1; \
    node_info.sel_item = 0; \
    node_info.show_item = 0;)

#define FIRSTTIME_INTO_NODE()    (CurrentNodeID!= NearLastNodeID)



/*-----------------Typedefs-----------------*/
enum node_type
{
    NODE_TYPE_ORPHAN,
    NODE_TYPE_TREE,
};

typedef struct
{
    void(*display)(void);
    void(*on_key)(uint8 keys, uint8 status);
} MenuOper_t;

typedef struct
{
    uint8 len;
    uint8* p;
} buf_t;

typedef struct
{
    uint8   high_line;
    uint8   sel_item;
    uint8   show_item;
} node_info_t;

typedef struct
{
    uint8 id;
    node_info_t node_info;
} pipeline_t;

typedef struct
{
    uint8 id;
    node_info_t node_info;
} stack_p_t;

typedef struct
{
    uint8           stack_i;
    uint8           stack_depth;
    stack_p_t*   stack_p;
} stack_t;


typedef struct
{
    uint8 bell_gain;
    uint8 sound_gain;
    uint8 bell_ring_t;
    uint8 sms_ring_t;
    bool  padlock_ctl;
    bool  shake_ctl;
#ifdef MENU_CLOCKFORMAT
    uint8 timeformat_t;
#endif
    uint8 backlight_ctl;
#ifdef  MENU_TIMEUPDATE_CTL
    bool  time_autoupdate;
#endif
} set_info_t;

enum NV_ID
{
    SMS_NVID
};

enum MenuID
{
    MENU_ID_ROOT = 0,

    /*independent ID in the root, parrelled with main menu*/
    MENU_ID_POWERON_ANIMATION,
    MENU_ID_POWEROFF_ANIMATION,
    MENU_ID_INITNWK,
    MENU_ID_LONGTIME_CLOCK,

    MENU_ID_SHOWING_NUMBER,
    MENU_ID_DIALING,
    MENU_ID_TALKING,
    MENU_ID_INCOMINGCALL,
    MENU_ID_MISSINGCALL,
    MENU_ID_INCOMINGSMS,
    MENU_ID_MISSINGMESSAGE,
    MENU_ID_SM_SENDING,

    MENU_ID_CALLRECORD_DETAIL,
    //MENU_ID_CONTACT_DETAIL,
    MENU_ID_TYPEING,
    MENU_ID_INPUTCHINESE,
    MENU_ID_INPUTNAME,
    MENU_ID_INPUTNUMBER_SMS,
    MENU_ID_INPUTNUMBER_CONTACT,

    MENU_ID_INPUTSYMBOL,

    MENU_ID_SHOWMESSAGE,
    MENU_ID_SHOWQUESTION,
    MENU_ID_SHOWALERT,

    MENU_ID_BUSY,
    MENU_ID_ADJUSTVOLUME,
    MENU_ID_ADJUSTTIME,
    MENU_ID_ADJUSTDATE,
    MENU_ID_CANLENDAR,

    MENU_ID_MAIN,//27

    /*--------separetor  of orphan nodes and tree nodes------------------*/
    MENU_ID_SEPARATOR,

    /*the tree menu begins from functionlist*/
    MENU_ID_FUNCTIONLIST,

    /*Pages in FUNCTIONLIST */
    MENU_ID_CONTACTLIST,
    MENU_ID_SHORTMESSAGE,
    MENU_ID_CALLRECORD,
    MENU_ID_SETTINGS,
    MENU_ID_TOOLS,


#if 0
    /*Pages in CONTACTLIST*/
    MENU_ID_CONTACTLIST_DETAILS,
    MENU_ID_CONTACTLIST_OPTIONS,

    /*Pages in ShortMessage */
    MENU_ID_SHORTMESSAGE_INCOMINGBOX,
    MENU_ID_SHORTMESSAGE_SENDINGBOX,
    MENU_ID_SHORTMESSAGE_DELETEMESSAGE,


    MENU_ID_CALLRECORD_OPTIONS,

    MENU_ID_CALLRECORD_MESSEDCALL_DETAILS,
    MENU_ID_CALLRECORD_ANSWEREDCALL_DETAILS,
    MENU_ID_CALLRECORD_DIALEDCALL_DETAILS,
#endif

    /* Pages in functionlist*/
    MENU_ID_CONTACT_HANDLE,
    MENU_ID_CONTACT_DETAIL,
    MENU_ID_CONTACT_DELETE,
    MENU_ID_CONTACT_ADD,

    /*Pages in ShortMessage */
    MENU_ID_SHORTMESSAGE_INCOMINGBOX,
    MENU_ID_SHORTMESSAGE_WRITINGBOX,
#ifdef SMS_SENDBOX
    //MENU_ID_SMS_EDIT_HANDLE,
    MENU_ID_SMS_SENDBOX,
#endif
#ifdef SMS_TEMPLATE
    MENU_ID_SMS_TEMPLATE,
#endif
#ifdef SMS_SENDBOX
    MENU_ID_SMS_EDIT_HANDLE,
#endif
#ifdef SMS_TEMPLATE
    MENU_ID_SMS_TEMPLATE_EDIT,
    MENU_ID_SMS_TEMPLATE_HANDLE,
#endif
#ifdef SMS_SENDBOX
    MENU_ID_SMS_SENDBOX_EDIT,
    MENU_ID_SMS_SENDBOX_HANDLE,
    MENU_ID_SMS_SENDBOX_DELETE,
#endif
    MENU_ID_SHORTMESSAGE_READ,
    MENU_ID_SHORTMESSAGE_HANDLE,
    MENU_ID_SHORTMESSAGE_REPLY,
    MENU_ID_SHORTMESSAGE_DELETE,
    MENU_ID_SHORTMESSAGE_DELETEALL,

    /*Pages in CALLRECORD */
    MENU_ID_CALLRECORD_MISSEDCALL,
    MENU_ID_CALLRECORD_ANSWEREDCALL,
    MENU_ID_CALLRECORD_DIALEDCALL,
    MENU_ID_CALLRECORD_DELETE,

    /*Pages in settings */
    MENU_ID_SETTINGS_RING,
    MENU_ID_SETTINGS_TIME,
    MENU_ID_SETTINGS_BACKLIGHT,
    MENU_ID_SETTINGS_PADLOCK,
#ifdef MENU_CONTACTUPDATE_CTL	    
    MENU_ID_UPDATE_CONTACT_ONLINE,
#endif    
    MENU_ID_SETTINGS_RESTORE_DEFAULT,
//#ifdef MENU_RF_DEBUG
    MENU_ID_DO_LCD_AGING,
    MENU_ID_LCD_PARAMETER_OPTION,
    MENU_ID_SETTINGS_SET_CHANNEL,
    MENU_ID_SETTINGS_SET_PANID,
    MENU_ID_SETTINGS_SET_PHONENUM,

//#endif
    /*Pages in Tools*/

    /*Pages in ringsetting */
    MENU_ID_SETTINGS_RING_BELLSEL,
    MENU_ID_SETTINGS_RING_SMSSEL,
    MENU_ID_SETTINGS_RING_BELLVOL,
    MENU_ID_SETTINGS_RING_SHAKE,

    /*Pages in timesetting */
#ifdef MENU_CLOCKFORMAT
    MENU_ID_SETTINGS_TIME_TIMEFORMAT,
#endif
#ifdef  MENU_TIMEUPDATE_CTL
    MENU_ID_SETTINGS_TIME_TIMEAUTOUPDATE,
#endif
    /*
    #ifdef MP_INFORMATION
    MENU_ID_MP_INFORMATION
    #endif*/

    /* the  last  one */
    MENU_ID_END,
};

/*-----------------global variable declares-----------------*/
extern  bool              shortcuts_flag;
extern  uint8             CurrentNodeID;
extern  uint8             NearLastNodeID;
extern  uint8             missed_call_amount;

extern node_info_t     node_info;
extern node_info_t     node_info_jumpbackup;
extern buf_t              data_buf;
extern buf_t              num_buf;
extern buf_t              dialnum_buf;
//extern bool               new_sms_flag;
extern stack_t           global_stack;
//extern set_info_t       set_info;
/* global buffer to send variables or strings when state jump, no detail defination, you can define its usage by spectial use*/
extern uint8             g_jump_buf[MAX_DATA_BUF];

/*-----------------Function declares-----------------*/

uint8 GetTypeFromID(uint8 ID);
uint8 GetIDFromIdx(uint8 node_type, uint8 idx);
uint8 GetIdxFromID(uint8 id);

void MP_SettingInformation_GetDefault(set_info_t* p);
void MP_SettingInformation_Collect(set_info_t* set_info);
void MP_SettingInformation_Handout(const set_info_t* set_info);

uint8 MP_SettingInformation_ReadFlash(set_info_t* p);
uint8 MP_SettingInformation_WriteFlash(const set_info_t* p);
bool MP_SettingInformation_IsValid(const set_info_t* p);
uint8 MP_Key2ASCII(uint8 key);

void* Buffer_Init(buf_t* const, uint8);
void Buffer_Free(buf_t* const);
void Buffer_Copy(buf_t*  dstbuf, const buf_t * srcbuf);
void Buffer_Clear(buf_t *buf);
void Menu_Stack_Init(void);
void Stack_Push(stack_t* const, uint8, node_info_t const*);
bool  Stack_Pop(stack_t* const, uint8*, node_info_t*);
void Stack_Clear(stack_t* const);
/*
void Push_PipeLine(uint8 NodeID, node_info_t* node_info);
void Pop_PipeLine(void);
void Clear_PipeLine(void);
void Drop_Unit_PipeLine(void);
*/
void   menu_display(void);
void  menu_JumptoMenu(uint8 ID);
void  menu_JumpandMark(uint8 ID);
void  menu_JumpBackWithMark(void);
void menu_JumpBackMarkParent(void);
uint8 menu_GetJumpMark(void);
//void Clr_Num_Buf(void);
//void  menu_Dial(void);
void  menu_Dial(const buf_t numbuff);

#endif
