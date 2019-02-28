#include "OSAL.h"
#include "OSAL_Nv.h"
#include "App_cfg.h"
#include "string.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "key.h"
#include "hal_audio.h"
#include "lcd_serial.h"
#include "TimeUtil.h"
#include "ZComDef.h"

#include "MenuLib_orphan.h"
#include "MenuLib_global.h"
#include "MobilePhone_MenuLib.h"
#include "MobilePhone_global.h"
#include "MobilePhone_MenuLibChinese.h"
#include "MenuChineseInputUtil.h"
#include "MenuAdjustUtil.h"
#include "MobilePhone_Function.h"
#include "MenuLib_Nv.h"
#ifdef CELLSWITCH_DEBUG
#include "OnBoard.h"
#endif

#include "numtrans.h"
#include "MenuLib_tree.h"

static  uint8             sig_index = 0;
static  uint8             bat_index;

extern uint8 contact_item_L[MAX_CONTACT_NUM];
const char Change_mode[6]={'*','1','2','3','4','\0'};

typedef struct
{
    uint8 ID;
    char* name;
    uint8 ParentID;
    uint8 FirstChildID;
    uint8 ChildNum;
    char* const  *ItemName_CH;
    MenuOper_t oper;
} Orphan_node_t;

/*dial number*/

/*identical functions */
static void    menu_poweron_animation_display(void);
static void    menu_poweroff_animation_display(void);
static void    menu_initnwk_display(void);
static void    menu_initnwk_onkey(uint8 keys, uint8 status);
static void    menu_longtime_clock_display(void);
static void    menu_longtime_clock_onkey(uint8 keys, uint8 status);

static void    menu_showing_number_display(void);
static void    menu_showing_number_onkey(uint8 keys, uint8 status);
static void    menu_dialing_display(void);
static void    menu_dialing_onkey(uint8 keys, uint8 status);
static void    menu_talking_display(void);
static void    menu_talking_onkey(uint8 keys, uint8 status);
static void    menu_incomingcall_display(void);
static void    menu_incomingcall_onkey(uint8 keys, uint8 status);
static void    menu_missingcall_display(void);
static void    menu_sm_sending_display(void);
static void    menu_missingcall_onkey(uint8 keys, uint8 status);

static void    menu_callrecord_detail_display(void);
static void    menu_callrecord_detail_onkey(uint8 keys, uint8 status);
static void    menu_incomingmessage_display(void);
static void    menu_incomingmessage_onkey(uint8 keys, uint8 status);
static void    menu_missingmessage_display(void);
static void    menu_missingmessage_onkey(uint8 keys, uint8 status);

static void    menu_typeing_display(void);
static void    menu_typeing_onkey(uint8 keys, uint8 status);
static void    menu_inputname_display(void);
static void    menu_inputname_onkey(uint8 keys, uint8 status);
static void    menu_inputnumber_display(void);
static void    menu_inputnumber_onkey(uint8 keys, uint8 status);
static void    menu_inputnumber_sms_onkey(uint8 keys, uint8 status);
static void    menu_inputnumber_contact_onkey(uint8 keys, uint8 status);

static void    menu_inputsymbol_display(void);
static void    menu_inputsymbol_onkey(uint8 keys, uint8 status);

static void    menu_showmessage_display(void);
//static void    menu_showmessage_onkey(uint8 keys, uint8 status);
static void    menu_showquestion_display(void);
static void    menu_showquestion_onkey(uint8 keys, uint8 status);
static void    menu_showalert_display(void);
static void    menu_showalert_onkey(uint8 keys, uint8 status);
static void    menu_busy_display(void);
static void    menu_busy_onkey(uint8 keys, uint8 status);
static void    menu_canlendar_display(void);
static void    menu_canlendar_onkey(uint8 keys, uint8 status);
static void    menu_main_display(void);
static void    menu_main_onkey(uint8 keys, uint8 status);
static void    Change_mode_between_D_R(void);

extern void MP_SetR_or_DInfo(void);
extern void MP_to_release_vesion(void);

/*internal functions */
#ifdef CELLSWITCH_DEBUG
void menu_ShowCellSwitch(uint16 panID);
#endif

/*Menu Defination */
static Orphan_node_t const  Menu_Orphan[] =
{
    {
        .ID = MENU_ID_ROOT,
        .oper.display = NULL,
        .oper.on_key = NULL,
    }
    ,
    /*independent ID in the root, parrelled with main menu*/
    {
        .ID = MENU_ID_POWERON_ANIMATION,
        .oper.display =menu_poweron_animation_display,
        .oper.on_key = NULL,
    }
    ,
    {
        .ID = MENU_ID_POWEROFF_ANIMATION,
        .oper.display = menu_poweroff_animation_display,
        .oper.on_key = NULL,
    }
    ,
    {
        .ID = MENU_ID_INITNWK,
        .oper.display = menu_initnwk_display,
        .oper.on_key = menu_initnwk_onkey,
    }
    ,
    {
        .ID = MENU_ID_LONGTIME_CLOCK,
        .oper.display = menu_longtime_clock_display,
        .oper.on_key = menu_longtime_clock_onkey,
    }
    ,
    {
        .ID = MENU_ID_SHOWING_NUMBER,
        .oper.display = menu_showing_number_display,
        .oper.on_key = menu_showing_number_onkey
    }
    ,
    {
        .ID = MENU_ID_DIALING,
        .oper.display = menu_dialing_display,
        .oper.on_key = menu_dialing_onkey
    }
    ,
    {
        .ID = MENU_ID_TALKING,
        .oper.display = menu_talking_display,
        .oper.on_key = menu_talking_onkey
    }
    ,
    {
        .ID = MENU_ID_INCOMINGCALL,
        .oper.display = menu_incomingcall_display,
        .oper.on_key = menu_incomingcall_onkey
    }
    ,
    {
        .ID = MENU_ID_MISSINGCALL,
        .oper.display = menu_missingcall_display,
        .oper.on_key = menu_missingcall_onkey
    }
    ,
    {
        .ID = MENU_ID_INCOMINGSMS,
        .oper.display = menu_incomingmessage_display,
        .oper.on_key = menu_incomingmessage_onkey,
    }
    ,
    {
        .ID = MENU_ID_MISSINGMESSAGE,
        .oper.display = menu_missingmessage_display,
        .oper.on_key = menu_missingmessage_onkey
    }
    ,
    {
        .ID = MENU_ID_SM_SENDING,
        .oper.display = menu_sm_sending_display,
        .oper.on_key = NULL
    }
    ,
    {
        .ID = MENU_ID_CALLRECORD_DETAIL,
        .oper.display =  menu_callrecord_detail_display,//NULL,
        .oper.on_key = menu_callrecord_detail_onkey
    }
    ,
    {
        .ID = MENU_ID_TYPEING,
        .oper.display = menu_typeing_display,
        .oper.on_key = menu_typeing_onkey
    }
    ,
    {
        .ID = MENU_ID_INPUTCHINESE,
        .oper.display = NULL,//menu_inputchinese_display,
        .oper.on_key = NULL,//menu_inputchinese_onkey
    }
    ,
    {
        .ID = MENU_ID_INPUTNAME,
        .oper.display = menu_inputname_display,
        .oper.on_key = menu_inputname_onkey
    }
    ,
    {
        .ID = MENU_ID_INPUTNUMBER_SMS,
        .oper.display = menu_inputnumber_display,
        .oper.on_key = menu_inputnumber_sms_onkey,
    }
    ,
    {
        .ID = MENU_ID_INPUTNUMBER_CONTACT,
        .oper.display = menu_inputnumber_display,
        .oper.on_key = menu_inputnumber_contact_onkey,
    }
    ,
    {
        .ID = MENU_ID_INPUTSYMBOL,
        .oper.display = menu_inputsymbol_display,
        .oper.on_key = menu_inputsymbol_onkey
    }
    ,
    {
        .ID = MENU_ID_SHOWMESSAGE,
        .oper.display = menu_showmessage_display,
        .oper.on_key = NULL//menu_showmessage_onkey
    }
    ,
    {
        .ID = MENU_ID_SHOWQUESTION,
        .oper.display = menu_showquestion_display,
        .oper.on_key = menu_showquestion_onkey
    }
    ,
    {
        .ID = MENU_ID_SHOWALERT,
        .oper.display = menu_showalert_display,
        .oper.on_key = menu_showalert_onkey
    }
    ,
    {
        .ID = MENU_ID_BUSY,
        .oper.display = menu_busy_display,
        .oper.on_key = menu_busy_onkey
    }
    ,
    {
        .ID = MENU_ID_ADJUSTVOLUME,
        .oper.display = menu_adjustvol_display,
        .oper.on_key = menu_adjustvol_onkey
    }
    ,
    {
        .ID = MENU_ID_ADJUSTTIME,
        .oper.display = menu_adjusttime_display,
        .oper.on_key = menu_adjusttime_onkey
    }
    ,
    {
        .ID = MENU_ID_ADJUSTDATE,
        .oper.on_key = menu_adjustdate_onkey
    }
    ,
    {
        .ID = MENU_ID_CANLENDAR,
        .oper.display = menu_canlendar_display,
        .oper.on_key = menu_canlendar_onkey
    }
    ,
    {
        .ID = MENU_ID_MAIN,
        .oper.display = menu_main_display,
        .oper.on_key = menu_main_onkey
    }

};
/*------------------- functions ----------------------------*/
void    menu_orphan_nodeID_check()
{
    uint8 len = sizeof(Menu_Orphan)/sizeof(Menu_Orphan[0]);
    for(uint8 i =0; i<len; i++)
    {
        if(Menu_Orphan[i].ID != GetIDFromIdx(NODE_TYPE_ORPHAN,i))
        {
            while(1);
        }
    }
}
void menu_orphan_handle_key(uint8 keys, uint8 status)
{
    if(GetTypeFromID(CurrentNodeID) == NODE_TYPE_ORPHAN)
    {
        uint8 idx = GetIdxFromID(CurrentNodeID);
        if(Menu_Orphan[idx].oper.on_key)
            Menu_Orphan[idx].oper.on_key(keys,status);
    }
}

void menu_orphan_display()
{
    if(GetTypeFromID(CurrentNodeID) == NODE_TYPE_ORPHAN)
    {
        uint8 idx = GetIdxFromID(CurrentNodeID);
        if(Menu_Orphan[idx].oper.display)
            Menu_Orphan[idx].oper.display();
    }
}

void menu_set_signal(uint8 idx)
{
    sig_index = idx;
}
void menu_set_battery(uint8 idx)
{
    bat_index = idx;
}

/*identical functions */
 static void    menu_poweron_animation_display(void)
{
    HalResetBackLightEvent();
}

static void    menu_poweroff_animation_display(void)
{

}

static void    menu_initnwk_display(void)
{
    if(CurrentNodeID != MENU_ID_INITNWK)
        return;

}

static void    menu_initnwk_onkey(uint8 keys, uint8 status)
{
}
static void    menu_longtime_clock_display(void)
{
}
static void    menu_longtime_clock_onkey(uint8 keys, uint8 status)
{
}

static void    menu_showing_number_display(void)
{

}

static void Change_mode_between_D_R()
{
	MP_SetR_or_DInfo();
	SystemReset();
}

static void    menu_showing_number_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_POUND:
	 if(!strcmp((char *)Change_mode, num_buf.p))
         {
	        Change_mode_between_D_R();
                break;
         }
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
    case HAL_KEY_STAR:
        if(num_buf.len < APP_NMBRDIGIT*2 - 1)
        {
            num_buf.p[num_buf.len++] = MP_Key2ASCII(keys);
            num_buf.p[num_buf.len] = '\0';
            menu_display();
        }
        break;

    case HAL_KEY_CALL:
        Buffer_Copy(&dialnum_buf,&num_buf);
        menu_Dial(dialnum_buf);
        break;
    case HAL_KEY_SELECT:
        Stack_Push(&global_stack, CurrentNodeID, NULL);
        shortcuts_flag = TRUE;
        menu_JumptoMenu(MENU_ID_INPUTNAME);
        break;
    case HAL_KEY_BACKSPACE:
        num_buf.p[--num_buf.len]  = '\0';
        if(num_buf.len == 0)
        {
            menu_JumptoMenu(MENU_ID_MAIN);
        }
        else
        {
            menu_display();
        }
        break;
    default:
        break;
    }
}
static void    menu_dialing_display(void)
{
}
static void    menu_dialing_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_CALL:
        SET_ON_CALLINGWAIT();
        break;
    case HAL_KEY_RIGHT:
    case HAL_KEY_LEFT:
        menu_JumpandMark(MENU_ID_ADJUSTVOLUME);
        //osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 1000);
        MP_StartMenuLibEvt(1000);
        break;
    default:
        break;
    }

}
static void    menu_talking_display(void)
{

}
static void    menu_talking_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_RIGHT:
    case HAL_KEY_LEFT:
        menu_JumpandMark(MENU_ID_ADJUSTVOLUME);
        //osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 1000);
        MP_StartMenuLibEvt(1000);
        break;
    case HAL_KEY_CANCEL:
        if(NearLastNodeID == MENU_ID_ROOT)
        {
            menu_JumptoMenu(MENU_ID_MAIN);
        }
        break;

    default:
        break;
    }
}
static void    menu_incomingcall_display(void)
{
}
static void    menu_incomingcall_onkey(uint8 keys, uint8 status)
{
    Contact_Node c_node;
    Record new_record;
    uint8 idx;

    switch(keys)
    {
    case HAL_KEY_CALL:
        menu_JumptoMenu(MENU_ID_TALKING);
        if(dialnum_buf.p != NULL)
        {
            GetTimeChar(new_record.time);
            //osal_memcpy(&new_record.num, dialnum_buf.p, dialnum_buf.len+1);
            //Clr_Num_Buf();

            num_str2term((app_termNbr_t*)new_record.num.nbr,dialnum_buf.p);
            if(ZSuccess == menu_Contact_SearchContactByNum(&c_node, &idx, new_record.num.nbr))
                new_record.Contect_item_L=*((uint8 *)contact_item_L+idx);
            else  new_record.Contect_item_L=LIST_ITEM_NULL;
            Add_CallRecord(MENU_ID_CALLRECORD_ANSWEREDCALL, &new_record);
        }
        //Buffer_Free(&dialnum_buf);
        break;
    case HAL_KEY_CANCEL:
        //menu_JumptoMenu(MENU_ID_MAIN);
        if(dialnum_buf.p != NULL)
        {
            GetTimeChar(new_record.time);
            //osal_memcpy(&new_record.num, dialnum_buf.p, dialnum_buf.len+1);
            num_str2term((app_termNbr_t*)new_record.num.nbr,dialnum_buf.p);
            Add_CallRecord(MENU_ID_CALLRECORD_ANSWEREDCALL, &new_record);
        }
        //Buffer_Free(&dialnum_buf);
        break;
    default:
        break;
    }


}

static void    menu_missingcall_display(void)
{
}

static void    menu_missingcall_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_SELECT:
        shortcuts_flag = TRUE;
        missed_call_amount = 0;
        menu_JumptoMenu(MENU_ID_CALLRECORD_MISSEDCALL);
        break;
    case HAL_KEY_BACKSPACE:
        shortcuts_flag = FALSE;
        missed_call_amount = 0;
        menu_JumptoMenu(MENU_ID_MAIN);
        break;
    case HAL_KEY_CANCEL:
        shortcuts_flag = FALSE;
        missed_call_amount = 0;
        break;
    default:
        break;
    }
    //Buffer_Free(&dialnum_buf);
}

static void    menu_callrecord_detail_display(void)
{
        //Record_type recordtype;
        Record record;
        menu_Record_ReadRecord(&record,(node_info_jumpbackup.sel_item-1),s_recordtype);
}

static void    menu_callrecord_detail_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_CALL:
        Buffer_Copy(&dialnum_buf, &num_buf);
        menu_Dial(dialnum_buf);
        break;
    case HAL_KEY_SELECT:
        menu_JumptoMenu(MENU_ID_SHOWING_NUMBER);
        break;
    case HAL_KEY_BACKSPACE:
        menu_JumpBackWithMark();
        break;
    default:
        break;
    }
}


static void    menu_incomingmessage_display(void)
{

}

static void    menu_incomingmessage_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_SELECT:
        if(NearLastNodeID != MENU_ID_SHORTMESSAGE_INCOMINGBOX)
            shortcuts_flag = TRUE;
        menu_JumptoMenu(MENU_ID_SHORTMESSAGE_INCOMINGBOX);
        break;
    case HAL_KEY_BACKSPACE:
        menu_JumptoMenu(MENU_ID_MAIN);
        break;
    default:
        break;
    }
}
static void    menu_missingmessage_display(void)
{
}
static void    menu_sm_sending_display(void)
{

}
static void    menu_missingmessage_onkey(uint8 keys, uint8 status)
{
}

static void    menu_typeing_display(void)
{
}
static void    menu_typeing_onkey(uint8 keys, uint8 status)
{
}
static void    menu_inputname_display(void)
{
    if(NULL== data_buf.p)
    {
        if(NULL == Buffer_Init(&data_buf, MAX_NAME_LEN+1))
            return;
    }
    menu_inputchinese_display();
}

static bool    menu_inputname_output_handle(uint8 keys, uint8 input_status)
{

    if(input_status == OUTPUT_STATUS)
    {
        if(keys == HAL_KEY_BACKSPACE)
        {
            if(data_buf.len == 0)
            {
                Buffer_Free(&data_buf);
                Stack_Pop(&global_stack, &CurrentNodeID, NULL);
                menu_display();
                //menu_JumptoMenu(NearLastNodeID);
            }
            else
            {
                if(data_buf.p[data_buf.len-1] > 0x80)
                {
                    data_buf.p[--data_buf.len] = '\0';
                    data_buf.p[--data_buf.len] = '\0';
                }
                else
                {
                    data_buf.p[--data_buf.len] = '\0';
                }
            }

            return TRUE;
        }
        else if(keys == HAL_KEY_SELECT)
        {
            if(data_buf.len > 0)
            {
                uint8 contact_num = 0;
#ifdef NEW_DOUBLE_NVID_OP
                menu_Contact_ReadContactNum(&contact_num);
#endif
                if(contact_num >= MAX_CONTACT_NUM)
                {
                    strcpy((char *)g_jump_buf,FULL_CONTACTLIST_CHINA);
                    menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
                    MP_StartMenuLibEvt(1000);
                }
                else
                {
#ifdef NEW_DOUBLE_NVID_OP
                    if(strlen((char*)num_buf.p) <(APP_NMBRDIGIT*2) && strlen((char*)data_buf.p) < MAX_NAME_LEN)
                    {
                        Contact_Node Node;
                        num_str2term((app_termNbr_t*)Node.num.nbr,num_buf.p);
                        strcpy((char*)Node.name,(char*)data_buf.p);
                        menu_Contact_AddContact(&Node);
                    }
#endif
                    Buffer_Free(&data_buf);
                    Stack_Clear(&global_stack);
                    menu_JumptoMenu(MENU_ID_CONTACTLIST);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}


static void    menu_inputname_onkey(uint8 keys, uint8 status)
{
    static uint8 input_status = OUTPUT_STATUS;
    uint8 *output_p = NULL;

    //it will return after handle the character of name
    if(menu_inputname_output_handle(keys, input_status))
        return;

    //input new character for name of contact
    input_status = menu_inputchinese_onkey(keys, status);

    //if the status of input function is OUTPUT_STATUS, the new character
    //should be print on the LCD
    if(input_status == OUTPUT_STATUS)
    {
        output_p = menu_ChineseOutput();

        if(menu_ChineseOutput_Length() > 0)
        {
            uint8 len_output = 0;

            if((data_buf.len >= (MAX_NAME_LEN-1)) || ((data_buf.len >= (MAX_NAME_LEN-2))&&(output_p[0]>0x80)))
            {
                strcpy((char *)g_jump_buf, NAME_INPUT_FULL_CHINA);
                menu_JumpandMark(MENU_ID_SHOWMESSAGE);
                return;
            }
            len_output = menu_ChineseOutput_Length();
            osal_memcpy(data_buf.p+data_buf.len, output_p, len_output);
            data_buf.p[data_buf.len+len_output] = '\0';
            data_buf.len += len_output;
            menu_ChineseOutputClear();
        }
    }
}

static void    menu_inputnumber_display(void)
{

}

static void    menu_inputnumber_sms_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_SELECT)
    {
        if(num_buf.len > 0)
        {
            //set shortmessage sending status
            SET_ON_SM_SENDING();
            Stack_Pop(&global_stack, &CurrentNodeID, NULL);
            menu_JumptoMenu(MENU_ID_SM_SENDING);
        }
    }
    else
    {
        menu_inputnumber_onkey( keys,  status);
    }
}
static void    menu_inputnumber_contact_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_SELECT)
    {
        if(num_buf.len > 0)
        {
            Stack_Push(&global_stack, CurrentNodeID, NULL);
            menu_JumptoMenu(MENU_ID_INPUTNAME);
        }
    }
    else
    {
        menu_inputnumber_onkey( keys,  status);
    }

}

static void    menu_inputnumber_onkey(uint8 keys, uint8 status)
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
        if(num_buf.len < APP_NMBRDIGIT*2-1)
        {
            num_buf.p[num_buf.len++] = MP_Key2ASCII(keys);
            num_buf.p[num_buf.len] = '\0';
            menu_display();
        }
        break;

    case HAL_KEY_BACKSPACE:
        if(num_buf.len == 0)
        {
            Buffer_Clear(&num_buf);
            if(NearLastNodeID == MENU_ID_SHORTMESSAGE_WRITINGBOX)
            {
                Stack_Pop(&global_stack, &CurrentNodeID, NULL);
                //Pop_PipeLine();
                menu_display();
            }
#ifdef SMS_TEMPLATE
            else if(NearLastNodeID == MENU_ID_SMS_TEMPLATE_HANDLE)
            {
                Stack_Pop(&global_stack, &CurrentNodeID, NULL);
                Stack_Pop(&global_stack, &CurrentNodeID, NULL);
                menu_display();
            }
#endif
#ifdef SMS_SENDBOX
            else if(NearLastNodeID == MENU_ID_SMS_SENDBOX_HANDLE)
            {
                Stack_Pop(&global_stack, &CurrentNodeID, NULL);
                Stack_Pop(&global_stack, &CurrentNodeID, NULL);
                menu_display();
            }
#endif
            else
            {
                NearLastNodeID = CurrentNodeID;
                Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
                menu_display();
                //menu_JumpBackWithMark();
            }
        }
        else
        {
            num_buf.p[--num_buf.len]  = '\0';
            menu_display();
        }
        break;
    default:
        break;
    }
}

static void    menu_inputsymbol_display(void)
{
}
static void    menu_inputsymbol_onkey(uint8 keys, uint8 status)
{
}

static void    menu_showmessage_display(void)
{
    MP_StartMenuLibEvt(500);
}

/* question should be put into g_buf  first*/
static void    menu_showquestion_display(void)
{
    switch(menu_GetJumpMark())
    {
    case    MENU_ID_SETTINGS:
    {
        break;
    }
    case   MENU_ID_CALLRECORD_DELETE:
    {
        break;
    }
    }
}

static void    menu_showquestion_onkey(uint8 keys, uint8 status)
{
    if(keys ==  HAL_KEY_SELECT)
    {
        switch(menu_GetJumpMark())
        {
        case    MENU_ID_SETTINGS:
        {
            set_info_t set_info;
            MP_SettingInformation_GetDefault(&set_info);
            MP_SettingInformation_Handout(&set_info);
            MP_SettingInformation_WriteFlash(&set_info);
	     MP_to_release_vesion();
            strcpy((char *)g_jump_buf,SETTED_CHINA);
            menu_JumptoMenu(MENU_ID_SHOWMESSAGE);

            break;
        }
        case   MENU_ID_CALLRECORD_DELETE:
        {
            uint8 delete_sel = g_jump_buf[0];
            if(delete_sel == 0)   //missed call
            {
                menu_Record_DeleteAll(Record_type_MISSED);
            }
            else if(delete_sel == 1)   //answered call
            {
                menu_Record_DeleteAll(Record_type_ANSWERED);
            }
            else if(delete_sel == 2)   //dialed call
            {
                menu_Record_DeleteAll(Record_type_DIALED);
            }
            strcpy((char *)g_jump_buf,DELETED_CHINA);
            menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
            break;
        }
        }

    }
    else if(keys == HAL_KEY_BACKSPACE)
    {
        menu_JumpBackWithMark();
    }
}
static void    menu_showalert_display(void)
{
}
static void    menu_showalert_onkey(uint8 keys, uint8 status)
{
}
static void    menu_busy_display(void)
{
}
static void    menu_busy_onkey(uint8 keys, uint8 status)
{
}

static void    menu_canlendar_display(void)
{
    uint8 date_str[DATE_LEN];
    uint8 week_str[16];

    GetDateChar(date_str);
    GetWeekChar(week_str);
}
static void    menu_canlendar_onkey(uint8 keys, uint8 status)
{
    if(keys == HAL_KEY_SELECT)
    {
        menu_JumpBackWithMark();
    }
}
static void    menu_main_display(void)
{
    static uint8 s_unread_sms;

#ifdef NEW_DOUBLE_NVID_OP
    if(isneed_judge_sms==true)
    	{
    	menu_SMS_Read_unread(&s_unread_sms);
    	}
#endif
    /*
    osal_nv_read(MINEAPP_NV_SMS, sizeof(uint8)+ MAX_SMS_NUM*SMS_NV_LEN, sizeof(uint8), &new_sms_flag);
    if(new_sms_flag)
    {
    LCD_SMS_ICON_Show();
    }*/
    Menu_UpdateTime();
    Buffer_Free(&data_buf);
    Buffer_Clear(&num_buf);
    Buffer_Clear(&dialnum_buf);
    Stack_Clear(&global_stack);

    menu_ChineseOutputClear();
}

static void    menu_main_onkey(uint8 keys, uint8 status)
{

    //if(HalGetPadLockEnable())
    {
        NearLastNodeID = CurrentNodeID;
        uint8 stat = HalGetPadLockStat();
        if(stat == PADLOCK_LOCKED || stat == PADLOCK_ALERT)
        {
            if(keys == HAL_KEY_SELECT)
            {
                HalSetPadLockStat(PADLOCK_MID);
                MP_StartMenuLibEvt(3000);
            }
            else
            {
                HalSetPadLockStat(PADLOCK_ALERT);
                MP_StartMenuLibEvt(1000);
            }
            menu_display();
            return;
        }
        else if(stat == PADLOCK_MID)
        {
            if(keys == HAL_KEY_STAR)
            {
                HalSetPadLockStat(PADLOCK_UNLOCKED);
                //osal_stop_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT);
                MP_StopMenuLibEvt();
            }
            else
            {
                HalSetPadLockStat(PADLOCK_ALERT);
                //osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 1000);
                MP_StartMenuLibEvt(1000);
            }
            menu_display();
            return;
        }
    }

    /* other situations are normal*/
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
        case HAL_KEY_STAR:
        case HAL_KEY_POUND:
            num_buf.p[num_buf.len++] = MP_Key2ASCII(keys);
            num_buf.p[num_buf.len] = '\0';
            menu_JumptoMenu(MENU_ID_SHOWING_NUMBER);
            break;
        case HAL_KEY_CALL:
            shortcuts_flag = TRUE;
            menu_JumptoMenu(MENU_ID_CALLRECORD_DIALEDCALL);
            break;
        case HAL_KEY_SELECT:
            menu_JumptoMenu(MENU_ID_FUNCTIONLIST);
            break;
        case HAL_KEY_BACKSPACE:
            shortcuts_flag = TRUE;
            menu_JumptoMenu(MENU_ID_CONTACTLIST);
            break;
        default:
            break;
        }
    }
}


#ifdef CELLSWITCH_DEBUG
void menu_ShowCellSwitch(uint16 panID)
{
    uint8 start_pos = 3;
    uint8 p[8];
    _itoa(panID, (char *)p, 10);
}
#endif

