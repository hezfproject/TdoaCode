#include "OSAL_Nv.h"
#include "App_cfg.h"
#include "hal_key.h"
#include "hal_key_cfg.h"
#include "lcd_serial.h"
#include "TimeUtil.h"
#include "Hal_drivers.h"


#include "Hal_audio.h"
#include "string.h"

#include "MenuLibController_tree.h"
#include "MineController_global.h"
#include "MenuLibController_global.h"
#include "MineController.h"
#include "MineController_MenuLib.h"
#include "MineController_MenuLibChinese.h"

//#include "MenuAdjustUtil.h"
//#include "MineApp_MP_Function.h"
//#include "MenuChineseInputUtil.h"
#include "WatchDogUtil.h"
#include "OnBoard.h"
#include "AppProtocolWrapper.h"
#define MENU_SMS_PAGE_MAX          5
#define MENU_TREE_DEPTH          4
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
}Tree_node_t;

typedef struct
{
      uint8 data_pos;
      uint8 cursor_pos;
      uint8 page_id;
      uint8 page_pos[MENU_SMS_PAGE_MAX];
}data_info_t;

/*static variable*/

//static data_info_t    data_info;
//static  node_info_t   node_info_temp = {1,0,0};

static uint16 med_sleeptime = MED_SLEEP_PERIOD;
static uint16 med_poll_interval = MED_POLL_INTERVAL;
static uint16 med_poll_timeout = MED_POLL_TIMEOUT;

/* setting Volume */
//static uint8             volsetting;
static stack_p_t       tree_stack_p[MENU_TREE_DEPTH];
static stack_t          tree_stack;

/*general functions */
static void menu_list_display(void);
static void menu_list_up_down_onkey(uint8 keys, uint8 list_len, bool list_direction);
static void menu_list_onkey(uint8 keys, uint8 status);
static void menu_selectlist_display(uint8 *p);
static void menu_selectlist_onkey(uint8 keys, uint8 status);

/*special functions */
static void menu_functionlist_onkey(uint8 keys, uint8 status);
static void menu_card_setting_dispaly(void);
static void menu_card_setting_onkey(uint8 keys, uint8 status);

static void menu_card_sleeptime_setting_display(void);
static void menu_card_sleeptime_setting_onkey(uint8 keys, uint8 status);
static void menu_card_pollinterval_setting_display(void);
static void menu_card_pollinterval_setting_onkey(uint8 keys, uint8 status);
static void menu_card_polltimeout_setting_display(void);
static void menu_card_polltimeout_setting_onkey(uint8 keys, uint8 status);

#ifdef MENU_RF_DEBUG
static void menu_set_channel_display(void);
static void menu_set_channel_onkey(uint8 keys, uint8 status);
#endif


static char*  const __code  ItemList_FunctionList[] = 
#ifdef MENU_RF_DEBUG
{CARDDETECT_CH, STATIONDETECT_CH,CARDSETTING_CH,STATIONSETTING_CH, CHANNEL_SELECT_CH,LABELCALBRATE_CH};
#else
{CARDDETECT_CH, STATIONDETECT_CH,CARDSETTING_CH,STATIONSETTING_CH, LABELCALBRATE_CH};
#endif
static char*  const __code  ItemList_CardSettingList[] = {CARD_SLEEPTIME_CH,CARD_POLLINTERVAL_CH,CARD_POLLTIMEOUT_CH};

static Tree_node_t  const __code Menu_Tree[] = 
{
	/* Function List */
	{
		.ID = MENU_ID_FUNCTIONLIST,
			.name = FUNCTIONLIST_CHINA,
			.ParentID = MENU_ID_MAIN,
			.FirstChildID = MENU_ID_CARD_DETECT,
			.ChildNum = sizeof(ItemList_FunctionList)/sizeof(ItemList_FunctionList[0]),
			.ItemName_CH = ItemList_FunctionList,               
			.oper.display = menu_list_display,
			.oper.on_key = menu_functionlist_onkey,
	}
    ,
      {
		.ID = MENU_ID_CARD_DETECT,
			.name = CARDDETECT_CH,
			.ParentID = MENU_ID_FUNCTIONLIST,
			.FirstChildID = NULL,
			.ChildNum = 0,
			.ItemName_CH = NULL,               
			.oper.display = NULL,
			.oper.on_key = NULL,
	}
    ,
      {
		.ID = MENU_ID_STATION_DETECT,
			.name = STATIONDETECT_CH,
			.ParentID = MENU_ID_FUNCTIONLIST,
			.FirstChildID = NULL,
			.ChildNum = 0,
			.ItemName_CH = NULL,               
			.oper.display = NULL,
			.oper.on_key = NULL,
	}
/*    ,
      {
		.ID = MENU_ID_PHONE_DETECT,
			.name = PHONEDETECT_CH,
			.ParentID = MENU_ID_FUNCTIONLIST,
			.FirstChildID = NULL,
			.ChildNum = 0,
			.ItemName_CH = NULL,               
			.oper.display = NULL,
			.oper.on_key = NULL,
	}*/
    ,
      {
		.ID = MENU_ID_CARD_SETTING,
			.name = CARDSETTING_CH,
			.ParentID = MENU_ID_FUNCTIONLIST,
			.FirstChildID = MENU_ID_CARD_SLEEPTIME,
			.ChildNum = sizeof(ItemList_CardSettingList)/sizeof(ItemList_CardSettingList[0]),
			.ItemName_CH = ItemList_CardSettingList,               
			.oper.display = menu_card_setting_dispaly,
			.oper.on_key = menu_card_setting_onkey
	}
    ,
      {
		.ID = MENU_ID_STATION_SETTING,
			.name = STATIONSETTING_CH,
			.ParentID = MENU_ID_FUNCTIONLIST,
			.FirstChildID = NULL,
			.ChildNum = 0,
			.ItemName_CH = NULL,               
			.oper.display = NULL,
			.oper.on_key = NULL,
	}
     ,  
#ifdef MENU_RF_DEBUG
    	{
		.ID = MENU_ID_CHANNEL_SELECT,
			.name = CHANNEL_SELECT_CH,
			.ParentID = MENU_ID_FUNCTIONLIST,
			.FirstChildID = NULL,
			.ChildNum = 0,
			.ItemName_CH = NULL,               
			.oper.display = menu_set_channel_display,
			.oper.on_key = menu_set_channel_onkey,
	}
        ,
#endif
      {
		.ID = MENU_ID_LABEL_CALBRATE,
			.name = LABELCALBRATE_CH,
			.ParentID = MENU_ID_FUNCTIONLIST,
			.FirstChildID = NULL,
			.ChildNum = 0,
			.ItemName_CH = NULL,               
			.oper.display = NULL,
			.oper.on_key = NULL,
	}
,
      {
		.ID = MENU_ID_CARD_SLEEPTIME,
			.name = CARD_SLEEPTIME_CH,
			.ParentID = MENU_ID_CARD_SETTING,
			.FirstChildID = NULL,
			.ChildNum = 0,
			.ItemName_CH = NULL,               
			.oper.display = menu_card_sleeptime_setting_display,
			.oper.on_key = menu_card_sleeptime_setting_onkey,
	}
     , 
      {
		.ID = MENU_ID_CARD_POLLINTERVAL,
			.name = CARD_POLLINTERVAL_CH,
			.ParentID = MENU_ID_CARD_SETTING,
			.FirstChildID = NULL,
			.ChildNum = 0,
			.ItemName_CH = NULL,               
			.oper.display = menu_card_pollinterval_setting_display,
			.oper.on_key = menu_card_pollinterval_setting_onkey,
	}
     , 
      {
		.ID = MENU_ID_CARD_POLLTIMEOUT,
			.name = CARD_POLLTIMEOUT_CH,
			.ParentID = MENU_ID_CARD_SETTING,
			.FirstChildID = NULL,
			.ChildNum = 0,
			.ItemName_CH = NULL,               
			.oper.display = menu_card_polltimeout_setting_display,
			.oper.on_key = menu_card_polltimeout_setting_onkey,
	}
};
static uint16 convert_str2int(uint8* p)
{
        uint16 result;
        uint8 i, len;

        len = osal_strlen((char*)p);
	 if(len == 0)
	 	return 0;

        i = 0;	
	 result = 0;
        while(i<len)
        {
               result = 10*result + (p[i++] - '0');
	  }
	  	
	  return result;
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
	for(uint8 i =0; i<len;i++)
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
			Stack_Push(&tree_stack, NULL, &node_info);
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
#if 0
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
#endif
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

	//if(CurrentNodeID == MENU_ID_CONTACTLIST)
		//osal_nv_read(MINEAPP_NV_CONTACT, 0, 1, &node.ChildNum);

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
	LCD_Str_Print((uint8 *)node.name, (LCD_LINE_WIDTH-osal_strlen(node.name))/2, 0, TRUE);       

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

void menu_functionlist_onkey(uint8 keys, uint8 status)
{
	Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];

	if(keys == HAL_KEY_SELECT)
	{
	    switch(node.FirstChildID + node_info.sel_item)
            {
                case MENU_ID_CARD_DETECT:
                case MENU_ID_STATION_DETECT:
                   break;
                case MENU_ID_CARD_SETTING:
                    menu_steptochild(node.FirstChildID + node_info.sel_item, 0);					
                    break;
                case MENU_ID_STATION_SETTING:
                    break;
#ifdef MENU_RF_DEBUG
                case MENU_ID_CHANNEL_SELECT:
                    menu_steptochild(node.FirstChildID + node_info.sel_item, 0);
                    break;
#endif
                case MENU_ID_LABEL_CALBRATE:
                    break;                    
            }   
	}
	else
	{
		menu_list_onkey( keys,  status);
	}

}

static void menu_input_number_onkey(uint8 keys)
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
		if((num_buf.len != 0) || (Key2ASCII(keys) != '0'))
		{
			num_buf.p[num_buf.len++] = Key2ASCII(keys);
			num_buf.p[num_buf.len] = '\0';
			//*value = convert_str2int(num_buf.p);
			menu_display();
		}
		break;
	case HAL_KEY_BACKSPACE:
		if(num_buf.len > 0)
		{   
			num_buf.p[--num_buf.len] = '\0';
			//*value = convert_str2int(num_buf.p);
			menu_display();
		}
		else
		{
		       NearLastNodeID = CurrentNodeID;
                    Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
		       menu_display();
                    //menu_steptoparent();
		}
		break;
	default:
		break;
	}
}

static void menu_card_sleeptime_setting_display(void)
{
      uint8 len = 0;
	  
      if(FIRSTTIME_INTO_NODE())
      {
               LcdClearDisplay();
	        len = osal_strlen(CARD_SLEEPTIME_CH);
		 LCD_Memory_Print(CARD_SLEEPTIME_CH, len, (LCD_LINE_WIDTH-len)/2, 0);
      }
      else
      {
	  	LCD_Line_Clear(2);
      	}
	LCD_Str_Print(num_buf.p, 6, 2, TRUE); 
	LCD_ShowCursor(6+num_buf.len, 2);
}

static void menu_card_sleeptime_setting_onkey(uint8 keys, uint8 status)
{
	switch(keys)
	{
	case HAL_KEY_SELECT:
		if(num_buf.len>0)
		{
		       med_sleepcfg.sleeptype = SLEEPTIME;
			med_sleepcfg.value = med_sleeptime;
			SET_ON_CARDSETTING();
	             menu_JumptoMenu(MENU_ID_SHOWSETTING);
		}
             break; 
	case HAL_KEY_BACKSPACE:
		{		
		       menu_input_number_onkey(keys);
			med_sleeptime = convert_str2int(num_buf.p);
		       if(med_sleeptime == 0)
                           med_sleeptime = MED_SLEEP_PERIOD;
		}
             break;
	default:
		if(((num_buf.len == 4)&&(num_buf.p[0] >= '6')) || 
		    (num_buf.len>= 5))
		{
	              med_sleeptime = MED_SLEEP_PERIOD;//overflow
		       Clr_Num_Buf();
			strcpy((char *)g_jump_buf, CARD_INVALID_DATA_CH);
                    menu_JumpandMark(MENU_ID_SHOWMESSAGE);
 		}
		else
		{		
		       menu_input_number_onkey(keys);
			med_sleeptime = convert_str2int(num_buf.p);
		}
		break;
	}

}

static void menu_card_pollinterval_setting_display(void)
{
      uint8 len = 0;
	  
      if(FIRSTTIME_INTO_NODE())
      {
               LcdClearDisplay();
	        len = osal_strlen(CARD_POLLINTERVAL_CH);
		 LCD_Memory_Print(CARD_POLLINTERVAL_CH, len, (LCD_LINE_WIDTH-len)/2, 0);
      }
      else
      {
	  	LCD_Line_Clear(2);
      	}
	LCD_Str_Print(num_buf.p, 7, 2, TRUE); 
	LCD_ShowCursor(7+num_buf.len, 2);
}

static void menu_card_pollinterval_setting_onkey(uint8 keys, uint8 status)
{
	switch(keys)
	{
	case HAL_KEY_SELECT:
		if(num_buf.len>0)
		{
		       med_sleepcfg.sleeptype = POLL_INTERVAL;
			med_sleepcfg.value = med_poll_interval;
			SET_ON_CARDSETTING();
	             menu_JumptoMenu(MENU_ID_SHOWSETTING);
		}
             break; 
	case HAL_KEY_BACKSPACE:
		{		
		       menu_input_number_onkey(keys);
			med_poll_interval = convert_str2int(num_buf.p);
		       if(med_poll_interval== 0)
                           med_poll_interval = MED_POLL_INTERVAL;
		}
             break; 
	default:
		if(((num_buf.len == 2)&&((num_buf.p[0] > '2') ||((num_buf.p[0] == '2')&&(num_buf.p[1] >= '5')))) || 
		    (num_buf.len>= 3))
		{
	              med_poll_interval = MED_POLL_INTERVAL;//overflow
		       Clr_Num_Buf();
			strcpy((char *)g_jump_buf, CARD_INVALID_DATA_CH);
                    menu_JumpandMark(MENU_ID_SHOWMESSAGE);
 		}
		else
		{		
		       menu_input_number_onkey(keys);
			med_poll_interval = convert_str2int(num_buf.p);
		}
		break;
	}

}
static void menu_card_polltimeout_setting_display(void)
{
      uint8 len = 0;
	  
      if(FIRSTTIME_INTO_NODE())
      {
               LcdClearDisplay();
	        len = osal_strlen(CARD_POLLTIMEOUT_CH);
		 LCD_Memory_Print(CARD_POLLTIMEOUT_CH, len, (LCD_LINE_WIDTH-len)/2, 0);
      }
      else
      {
	  	LCD_Line_Clear(2);
      	}
	LCD_Str_Print(num_buf.p, 7, 2, TRUE); 
	LCD_ShowCursor(7+num_buf.len, 2);
}

static void menu_card_polltimeout_setting_onkey(uint8 keys, uint8 status)
{
	switch(keys)
	{
	case HAL_KEY_SELECT:
		if(num_buf.len>0)
		{
		       med_sleepcfg.sleeptype = POLL_TIMEOUT;
			med_sleepcfg.value = med_poll_timeout;
			SET_ON_CARDSETTING();
	             menu_JumptoMenu(MENU_ID_SHOWSETTING);
		}
             break; 
	case HAL_KEY_BACKSPACE:
		{		
		       menu_input_number_onkey(keys);
			med_poll_timeout = convert_str2int(num_buf.p);
		       if(med_poll_timeout == 0)
                           med_poll_timeout = MED_POLL_TIMEOUT;
		}
             break; 
	default:
		if(((num_buf.len == 2)&&((num_buf.p[0] > '2') ||((num_buf.p[0] == '2')&&(num_buf.p[1] >= '5')))) || 
		    (num_buf.len>= 3))
		{
	              med_poll_timeout = MED_POLL_TIMEOUT;//overflow
		       Clr_Num_Buf();
			strcpy((char *)g_jump_buf, CARD_INVALID_DATA_CH);
                    menu_JumpandMark(MENU_ID_SHOWMESSAGE);
 		}
		else
		{		
		       menu_input_number_onkey(keys);
			med_poll_timeout = convert_str2int(num_buf.p);
		}
		break;
	}

}

static void menu_card_setting_dispaly(void)
{
       uint8 p[8];
	LCD_CloseCursor();
       LCD_Str_Print(CARDSETTING_CH, (LCD_LINE_WIDTH- osal_strlen((char*)CARDSETTING_CH))/2, 0, TRUE);
	if(node_info.sel_item == 0)  /*sleeptime*/
	{
	       _ltoa(med_sleeptime, p, 10);
	}
	else if(node_info.sel_item == 1) 
	{
	       _ltoa(med_poll_interval, p, 10);
	}
	else if(node_info.sel_item == 2) 
	{
	       _ltoa(med_poll_timeout, p, 10);
	}
	menu_selectlist_display(p);

}

static void menu_card_setting_onkey(uint8 keys, uint8 status)
{
       if(HAL_KEY_BACKSPACE == keys)
	{
             menu_steptoparent();
	}
       if(HAL_KEY_SELECT == keys)
	{
		Tree_node_t node = Menu_Tree[GetIdxFromID(CurrentNodeID)];
		//SET_ON_IDLE();
             Clr_Num_Buf();
		Stack_Push(&global_stack, CurrentNodeID, &node_info);
		NearLastNodeID = CurrentNodeID;
		CurrentNodeID = node.FirstChildID + node_info.sel_item;
		menu_display();
             //menu_steptochild(node.FirstChildID + node_info.sel_item, 0);
	}
	else if((HAL_KEY_LEFT== keys) || (HAL_KEY_RIGHT == keys) ||
		   (HAL_KEY_UP== keys) || (HAL_KEY_DOWN== keys))
	{
		menu_selectlist_onkey(keys, status);
	}

}
#ifdef MENU_RF_DEBUG
static void menu_set_channel_display(void)
{
	if(FIRSTTIME_INTO_NODE())
	{
		LcdClearDisplay();
		Clr_Num_Buf();
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
			uint8 channel = 0;
			for(uint8 i=0; i<num_buf.len; i++)
				channel = 10*channel + (num_buf.p[i] - '0');
			if((channel > 26) || (channel < 11))
			{
				strcpy((char *)g_jump_buf,INVALID_CHANNEL_CHINA);
				menu_JumpandMark(MENU_ID_SHOWMESSAGE);
				//osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 1000);	
				//MineApp_StartMenuLibEvt(1000);
				return;
			}
                    else
                   {
                   		uint32 channel_mask = (uint32)((uint32)1<<channel);
                           MineController_JoinNWK(channel);
        			osal_nv_write( ZCD_NV_SET_CHANLIST, 0, sizeof(uint32), &channel_mask);
                    	strcpy((char *)g_jump_buf,SETTED_CHINA);
				menu_JumpandMark(MENU_ID_SHOWMESSAGE);
                   }
                   break;
		}
	case HAL_KEY_BACKSPACE:
		Clr_Num_Buf();
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
		if(num_buf.len < 2)
		{
			num_buf.p[num_buf.len++] = Key2ASCII(keys);
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

