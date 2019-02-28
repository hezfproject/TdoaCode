#include "OSAL.h"
#include "App_cfg.h"
#include "string.h"
#include "drivers.h"
#include "hal_key.h"
#include "TimeUtil.h"
#include "MacUtil.h"

#include "GasMonitor.h"
#include "GasMenuLib_orphan.h"
#include "GasMenuLib_global.h"
#include "GasMonitor_MenuLib.h"
#include "GasMonitor_MenuLibChinese.h"
#include "GasMonitor_sms.h"
#include "lcd_interface.h"
#include "OnBoard.h"
#include "MenuAdjustUtil.h"

static  uint8             sig_index = 0;
static  uint8             bat_index = FULL_BAT;
static uint8		  menu_SOSseq;
static bool		  menu_SOSresult;

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
static void    menu_showmessage_display(void);
//static void    menu_showmessage_onkey(uint8 keys, uint8 status);
static void    menu_showquestion_display(void);
static void    menu_showquestion_onkey(uint8 keys, uint8 status);
static void    menu_showalert_display(void);
static void    menu_showalert_onkey(uint8 keys, uint8 status);
static void    menu_main_display(void);
static void    menu_main_onkey(uint8 keys, uint8 status);

static void    menu_SOS_display(void);
static void    menu_SOS_result_display(void);
static void    menu_SOS_Alarm_display(void);
static void    menu_SOS_Alarm_result_display(void);


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
		.ID = MENU_ID_SOS,
			.oper.display = menu_SOS_display,
			.oper.on_key = NULL		
	}
	,
	{
		.ID = MENU_ID_SOS_RESULT,
			.oper.display = menu_SOS_result_display,
			.oper.on_key = NULL		
	}
	,
	{
		.ID = MENU_ID_SOS_ALARM,
			.oper.display = menu_SOS_Alarm_display,
			.oper.on_key = NULL		
	}
	,
	{
		.ID = MENU_ID_SOS_ALARM_RESULT,
			.oper.display = menu_SOS_Alarm_result_display,
			.oper.on_key = NULL		
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
			.oper.display = menu_adjustdate_display,
			.oper.on_key = menu_adjustdate_onkey
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
	LcdClearDisplay();
	LCD_Str_Print(POWERON_CHINA, 3, 1, TRUE);
}

static void    menu_poweroff_animation_display(void)
{
	LcdClearDisplay();
	LCD_Str_Print(POWEROFF_CHINA, 3, 1, TRUE);
}

static void    menu_showmessage_display(void)
{
	LcdClearDisplay();
	/* show message in global buffer, and jump back with mark after 1 second */
    
#ifdef CFG_GAS_SHORTMESSAGE           
	if(NearLastNodeID == MENU_ID_SHORTMESSAGE_NUMJUDGE)
	{
              LCD_Str_Print((uint8 *)g_jump_buf, 0, 1, TRUE);   
	}
	else
#endif     
	{
	       LCD_Str_Print((uint8 *)g_jump_buf, (LCD_LINE_WIDTH-osal_strlen((char *)g_jump_buf))/2, 1, TRUE);      
	}
	if(NearLastNodeID == MENU_ID_SOS_RESULT||NearLastNodeID==MENU_ID_SOS_ALARM_RESULT)
	{
		GasMonitor_StartMenuLibEvt(1000);
	}
	else
	{
		GasMonitor_StartMenuLibEvt(500);
	}
}

/* question should be put into g_buf  first*/
static void    menu_showquestion_display(void)
{
}

static void    menu_showquestion_onkey(uint8 keys, uint8 status)
{
}
static void    menu_showalert_display(void)
{
}
static void    menu_showalert_onkey(uint8 keys, uint8 status)
{
}

static void    menu_main_display(void)
{	   
	//uint8 x,y;
	LcdClearDisplay();
	LCD_Signal_Show(sig_index);
	LCD_Battery_Show(bat_index);
	Menu_UpdateTime();
	LCD_BigCharPrint('.',GASDISP_START_X+2, GASDISP_START_Y*LCD_LINE_HIGH);
      
#ifdef CFG_GAS_SHORTMESSAGE
	uint8 unreadcnt = GasSms_GetUnReadSMSCnt();
	bool  isHaveOverFlowSms = GasSms_IsHaveOverFlowSms();
	if(unreadcnt > 0 || isHaveOverFlowSms)
	{
		LCD_SMS_ICON_Show(LCD_LINE_WIDTH-5, 0);
	}
#endif
	if(ch4_GetLdoStatus() == true) // if Ldo is On
	{
		uint16 density;
		uint8 n1,n2,n3;
		density = GasMonitor_GetDensity();
		ch4_getnum_fromdensity(&n1,&n2,&n3, density);
		menu_GasPrint(n1,n2,n3,true);
	}
	else
	{
		uint8 n1,n2,n3;
		ch4_getnum_fromdensity(&n1,&n2,&n3, GasMonitor_GetDensity());
		menu_GasPrint(n1,n2,n3,true);
		LCD_Str_Print((uint8 *)OVERDENSITYED_CHINA, (LCD_LINE_WIDTH-osal_strlen((char *)OVERDENSITYED_CHINA))/2, 3,TRUE);
	}

	char buf[] = "ch4";
	LCD_BigCharPrint('%', 	GASDISP_START_X+7, GASDISP_START_Y*LCD_LINE_HIGH);
	LCD_Str_Print_Pixel((uint8 *)buf, GASDISP_START_X+10, 1.8*LCD_LINE_HIGH);
}

static void    menu_main_onkey(uint8 keys, uint8 status)
{
	/* other situations are normal*/
	{
		switch(keys)
		{
		case	HAL_KEY_MODE:
			{
				menu_JumptoMenu(MENU_ID_FUNCTIONLIST);
				break;
			}
		default:
			break;
		}
	}
}

static void    menu_SOS_display(void)
{
	uint8 str[8];
	LcdClearDisplay();
	LCD_Str_Print(CALLINGHELP_CHINA, 1, 1, TRUE);

	if(menu_SOSseq <= CALLINGHELP_MAXCNT)
	{
		_itoa(menu_SOSseq, str, 10);
		LCD_Str_Print(str, 6, 2, TRUE);
	}
}

static void    menu_SOS_result_display(void)
{
	LcdClearDisplay();
	uint8 *p;
	if(menu_SOSresult)
	{
		p = (uint8*) SOS_SUCCESS_CH;
	}
	else
	{
		p = (uint8*) SOS_FAILED_CH;
	}
	strcpy((char *)g_jump_buf,(char *)p);
	menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
}

static void    menu_SOS_Alarm_display(void)
{
	uint8 str[8];
	LcdClearDisplay();
	LCD_Str_Print(SOSCALLINGHELP_CHINA, 1, 1, TRUE);

	if(menu_SOSseq <= CALLINGHELP_MAXCNT)
	{
		_itoa(menu_SOSseq, str, 10);
		LCD_Str_Print(str, 6, 2, TRUE);
	}
}

static void    menu_SOS_Alarm_result_display(void)
{
	LcdClearDisplay();
	uint8 *p;
	if(menu_SOSresult)
	{
		p = (uint8*) SOS_ALARM_SUCCESS_CHINA;
	}
	else
	{
		p = (uint8*) SOS_ALARM_FAILED_CHINA;
	}
	strcpy((char *)g_jump_buf,(char *)p);
	menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
}

void menu_set_SOSseq(uint8 data)
{
	menu_SOSseq = data;
}

void menu_set_SOSresult(bool flag)
{
	menu_SOSresult = flag;
}


