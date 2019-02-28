#include "OSAL.h"
#include "OSAL_Nv.h"
#include "App_cfg.h"
#include "string.h"
#include "hal_drivers.h"
#include "hal_key_cfg.h"
#include "hal_key.h"
#include "hal_audio.h"
#include "lcd_serial.h"
#include "TimeUtil.h"
#include "MacUtil.h"
#include "NWK.h"
#include "OnBoard.h"

#include "MenuLibController_orphan.h"
#include "MenuLibController_global.h"
#include "MineController_MenuLib.h"
//#include "MineController_global.h"
#include "MineController_MenuLibChinese.h"
//#include "MenuChineseInputUtil.h"
//#include "MenuAdjustUtil.h"
#include "MineController.h"
//#include "MineApp_MP_Function.h"

static  uint8             sig_index = 0;
static  uint8             bat_index = FULL_BAT;

typedef struct
{
	uint8 ID;
	char* name;
	uint8 ParentID;
	uint8 FirstChildID;
	uint8 ChildNum;
	char* const __code *ItemName_CH;
	MenuOper_t oper;
}Orphan_node_t;

/*dial number*/

/*identical functions */
static void    menu_poweron_animation_display(void);
static void    menu_poweroff_animation_display(void);
static void    menu_initnwk_display(void);
static void    menu_initnwk_onkey(uint8 keys, uint8 status);

static void    menu_inputname_display(void);
static void    menu_inputname_onkey(uint8 keys, uint8 status);
static void    menu_inputnumber_display(void);
static void    menu_inputnumber_onkey(uint8 keys, uint8 status);

static void    menu_showmessage_display(void);
static void    menu_showmessage_onkey(uint8 keys, uint8 status);
static void    menu_showquestion_display(void);
static void    menu_showquestion_onkey(uint8 keys, uint8 status);
static void    menu_showalert_display(void);
static void    menu_showalert_onkey(uint8 keys, uint8 status);
static void    menu_showsetting_display(void);

static void    menu_main_display(void);
static void    menu_main_onkey(uint8 keys, uint8 status);
/*
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
static void    menu_inputsymbol_display(void);
static void    menu_inputsymbol_onkey(uint8 keys, uint8 status);

static void    menu_showmessage_display(void);
static void    menu_showmessage_onkey(uint8 keys, uint8 status);
static void    menu_showquestion_display(void);
static void    menu_showquestion_onkey(uint8 keys, uint8 status);
static void    menu_showalert_display(void);
static void    menu_showalert_onkey(uint8 keys, uint8 status);
static void    menu_busy_display(void);
static void    menu_busy_onkey(uint8 keys, uint8 status);
static void    menu_canlendar_display(void);
static void    menu_canlendar_onkey(uint8 keys, uint8 status);
*/

/*internal functions */

/*Menu Defination */
static Orphan_node_t const __code Menu_Orphan[] = 
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
			.oper.display = menu_poweron_animation_display,
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
		.ID = MENU_ID_INPUTNAME,
			.oper.display = menu_inputname_display,
			.oper.on_key = menu_inputname_onkey			
	}
	,    
	{
		.ID = MENU_ID_INPUTNUMBER,
			.oper.display = menu_inputnumber_display,
			.oper.on_key = menu_inputnumber_onkey			
	}
	,    
	{
		.ID = MENU_ID_SHOWMESSAGE,
			.oper.display = menu_showmessage_display,
			.oper.on_key = menu_showmessage_onkey			
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
		.ID = MENU_ID_SHOWSETTING,
			.oper.display = menu_showsetting_display,
			.oper.on_key = NULL
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
	for(uint8 i =0; i<len;i++)
	{
		if(Menu_Orphan[i].ID != GetIDFromIdx(NODE_TYPE_ORPHAN,i))
		{
			LcdClearDisplay();
			LCD_Str_Print("Menu Node ID Incorrect!", 0, 0, TRUE);
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
	if(idx < MAX_SIG)
	{
		sig_index = idx;
	}
}
void menu_set_battery(uint8 idx)
{
	if(idx<MAX_BAT)
	{
		bat_index = idx;
	}
}

/*identical functions */
static void    menu_poweron_animation_display(void)
{
}
static void    menu_poweroff_animation_display(void)
{
	LcdClearDisplay();
	LCD_Str_Print(NO_POWER_CHINA, 3, 1, TRUE);
}
static void    menu_initnwk_display(void)
{

	if(CurrentNodeID != MENU_ID_INITNWK)
		return;
	LcdClearDisplay();
	LCD_Str_Print(NWK_INIT_CHINA, 3, 1, TRUE);

}

static void    menu_initnwk_onkey(uint8 keys, uint8 status)
{
}

static void    menu_inputname_display(void)
{
#if 0
	if(NULL== data_buf.p)
	{
		if(NULL == Buffer_Init(&data_buf, MAX_NAME_LEN+1))
			return;
	}
	menu_inputchinese_display();
	LCD_ShowCursor(0, 1);
	if(data_buf.len > 0)
	{
             LCD_SMS_Print(&data_buf.p[0], data_buf.len, 0, 1);
		LCD_ShowCursor(data_buf.len, 1);
	}
#endif
}
#if 0
static bool    menu_inputname_output_handle(uint8 keys, uint8 input_status)
{


	if(input_status == OUTPUT_STATUS)
	{
		if(keys == HAL_KEY_BACKSPACE)
		{
			if(data_buf.len == 0)
			{
				LCD_CloseCursor();
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
					LCD_Clear(data_buf.len, 1, data_buf.len+2, 1);
				}
				else
				{
					data_buf.p[--data_buf.len] = '\0';
					LCD_Clear(data_buf.len, 1, data_buf.len+1, 1);
				}
				LCD_ShowCursor(data_buf.len%LCD_LINE_WIDTH, 1);
			}

			return TRUE;
		}
		else if(keys == HAL_KEY_SELECT)
		{
			if(data_buf.len > 0)
			{
				uint8 contact_num = 0;

				osal_nv_read(MINEAPP_NV_CONTACT, 0, 1, &contact_num);

				if(contact_num >= MAX_CONTACT_NUM)
				{
					strcpy((char *)g_jump_buf,FULL_CONTACTLIST_CHINA);
					menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
					//osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 1000);	
					MineApp_StartMenuLibEvt(1000);
				}
				else
				{
					osal_nv_write(MINEAPP_NV_CONTACT, contact_num*sizeof(Contact_Node)+1, num_buf.len+1, num_buf.p);
					osal_nv_write(MINEAPP_NV_CONTACT, contact_num*sizeof(Contact_Node)+1+NMBRDIGIT, data_buf.len+1, data_buf.p);
					++contact_num;
					osal_nv_write(MINEAPP_NV_CONTACT, 0, 1, &contact_num);
					shortcuts_flag = TRUE;
					Buffer_Free(&data_buf);
					Stack_Clear(&global_stack);
					menu_JumptoMenu(MENU_ID_CONTACTLIST);
				}
				LCD_CloseCursor();
			}
			return TRUE;
		}
	}
	return FALSE;
}
#endif

static void    menu_inputname_onkey(uint8 keys, uint8 status)
{
#if 0
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

			if((data_buf.len >= MAX_NAME_LEN) || ((data_buf.len >= MAX_NAME_LEN-1)&&(output_p[0]>0x80)))
			{
				strcpy((char *)g_jump_buf, NAME_INPUT_FULL_CHINA);
				menu_JumpandMark(MENU_ID_SHOWMESSAGE);
				return;
			}
			len_output = osal_strlen((char*)output_p);
			osal_memcpy(&data_buf.p[data_buf.len], output_p, len_output);
			data_buf.p[data_buf.len+len_output] = '\0';
			LCD_Str_Print(output_p, data_buf.len, 1, TRUE);
			data_buf.len += len_output;
			menu_ChineseOutputClear();
		}
		LCD_ShowCursor(data_buf.len%LCD_LINE_WIDTH, 1);
	}
#endif
}

static void    menu_inputnumber_display(void)
{
	LCD_Clear(8, 0, 16, 3);
	LCD_Str_Print(INPUT_NUM_CHINA, 0, 0, TRUE); 
	LCD_Str_Print(num_buf.p , 0 , 3, FALSE);
	LCD_ShowCursor(LCD_LINE_WIDTH-1, 3);
}

static void    menu_inputnumber_onkey(uint8 keys, uint8 status)
{
/*
	//num_buf.len = osal_strlen((char*)C_num);

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
		if(num_buf.len < NMBRDIGIT-1)
		{
			num_buf.p[num_buf.len++] = Key2ASCII(keys);
			num_buf.p[num_buf.len] = '\0';
			menu_display();
		}
		break;
	case HAL_KEY_SELECT:
		if(num_buf.len > 0)
		{
			LCD_CloseCursor();
			if(NearLastNodeID == MENU_ID_SHORTMESSAGE_WRITINGBOX)
			{
				//set shortmessage sending status
				SET_ON_SM_SENDING();
				menu_JumptoMenu(MENU_ID_SM_SENDING);
			}
			else
			{
	                    Stack_Push(&global_stack, &CurrentNodeID, NULL);
				menu_JumptoMenu(MENU_ID_INPUTNAME);
			}
		}
		break;
	case HAL_KEY_BACKSPACE:
		if(num_buf.len == 0)
		{
			Clr_Num_Buf();
			if(NearLastNodeID == MENU_ID_SHORTMESSAGE_WRITINGBOX)
			{
			       Stack_Pop(&global_stack, &CurrentNodeID, NULL);
				//Pop_PipeLine();
				menu_display();
			}
			else if(menu_GetJumpMark() == MENU_ID_CONTACTLIST)
			{
				menu_JumpBackWithMark();
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
	*/
}

static void    menu_showmessage_display(void)
{
	LcdClearDisplay();
	/* show message in global buffer, and jump back with mark after 1 second */
	LCD_Str_Print((uint8 *)g_jump_buf, (LCD_LINE_WIDTH-osal_strlen((char *)g_jump_buf))/2, 1, TRUE);      
	//osal_start_timerEx(MineApp_Function_TaskID,MINEAPP_MENULIB_EVENT, 500);
	MineController_StartMenuLibEvt(500);
}
static void    menu_showmessage_onkey(uint8 keys, uint8 status)
{
}

/* question should be put into g_buf  first*/
static void    menu_showquestion_display(void)
{
#if 0
	LcdClearDisplay();
	switch(menu_GetJumpMark())
	{
	case    MENU_ID_SETTINGS:
		{       
			LCD_Str_Print((uint8 *)g_jump_buf, (LCD_LINE_WIDTH-osal_strlen((char *)g_jump_buf))/2, 1, TRUE);      
			break;
		}
	case   MENU_ID_CALLRECORD_DELETE:
		{
			uint8 sel_item = g_jump_buf[0];
			LCD_Str_Print((uint8 *)DELETE_ALL_CHINA, 4, 0, TRUE);      
			if(sel_item == 0)   // delete missed call?
			{
				LCD_Str_Print((uint8 *)DELETE_MISSEDCALL_CHINA, 4, 1, TRUE);      
			}
			else if(sel_item == 1) // delete answered call?
			{
				LCD_Str_Print((uint8 *)DELETE_ANSWEREDCALL_CHINA, 4, 1, TRUE);      
			}
			else if(sel_item == 2)  // delete dialed call?
			{
				LCD_Str_Print((uint8 *)DELETE_DIALEDCALL_CHINA, 4, 1, TRUE);      
			}
			break;
		}
	}
	LCD_Str_Print(CONFIRM_CHINA, 0, 3, TRUE);
	LCD_Str_Print(CANCEL_CHINA, 12, 3, TRUE);
#endif 
}

static void    menu_showquestion_onkey(uint8 keys, uint8 status)
{
#if 0
	if(keys ==  HAL_KEY_SELECT)
	{
		switch(menu_GetJumpMark())
		{
		case    MENU_ID_SETTINGS:
			{       
				HalRingSetBellName(RING_BELL_1);
				HalRingSetSMSBell(RING_BELL_4);
				HalRingSetGain(RING_TYPE_BELL, MAX_GAIN_LEVEL-1);
				AudioSetOutputGain( MAX_GAIN_LEVEL-1);
				HalRingSetGain(RING_TYPE_SOUND, MAX_GAIN_LEVEL-1);
				HalRingSetShake(FALSE);
				LCDSetBackLightCtl(BACKLIGHT_CTL_5S);
				ResetTimeDate();
				strcpy((char *)g_jump_buf,SETTED_CHINA);
				menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
				//menu_JumpBackWithMark();
				//strcpy(g_jump_buf,SETTED_CHINA);
				//menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
				break;
			}
		case   MENU_ID_CALLRECORD_DELETE:
			{ 
				uint8 delete_sel = g_jump_buf[0];
				uint8 pos = 0;
				if(delete_sel == 0)   //missed call
				{
					osal_nv_write(MINEAPP_NV_MISSED,0,1,&pos);
				}
				else if(delete_sel == 1)   //answered call
				{
					osal_nv_write(MINEAPP_NV_ANSWERED,0,1,&pos);
				}
				else if(delete_sel == 2)   //dialed call
				{   
					osal_nv_write(MINEAPP_NV_DIALED,0,1,&pos);
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
#endif
}
static void    menu_showalert_display(void)
{
}
static void    menu_showalert_onkey(uint8 keys, uint8 status)
{
}
static void    menu_showsetting_display(void)
{
       LcdClearDisplay();
	LCD_Str_Print(SETTINGS_CHINA, (LCD_LINE_WIDTH-osal_strlen(SETTINGS_CHINA))/2, 1, TRUE);      
}
static void    menu_main_display(void)
{
	LcdClearDisplay();
	//LCD_Signal_Show(sig_index);
	LCD_Battery_Show(bat_index);
	//Menu_UpdateTime();

        uint8 channelStr[8];
       _itoa(_NIB.nwkLogicalChannel, channelStr, 10);
       LCD_Str_Print("CH", 0, 0, TRUE);
	 LCD_Str_Print(channelStr, 2, 0, TRUE);

	//Buffer_Free(&data_buf);
	Clr_Num_Buf();
	//Stack_Clear(&global_stack);

	LCD_Str_Print_Pixel(LOG_CHINA, (LCD_LINE_WIDTH-osal_strlen(LOG_CHINA))/2, 1.5*LCD_LINE_HIGH);
	//menu_ChineseOutputClear();

      LCD_Str_Print(FUNCTIONLIST_CHINA, 0, 3, TRUE);
    #if 0
	if(HalGetPadLockEnable())
	{
		if(HalGetPadLockStat() == PADLOCK_LOCKED)
		{		
			//LCD_BigAscii_Print(0x94, 3, 0);  //key icon
			LCD_Str_Print(OPENLOCK_CHINA, 0, 3, TRUE);
			//LCD_Str_Print(time_str, 5, 1, TRUE);
			return ;
		}
		else if( HalGetPadLockStat() == PADLOCK_MID)
		{
			//LCD_BigAscii_Print(0x94, 3, 0);
			LCD_Line_Clear(1);
			LCD_Line_Clear(2);
			LCD_Str_Print(REQUESTSTAR_CHINA, 1, 1, TRUE);
			LCD_Str_Print(OPENLOCK_CHINA, 0, 3, TRUE);
			return;
		}
		else if(HalGetPadLockStat() == PADLOCK_ALERT)
		{
			//LCD_BigAscii_Print(0x94, 3, 0);
			LCD_Line_Clear(1);
			LCD_Line_Clear(2);
			LCD_Str_Print(HOWTOUNLOCK1_CHINA, 0, 1, TRUE);
			LCD_Str_Print(HOWTOUNLOCK2_CHINA, 0, 2, TRUE);
			return;
		}
	}

	{
		//LCD_Str_Print(time_str, 4, 0, TRUE);
		LCD_Str_Print(FUNCTIONLIST_CHINA, 0, 3, TRUE);
		//LCD_Str_Print(CONTACTLIST_CHINA, 10, 3, TRUE);
		//LCD_BigAscii_Print(0xB0, 7, 3);
	}
    #endif 
}

static void    menu_main_onkey(uint8 keys, uint8 status)
{
#if 0
	if(HalGetPadLockEnable())
	{
		NearLastNodeID = CurrentNodeID;
		uint8 stat = HalGetPadLockStat();
		if(stat == PADLOCK_LOCKED || stat == PADLOCK_ALERT)
		{
			if(keys == HAL_KEY_SELECT)
			{        
				HalSetPadLockStat(PADLOCK_MID);	
				//osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 3000);
				MineController_StartMenuLibEvt(3000);
			}
			else
			{
				HalSetPadLockStat(PADLOCK_ALERT);
				//osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 1000);
				MineController_StartMenuLibEvt(1000);
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
				MineController_StopMenuLibEvt();
			}
			else
			{
				HalSetPadLockStat(PADLOCK_ALERT);
				//osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 1000);
				MineController_StartMenuLibEvt(1000);
			}
			menu_display();
			return;
		}
	}
#endif 
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
			//num_buf.p[num_buf.len++] = Key2ASCII(keys);
			//num_buf.p[num_buf.len] = '\0';
			//menu_JumptoMenu(MENU_ID_SHOWING_NUMBER);
			break;
		case HAL_KEY_CALL:
			//shortcuts_flag = TRUE;
			//menu_JumptoMenu(MENU_ID_CALLRECORD_DIALEDCALL);
			break;
		case HAL_KEY_SELECT:
			menu_JumptoMenu(MENU_ID_FUNCTIONLIST);
			break;
		case HAL_KEY_BACKSPACE:
			//shortcuts_flag = TRUE;
			//menu_JumptoMenu(MENU_ID_CONTACTLIST);
			break;
		default:
			break;
		}
	}
}

