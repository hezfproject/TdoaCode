#include "MenuLib_global.h"
#include "MenuLib_orphan.h"
#include "MenuChineseInputUtil.h"
#include "MenuLib_tree.h"
#include "MenuLib_Nv.h"

#include "ZComDef.h"
#include "osal_NV.h"
#include "App_cfg.h"
#include "osal.h"
#include "MobilePhone_global.h"
#include "MobilePhone_MenuLibChinese.h"
#include "string.h"
#include "TimeUtil.h"
#include "hal_audio.h"
#include "lcd_serial.h"
#include "MenuAdjustUtil.h"
#include "Hal_drivers.h"
#include  "key.h"
#include "MobilePhone_Function.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchdogUtil.h"
#endif
#include "numtrans.h"
/*----------------- variable-----------------*/
/*global */
bool              shortcuts_flag = FALSE;
uint8             CurrentNodeID = MENU_ID_ROOT;
uint8             NearLastNodeID = MENU_ID_MAIN;
uint8             missed_call_amount = 0;
uint8             g_jump_buf[MAX_DATA_BUF];
buf_t             num_buf = {0, NULL};
buf_t             dialnum_buf = {0, NULL};
buf_t             data_buf = {0, NULL};
//bool              new_sms_flag = FALSE;
stack_t          global_stack;
node_info_t    node_info = {1,0,0};
/*static */
static stack_p_t        pipeline[PIPELINE_DEPTH];
node_info_t     node_info_jumpbackup;
static uint8               NodeID_jumpbackup;

extern uint8 contact_item_L[MAX_CONTACT_NUM];


/* convert key to ASCII */
#define CHAR_0     0x30 //the ascii of '0'.
#define KEY2ASC_0  (0x0 + CHAR_0)   //'0'
#define KEY2ASC_1  (0x1 + CHAR_0)   //'1'
#define KEY2ASC_2  (0x2 + CHAR_0)   //'2'
#define KEY2ASC_3  (0x3 + CHAR_0)   //'3'
#define KEY2ASC_4  (0x4 + CHAR_0)   //'4'
#define KEY2ASC_5  (0x5 + CHAR_0)   //'5'
#define KEY2ASC_6  (0x6 + CHAR_0)   //'6'
#define KEY2ASC_7  (0x7 + CHAR_0)   //'7'
#define KEY2ASC_8  (0x8 + CHAR_0)   //'8'
#define KEY2ASC_9  (0x9 + CHAR_0)   //'9'
#define KEY2ASC_STAR  42   //'*'
#define KEY2ASC_POUND  35   //'#'

#define KEY2ASC_INVAID  0xFF


/*-----------------static Functions -----------------*/

static bool MP_SettingInformation_IsValid(const set_info_t* p);
/*-----------------Global Functions -----------------*/

void MP_SettingInformation_GetDefault(set_info_t* p)
{
    p->bell_gain = RING_GAIN_LEVEL_4;
    p->sound_gain = DEFAULT_OUTGAIN_LEVEL;
    p->bell_ring_t = RING_BELL_1-RING_BELL_1;
    p->sms_ring_t = RING_BELL_4-RING_BELL_1;
    p->padlock_ctl = TRUE;
    p->shake_ctl   = TRUE;
#ifdef MENU_CLOCKFORMAT
    p->timeformat_t = TIME_FORMAT_24;
#endif
    p->backlight_ctl  = BACKLIGHT_CTL_10S;
#ifdef 	MENU_TIMEUPDATE_CTL
    p->time_autoupdate = TRUE;
#endif
}

void MP_SettingInformation_Collect(set_info_t* set_info)
{
    HalRingGetGain(RING_TYPE_BELL, &set_info->bell_gain);
    set_info->sound_gain = AudioGetOutputGain();
    set_info->bell_ring_t = HalRingGetBellName() - RING_BELL_1;
    set_info->sms_ring_t = HalRingGetSMSBell() - RING_BELL_1;
    set_info->padlock_ctl = HalGetPadLockEnable();
    set_info->backlight_ctl = LCDGetBackLightCtl();
    set_info->shake_ctl = HalRingGetShakeCtl();

#ifdef MENU_CLOCKFORMAT
    set_info->timeformat_t = GetTimeFormat();
#endif

#ifdef 	MENU_TIMEUPDATE_CTL
    set_info->GetTimeAutoUpdate();
#endif

}

void MP_SettingInformation_Handout(const set_info_t* set_info)
{
    HalRingSetGain(RING_TYPE_BELL, set_info->bell_gain);
    AudioSetOutputGain(set_info->sound_gain);
    HalRingSetGain(RING_TYPE_SOUND, set_info->sound_gain);
    HalRingSetBellName((RingName)(set_info->bell_ring_t + RING_BELL_1));
    HalRingSetSMSBell((RingName)(set_info->sms_ring_t + RING_BELL_1));
    HalSetPadLockEnable(set_info->padlock_ctl);
    LCDSetBackLightCtl(set_info->backlight_ctl);
#ifdef MENU_CLOCKFORMAT
    SetTimeFormat(set_info->timeformat_t);
#endif
    HalRingSetShakeCtl(set_info->shake_ctl);
#ifdef 	MENU_TIMEUPDATE_CTL
    SetTimeAutoUpdate(set_info->time_autoupdate);
#endif
}
uint8 MP_SettingInformation_ReadFlash(set_info_t* p)
{
    uint8 flag = osal_nv_read(MINEAPP_NV_SET_INFORMATION, 0, sizeof(set_info_t),p);
    if(flag != ZSuccess)
    {
        return flag;
    }

    if(!MP_SettingInformation_IsValid(p))
    {
        MP_SettingInformation_GetDefault(p);
        MP_SettingInformation_WriteFlash(p);
        return ZInvalidParameter;
    }
    return ZSuccess;
}
uint8 MP_SettingInformation_WriteFlash(const set_info_t* p)
{
    return osal_nv_write(MINEAPP_NV_SET_INFORMATION, 0, sizeof(set_info_t),(void*)p);
}

bool MP_SettingInformation_IsValid(const set_info_t* p)
{
    if( p->bell_gain < MAX_GAIN_LEVEL
            && p->sound_gain < MAX_GAIN_LEVEL
            && p->bell_ring_t <= (uint8)(RING_BELL_4- RING_BELL_1)
            && p->sms_ring_t <= (uint8)(RING_BELL_4- RING_BELL_1)
            && p->backlight_ctl <= BACKLIGHT_CTL_30S
#ifdef MENU_CLOCKFORMAT
            && p->timeformat_t <= TIME_FORMAT_12
#endif
      )
    {
        return true;
    }
    else
    {
        return false;
    }
}

void* Buffer_Init(buf_t* const buf, uint8 buf_len)
{
    buf->len = 0;

    if(buf->p == NULL)
        buf->p = (uint8*)osal_mem_alloc(buf_len);

    return buf->p;
}
//free the buffer
void Buffer_Free(buf_t* const buf)
{
    buf->len = 0;
    if(buf->p != NULL)
    {
        osal_mem_free(buf->p);
        buf->p = NULL;
    }
}
void Buffer_Copy(buf_t*  dstbuf, const buf_t * srcbuf)
{
    if(srcbuf->len>0)
    {
        dstbuf->len = srcbuf->len;
        osal_memcpy(dstbuf->p, srcbuf->p, srcbuf->len+1);
        //strcpy((char *)dstbuf->p, (const char *)srcbuf->p);
    }
}
void Buffer_Clear(buf_t*  buf)
{
    buf->len = 0;
    if(buf->p != NULL)
    {
        *buf->p =  '\0';
    }
}
static void Stack_Init(stack_t* const stack, stack_p_t* stack_p, uint8 stack_depth)
{
    stack->stack_depth = stack_depth;
    stack->stack_p       = stack_p;
    stack->stack_i        = 0;
}
void Menu_Stack_Init(void)
{
    Stack_Init(&global_stack, pipeline, PIPELINE_DEPTH);
    menu_tree_stack_init();
}

void Stack_Push(stack_t* const stack, uint8 NodeID, const node_info_t* node_info)
{
    if(stack->stack_i < stack->stack_depth)
    {
        stack->stack_p[stack->stack_i].id = NodeID;
        if(node_info != NULL)
            stack->stack_p[stack->stack_i].node_info = *node_info;
        stack->stack_i++;
    }

}

bool  Stack_Pop(stack_t* const stack, uint8 *NodeID, node_info_t* node_info)
{
    if(stack->stack_i > 0)
    {
        stack->stack_i--;
        if(NodeID != NULL)
            *NodeID = stack->stack_p[stack->stack_i].id;
        if(node_info != NULL)
            *node_info = stack->stack_p[stack->stack_i].node_info;
        return true;
    }
    else
    {
        return false;
    }
}
void Stack_Clear(stack_t* const stack)
{
    stack->stack_i = 0;
}

uint8 GetTypeFromID(uint8 ID)
{
    if(ID<MENU_ID_SEPARATOR)
    {
        return NODE_TYPE_ORPHAN;
    }
    else
    {
        return NODE_TYPE_TREE;
    }
}
uint8 GetIDFromIdx(uint8 node_type, uint8 idx)
{
    if(node_type == NODE_TYPE_ORPHAN)
    {
        return idx;
    }
    else if(node_type == NODE_TYPE_TREE)
    {
        return idx+MENU_ID_SEPARATOR+1;
    }
    return 0;
}
uint8 GetIdxFromID(uint8 id)
{
    if(id < MENU_ID_SEPARATOR)
    {
        return id;
    }
    else
    {
        return id-(MENU_ID_SEPARATOR+1);
    }
}
void  menu_display(void)
{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
    FeedWatchDog();
#endif
    if(GetTypeFromID(CurrentNodeID) == NODE_TYPE_ORPHAN)
    {
        menu_orphan_display();
    }
    else
    {
        menu_tree_display();
    }
}

void  menu_JumptoMenu(uint8 ID)
{

    if(ID == MENU_ID_INITNWK ||ID == MENU_ID_MAIN)
    {
        //Clear_Stack(&tree_stack);
        menu_tree_stack_clear();
    }
    if(ID != CurrentNodeID)
    {
        NearLastNodeID = CurrentNodeID;
        CurrentNodeID = ID;
        MENU_RESET_NODEINFO();
        menu_display();
    }
}
void  menu_JumpandMark(uint8 ID)
{

    if(ID != CurrentNodeID)
    {
        NodeID_jumpbackup = CurrentNodeID;
        node_info_jumpbackup = node_info;
        menu_JumptoMenu(ID);
    }
}

void menu_JumpBackWithMark(void)
{
    NearLastNodeID = CurrentNodeID;
    CurrentNodeID = NodeID_jumpbackup;
    node_info = node_info_jumpbackup;
    menu_display();
}
void menu_JumpBackMarkParent(void)
{
    NearLastNodeID = CurrentNodeID;
    CurrentNodeID = NodeID_jumpbackup;
    menu_steptoparent();
}

uint8 menu_GetJumpMark(void)
{
    return NodeID_jumpbackup;
}

/*
void Clr_Num_Buf(void)
{
osal_memset(num_buf.p, '\0', 1);
num_buf.len = 0;
}
*/
void  menu_Dial(const buf_t numbuff)
{
    Contact_Node c_node;
    Record new_record;
    uint8 idx;

    if(MP_IsNwkOn())
    {
        SET_ON_CALLING();
        menu_JumptoMenu(MENU_ID_DIALING);
        GetTimeChar(new_record.time);
        //num_buf.len = osal_strlen((char*)num_buf.p);
        //num_buf.p[num_buf.len] = '\0';

        //osal_memcpy(&new_record.num, numbuff.p, numbuff.len+1);
        num_str2term((app_termNbr_t*)new_record.num.nbr,numbuff.p);
        // Add_CallRecord(MENU_ID_CALLRECORD_DIALEDCALL, &new_record);

        //strcpy(new_record.num.Nmbr, numbuff.p);
        if(ZSuccess == menu_Contact_SearchContactByNum(&c_node, &idx, new_record.num.nbr))
            new_record.Contect_item_L=*((uint8 *)contact_item_L+idx);
        else  new_record.Contect_item_L=LIST_ITEM_NULL;
        Add_CallRecord(MENU_ID_CALLRECORD_DIALEDCALL, &new_record);

    }
    else
    {
        strcpy((char*)g_jump_buf, NONWK_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
        MP_StartMenuLibEvt(3800);
        HalRingOpen(RING_BUSY,OPENFLAG_ASSMS_POW);
        osal_start_timerEx(Hal_TaskID, HAL_RING_EVENT, 15);
    }

}

uint8 MP_Key2ASCII(uint8 key)
{
    if (key == HAL_KEY_0)
        return KEY2ASC_0;
    else if (key == HAL_KEY_1)
        return KEY2ASC_1;
    else if (key == HAL_KEY_2)
        return KEY2ASC_2;
    else if (key == HAL_KEY_3)
        return KEY2ASC_3;
    else if (key == HAL_KEY_4)
        return KEY2ASC_4;
    else if (key == HAL_KEY_5)
        return KEY2ASC_5;
    else if (key == HAL_KEY_6)
        return KEY2ASC_6;
    else if (key == HAL_KEY_7)
        return KEY2ASC_7;
    else if (key == HAL_KEY_8)
        return KEY2ASC_8;
    else if (key == HAL_KEY_9)
        return KEY2ASC_9;
    else if (key == HAL_KEY_STAR)
        return KEY2ASC_STAR;
    else if (key == HAL_KEY_POUND)
        return KEY2ASC_POUND;
    else
        return KEY2ASC_INVAID;
}



