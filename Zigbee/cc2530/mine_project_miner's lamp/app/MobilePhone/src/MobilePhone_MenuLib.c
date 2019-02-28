#include "OSAL.h"
#include "string.h"
#include "OSAL_NV.h"
#include "App_cfg.h"
#include "hal_key.h"
#include "key.h"
#include "TimeUtil.h"
#include "lcd_serial.h"
#include "ZComDef.h"

#include "MobilePhone_MenuLib.h"
#include "MobilePhone_MenuLibChinese.h"
#include "MenuChineseInputUtil.h"
#include "MobilePhone_global.h"
#include "MenuLib_global.h"
#include "MenuLib_orphan.h"
#include "MenuLib_tree.h"
#include "MenuAdjustUtil.h"
#include "MobilePhone_Function.h"
#include "MenuLib_Nv.h"

#include "hal_audio.h"
#include "hal_drivers.h"
#include "app_protocol.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchdogUtil.h"
#endif
//#ifdef RSSI_INFORMATION
#include "Onboard.h"
#include "Mac_radio_defs.h"

//#endif
#include "MobilePhone.h"
#include "numtrans.h"

#define SAVE_SUCCESS 0
#define SAVE_FAILED    1

/*-----------------Function declares-----------------*/
extern void MP_to_release_vesion(void);

/*Internal functions */
static bool       Full_SMS_Ring_Flag = TRUE;
static uint8     menu_nv_init(void);
static void      menu_nodeID_check(void);
#ifdef SMS_TEMPLATE
static char  *const   SMS_TepmlateList[] = {SMS_TEMPLATE_1, SMS_TEMPLATE_2, SMS_TEMPLATE_3};
#endif
extern bool  Menu_rf_debug;

/*general functions */
bool Menu_Get_SMS_Full_Ring_Flag(void)
{
    return Full_SMS_Ring_Flag;
}
void Menu_Set_SMS_Full_Ring_Flag(bool flag)
{
    Full_SMS_Ring_Flag = flag;
}
uint8 *Menu_Get_SMS_Data(void)
{
    return data_buf.p;
}

uint8 Menu_Get_SMS_Len(void)
{
    if(data_buf.p != NULL)
        return data_buf.len;
    return 0;
}

uint8 Menu_Init(void)
{
    SET_ON_IDLE();
    menu_nodeID_check();
    Buffer_Init(&num_buf, APP_NMBRDIGIT*2+1);
    Buffer_Init(&dialnum_buf, APP_NMBRDIGIT*2+1);
    Menu_Stack_Init();
    menu_nv_init();

    //must be after nv init;
    InitialLcd();

    set_info_t set_info;
    MP_SettingInformation_ReadFlash(&set_info);
    MP_SettingInformation_Handout(&set_info);
    return 0;
}

static uint8     menu_nv_init()
{
    uint8   temp, pos = 0;
    uint16 len;

    len = sizeof(set_info_t);//the length of mp setting information

    temp = osal_nv_item_init(MINEAPP_NV_SET_INFORMATION, len, NULL);
    if(temp == NV_ITEM_UNINIT)
    {
        set_info_t set_info;
        MP_SettingInformation_GetDefault(&set_info);
        MP_SettingInformation_Handout(&set_info);
        MP_SettingInformation_WriteFlash(&set_info);

    }
    else if(temp == NV_OPER_FAILED)
    {
        return NV_OPER_FAILED;
    }

    menu_Contact_nv_init();
    menu_Record_nv_init();
    int_contact_item_L();
    int_SMS_item_L();

#ifdef NEW_DOUBLE_NVID_OP

    temp = SMS_List_init(SMSTYPE_INBOX);
    if(temp == NV_OPER_FAILED)
    {
        return NV_OPER_FAILED;
    }
    menu_SMS_Read_Num(&pos,SMSTYPE_INBOX);
#endif

    temp = osal_nv_item_init(MP_STOREPARAM_ITEM, sizeof(uint8), NULL);
    if(temp == NV_ITEM_UNINIT)
    {
        pos = 0x00;
        osal_nv_write(MP_STOREPARAM_ITEM,0, sizeof(uint8), &pos);
    }
    else if(temp == NV_OPER_FAILED)
    {
        return NV_OPER_FAILED;
    }


    temp = osal_nv_item_init(MP_SMS_SEQNUM, sizeof(uint16), NULL);
    if(temp == NV_ITEM_UNINIT)
    {
        len = 0;
        osal_nv_write(MP_SMS_SEQNUM,0, sizeof(uint16), &len);
    }
    else if(temp == NV_OPER_FAILED)
    {
        return NV_OPER_FAILED;
    }

    temp = osal_nv_item_init(MP_DISPLAY_PARAMETER, sizeof(displayParam_t), NULL);
    if(temp == NV_ITEM_UNINIT)
    {
        displayParam_t   displayparam;
        displayparam.paramTen =LCD_PARAMETER1;
        displayparam.paramTwelve =LCD_PARAMETER2;
	    displayparam.checkSum = displayparam.paramTen ^ displayparam.paramTwelve;

        osal_nv_write(MP_DISPLAY_PARAMETER,0, sizeof(displayParam_t), &displayparam);
    }
    else if(temp == NV_OPER_FAILED)
    {
        return NV_OPER_FAILED;
    }


#ifdef SMS_SENDBOX
    temp = SMS_List_init(SMSTYPE_SEND);
    if(temp == NV_OPER_FAILED)
        return NV_OPER_FAILED;
#endif


#ifdef SMS_TEMPLATE
    for ( uint8 i=0; i< MAX_SMS_NUM_TEMPLATE; i++ )
    {
        temp = osal_nv_item_init ( MP_NV_SMS_TEMPLATE_BASE + i, sizeof ( sms_saved_t ), NULL );
        if(temp == NV_ITEM_UNINIT)
        {
            sms_saved_t  sms;
            strcpy((char*)sms.content.p,(char *) code_strlen(SMS_TepmlateList[i]));
            sms.head.item_head.isvalid=false;
            if(i==0)
            {
                sms.head.item_head.item_self=LIST_ITEM_START;
                sms.head.item_head.item_next=MP_NV_SMS_TEMPLATE_BASE+i;
            }
            else
                sms.head.item_head.item_self=MP_NV_SMS_TEMPLATE_BASE + i;
            if(i==(MAX_SMS_NUM_TEMPLATE-1))
                sms.head.item_head.item_next=LIST_ITEM_END;
            else if(i!=0)
                sms.head.item_head.item_next=sms.head.item_head.item_self+1;
            if ( ZSuccess!=osal_nv_write ( MP_NV_SMS_TEMPLATE_BASE + i,0, sizeof ( sms_saved_t ),  &sms ) )
                temp = MP_STATUS_OPER_FAILED;

        }
        else if(temp == NV_OPER_FAILED)
        {
            return NV_OPER_FAILED;
        }
    }

#endif
    return ZSUCCESS;
}

void Read_display_param_form_flash(displayParam_t *displayParam)
{
    uint8 flag=0;

    displayParam->paramTen = LCD_PARAMETER1;
    displayParam->paramTwelve = LCD_PARAMETER2;

    flag = osal_nv_read(MP_DISPLAY_PARAMETER,0, sizeof(displayParam_t), displayParam);
    if((flag!=ZSUCCESS)
        ||(displayParam->checkSum  != (displayParam->paramTen^displayParam->paramTwelve))
      )
    {
            displayParam->paramTen = LCD_PARAMETER1;
            displayParam->paramTwelve = LCD_PARAMETER2;
    }
}

void Set_display_param_to_flash(uint8 paramTen, uint8 paramTwelve )
{
            displayParam_t displayparam;
            displayparam.paramTen = paramTen;
            displayparam.paramTwelve = paramTwelve;
            displayparam.checkSum  = paramTen^paramTwelve;
            osal_nv_write(MP_DISPLAY_PARAMETER,0, sizeof(displayParam_t), &displayparam);
}

void Menu_handle_key(uint8 keys, uint8 status)
{
    if(GetTypeFromID(CurrentNodeID) == NODE_TYPE_ORPHAN)
    {
        menu_orphan_handle_key( keys,  status);
    }
    else
    {
        menu_tree_handle_key( keys,  status);
    }


    //if(new_sms_flag)
    //	HalRingClose();

    /* process power short press here */
    if(keys == HAL_KEY_POWER && CurrentNodeID != MENU_ID_INITNWK
            && CurrentNodeID != MENU_ID_POWERON_ANIMATION
            && CurrentNodeID != MENU_ID_POWEROFF_ANIMATION)
    {
        SET_ON_IDLE();
        if(((CurrentNodeID == MENU_ID_TALKING) || ((CurrentNodeID == MENU_ID_INCOMINGCALL))) && (NearLastNodeID != MENU_ID_DIALING))
        {

            NearLastNodeID = CurrentNodeID;
            if(true == Stack_Pop(&global_stack, &CurrentNodeID, &node_info))
            {
                menu_display();
                return;
            }
        }
#ifdef CFG_STATION_CHECK
        if(CurrentNodeID == MENU_ID_SETTINGS_STATION_CHECK)
        {
            SystemReset();
        }
#endif
        Buffer_Clear(&num_buf);
        Buffer_Clear(&dialnum_buf);
        Buffer_Free(&data_buf);
        shortcuts_flag = FALSE;
        menu_JumptoMenu(MENU_ID_MAIN);
        //Buffer_Free(&dialnum_buf);
    }
    return;
}

void Menu_handle_msg(uint8 MSG, const char *p, uint8 len)
{
    Record new_record;

    switch(MSG)
    {
    case MSG_INIT_NWK:
        //if(MP_NwkInfo.nwkStateDetail != NWK_DETAIL_JOINASSOCING)
        menu_JumptoMenu(MENU_ID_INITNWK);
        break;
    case MSG_INIT_MAIN:
        SET_ON_IDLE();
        if(CurrentNodeID != MENU_ID_MAIN)
        {
            HalSetPadLockStat(PADLOCK_UNLOCKED);
            menu_JumptoMenu(MENU_ID_MAIN);
        }
        break;
    case MSG_INCOMING_CALL:
        Stack_Push(&global_stack, CurrentNodeID, &node_info);//backup the message of current menu
        SET_ON_CALLED();
        //if(dialnum_buf.p != NULL)
        //{
        //    dialnum_buf.len = osal_strlen((char *)p);
        //    osal_memcpy(dialnum_buf.p, p,  dialnum_buf.len);
       //     dialnum_buf.p[dialnum_buf.len] = '\0';
       // }
        menu_JumptoMenu(MENU_ID_INCOMINGCALL);
	//CurrentNodeID=MENU_ID_INCOMINGCALL;
        if(ON_CALLED())    //Called side.
        {
           // if(MENU_ID_INCOMINGCALL != CurrentNodeID)
            //{
                menu_JumptoMenu(MENU_ID_TALKING);
            //}
            //MP_SendCmd(MP_UP_ACCEPT, &MP_AudioInfo.peer_termnbr, MP_AudioInfo.cmdseqnum++);
            MP_AudioInfo.accept_cnt = 0;
            MP_AudioInfo.retrying_bitmap |= MP_ACCEPT_RETRY_BIT;
            MP_start_timerEx(MP_TaskId, MP_ACCEPT_RETRY_EVENT, 200);

            MP_StartTalk();
            MP_ResetAudioStatus(MP_STOP_AUDIO_TIMEOUT);//FIXME:To avoid the other side not start normally.
        }


		
        break;
    case MSG_DIALING_SUCCESS:
        SET_ON_AUDIO();
        menu_JumptoMenu(MENU_ID_TALKING);
        break;
    case MSG_MISSED_CALL:
        missed_call_amount++;
        SET_ON_IDLE();
        if(dialnum_buf.p != NULL)
        {
            GetTimeChar(new_record.time);
            //osal_memcpy(&new_record.num, dialnum_buf.p, dialnum_buf.len + 1);
            num_str2term((app_termNbr_t*)new_record.num.nbr,dialnum_buf.p);
            Add_CallRecord(MENU_ID_CALLRECORD_MISSEDCALL, &new_record);
        }
        menu_JumptoMenu(MENU_ID_MISSINGCALL);
        break;
    case MSG_NO_POWER:
        //menu_JumptoMenu(MENU_ID_POWEROFF_ANIMATION);
    {
        //LcdClearDisplay();
        //LCD_Str_Print(NO_POWER_CHINA, 3, 1, TRUE);
        strcpy((char *)g_jump_buf, NO_POWER_CHINA);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
    }
    break;
    case MSG_PAD_LOCK:
        if((CurrentNodeID == MENU_ID_MAIN ) && HalGetPadLockEnable())
        {

            HalSetPadLockStat(PADLOCK_LOCKED);
            menu_JumptoMenu(MENU_ID_MAIN);
            menu_display();
        }
        break;
    case MSG_VOICE_FINISH:
        SET_ON_IDLE();
        if((NearLastNodeID == MENU_ID_DIALING)||(NearLastNodeID == MENU_ID_ROOT))
        {
            menu_JumptoMenu(MENU_ID_MAIN);
        }
        else
        {
            //Buffer_Free(&dialnum_buf);
            Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
            if(
                (CurrentNodeID>=MENU_ID_END)
                /* when CurrentNodeID=them , .display=null*/
                ||(CurrentNodeID==MENU_ID_CALLRECORD_DETAIL)
                ||(CurrentNodeID==MENU_ID_SHOWMESSAGE)
                /* not  use */
                //||(CurrentNodeID==MENU_ID_INPUTCHINESE)
                //||(CurrentNodeID==MENU_ID_CONTACT_ADD)
                //||(CurrentNodeID==MENU_ID_SETTINGS_RESTORE_DEFAULT)
             )
                {
                 CurrentNodeID=MENU_ID_MAIN;
                }
            menu_display();
        }
        break;
    case MSG_SMS_SUCCESS:
        SET_ON_IDLE();
        strcpy((char *)g_jump_buf, SM_SENDING_CHINA);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
        Buffer_Free(&data_buf);
        MP_StartMenuLibEvt(1000);
        break;
    case MSG_SMS_FAILED:
        SET_ON_IDLE();
        strcpy((char *)g_jump_buf, SM_SENDING_FAIL_CHINA);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
        MP_StartMenuLibEvt(1000);
        break;
    case MSG_SMS_INCOMING:
        //if(Get_SMS_Quantity() < MAX_SMS_NUM)
        if(Menu_Get_SMS_Full_Ring_Flag())
        {
            if((CurrentNodeID == MENU_ID_MAIN) || (CurrentNodeID == MENU_ID_SHORTMESSAGE_INCOMINGBOX)
                    || (CurrentNodeID == MENU_ID_SHORTMESSAGE_READ))
            {
                Stack_Clear(&global_stack);
                shortcuts_flag = TRUE;
                menu_JumptoMenu(MENU_ID_INCOMINGSMS);
            }
            else
            {
                if((CurrentNodeID == MENU_ID_SETTINGS_PADLOCK) || (CurrentNodeID == MENU_ID_SETTINGS_RING))
                    LCD_SMS_ICON_Show(0, 0);
                else
                    LCD_SMS_ICON_Show(LCD_LINE_WIDTH - 4, 0);
            }
        }
        else
        {
            if((CurrentNodeID == MENU_ID_SETTINGS_PADLOCK) || (CurrentNodeID == MENU_ID_SETTINGS_RING))
                LCD_SMS_ICON_Show(0, 0);
            else
                LCD_SMS_ICON_Show(LCD_LINE_WIDTH - 4, 0);
        }
        break;
    case MSG_POLL_END:
        if(ON_WAKE())
        {
            SET_ON_IDLE();
        }
        break;
    case MSG_POLL_START:
        if(IS_IDLE())
        {
            SET_ON_WAKE();
        }
        break;
    case MSG_REFRESH_SCREEN:
    {
        menu_display();
        break;
    }
    case MSG_POWERON_ANIM:
    {
        menu_JumptoMenu(MENU_ID_POWERON_ANIMATION);
        break;
    }
    case MSG_POWEROFF_ANIM:
    {
        MP_to_release_vesion();
        menu_JumptoMenu(MENU_ID_POWEROFF_ANIMATION);
        break;
    }
    default:
        break;
    }

}

void Menu_UpdateTime(void)
{
    uint8 p[TIME_LEN];
    GetTimeChar(p);
    //	if(CurrentNodeID == MENU_ID_MAIN &&
    //		!(HalGetPadLockEnable() && (HalGetPadLockStat() == PADLOCK_ALERT || HalGetPadLockStat() == PADLOCK_ALERT)) )
    if((CurrentNodeID == MENU_ID_MAIN) || (CurrentNodeID == MENU_ID_TALKING) || (CurrentNodeID == MENU_ID_DIALING) )
    {
        //LCD_Line_Clear(1);
        //LCD_Line_Clear(2);
        LCD_Str_Print(p, TOP_POS_TIME, 0, TRUE);
    }
}

void Menu_UpdateSignal(uint8 level)
{
    if(MP_IsNwkOn())
    {
        menu_set_signal(level);
    }
    else
    {
        menu_set_signal(0);
    }

    if((CurrentNodeID == MENU_ID_MAIN) || (CurrentNodeID == MENU_ID_TALKING) || (CurrentNodeID == MENU_ID_DIALING) )
    {
        LCD_Clear(1, 0, 2, 0);
        LCD_Signal_Show(level);
    }
}

void Menu_UpdateBattery(uint8 level)
{
    menu_set_battery(level);
    if((CurrentNodeID == MENU_ID_MAIN) || (CurrentNodeID == MENU_ID_TALKING) || (CurrentNodeID == MENU_ID_DIALING) )
    {
        LCD_Clear(14, 0, 16, 0);
        LCD_Battery_Show(level);
    }
}

#if  1//def 	RSSI_INFORMATION
void Menu_UpdateRSSI(int8 rssi)
{
    uint8 str[8];
    uint8 *p = str;

    if(CurrentNodeID == MENU_ID_MAIN)
    {
        if(rssi < 0)
        {
            rssi = -rssi;
            *p++ = '-';
            _itoa((uint16) rssi, (char *) p, 10);
        }
        else
        {
            _itoa((uint16) rssi, (char *) p, 10);
        }
        uint8 x_pos = 6;
        uint8 y_pos = 3;

        LCD_Clear(x_pos, y_pos, x_pos + 4, y_pos);
        LCD_Str_Print(str, x_pos, y_pos, true);
    }
}

void Menu_UpdateLinkFlag(bool flag)
{
    /* print Link flag */
    if(CurrentNodeID == MENU_ID_MAIN)
    {
        if(flag)
            LCD_BigAscii_Print(0x0E, 3, 0);
        else
            LCD_BigAscii_Print(0x00, 3, 0);
    }
}
#endif

#if  1//def 	PACKAGE_INFORMATION
void Menu_UpdatePackage( uint16 recvpagenum, uint16 errpacknum)
{
    uint8 str[8];

    if(CurrentNodeID == MENU_ID_TALKING)
    {

        LCD_Line_Clear(1);
        LCD_Line_Clear(2);

        _itoa(recvpagenum, (char *)str, 10);
        LCD_Str_Print("recv:", 0, 1, true);
        LCD_Str_Print(str, osal_strlen("recv:"), 1, true);

        _itoa(errpacknum, (char *)str, 10);
        LCD_Str_Print("err:", 0, 2, true);
        LCD_Str_Print(str, osal_strlen("err:"), 2, true);
    }
}

#endif
/*-----------------Static Functions-----------------*/

/*internal functions */

static void menu_nodeID_check(void)
{
    menu_orphan_nodeID_check();
    menu_tree_nodeID_check();
}

uint8 *Menu_GetDialNumBuf(void)
{
    return dialnum_buf.p;
}

uint8 *Menu_GetNumBuf(void)
{
    return num_buf.p;
}

uint8 Get_SMS_Quantity(void)
{
    uint8 quantity = 0;

#ifdef NEW_DOUBLE_NVID_OP
    //osal_nv_read(MINEAPP_NV_SMS1, 0, 1, &quantity);
    menu_SMS_Read_Num(&quantity,SMSTYPE_INBOX);
#endif
    return quantity;
}
void Menu_RefreshNwkDisp()
{
    if(CurrentNodeID == MENU_ID_INITNWK || CurrentNodeID == MENU_ID_POWERON_ANIMATION)
    {
        Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
    }
    /*  when need display panid, refresh screen */
    else if(CurrentNodeID == MENU_ID_MAIN || CurrentNodeID == MENU_ID_TALKING || MENU_ID_TOOLS)
    {
        menu_display();
    }

}

/* process kinds of timers used in menulib, like node jump with a timer */
void Menu_ProcessMenuLibEvt()
{
    uint8 jumpmark = menu_GetJumpMark();

    /* Pad lock timeout*/
    if((CurrentNodeID == MENU_ID_MAIN) && HalGetPadLockEnable())
    {
        HalSetPadLockStat(PADLOCK_LOCKED);
        menu_display();
    }
    else if(NearLastNodeID == MENU_ID_SM_SENDING)
    {

        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();

    }
    else if(NearLastNodeID == MENU_ID_CONTACT_HANDLE)
    {
        Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
        menu_display();
        //menu_JumpBackWithMark();

    }
    else if(NearLastNodeID == MENU_ID_INPUTNAME)
    {
        menu_JumpBackWithMark();

    }
    else if(jumpmark == MENU_ID_SETTINGS_RING_BELLSEL  ||
            jumpmark == MENU_ID_SETTINGS_RING_SMSSEL	||
            jumpmark == MENU_ID_SETTINGS_RING_BELLVOL	||
            jumpmark == MENU_ID_SETTINGS_RING_SHAKE     ||
            jumpmark == MENU_ID_SETTINGS_PADLOCK        ||
            jumpmark == MENU_ID_SETTINGS_BACKLIGHT
#ifdef MENU_CLOCKFORMAT
            || jumpmark == MENU_ID_SETTINGS_TIME_TIMEFORMAT
#endif
#ifdef 	MENU_TIMEUPDATE_CTL
            ||
            jumpmark == MENU_ID_SETTINGS_TIME_TIMEAUTOUPDATE
#endif
//#ifdef MENU_RF_DEBUG
            ||
            jumpmark == MENU_ID_DO_LCD_AGING||
            jumpmark == MENU_ID_SETTINGS_SET_CHANNEL ||
            jumpmark == MENU_ID_SETTINGS_SET_PANID
            ||((Menu_rf_debug==false)&&(jumpmark == MENU_ID_SETTINGS))
//#endif
           )
    {
        menu_JumpBackMarkParent();
    }
    else    // the general process locgic is jumpback
    {
        if((CurrentNodeID != MENU_ID_TALKING)&&(CurrentNodeID != MENU_ID_INCOMINGCALL))
        {
            menu_JumpBackWithMark();
        }
    }

}


