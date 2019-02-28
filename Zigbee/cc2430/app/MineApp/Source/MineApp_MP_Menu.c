/**************************************************************************************************
  Filename:       MineApp_MP_Menu.h
  Revised:        $Date: 2009/08/13 03:44:13 $
  Revision:       $Revision: 1.25 $

  Description:    Menu Application of mobile phone.
  **************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "MineApp_MP_Menu.h"
#include "App_cfg.h"

#include "MineApp_MP_Chinese.h"
#include "MineApp_global.h"
#include "hal_key_cfg.h"
#include "hal_key.h"
#include "lcd_serial.h"
#include "TimeUtil.h"

/*********************************************************************
 * Some constant
 */
#define CONTACT_NUM 6
#define MAX_LCD_LINE 4
#define FUNC_ITEM_NUM 3
#define RECENT_CALL_ITEM_NUM 3
#define MAX_CALL_NUM 10         // the max number of recent call record
#define CONTACT_READ_NUM 3 //the max amount of contact node that read from flash
#define TIME_LEN 6
#define DATE_LENTH 11

/*********************************************************************
 * MENU Define
 */
#define ON_NWK_INIT_MENU() (menu_status == NWK_INIT_MENU)
#define ON_MAIN_MENU() (menu_status == MAIN_MENU)
#define ON_FUNCTION_MENU() (menu_status == FUNCTION_MENU)
#define ON_CONTACT_MENU() (menu_status == CONTACT_MENU)
#define ON_CONTACT_NUM_MENU() (menu_status == CONTACT_NUM_MENU)
#define ON_INCOMING_MENU() (menu_status == INCOMING_MENU)
#define ON_RECENT_CALL_MENU() (menu_status == RECENT_CALL_MENU)
#define ON_MISSED_MENU() (menu_status == MISSED_MENU)
#define ON_DIALING_MENU() (menu_status == DIALING_MENU)
#define ON_NUM_MENU() (menu_status == NUM_MENU)
#define ON_ANSWERED_RECORD_MENU() (menu_status == ANSWERED_RECORD_MENU)
#define ON_MISSED_RECORD_MENU() (menu_status == MISSED_RECORD_MENU)
#define ON_DIALED_RECORD_MENU() (menu_status == DIALED_RECORD_MENU)
#define ON_DIALED_RECORD_TIME_MENU() (menu_status == DIALED_RECORD_TIME_MENU)
#define ON_MISSED_RECORD_TIME_MENU() (menu_status == MISSED_RECORD_TIME_MENU)
#define ON_ANSWERED_RECORD_TIME_MENU() (menu_status == ANSWERED_RECORD_TIME_MENU)
#define ON_MESSAGE_MENU() (menu_status == MESSAGE_MENU)
#define ON_CALENDAR_MENU() (menu_status == CALENDAR_MENU)
#define ON_TALKING_MENU() (menu_status == TALKING_MENU)
#define ON_UNREAD_MESSAGE_MENU() (menu_status == UNREAD_MESSAGE_MENU)

#define SET_NWK_INIT_MENU_ON() (menu_status = NWK_INIT_MENU)
#define SET_MAIN_MENU_ON() (menu_status = MAIN_MENU)
#define SET_FUNCTION_MENU_ON() (menu_status = FUNCTION_MENU)
#define SET_CONTACT_MENU_ON() (menu_status = CONTACT_MENU)
#define SET_CONTACT_NUM_MENU_ON() (menu_status = CONTACT_NUM_MENU)
#define SET_INCOMING_MENU_ON() (menu_status = INCOMING_MENU)
#define SET_RECENT_CALL_MENU_ON() (menu_status = RECENT_CALL_MENU)
#define SET_MISSED_MENU_ON() (menu_status = MISSED_MENU)
#define SET_DIALING_MENU_ON() (menu_status = DIALING_MENU)
#define SET_NUM_MENU_ON() (menu_status = NUM_MENU)
#define SET_ANSWERED_RECORD_MENU_ON() (menu_status = ANSWERED_RECORD_MENU)
#define SET_MISSED_RECORD_MENU_ON() (menu_status = MISSED_RECORD_MENU)
#define SET_DIALED_RECORD_MENU_ON() (menu_status = DIALED_RECORD_MENU)
#define SET_DIALED_RECORD_TIME_MENU_ON() (menu_status = DIALED_RECORD_TIME_MENU)
#define SET_MISSED_RECORD_TIME_MENU_ON() (menu_status = MISSED_RECORD_TIME_MENU)
#define SET_ANSWERED_RECORD_TIME_MENU_ON() (menu_status = ANSWERED_RECORD_TIME_MENU)
#define SET_MESSAGE_MENU_ON() (menu_status = MESSAGE_MENU)
#define SET_CALENDAR_MENU_ON() (menu_status = CALENDAR_MENU)
#define SET_TALKING_MENU_ON() (menu_status = TALKING_MENU)
#define SET_UNREAD_MESSAGE_MENU_ON() (menu_status = UNREAD_MESSAGE_MENU)

/*********************************************************************
 * Call Status Define
 */
/*#define SET_CALL_STATUS(Status) \
st ( \
	CallingStatus = Status;\
)
#define SET_ON_CALLING() SET_CALL_STATUS(STATUS_CALLING)
#define SET_ON_CALLED() SET_CALL_STATUS(STATUS_CALLIED)
#define SET_ON_CALL_IDLE() SET_CALL_STATUS(STATUS_IDLE)
*/
/*********************************************************************
 * Menu enums
 */
enum MENU_STATUS
{
    NWK_INIT_MENU,
    MAIN_MENU,
    CONTACT_MENU,
    CONTACT_NUM_MENU,
    FUNCTION_MENU,
    INCOMING_MENU,
    MISSED_MENU,
    RECENT_CALL_MENU,
    DIALING_MENU,
    NUM_MENU,
    MISSED_RECORD_MENU,
    ANSWERED_RECORD_MENU,
    DIALED_RECORD_MENU,
    DIALED_RECORD_TIME_MENU,
    ANSWERED_RECORD_TIME_MENU,
    MISSED_RECORD_TIME_MENU,
    MESSAGE_MENU,
    CALENDAR_MENU,
    UNREAD_MESSAGE_MENU,
    TALKING_MENU,
    NEXT_ITEM_MENU,
    FRONT_ITEM_MENU,
    NO_POWER_MENU,
    MAX_MENU,
};

enum
{
    RECENT_CALL,
    CONTACT,
    MESSAGE,
    CALENDAR,
    FUNCTION_END
};

enum
{
    MISSED_CALL=1,
    ANSWERED_CALL,
    DIALED_CALL
};

/*********************************************************************
 * Global struct
 */

//It is the call node which include name, tel_num  and time
typedef struct
{
  uint8              num[NUM_LEN];
  uint8              time[TIME_LEN];
}Record;
/*-----------------Local variable-----------------*/
static uint8             num_key = 0;            // the amount of pressed num_key
static uint8             sig_index = 0;
static uint8             bat_index = 0;
static uint8             high_line = 0;          // indicate the selected item which in the LCD
static uint8             sel_item = 0;
static uint8             show_item = 0;
static uint8             menu_status = 0;
static uint8             missed_call_amount;
static uint8             C_num[NUM_LEN];  // store the number 
static bool              shortcuts_flag = FALSE;        // it is the flag of key shortcut

/*-----------------Global variable-----------------*/
uint8             CallingStatus;

/*********************************************************************
 * Functions
 */
 
 /*********************************************************************
 * @fn      Init_Menu_NV
 *
 * @brief   Initialization function for the Memory that MineApp Menu used.
 *
 * @param   none
 *
 * @return  none
 */
uint8 Init_Menu_NV(void)
{

    uint8 temp, pos = 0;
    uint16 len;

    len = 10*sizeof(Record) + 1;

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

   if( NV_OPER_FAILED == osal_nv_item_init(MINEAPP_NV_CONTACT, sizeof(contactor), NULL))
   {
	    return NV_OPER_FAILED;
   }
   else
   {
        uint8 i; 
        len = sizeof(contactor)/sizeof(C_node);
        for(i=0; i<len; i++)
		osal_nv_write(MINEAPP_NV_CONTACT, i*sizeof(C_node), sizeof(C_node), &contactor[i]);

   }
   
   return ZSUCCESS;

}


 /*********************************************************************
 * @fn      Add_Record_To_List
 *
 * @brief   Move the record from tail to head and add the new_reocrd to tail of list.
 *
 * @param   index   --the index of record list that will be operated
 *               new_record--the reocrd that will be added  
 *
 * @return  none
 */
static void Add_Record_To_List(uint8 index, Record* new_record)
{
       uint8 i, j, len, pos;
	uint16 nv_id;
	Record record;


	if(index == MISSED_CALL)
		nv_id = MINEAPP_NV_MISSED;
	else if(index == DIALED_CALL)
		nv_id = MINEAPP_NV_DIALED;
	else if(index == ANSWERED_CALL)
		nv_id = MINEAPP_NV_ANSWERED;
	else
		return;

	osal_nv_read(nv_id, 0, 1, &pos);

       len = sizeof(Record);
	for(i=0; i<pos; i++)
	{
	      osal_nv_read(nv_id, (pos-1-i)*len+1, len, &record);
	      if(!osal_strcmp(record.num, new_record->num))
	      {
	            for(j=pos-1-i; j<pos-1; j++){
	                 osal_nv_read(nv_id, (j+1)*len+1, len, &record);
	                 osal_nv_write(nv_id, j*len+1, len, &record);
	            	}
	             osal_nv_write(nv_id, j*len+1, len, new_record);
		      return;
	      }
	}
	

    if((pos < MAX_CALL_NUM))
    {
	    osal_nv_write(nv_id, pos*len+1, len, new_record);
           pos++;
    }
    else if (pos == MAX_CALL_NUM)
    {
           
	    for(i=1; i<MAX_CALL_NUM-1; i++)
	    {
	         osal_nv_read(nv_id, i*len+1, len, &record);
		  osal_nv_write(nv_id, (i-1)*len+1, len, &record);
	    }
	    osal_nv_write(nv_id, i*len+1, len, new_record);
	    pos = MAX_CALL_NUM;
    }
	
    osal_nv_write(nv_id, 0, 1, &pos);
}

 /*********************************************************************
 * @fn      Clr_Num_Buf
 *
 * @brief   clear the num.
 *
 * @param   none
 *
 * @return  none
 */
 static void Clr_Num_Buf(void)
 {
      uint8 len = osal_strlen((char*)C_num);
      osal_memset(C_num, '\0', len);
      num_key = 0;
 }

 /*********************************************************************
 * @fn      Show_Calendar
 *
 * @brief   show the calendar.
 *
 * @param   none
 *
 * @return  none
 */
static void Show_Calendar(void)
{    
      uint8 *p = NULL;

      if(NULL == (p = osal_mem_alloc(DATE_LENTH)))
	  	return;
      LcdClearDisplay();
      LCD_Str_Print(CALENDAR_CHINA, 4, 6, 0, TRUE);
      GetDateChar(p);
      LCD_Str_Print(p, 10, 3, 1, TRUE);
      GetWeekChar(p);
      LCD_Str_Print(p, 6, 5, 2, TRUE);
      osal_mem_free(p);
      
}
 /*********************************************************************
 * @fn      Show_Call_Record
 *
 * @brief   Show the call record menu
 *
 * @param   index--the index of the record
 *
 * @return  none
 */
static void Show_Call_Record(const uint8 index)
{
    Record record;
    uint16 nv_id;
    uint8 i, j, k, len, pos;


       LcdClearDisplay();

    	if (index == DIALED_CALL)
	{
	     nv_id = MINEAPP_NV_DIALED;
            LCD_Str_Print(DIALED_CALL_CHINA, 8, 4, 0, TRUE);
    	}
	else if(index == MISSED_CALL)
	{
	      nv_id = MINEAPP_NV_MISSED;
            LCD_Str_Print(MISSED_CALL_CHINA, 8, 4, 0, TRUE);
    	}
	else if(index == ANSWERED_CALL)
	{
	     nv_id = MINEAPP_NV_ANSWERED;
            LCD_Str_Print(ANSWERED_CALL_CHINA, 8, 4, 0, TRUE);
    	}


	     osal_nv_read(nv_id, 0, 1, &pos);
	     if(pos == 0)
		 	return;
	     
            len = sizeof(Record);

            if((pos > 0) &&(pos <= MAX_LCD_LINE-1))
            {	   
                 k = pos + '0';
		   for(i=pos, j=1; i>=1; i--, j++)
		   {
                     len = sizeof(Record);
		        osal_nv_read(nv_id, (show_item-j)*len+1, len, &record);
			 LCD_Str_Print(&k, 1, 0, j, TRUE);
	              LCD_Str_Print(")", 1, 1, j, TRUE);
	              len = (uint8)osal_strlen((char *)record.num);
	              LCD_Str_Print(record.num, len, 2, j, TRUE);
		       --k;
		  }
    	     }
	     else if((pos > 0) &&(pos <= MAX_CALL_NUM))
            {     
		   k = show_item+ '0';
		   for(i = 1, j=show_item-1; i<=3; i++)
		   {
                     len = sizeof(Record);
		        osal_nv_read(nv_id, j*len+1, len, &record);
		        if( k<='9' )
		        {
		             LCD_Str_Print(&k, 1, 0, i, TRUE);
		             LCD_Str_Print(")", 1, 1, i, TRUE);
	                   len = (uint8)osal_strlen((char *)record.num);
		             LCD_Str_Print(record.num, len, 2, i, TRUE);
		        }
		        else
		        {
		             uint8 m = k - 10;
		             LCD_Str_Print("1", 1, 0, i, TRUE);
		             LCD_Str_Print(&m, 1, 1, i, TRUE);
		             LCD_Str_Print(")", 1, 2, i, TRUE);
	                    len = (uint8)osal_strlen((char *)record.num);
		             LCD_Str_Print(record.num, len, 3, i, TRUE);
		       }
		       if(k=='1')
			      k = pos+ '0';
		       else
			      --k;
		       if(j==0)
	   	            j = pos - 1;
	             else
	   	          --j;			
		}
	  }

	 if(high_line!=0)
             LCD_Line_Inv(high_line);
}
 /*********************************************************************
 * @fn      Show_Call_Record_Time
 *
 * @brief   Show the time of a record
 *
 * @param   index--the index of the record
 *
 * @return  none
 */
 static void Show_Call_Record_Time(const uint8 index)
{
    Record record;
    uint16 nv_id;
    uint8 len;


       LcdClearDisplay();

    	if (index == DIALED_RECORD_TIME_MENU)
	{
	     nv_id = MINEAPP_NV_DIALED;
            LCD_Str_Print(DIALED_CALL_CHINA, 8, 4, 0, TRUE);
    	}
	else if(index == MISSED_RECORD_TIME_MENU)
	{
	      nv_id = MINEAPP_NV_MISSED;
            LCD_Str_Print(MISSED_CALL_CHINA, 8, 4, 0, TRUE);
    	}
	else if(index == ANSWERED_RECORD_TIME_MENU)
	{
	     nv_id = MINEAPP_NV_ANSWERED;
            LCD_Str_Print(ANSWERED_CALL_CHINA, 8, 4, 0, TRUE);
    	}

      len = sizeof(Record);
      osal_nv_read(nv_id, (sel_item-1)*len+1, len, &record);
	len = (uint8)osal_strlen((char *)record.num);
	LCD_Str_Print(record.num, len, 0, 1, TRUE);
	len = (uint8)osal_strlen((char *)record.time);
	LCD_Str_Print(record.time, len, 0, 2, TRUE);
 }
 /*********************************************************************
 * @fn      Show_Contact
 *
 * @brief   Show contact menu
 *
 * @param   none
 *
 * @return  none
 */
 static void Show_Contact(void)
 {
    uint8 i, len, temp, contact_offset;
    C_node contact_temp; 

    LcdClearDisplay();
    LCD_Str_Print(CONTACT_CHINA, 6, 5, 0, TRUE);
	
    len = sizeof(C_node);

    contact_offset = show_item;
    for(i=0; i<CONTACT_READ_NUM; i++)
    {
	   osal_nv_read(MINEAPP_NV_CONTACT, (contact_offset++)*len,  len, &contact_temp);

	   if(contact_offset < 10)
	   {
             temp = contact_offset + '0';	
             LCD_Str_Print(&temp, 1, 0, i+1, TRUE);
             LCD_Str_Print(")", 1, 1, i+1, TRUE);
	      LCD_Str_Print(contact_temp.name, MAX_NAME_LEN, 2, i+1, TRUE);
	   }
	   else
	   {
             temp = contact_offset/10 + '0';
             LCD_Str_Print(&temp, 1, 0, i+1, TRUE);
             temp = contact_offset%10 + '0';
             LCD_Str_Print(&temp, 1, 1, i+1, TRUE);
             LCD_Str_Print(")", 1, 2, i+1, TRUE);
	      LCD_Str_Print(contact_temp.name, MAX_NAME_LEN, 3, i+1, TRUE);
	   }
	   if(contact_offset >= (sizeof(contactor)/sizeof(C_node)))
	   	  contact_offset = 0;
    }	
    if(high_line!=0)
          LCD_Line_Inv(high_line);
	
 }
 
  /*********************************************************************
 * @fn      Show_Contact_Num
 *
 * @brief   Show the number of a contact
 *
 * @param   none
 *
 * @return  none
 */
static void Show_Contact_Num(void)
{       
       uint8 len;
       C_node contact_temp;
	   
	len = (sizeof(C_node));
	osal_nv_read(MINEAPP_NV_CONTACT, sel_item*len, len, &contact_temp);
	LcdClearDisplay();
	len = (uint8)osal_strlen((char*)contact_temp.name);
       LCD_Str_Print(contact_temp.name, MAX_NAME_LEN, (LCD_LINE_WIDTH-len)/2, 0, TRUE);
       LCD_Str_Print(contact_temp.num, NUM_LEN, 0, 1, TRUE);
	LCD_Line_Inv(1);

}
  /*********************************************************************
 * @fn      Show_Function
 *
 * @brief   Show the function of MP
 *
 * @param   none
 *
 * @return  none
 */
static void Show_Function(void)
{

    LcdClearDisplay();
    LCD_Str_Print(FUNCTION_CHINA, 6, 5, 0, TRUE);
    if(show_item == 0)
    {
	    LCD_Str_Print("1)", 2, 0, 1, TRUE);
	    LCD_Str_Print(CALL_RECORD_CHINA, 8, 2, 1, TRUE);
	    LCD_Str_Print("2)", 2, 0, 2, TRUE);
	    LCD_Str_Print(CONTACT_CHINA, 6, 2, 2, TRUE);
	    LCD_Str_Print("3)", 2, 0, 3, TRUE);
	    LCD_Str_Print(MESSAGE_CHINA, 6, 2, 3, TRUE);
    }
    else if(show_item == 1)
    {
	    LCD_Str_Print("2)", 2, 0, 1, TRUE);
	    LCD_Str_Print(CONTACT_CHINA, 6, 2, 1, TRUE);
	    LCD_Str_Print("3)", 2, 0, 2, TRUE);
	    LCD_Str_Print(MESSAGE_CHINA, 6, 2, 2, TRUE);
	    LCD_Str_Print("4)", 2, 0, 3, TRUE);
	    LCD_Str_Print(CALENDAR_CHINA, 4, 2, 3, TRUE);
    }
    LCD_Line_Inv(high_line);
	
}


 /*********************************************************************
 * @fn      Show_MP_Menu
 *
 * @brief   set the buffer that has the context of menu.
 *
 * @param   item--the index of the menu which buffer will be setted
 *
 * @return  none
 */

static int Show_MP_Menu(const uint8 item)
{
   uint8  len, temp;
   uint8 *p = NULL;

  if (item >= MAX_MENU)
    return -1;
   
  switch (item)
  {
  case NWK_INIT_MENU:
    LcdClearDisplay();
    LCD_Str_Print(NWK_INIT_CHINA, 11 , 3, 1, TRUE);  	
    break;
  case MAIN_MENU:
    LcdClearDisplay();
    LCD_Signal_Show(sig_index);
    LCD_Battery_Show(bat_index);
    LCD_Str_Print(FUNCTION_CHINA, 6 , 0, 3, TRUE);
    LCD_Str_Print(CONTACT_CHINA, 6, 10, 3, TRUE);
    if(NULL== (p = osal_mem_alloc(TIME_LEN)))
		break;
    GetTimeChar(p);
    LCD_Str_Print(p, 5, 5, 1, TRUE);
    osal_mem_free(p);
    p = NULL;
    break;
  case FUNCTION_MENU:
       Show_Function();
       break;
  case CONTACT_MENU:
       Show_Contact();
       break;
  case CONTACT_NUM_MENU:
       Show_Contact_Num();
  	break;
  case NUM_MENU:
	len = (uint8)osal_strlen((char*)C_num);
	LcdClearDisplay();
	LCD_Str_Print(C_num , len, 0 , 3, FALSE);
    break;
  case MISSED_MENU:
    LcdClearDisplay();
    temp = missed_call_amount +'0'; 
    if(temp <= '9')
         LCD_Str_Print(&temp, 1, 3, 2, TRUE);
   else
   {
         temp = missed_call_amount/10;
	  temp += '0';
         LCD_Str_Print(&temp, 1, 2, 2, TRUE);
         temp = missed_call_amount - 10*(missed_call_amount/10) + '0'; 
         LCD_Str_Print(&temp, 1, 3, 2, TRUE);
   }
    LCD_Str_Print(MISSED_CALL_MENU_CHINA, 10, 4, 2, TRUE);
    break;
  case RECENT_CALL_MENU:
    LcdClearDisplay();
    LCD_Str_Print(CALL_RECORD_CHINA, 8, 4, 0, TRUE);
    LCD_Str_Print("1)", 2, 0, 1, TRUE);
    LCD_Str_Print(MISSED_CALL_CHINA, 8, 2, 1, TRUE);
    LCD_Str_Print("2)", 2, 0, 2, TRUE);
    LCD_Str_Print(ANSWERED_CALL_CHINA, 8, 2, 2, TRUE);
    LCD_Str_Print("3)", 2, 0, 3, TRUE);
    LCD_Str_Print(DIALED_CALL_CHINA, 8, 2, 3, TRUE);
    LCD_Line_Inv(high_line);
    break;
  case DIALING_MENU:
    LcdClearDisplay();
    LCD_Signal_Show(sig_index);
    LCD_Battery_Show(bat_index);
    LCD_Str_Print(CALLING_CHINA, 11, 3, 1, TRUE);
    len = (uint8)osal_strlen((char*)C_num);
	
    if(len<=16)
           LCD_Str_Print(C_num, len, (16-len)/2, 2, TRUE);
    else
           LCD_Str_Print(C_num, len, 0, 2, TRUE);
    break;
  case MISSED_RECORD_MENU:
    Show_Call_Record(MISSED_CALL);
    break;
  case ANSWERED_RECORD_MENU:
    Show_Call_Record(ANSWERED_CALL);
    break;
  case DIALED_RECORD_MENU:
    Show_Call_Record(DIALED_CALL);
    break;
  case DIALED_RECORD_TIME_MENU:
  case MISSED_RECORD_TIME_MENU:
  case ANSWERED_RECORD_TIME_MENU:
    Show_Call_Record_Time(item);
    break;
  case INCOMING_MENU:
    LcdClearDisplay();
    LCD_Signal_Show(sig_index);
    LCD_Battery_Show(bat_index);
    len = (uint8)osal_strlen((char*)C_num);
    LCD_Str_Print(C_num, len, (16-len)/2, 1, TRUE);
    LCD_Str_Print(INCOMING_CHINA, 4, 6, 2, TRUE);
    break;
  case MESSAGE_MENU:
    LcdClearDisplay();
    LCD_Str_Print(MESSAGE_CHINA, 6, 5, 0, TRUE);
    LCD_Str_Print(MESSAGE_RECEIVE_CHINA, 11, 0, 1, TRUE);
    LCD_Str_Print(MESSAGE_WRITE_CHINA, 9, 0, 2, TRUE);
    LCD_Line_Inv(1);
    break;
  case CALENDAR_MENU:
    Show_Calendar();
    break;
  case UNREAD_MESSAGE_MENU:
    LcdClearDisplay();
    LCD_Str_Print(UNREAD_MESSAGE_CHINA, 6, 5, 2, TRUE);
    break;
  case TALKING_MENU:
    LcdClearDisplay();
    LCD_Signal_Show(sig_index);
    LCD_Battery_Show(bat_index);
    LCD_Str_Print(VOICING_CHINA, 8, 4, 1, TRUE);
    len = (uint8)osal_strlen((char*)C_num);
    LCD_Str_Print(C_num, len, (16-len)/2, 2, TRUE);
    break;
  case NEXT_ITEM_MENU:
  case FRONT_ITEM_MENU:
    LCD_Line_Inv(high_line);
    break;
  case NO_POWER_MENU:
    LcdClearDisplay();
    LCD_Str_Print(NO_POWER_CHINA, 10, 3, 1, TRUE);
    break;
  default:
    break;
  }
  return ZSUCCESS;
}

 /*********************************************************************
 * @fn      On_NumKey
 *
 * @brief  Handle the digital key event of menu.
 *
 * @param   key_value --the value of key that has been pressed
 *
 * @return  none
 */
static void On_NumKey(const uint8 key_value)
{
  if (ON_MAIN_MENU())
  {
    C_num[num_key++] = key_value;
    C_num[num_key] = '\0';
    Show_MP_Menu(NUM_MENU);
    SET_NUM_MENU_ON();
  }
  else if (ON_NUM_MENU())
  {
       if(num_key<19)
  	{
    		C_num[num_key++] = key_value;
    		C_num[num_key] = '\0';
    		Show_MP_Menu(NUM_MENU);
	}
	else
	{
    		Show_MP_Menu(NUM_MENU);
	}
	
  }
}
  /*********************************************************************
 * @fn      On_Star
 *
 * @brief  Handle the Star key event of menu.
 *
 * @param   none
 *
 * @return  none
 */
static void On_Star(const uint8 key_value)
{
  if (ON_MAIN_MENU())
  {
    C_num[num_key++] = key_value;
    C_num[num_key] = '\0';
    Show_MP_Menu(NUM_MENU);
    SET_NUM_MENU_ON();
  }
  else if (ON_NUM_MENU())
  {
       if(num_key<19)
  	{
    		C_num[num_key++] = key_value;
    		C_num[num_key] = '\0';
    		Show_MP_Menu(NUM_MENU);
	}
	else
	{
    		Show_MP_Menu(NUM_MENU);
	}
  }
}
  /*********************************************************************
 * @fn      On_Pound
 *
 * @brief  Handle the Dial key event of menu.
 *
 * @param   none
 *
 * @return  none
 */
static void On_Pound(const uint8 key_value)
{ 
  if (ON_MAIN_MENU())
  {
    C_num[num_key++] = key_value;
    C_num[num_key] = '\0';
    Show_MP_Menu(NUM_MENU);
    SET_NUM_MENU_ON();
  }
  else if (ON_NUM_MENU())
  {
       if(num_key<19)
  	{
    		C_num[num_key++] = key_value;
    		C_num[num_key] = '\0';
    		Show_MP_Menu(NUM_MENU);
	}
	else
	{
    		Show_MP_Menu(NUM_MENU);
	}
  }

}
 /*********************************************************************
 * @fn      On_Dial
 *
 * @brief  Handle the Dial key event of menu.
 *
 * @param   none
 *
 * @return  none
 */
static void On_Dial(void)
{

  if (ON_NUM_MENU())
  {
     Record new_record;

     Show_MP_Menu(DIALING_MENU);
     SET_DIALING_MENU_ON();
     SET_ON_CALLING();
     osal_memcpy(new_record.num, C_num, osal_strlen((char*)C_num)+1);
     GetTimeChar(new_record.time);
     Add_Record_To_List(DIALED_CALL, &new_record);
  }
  else if (ON_INCOMING_MENU())
  {
      Record new_record;

      Show_MP_Menu(TALKING_MENU);
      SET_TALKING_MENU_ON();
      SET_ON_CALLED();
      osal_memcpy(new_record.num, C_num, osal_strlen((char*)C_num)+1);
      GetTimeChar(new_record.time);
      Add_Record_To_List(ANSWERED_CALL, &new_record);
  }
  else if (ON_MAIN_MENU())
  { 
      uint8 pos;

      osal_nv_read(MINEAPP_NV_DIALED, 0, 1, &pos);
      sel_item = pos;
      show_item = pos;
      high_line = 1;
      shortcuts_flag = TRUE;
      Show_MP_Menu(DIALED_RECORD_MENU);
      SET_DIALED_RECORD_MENU_ON();
  }
  else if (ON_DIALED_RECORD_MENU()||ON_MISSED_RECORD_MENU()||ON_ANSWERED_RECORD_MENU())
  {
      uint8 temp, len;
      uint16  nv_id;
      Record* new_record;
	  
    	if (ON_DIALED_RECORD_MENU())
	{
		temp = DIALED_CALL;
		nv_id = MINEAPP_NV_DIALED;
    	}
	else if(ON_MISSED_RECORD_MENU())
	{
		temp = MISSED_CALL;
		nv_id = MINEAPP_NV_MISSED;
    	}
	else if(ON_ANSWERED_RECORD_MENU())
	{
		temp = ANSWERED_CALL;
		nv_id = MINEAPP_NV_ANSWERED;
    	}
	
       osal_nv_read(nv_id, 0, 1, &len);
       if(len == 0)
	 	 return;
	  
       len = sizeof(Record);
       new_record = osal_mem_alloc(len);
       osal_nv_read(nv_id, (sel_item-1)*len+1, len, new_record);
	GetTimeChar(new_record->time);
	len = (uint8)osal_strlen((char*)new_record->num);
	osal_memcpy(C_num, new_record->num,  len);
	C_num[len] = '\0';
       Add_Record_To_List(temp, new_record);
	osal_mem_free(new_record);
	
	Show_MP_Menu(DIALING_MENU);
	SET_DIALING_MENU_ON();
       SET_ON_CALLING();

  }
  else if (ON_CONTACT_MENU() || ON_CONTACT_NUM_MENU())
  {
        Record new_record;
        C_node contact_temp;
        uint16 len = 0;
	
        len = sizeof(C_node);
        osal_nv_read(MINEAPP_NV_CONTACT, sel_item*len, len, &contact_temp);
        len = osal_strlen((char*)contact_temp.num);
        osal_memcpy(C_num, contact_temp.num, len);
        C_num[len] = '\0';

        Show_MP_Menu(DIALING_MENU);
        SET_DIALING_MENU_ON();
        SET_ON_CALLING();
        osal_memcpy(new_record.num, C_num, osal_strlen((char*)C_num)+1);
        GetTimeChar(new_record.time);
        Add_Record_To_List(DIALED_CALL, &new_record);
  } 
}
 
 /*********************************************************************
 * @fn      On_Ok
 *
 * @brief  Handle the Ok key event of menu.
 *
 * @param   none
 *
 * @return  none
 */
static void On_Ok(void)
{
  
  if (ON_MAIN_MENU())
  {
     high_line = 1;
     sel_item = RECENT_CALL;
     show_item = RECENT_CALL;
     Show_MP_Menu(FUNCTION_MENU);
     SET_FUNCTION_MENU_ON();
  }
  else if (ON_FUNCTION_MENU())
  {
    if (sel_item == RECENT_CALL)
    {
      high_line = MISSED_CALL;
      show_item = MISSED_CALL;
       sel_item = MISSED_CALL;
      Show_MP_Menu(RECENT_CALL_MENU);
      SET_RECENT_CALL_MENU_ON();
    }
    else if (sel_item == CONTACT)
    {
       high_line = 1;
       show_item = 0;
       sel_item = 0;
       Show_MP_Menu(CONTACT_MENU);
       SET_CONTACT_MENU_ON();
    }
    else if (sel_item == MESSAGE)
    {
    
       Show_MP_Menu(MESSAGE_MENU);
       SET_MESSAGE_MENU_ON();
    }
    else if (sel_item == CALENDAR)
    {
       Show_MP_Menu(CALENDAR_MENU);
       SET_CALENDAR_MENU_ON();
    }
  }
  else if(ON_CONTACT_MENU())
  {
        Show_MP_Menu(CONTACT_NUM_MENU);
	 SET_CONTACT_NUM_MENU_ON();
  }
  else if (ON_RECENT_CALL_MENU())
  {
        uint8 pos;
	
        if (sel_item == DIALED_CALL)
        {
	      osal_nv_read(MINEAPP_NV_DIALED, 0, 1, &pos);
	      show_item = pos ;
	      sel_item = pos;
	      if((pos>0) && (pos <= MAX_CALL_NUM))
	            high_line = 1;
	      else
	   	     high_line = 0;
	      Show_MP_Menu(DIALED_RECORD_MENU);
	      SET_DIALED_RECORD_MENU_ON();
       }
       else if (sel_item == MISSED_CALL)
       {
	      missed_call_amount = 0;
	      osal_nv_read(MINEAPP_NV_MISSED, 0, 1, &pos);
	      show_item = pos ;
	      sel_item = pos;
	      if((pos>0) && (pos <= MAX_CALL_NUM))
	           high_line = 1;
	      else
	   	    high_line = 0;
	      Show_MP_Menu(MISSED_RECORD_MENU);
	      SET_MISSED_RECORD_MENU_ON();
       }
       else if (sel_item == ANSWERED_CALL)
       {
	      osal_nv_read(MINEAPP_NV_ANSWERED, 0, 1, &pos);
	      show_item = pos ;
	      sel_item = pos;
	      if((pos>0) && (pos <= MAX_CALL_NUM))
	           high_line = 1;
	      else
	   	    high_line = 0;
	      Show_MP_Menu(ANSWERED_RECORD_MENU);
	      SET_ANSWERED_RECORD_MENU_ON();
        }
  }
  else if (ON_NUM_MENU())
  {
	  Show_MP_Menu(MAIN_MENU);
	  SET_MAIN_MENU_ON();
	  Clr_Num_Buf();
  }
  else if (ON_MISSED_MENU())
  {
         uint8 pos;
	
         shortcuts_flag = TRUE;
         missed_call_amount = 0;
         osal_nv_read(MINEAPP_NV_MISSED, 0, 1, &pos);
         show_item = pos ;
         sel_item = pos;
         if((pos>0) && (pos <= MAX_CALL_NUM))
	        high_line = 1;
         else
	        high_line = 0;
         Show_MP_Menu(MISSED_RECORD_MENU);
         SET_MISSED_RECORD_MENU_ON();
         Clr_Num_Buf();
  }
  else if (ON_DIALED_RECORD_MENU() )
  {
          Show_MP_Menu(DIALED_RECORD_TIME_MENU);
	   SET_DIALED_RECORD_TIME_MENU_ON();
  }
  else if (ON_MISSED_RECORD_MENU() )
  {
          Show_MP_Menu(MISSED_RECORD_TIME_MENU);
	   SET_MISSED_RECORD_TIME_MENU_ON();
  }
  else if (ON_ANSWERED_RECORD_MENU() )
  {
          Show_MP_Menu(ANSWERED_RECORD_TIME_MENU);
	   SET_ANSWERED_RECORD_TIME_MENU_ON();
  }
}
 /*********************************************************************
 * @fn      On_Back
 *
 * @brief  Handle the Back key event of menu.
 *
 * @param   none
 *
 * @return  none
 */
static void On_Back(void)
{

  if (ON_MAIN_MENU())
  {
    shortcuts_flag = TRUE;
    high_line = 1;
    show_item = 0;
    sel_item = 0;
    Show_MP_Menu(CONTACT_MENU);
    SET_CONTACT_MENU_ON();
  }
  else if (ON_CONTACT_MENU())
  {
    if (shortcuts_flag == TRUE)
    {
      high_line = 0;
	shortcuts_flag = FALSE;
       Show_MP_Menu(MAIN_MENU);
      SET_MAIN_MENU_ON();
    }
    else
    {
        show_item =  RECENT_CALL;
        high_line = CONTACT+1;
	 sel_item = CONTACT;
        Show_MP_Menu(FUNCTION_MENU);
        SET_FUNCTION_MENU_ON();
    }
  }
  else if(ON_CONTACT_NUM_MENU())
  {
       Show_MP_Menu(CONTACT_MENU);
	SET_CONTACT_MENU_ON();
  }
  else if (ON_RECENT_CALL_MENU())
  {
         show_item =  RECENT_CALL;
	  sel_item = RECENT_CALL;
	  high_line = RECENT_CALL+1;
        Show_MP_Menu(FUNCTION_MENU);
        SET_FUNCTION_MENU_ON();
  }
  else if (ON_MESSAGE_MENU())
  {
         show_item =  RECENT_CALL;
	  sel_item = MESSAGE;
	  high_line = MESSAGE+1;
        Show_MP_Menu(FUNCTION_MENU);
        SET_FUNCTION_MENU_ON();
  }
  else if (ON_DIALED_RECORD_MENU())
  {
    if (shortcuts_flag == FALSE)
    {
         show_item =  MISSED_CALL;
	  sel_item = DIALED_CALL;
	  high_line = DIALED_CALL;
         Show_MP_Menu(RECENT_CALL_MENU);
         SET_RECENT_CALL_MENU_ON();
    }
    else
    {
         Show_MP_Menu(MAIN_MENU);
         shortcuts_flag = FALSE;
         SET_MAIN_MENU_ON();
    }
  }
  else if (ON_MISSED_RECORD_MENU()) 
  {
    if (shortcuts_flag == FALSE)
    {
         show_item =  MISSED_CALL;
	  sel_item = MISSED_CALL;
	   high_line = MISSED_CALL;
        Show_MP_Menu(RECENT_CALL_MENU);
        SET_RECENT_CALL_MENU_ON();
    }
    else
    {
         shortcuts_flag = FALSE;
         Show_MP_Menu(MAIN_MENU);
         SET_MAIN_MENU_ON();
    }
  }
  else if (ON_ANSWERED_RECORD_MENU()) 
  {
         show_item =  MISSED_CALL;
	  sel_item = ANSWERED_CALL;
	   high_line = ANSWERED_CALL;
         Show_MP_Menu(RECENT_CALL_MENU);
         SET_RECENT_CALL_MENU_ON();
  }
  else if (ON_NUM_MENU())
  {
       C_num[--num_key]  = '\0';
	if(num_key == 0)
	{
		SET_MAIN_MENU_ON();
	       Show_MP_Menu(MAIN_MENU);
	}
	else
	{
             SET_NUM_MENU_ON();
             Show_MP_Menu(NUM_MENU);
	}
  }
  else  if (ON_FUNCTION_MENU())
  {
    high_line = 0;
    Show_MP_Menu(MAIN_MENU);
    SET_MAIN_MENU_ON();
  }
  else  if (ON_MISSED_MENU())
  {
    high_line = 0;
    Show_MP_Menu(MAIN_MENU);
    SET_MAIN_MENU_ON();
  }
  else  if (ON_DIALED_RECORD_TIME_MENU())
  {
    Show_MP_Menu(DIALED_RECORD_MENU);
    SET_DIALED_RECORD_MENU_ON();
  }
  else  if (ON_MISSED_RECORD_TIME_MENU())
  {
    Show_MP_Menu(MISSED_RECORD_MENU);
    SET_MISSED_RECORD_MENU_ON();
  }
  else  if (ON_ANSWERED_RECORD_TIME_MENU())
  {
    Show_MP_Menu(ANSWERED_RECORD_MENU);
    SET_ANSWERED_RECORD_MENU_ON();
  }
  else if(ON_CALENDAR_MENU())
  {
    Show_MP_Menu(FUNCTION_MENU);
    SET_FUNCTION_MENU_ON();
  }
}
 /*********************************************************************
 * @fn      On_Cancel
 *
 * @brief  Handle the Cancel key event of menu.
 *
 * @param   none
 *
 * @return  none
 */
static void On_Cancel(void)
{
  high_line = 0;
  Clr_Num_Buf();
  
  if (!ON_MAIN_MENU())
  {
    shortcuts_flag = FALSE;
    Show_MP_Menu(MAIN_MENU);
    SET_MAIN_MENU_ON();
    SET_ON_IDLE();
  }
}
 /*********************************************************************
 * @fn      On_Up
 *
 * @brief  Handle the Up key event of menu.
 *
 * @param   none
 *
 * @return  none
 */
static void On_Up(void)
{
 
  if ( ON_DIALED_RECORD_MENU()||
  	ON_MISSED_RECORD_MENU()||
  	ON_ANSWERED_RECORD_MENU())
  {
	uint8 pos;
		 
	if (ON_DIALED_RECORD_MENU())
		osal_nv_read(MINEAPP_NV_DIALED, 0, 1, &pos);
	else if(ON_MISSED_RECORD_MENU())
		osal_nv_read(MINEAPP_NV_MISSED, 0, 1, &pos);
	else if(ON_ANSWERED_RECORD_MENU())
		osal_nv_read(MINEAPP_NV_ANSWERED, 0, 1, &pos);

       if((pos > 0) && (pos <= MAX_LCD_LINE-1))
  	{		
	    if(high_line == 1)
		  high_line = pos;
	    else
	         --high_line;

	    if(sel_item == pos)
	   	  sel_item = 1;
	    else
	   	  ++sel_item;
           Show_MP_Menu(FRONT_ITEM_MENU);
  	}
	else if(pos > MAX_LCD_LINE-1)
  	{
	    if(high_line==1)
	    {
	           if(sel_item== pos)
	           {
	                 sel_item = 1;
		           high_line = 3;
			    show_item = sel_item + MAX_LCD_LINE-2;
	           }
		    else
		    {
				++sel_item;
				++show_item;
		    }
		    Show_MP_Menu(DIALED_RECORD_MENU);
	    }
	    else
	    {
		    ++sel_item;
		    --high_line;
                 Show_MP_Menu(FRONT_ITEM_MENU);
	    }
	}
  }
  else if (ON_FUNCTION_MENU())
  {
	    if(high_line==1)
	    {
	           if(sel_item== RECENT_CALL)
	           {
	                  sel_item = FUNCTION_END-1;
		           high_line = MAX_LCD_LINE - 1;
			    show_item = sel_item - (MAX_LCD_LINE-2);
	           }
		    else
		    {
				--sel_item;
				--show_item;
		    }
		    Show_MP_Menu(FUNCTION_MENU);
	    }else
	    {
		    --sel_item;
		    --high_line;
                 Show_MP_Menu(FRONT_ITEM_MENU);
	    }
  }
  else if (ON_RECENT_CALL_MENU())
  {
    if (high_line == MISSED_CALL)
    {
         high_line = DIALED_CALL;
         sel_item = DIALED_CALL;
    }
    else
    {
         high_line--;
         sel_item--;
    } 
    Show_MP_Menu(FRONT_ITEM_MENU);
  }
  else if (ON_CONTACT_MENU())
  {
      uint8 len;

	len = sizeof(contactor)/sizeof(C_node);
	if(len == 0)
		return;
      if (high_line == 1)
      {
          if(sel_item == 0)
	   {
               high_line = MAX_LCD_LINE - 1;
		  if(len < MAX_LCD_LINE - 1)
			show_item = len - 1;
		  else
		       show_item = len - 3;
	         sel_item = len-1;
               Show_MP_Menu(CONTACT_MENU);
         }
	  else
	  {
	         --sel_item;
		  --show_item;
               Show_MP_Menu(CONTACT_MENU);
	  }
    }
    else
    {
        --high_line;
        --sel_item;
        Show_MP_Menu(FRONT_ITEM_MENU);
    }
  }
  else if (ON_MESSAGE_MENU())
  {
       if( high_line == 1)
	   	high_line = 2;
	else
		high_line = 1;
	Show_MP_Menu(FRONT_ITEM_MENU);
  }
}
 /*********************************************************************
 * @fn      On_Down
 *
 * @brief  Handle the Down key event of menu.
 *
 * @param   none
 *
 * @return  none
 */
static void On_Down(void)
{
  if ( ON_DIALED_RECORD_MENU()||
  	ON_MISSED_RECORD_MENU()||
  	ON_ANSWERED_RECORD_MENU())
  {
       uint8 pos;
	   
	if (ON_DIALED_RECORD_MENU())
		osal_nv_read(MINEAPP_NV_DIALED, 0, 1, &pos);
	else if(ON_MISSED_RECORD_MENU())
		osal_nv_read(MINEAPP_NV_MISSED, 0, 1, &pos);
	else if(ON_ANSWERED_RECORD_MENU())
		osal_nv_read(MINEAPP_NV_ANSWERED, 0, 1, &pos);
	
	if((pos > 0) && (pos <= MAX_LCD_LINE-1))
  	{		
	    if(high_line == pos)
		  high_line = 1;
	    else
	        ++high_line;

	    if(sel_item == 1)
	   	 sel_item = pos;
	    else
	   	 --sel_item;
           Show_MP_Menu(NEXT_ITEM_MENU);
  	}
	else if(pos > MAX_LCD_LINE-1)
  	{
	    if(high_line==MAX_LCD_LINE-1)
	    {
	           if(sel_item== 1)
	           {
	                 sel_item = pos;
		           high_line = 1;
			    show_item = pos;
	           }
		    else
		    {
			    --sel_item;
			    show_item = sel_item + MAX_LCD_LINE-2;
		    }
		   Show_MP_Menu(DIALED_RECORD_MENU);
	    }
	    else
	    {
		   --sel_item;
		   ++high_line;
                Show_MP_Menu(NEXT_ITEM_MENU);
	    }
	}
  }
  else if (ON_FUNCTION_MENU())
  {
	    if(high_line==MAX_LCD_LINE-1)
	    {
	           if(sel_item== FUNCTION_END-1)
	           {
	                 sel_item = 0;
		           high_line = 1;
			    show_item = 0;
	           }
		    else
		    {
				++sel_item;
				++show_item;
		    }
		    Show_MP_Menu(FUNCTION_MENU);
	    }
	    else
	    {
		   ++sel_item;
		   ++high_line;
                Show_MP_Menu(NEXT_ITEM_MENU);
	    }
  }
  else if (ON_RECENT_CALL_MENU())
  {
    if (high_line >= DIALED_CALL)
    {
         high_line = MISSED_CALL;
         sel_item = MISSED_CALL;
    }
    else
    {
         ++high_line;
         ++sel_item;
    }
    Show_MP_Menu(NEXT_ITEM_MENU);
  }
  else if (ON_CONTACT_MENU())
  {
		 
    if (high_line == MAX_LCD_LINE - 1)
    {
         if(sel_item >= (sizeof(contactor)/sizeof(C_node)-1))
	  {
                high_line = 1;
               show_item = 0;
               sel_item = 0;
               Show_MP_Menu(CONTACT_MENU);
         }
         else
	  {
               ++sel_item;
		 ++show_item;
               Show_MP_Menu(CONTACT_MENU);
	  }
    }
    else
    {
         ++high_line;
         ++sel_item;
         Show_MP_Menu(NEXT_ITEM_MENU);
    }
  }
  else if (ON_MESSAGE_MENU())
  {
       if( high_line == 2)
	   	high_line =1;
	else
		high_line = 2;
	Show_MP_Menu(NEXT_ITEM_MENU);

  }
}
 /*********************************************************************
 * @fn      Handle_Key
 *
 * @brief  key process
 *
 * @param   none
 *
 * @return  none
 */
void Handle_Key(uint8 keys)
 {
     switch(keys)
    {
       case HAL_KEY_SELECT:
	        On_Ok();
		 break;
       case HAL_KEY_UP:
	        On_Up();
		 break;
       case HAL_KEY_DOWN:
	        On_Down();
		 break;
       case HAL_KEY_BACKSPACE:
	        On_Back();
		 break;
       case HAL_KEY_CALL:
	        On_Dial();
		 break;
       case HAL_KEY_CANCEL:
	        On_Cancel();
		 break;
       case HAL_KEY_STAR:
	   	 On_Star(Key2ASCII(keys));
		 break;
       case HAL_KEY_POUND:
	   	 On_Pound(Key2ASCII(keys));
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
              On_NumKey(Key2ASCII(keys));
	   	 break;
	default:
		break;
    }
 }

 /*********************************************************************
 * @fn      Init_NWK_Menu
 *
 * @brief  Initial the NWK initial menu.
 *
 * @param   none
 *
 * @return  none
 */
  static void Init_NWK_Menu(void)
 {
       Show_MP_Menu(NWK_INIT_MENU);
       SET_NWK_INIT_MENU_ON();
       SET_ON_IDLE();
 }
 
 /*********************************************************************
 * @fn      Init_MainMenu
 *
 * @brief  Initial the MainMenu
 *
 * @param   none
 *
 * @return  none
 */
static void Init_MainMenu(void)
{

    Show_MP_Menu(MAIN_MENU);
    SET_MAIN_MENU_ON();
    Clr_Num_Buf();
    SET_ON_IDLE();
}

 /*********************************************************************
 * @fn      Show_IncomingCall
 *
 * @brief  Show the IncomingCall Menu
 *
 * @param   none
 *
 * @return  none
 */
static void Show_IncomingCall(uint8* tel_num)
{
     uint8 len = 0;

     len = (uint8)osal_strlen((char*)tel_num);
     osal_memcpy(C_num, tel_num, len);
     C_num[len] = '\0';
     Show_MP_Menu(INCOMING_MENU);
     SET_INCOMING_MENU_ON();
     SET_ON_CALLED();
}
 /*********************************************************************
 * @fn      Show_IncomingMsg
 *
 * @brief  Show the IncomingMessage Menu
 *
 * @param   none
 *
 * @return  none
 */
static void Show_IncomingMsg(void)
{
      Show_MP_Menu(UNREAD_MESSAGE_MENU);
	SET_UNREAD_MESSAGE_MENU_ON();
}
  /*********************************************************************
 * @fn      Show_NoPowerMsg
 *
 * @brief  Show the No Power Menu
 *
 * @param   none
 *
 * @return  none
 */

static void Show_NoPowerMsg(void)
{
      Show_MP_Menu(NO_POWER_MENU);
}
 /*********************************************************************
 * @fn      Show_MissedCall
 *
 * @brief  Show the MissedCall Menu
 *
 * @param   none
 *
 * @return  none
 */
static void Show_MissedCall(void)
{


     uint8 len = 0;
     Record new_record;

      missed_call_amount++;
      Show_MP_Menu(MISSED_MENU);
      SET_MISSED_MENU_ON();
      SET_ON_IDLE();
	  
     len = (uint8)osal_strlen((char*)C_num);
     osal_memcpy(new_record.num, C_num, len+1);
     GetTimeChar(new_record.time);
     Add_Record_To_List(MISSED_CALL, &new_record);

}
 

 /*********************************************************************
 * @fn      Display_Msg_Menu
 *
 * @brief  Show the Msg Menu
 *
 * @param 
 *
 * @return  none
 */
 void Display_Msg_Menu(uint8 msg, uint8*tel_num)
 {
       switch(msg)
 	{
 	case INIT_NWK_MSG:
		Init_NWK_Menu();
		break;
	case INIT_MAIN_MSG:
		Init_MainMenu();
		break;
	case MISSED_CALL_MSG:
		Show_MissedCall();
		break;
	case INCOMING_CALL_MSG:
		Show_IncomingCall(tel_num);
		break;
	case DIALING_SUCCESS_MSG:
		Show_MP_Menu(TALKING_MENU);
		break;
	case INCOMING_MESSAGE_MSG:
		Show_IncomingMsg();
		break;
        case NO_POWER_MSG:
            Show_NoPowerMsg();
            break;
	default:
		break;
 	}

 }
 
 /*********************************************************************
 * @fn      Update_Signal_Battery
 *
 * @brief  update the signal and battery
 *
 * @param  
 *
 * @return  none
 */
void Update_Signal_Battery(uint8 index, uint8 value)
{
      if(index == SIGNAL_STRENTH)
      {
	             sig_index = value;
      }
      else if(index == BATTERY_STRENTH)
      {
                   bat_index = value;
      }
      if(ON_MAIN_MENU() ||ON_TALKING_MENU() || ON_DIALING_MENU())
      	{
            LCD_Sig_Bat_Clear(index);

            if(index == SIGNAL_STRENTH)
            {
                    LCD_Signal_Show(sig_index);
            }
            else if(index == BATTERY_STRENTH)
            {
		      LCD_Battery_Show(bat_index);
            }
      	}
}
  /*********************************************************************
 * @fn      Update_Time
 *
 * @brief  update time
 *
 * @param  
 *
 * @return  none
 */
void Update_Time(void)
{
      uint8* p = NULL;

      if(ON_MAIN_MENU())
      	{	
	     if(NULL == (p = osal_mem_alloc(TIME_LEN)))
		    return;
            LCD_Line_Clear(1);
	     GetTimeChar(p);
	     LCD_Str_Print(p, 5, 5, 1, TRUE);
	     osal_mem_free(p);
      	}
}
  /*********************************************************************
 * @fn      Get_Num_From_Menu
 *
 * @brief  get the num which will be used by RF
 *
 * @param   num_buf
 *
 * @return  none
 */
 void Get_Num_From_Menu(uint8* num_buf)
{
      osal_memcpy(num_buf, C_num, osal_strlen((char*)C_num)+1);
}
