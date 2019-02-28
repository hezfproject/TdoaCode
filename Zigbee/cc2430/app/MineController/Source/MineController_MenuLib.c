#include "OSAL.h"
#include "OSAL_NV.h"
#include "string.h"
#include "App_cfg.h"
#include "hal_key_cfg.h"
#include "TimeUtil.h"
#include "lcd_serial.h"
#include "MineController_global.h"
#include "MineController_MenuLib.h"
#include "MineController_MenuLibChinese.h"
/*
#include "MenuChineseInputUtil.h"
#include "MineApp_global.h"
#include "MenuLib_global.h"
#include "MenuLib_orphan.h"
#include "MenuLib_tree.h"
#include "MenuAdjustUtil.h"
#include "MineApp_MP_Function.h"
*/
#include "MenuLibController_global.h"
#include "MenuLibController_orphan.h"
#include "MenuLibController_tree.h"

#include "hal_drivers.h"
#include "AppProtocol.h"
#include "AppProtocolWrapper.h"

#define SAVE_SUCCESS 0
#define SAVE_FAILED    1

/*----------------- variables  -----------------*/
//static bool      nwk_stat = FALSE;

/*-----------------Function declares-----------------*/

/*Internal functions */
//static uint8     menu_nv_init(void);
static void     menu_nodeID_check(void);

/*general functions */

/*
uint8* Menu_Get_SMS_Data(void)
{
       return data_buf.p;
}

uint8 Menu_Get_SMS_Len(void)
{
       if(data_buf.p != NULL)
           	return data_buf.len;
       return 0;
}
*/

uint8 Menu_Init(void)
{
       menu_nodeID_check();
       /*
	SET_ON_IDLE();
	Menu_Stack_Init();
      menu_nv_init();
      */
	SET_ON_IDLE();
      Menu_Stack_Init();
      Buffer_Init(&num_buf, NMBRDIGIT);
	return 0;
}
void Get_Sleeptime_data(app_Sleep_t* app_data)
{
       *app_data = med_sleepcfg;
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

	/* process power short press here */
	if(keys== HAL_KEY_POWER && CurrentNodeID!=MENU_ID_INITNWK)
	{
	       //SET_ON_IDLE();
		menu_JumptoMenu(MENU_ID_MAIN);
	}
	return;
}

void Menu_handle_msg(uint8 MSG, uint8 *p, uint8 len)
{
//	Record new_record;

	switch(MSG)
	{
	case MSG_INIT_NWK:    	
		menu_JumptoMenu(MENU_ID_INITNWK);
		break;
	case MSG_INIT_MAIN:
		//SET_ON_IDLE();
		menu_JumptoMenu(MENU_ID_MAIN);
		break;
      case MSG_NO_POWER:
		//menu_JumptoMenu(MENU_ID_POWEROFF_ANIMATION);
		break;
      case MSG_SET_SUCCESSFUL:
		//SET_ON_IDLE();
		if(CurrentNodeID == MENU_ID_SHOWSETTING)
		{
			Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
			menu_display();
		}
		break;
#if 0
	case MSG_INCOMING_CALL:
		Stack_Push(&global_stack, &CurrentNodeID, &node_info);//backup the message of current menu
		//Push_PipeLine(CurrentNodeID, &node_info);
		SET_ON_CALLED();
		num_buf.len = osal_strlen((char*)p);
		osal_memcpy(num_buf.p, p,  num_buf.len);
		num_buf.p[num_buf.len] = '\0';
		menu_JumptoMenu(MENU_ID_INCOMINGCALL);
		break;
	case MSG_DIALING_SUCCESS:
		SET_ON_AUDIO();
		menu_JumptoMenu(MENU_ID_TALKING);
		break;
	case MSG_MISSED_CALL:
		missed_call_amount++;
		SET_ON_IDLE();
		GetTimeChar(new_record.time);
		//num_buf.len = osal_strlen((char*)p);
		//osal_memcpy(num_buf.p, p,  num_buf.len);
		//num_buf.p[num_buf.len] = '\0';
		osal_memcpy(new_record.num, num_buf.p, num_buf.len+1);
		Add_CallRecord(MENU_ID_CALLRECORD_MISSEDCALL, &new_record);
		menu_JumptoMenu(MENU_ID_MISSINGCALL);
		break;
	case MSG_NO_POWER:
		menu_JumptoMenu(MENU_ID_POWEROFF_ANIMATION);
		break;
	case MSG_PAD_LOCK:
		if(CurrentNodeID==MENU_ID_MAIN && HalGetPadLockEnable())
		{
			HalSetPadLockStat(PADLOCK_LOCKED);
			menu_JumptoMenu(MENU_ID_MAIN);
		}
		break;
	case MSG_VOICE_FINISH:
		SET_ON_IDLE();
		Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
		//Pop_PipeLine();
		menu_display();
		break;
	case MSG_SMS_SUCCESS:
		SET_ON_IDLE();
		strcpy((char *)g_jump_buf, SM_SENDING_SUCCESS_CHINA);
		menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
		Buffer_Free(&data_buf);
		MineApp_StartMenuLibEvt(1000);
		break;
	case MSG_SMS_FAILED:
		SET_ON_IDLE();
		strcpy((char *)g_jump_buf, SM_SENDING_FAIL_CHINA);
		menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
		MineApp_StartMenuLibEvt(1000);
		break;
	case MSG_SMS_INCOMING:
		if(CurrentNodeID == MENU_ID_MAIN)
		{
			menu_JumptoMenu(MENU_ID_INCOMINGSMS);
		}
		else
		{
			LCD_SMS_ICON_Show();
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
#endif
	default:
		break;
	}

}


void Menu_UpdateTime(void)
{
	uint8 p[TIME_LEN];
	GetTimeChar(p);
	if((CurrentNodeID == MENU_ID_MAIN))
	{	
		LCD_Str_Print(p, TOP_POS_TIME, 0, TRUE);
	}
}

/*
void Menu_UpdateSignal(uint8 level)
{
	if(Menu_IsNwkOn())
	{
		menu_set_signal(level);
	}
	else
	{
		menu_set_signal(0);
	}

	if((CurrentNodeID == MENU_ID_MAIN) ||(CurrentNodeID == MENU_ID_TALKING) || (CurrentNodeID == MENU_ID_DIALING) )
	{
		LCD_Clear(1,0,2,0);
		LCD_Signal_Show(level);
	}
}
*/
void Menu_UpdateBattery(uint8 level)
{
	menu_set_battery(level);
	if((CurrentNodeID == MENU_ID_MAIN))
	{
		LCD_Clear(14,0,16,0);
		LCD_Battery_Show(level);
	}
}

/* process kinds of timers used in menulib, like node jump with a timer */
void Menu_ProcessMenuLibEvt()
{
	uint8 jumpmark = menu_GetJumpMark();

#ifdef MENU_RF_DEBUG
	if(jumpmark == MENU_ID_CHANNEL_SELECT)
	{
		menu_JumpBackMarkParent();
	}
	else    // the general process locgic is jumpback
	{
		menu_JumpBackWithMark();
	}
#else
	menu_JumpBackWithMark();
#endif

#if 0
	if(CurrentNodeID==MENU_ID_MAIN && HalGetPadLockEnable())
	{
		HalSetPadLockStat(PADLOCK_LOCKED);
		menu_display();
	}
	else if(NearLastNodeID == MENU_ID_SM_SENDING)
	{
		Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
		//Pop_PipeLine();
		menu_display();

	}
	else if(NearLastNodeID == MENU_ID_CONTACT_HANDLE) 
	{
		menu_JumpBackWithMark();

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
		jumpmark == MENU_ID_SETTINGS_BACKLIGHT      ||
		jumpmark == MENU_ID_SETTINGS_TIME_TIMEFORMAT
#ifdef MENU_RF_DEBUG
             ||
		jumpmark == MENU_ID_SETTINGS_SET_CHANNEL||
		jumpmark == MENU_ID_SETTINGS_SET_PANID
#endif
		)
	{
		menu_JumpBackMarkParent();
	}
	else    // the general process locgic is jumpback
	{
		menu_JumpBackWithMark();
	}
#endif
}

/*-----------------Static Functions-----------------*/

static void menu_nodeID_check(void)
{
	menu_orphan_nodeID_check();
	menu_tree_nodeID_check();
}


#if 0
/*internal functions */
static uint8     menu_nv_init()
{
	uint8   temp, pos = 0;
	uint16 len;

	len = MAX_CALL_NUM*sizeof(Record) + 1;

	temp = osal_nv_item_init(MINEAPP_NV_DIALED, len, NULL);
	if(temp == NV_ITEM_UNINIT)
		osal_nv_write(MINEAPP_NV_DIALED, 0, 1, &pos);
	else if(temp == NV_OPER_FAILED)
		return NV_OPER_FAILED;

	temp = osal_nv_item_init(MINEAPP_NV_MISSED, len, NULL);
	if(temp == NV_ITEM_UNINIT)
		osal_nv_write(MINEAPP_NV_MISSED, 0, 1, &pos);
	else if(temp == NV_OPER_FAILED)
		return NV_OPER_FAILED;

	temp = osal_nv_item_init(MINEAPP_NV_ANSWERED, len, NULL);
	if(temp == NV_ITEM_UNINIT)
		osal_nv_write(MINEAPP_NV_ANSWERED, 0, 1, &pos);

	else if(temp == NV_OPER_FAILED)
		return NV_OPER_FAILED;

	len = MAX_CONTACT_NUM*sizeof(Contact_Node) + 1;
	temp = osal_nv_item_init(MINEAPP_NV_CONTACT, len, NULL);
	if(temp == NV_ITEM_UNINIT)
		osal_nv_write(MINEAPP_NV_CONTACT, 0, 1, &pos);
	else if(temp == NV_OPER_FAILED)
		return NV_OPER_FAILED;


	len = MAX_SMS_NUM*SMS_NV_LEN+ 1;
	temp = osal_nv_item_init(MINEAPP_NV_SMS, len, NULL);
	if(temp == NV_ITEM_UNINIT)
		osal_nv_write(MINEAPP_NV_SMS, 0, 1, &pos);
	else if(temp == NV_OPER_FAILED)
		return NV_OPER_FAILED;
	
#ifdef SMS_TEMPLATE
	len = MAX_SMS_NUM_TEMPLATE*SMS_NV_LEN+ 1;
	pos = MAX_SMS_NUM_TEMPLATE;
	temp = osal_nv_item_init(MINEAPP_NV_SMS_TEMPLATE, len, NULL);
	if(temp == NV_ITEM_UNINIT)
	{
		osal_nv_write(MINEAPP_NV_SMS_TEMPLATE, 0, 1, &pos);
		for(uint8 i=0; i<MAX_SMS_NUM_TEMPLATE; i++)
		{
		       osal_nv_write(MINEAPP_NV_SMS_TEMPLATE, 1+SMS_NV_LEN*i, osal_strlen((char *)SMS_TepmlateList[i]), SMS_TepmlateList[i]);
		}

	}
	else if(temp == NV_OPER_FAILED)
	{
		return NV_OPER_FAILED;
	}
#endif
		
#ifdef SMS_TEST
	osal_nv_write(MINEAPP_NV_SMS, 0, 1, &pos);//SMS_TEST
#endif
	return ZSUCCESS;
}



void Get_Num_From_Menu(uint8* number_buf)
{
	num_buf.len = osal_strlen((char*)num_buf.p);
	osal_memcpy(number_buf, num_buf.p, num_buf.len);
	number_buf[num_buf.len] = '\0';
}

uint8 Get_SMS_Quantity(void)
{
	uint8 quantity = 0;

	osal_nv_read(MINEAPP_NV_SMS, 0, 1, &quantity);
	return quantity;
}

uint8 Save_New_SMS(APPWrapper_t *sms)
{
	uint8 len;
	uint8 rtn = NV_OPER_FAILED;
	uint8 quantity = 0;
	uint16  offset;

	//save the sms data length, sms data and the number to NV//sizeof(termNbr_t)+sizeof(sms->app_SMS.len)+sms->app_SMS.len
	quantity = Get_SMS_Quantity();
	offset = quantity*SMS_NV_LEN+1;
	len = NMBRDIGIT+SMS_LEN_TYPE_SIZE+sms->app_SMS.len;
	rtn = osal_nv_write(MINEAPP_NV_SMS, offset, len, &sms->app_SMS.len);

	if(rtn == ZSUCCESS)
	{
		new_sms_flag = TRUE;
		++quantity;
		osal_nv_write(MINEAPP_NV_SMS, 0, 1, &quantity);
	}
	return rtn;
}
void Menu_SearchNwkFinish()
{
	if(CurrentNodeID== MENU_ID_INITNWK)
	{
		HalSetPadLockStat(PADLOCK_UNLOCKED);
		Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
		HalResetBackLightEvent();
	}	
	else if(CurrentNodeID== MENU_ID_MAIN)
	{
		Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
        menu_display();
	}

}
void Menu_UpdateNwkLogo()
{
	if(CurrentNodeID== MENU_ID_MAIN)
	{
		Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
	}
}

void Menu_SetNwkStat(bool val)
{
	nwk_stat = val;
      if(!val)
      {
        menu_set_signal(0);    // if no network, the signal is 0
      }
}

bool Menu_IsNwkOn(void)
{
	return nwk_stat;
}
#endif
