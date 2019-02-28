#include "OSAL.h"
#include "string.h"
#include "OSAL_NV.h"
#include "App_cfg.h"
#include "hal_key_cfg.h"
#include "TimeUtil.h"
#include "lcd_serial.h"
#include "OnBoard.h"
#include "ch4.h"
#include "stdio.h"

#include "lcd_serial.h"
#include "GasMonitor.h"
#include "GasMonitor_MenuLib.h"
#include "GasMonitor_MenuLibChinese.h"
#include "GasMenuLib_global.h"
#include "GasMenuLib_orphan.h"
#include "GasMenuLib_tree.h"
#include "GasMonitorDep.h"
#include "GasMonitor_sms.h"
#include "drivers.h"
#include "AppProtocol.h"
#include "AppProtocolWrapper.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchdogUtil.h"
#endif

/*-----------------	Defines	 -----------------*/
#define MENU_GLOBAL_STACK_DEPTH 5

/*-----------------	Statics 	-----------------*/
static stack_p_t       global_stack_p[MENU_GLOBAL_STACK_DEPTH];

/*-----------------Function declares-----------------*/
/*Internal functions */
static uint8     menu_nv_init(void);
static void      menu_nodeID_check(void);
static void 	menu_global_stack_init(void);

/*general functions */
uint8 Menu_Init(void)
{
	menu_nodeID_check();
	menu_nv_init();
	menu_global_stack_init();
	menu_tree_init();
	return 0;
}

void Menu_handle_key(uint8 keys, uint8 status)
{
	if(GetTypeFromID(CurrentNodeID) == NODE_TYPE_ORPHAN)
	{
		for(uint8 i=0;i<8;i++)
		{
			uint8 key_oneshot = keys&(1<<i);
			if(key_oneshot)
			{
				menu_orphan_handle_key( key_oneshot,  status);
			}
		}
	}
	else
	{
		for(uint8 i=0;i<8;i++)
		{
			uint8 key_oneshot = keys&(1<<i);
			if(key_oneshot)
			{
				menu_tree_handle_key( keys,  status);
			}
		}
	}
	return;
}

void Menu_handle_msg(uint8 MSG, uint8 *p, uint8 len)
{
	switch(MSG)
	{
	case MSG_INIT_MAIN:
		{     
	        if(CurrentNodeID == MENU_ID_SOS || CurrentNodeID == MENU_ID_SOS_ALARM)
		       {
		       	GasMonitor_SOSAlarm_endsend();
		       }
            if(CurrentNodeID==MENU_ID_CARDSEARCH||CurrentNodeID==MENU_ID_CARDSEARCH_RESULT)
               {
               	menu_CardSearch_end();
               }
			menu_JumptoMenu(MENU_ID_MAIN);

			break;
		}
	case MSG_NO_POWER:
		{
			strcpy((char *)g_jump_buf,NO_POWER_CHINA);
			menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
			GasMonitor_LongDelay(500, 4);\
				break;
		}
	case MSG_SOS:
		{
			uint8 data = *p;
			if(data == 0xFF)    // call for help end flag
			{
				HalStopBeeper(BEEP_TYPE_URGENT, true);
				menu_JumptoMenu(MENU_ID_SOS_RESULT);
			}
			else
			{
				menu_set_SOSseq(data);
				menu_JumptoMenu(MENU_ID_SOS);
				HalStartBeeper(BEEP_TYPE_URGENT,10);
			}
			break;
		}

        case MSG_SOSALARM:
              {
			uint8 data = *p;
			if(data == 0xFF)    // call for help end flag
			{
				HalStopBeeper(BEEP_TYPE_URGENT, true);
				menu_JumptoMenu(MENU_ID_SOS_ALARM_RESULT);
			}
			else
			{
				menu_set_SOSseq(data);
				menu_JumptoMenu(MENU_ID_SOS_ALARM);
				HalStartBeeper(BEEP_TYPE_URGENT,10);
			}
			break;
               }
			

       case MSG_POWERON_ANIM:
	       {
		   	menu_JumptoMenu(MENU_ID_POWERON_ANIMATION);
			break;
	 	}
	 case MSG_POWEROFF_ANIM:
	       {
		   	HalStartBeeper(BEEP_TYPE_POWERONOFF,0);
		   	menu_JumptoMenu(MENU_ID_POWEROFF_ANIMATION);
			break;
	 	}
#ifdef CFG_GAS_SHORTMESSAGE
	 case MSG_NEW_SHORTMSG:
	 	{
			Menu_UpdateSmsIcon();
			if(CurrentNodeID == MENU_ID_SHORTMESSAGE)
			{
				menu_JumptoItself();
			}
			break;
		}
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

void Menu_UpdateSignal(uint8 level)
{
	menu_set_signal(level);

	if((CurrentNodeID == MENU_ID_MAIN) )
	{
		LCD_Clear(1,0,2,0);
		LCD_Signal_Show(level);
	}
}

void Menu_UpdateBattery(uint8 level)
{
	menu_set_battery(level);
	if((CurrentNodeID == MENU_ID_MAIN))
	{
		LCD_Clear(14,0,16,0);
		LCD_Battery_Show(level);
	}
}
#ifdef CFG_GAS_SHORTMESSAGE
void Menu_UpdateSmsIcon(void)
{
	if(CurrentNodeID == MENU_ID_MAIN)
	{	
		const uint8 ICON_POS_X = LCD_LINE_WIDTH-5;
		const uint8 ICON_POS_Y = 0;
		
		LCD_Clear(ICON_POS_X, ICON_POS_Y, ICON_POS_X+1, ICON_POS_Y);
		
		uint8 unreadcnt = GasSms_GetUnReadSMSCnt();
		bool  isHaveOverFlowSms = GasSms_IsHaveOverFlowSms();
		if(unreadcnt > 0 || isHaveOverFlowSms)
		{
			LCD_SMS_ICON_Show(ICON_POS_X, ICON_POS_Y);
		}
	}
}
#endif

bool  Menu_UpdateGasDensity(uint16 value)
{
#if 1
	static bool Menu_IsOverDensityed = false;

	if(CurrentNodeID == MENU_ID_MAIN)
	{   
		if(Menu_IsOverDensityed == true)
		{
			LCD_Line_Clear(3);
			ch4_LdoCtrl(false);
			LCD_Str_Print((uint8 *)OVERDENSITYED_CHINA, (LCD_LINE_WIDTH-osal_strlen((char *)OVERDENSITYED_CHINA))/2, 3,TRUE);
		}
		else
		{
			uint8 n1,n2,n3;

			ch4_getnum_fromdensity(&n1, &n2, &n3, value);
			menu_GasPrint( n1, n2,  n3, false);

			if(value >= GAS_MAX_DENSITY)
			{
				ch4_LdoCtrl(false);
				LCD_Str_Print((uint8 *)OVERDENSITYED_CHINA, (LCD_LINE_WIDTH-osal_strlen((char *)OVERDENSITYED_CHINA))/2, 3,TRUE);
				Menu_IsOverDensityed = true;
			}
		}
	}
#else
	char buf[12];
	uint16 adc_value = ch4_get_adc_value();
	//int16 adc_value = ch4_get_temperature();
	_itoa(adc_value, buf, 10);
	LcdClearDisplay();
	LCD_Str_Print_Pixel((uint8 *)buf, 2, 1.5*LCD_LINE_HIGH);
#endif
	return true;
}

void menu_GasPrint(uint8 n1,uint8 n2, uint8 n3, bool forcePrint)
{
	static uint8 n1_bak, n2_bak,n3_bak;

	if(n1>9)
	{
		n1%=10;
	}
	if(n2>9)
	{
		n2%=10;
	}
	if(n3>9)
	{
		n3%=10;
	}

	uint8 x,y;
	if(forcePrint)
	{
		n1_bak = n1;
		n2_bak = n2;
		n3_bak = n3;

		x = GASDISP_START_X;
		y = GASDISP_START_Y;
		LCD_Clear(x,  y, x+1, y);
		LCD_BigCharPrint(n1+'0', x, y*LCD_LINE_HIGH);

		x = GASDISP_START_X+3;
		y = GASDISP_START_Y;
		LCD_Clear(x, y, x+1, y);
		LCD_BigCharPrint(n2+'0', x, y*LCD_LINE_HIGH);

		x = GASDISP_START_X+5;
		y = GASDISP_START_Y;
		LCD_Clear(x, y, x+1, y);
		LCD_BigCharPrint(n3+'0', x, y*LCD_LINE_HIGH);

	}
	else
	{
		if(n1!=n1_bak)
		{
			n1_bak = n1;
			x = GASDISP_START_X;
			y = GASDISP_START_Y;
			LCD_Clear(x,  y, x+1, y);
			LCD_BigCharPrint(n1+'0', x, y*LCD_LINE_HIGH);
		}
		if(n2!=n2_bak)
		{
			n2_bak = n2;
			x = GASDISP_START_X+3;
			y = GASDISP_START_Y;
			LCD_Clear(x, y, x+1, y);
			LCD_BigCharPrint(n2+'0', x, y*LCD_LINE_HIGH);
		}
		if(n3!=n3_bak)
		{
			n3_bak = n3;
			x = GASDISP_START_X+5;
			y = GASDISP_START_Y;
			LCD_Clear(x, y, x+1, y);
			LCD_BigCharPrint(n3+'0', x, y*LCD_LINE_HIGH);
		}
	}

}
#ifdef CFG_GAS_CARDCHECK
void menu_CardCheckPrint(uint16 cardnum)
{
    
    char buf[10];
    
    sprintf(buf,"%u",cardnum);
    
    uint8 x,y;
    LCD_Line_Clear(1);
    LCD_Line_Clear(2);

    for(uint8 i=0; i<osal_strlen(buf);i++)
    {
        x = GASDISP_START_X + 1 + 2*i;
        y = GASDISP_START_Y;
        LCD_BigCharPrint(buf[i], x, y*LCD_LINE_HIGH);
    }
}
#endif
/*-----------------Static Functions-----------------*/

/*internal functions */
static uint8     menu_nv_init()
{
	uint8   temp = 0;
	uint16 len;

	len = sizeof(set_info_t);

	temp = osal_nv_item_init(GASMONITOR_NV_SETTINGS, len, NULL);
	if(temp == NV_ITEM_UNINIT)
	{
		GasMonitor_InitFlashInfo();
	}
	else if(temp == NV_OPER_FAILED)
	{
		return NV_OPER_FAILED;
	}

	temp = GasMonitor_ReadInfoFromFlash();

	/* initial sms nv*/
	GasSms_NV_init();
	return temp;
}

static void menu_nodeID_check(void)
{
	menu_orphan_nodeID_check();
	menu_tree_nodeID_check();
}
static void menu_global_stack_init(void)
{
	global_stack.stack_depth = MENU_GLOBAL_STACK_DEPTH;
	global_stack.stack_p = global_stack_p;
       global_stack.stack_i =0; 
}
/* process kinds of timers used in menulib, like node jump with a timer */
void Menu_ProcessMenuLibEvt()
{
	uint8 jumpmark = menu_GetJumpMark();

	if(CurrentNodeID==MENU_ID_MAIN||CurrentNodeID==MENU_ID_FUNCTIONLIST)
	{
		menu_display();
	}
	else if(CurrentNodeID == MENU_ID_TEMPERATURE)
	{
		menu_JumptoMenu(MENU_ID_TEMPERATURE);
	}
	else if(NearLastNodeID == MENU_ID_ZEROADJUST)
	{
		if(ch4_GetLdoStatus()) // if ldo is working
		{
			menu_JumptoMenu(MENU_ID_ZEROADJUST_RESULT);
		}
		else
		{
			menu_JumpBackMarkParent();
		}
	}
	else if(NearLastNodeID == MENU_ID_CALIBRATION_SETDENSITY)
	{
		menu_JumptoMenu(MENU_ID_CALIBRATION_RESULT);
	}
	else if(NearLastNodeID == MENU_ID_SOS_RESULT)
	{
		menu_JumptoMenu(MENU_ID_MAIN);
	}

       else if(NearLastNodeID == MENU_ID_SOS_ALARM_RESULT
	   	 ||NearLastNodeID == MENU_ID_CARDSEARCH_RESULT)
	{
		menu_JumpBackMarkParent();
       }
#ifdef CFG_GAS_SHORTMESSAGE       
	else if(NearLastNodeID==MENU_ID_SHORTMESSAGE_NUMJUDGE)
	{
             menu_JumptoMenu(MENU_ID_SHORTMESSAGE);
	}
#endif    
       else if(NearLastNodeID == MENU_ID_CARDSEARCH)
	{
		menu_JumptoMenu(MENU_ID_CARDSEARCH_RESULT);
       }
	else if(NearLastNodeID == MENU_ID_ADJUSTDATE)
	{
             menu_JumptoMenu(MENU_ID_SETTINGS_TIME);
	}
#ifdef CFG_GAS_SHORTMESSAGE
	else if(NearLastNodeID == MENU_ID_SHORTMESSAGE_HANDLE)
	{
			
			menu_JumptoMenu(MENU_ID_SHORTMESSAGE_DELETE_RESULT);
	}
	/*
	else if(NearLastNodeID == MENU_ID_SHORTMESSAGE_DELETE_RESULT)
	{
		node_info.high_line = 1;
		node_info.sel_item = 0;
		node_info.show_item = 0;
		menu_JumptoMenu(MENU_ID_SHORTMESSAGE);
	}
	*/
#endif
	else if(NearLastNodeID == MENU_ID_CALIBRATION
		||NearLastNodeID == MENU_ID_ZEROADJUST_RESULT
		||NearLastNodeID == MENU_ID_CALIBRATION_RESULT
		||NearLastNodeID == MENU_ID_SETTINGS_OVERDENSITY)
	{
		menu_JumpBackMarkParent();
	}
	else    // the general process locgic is jumpback
	{
		menu_JumpBackWithMark();
	}

}


