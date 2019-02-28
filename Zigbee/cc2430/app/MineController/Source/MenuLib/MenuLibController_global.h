#ifndef MENULIBCONTROLLER_GLOBAL_H
#define MENULIBCONTROLLER_GLOBAL_H
#include "hal_types.h"
#include "Minecontroller_MenuLib.h"
#include "AppProtocol.h"
#include "AppProtocolWrapper.h"

#define SCREEN_LINES        3    

#define TIME_LEN                 8
#define DATE_LEN                11
#define MAX_DATA_BUF      32
#define MAX_NAME_LEN               8
#define SCREEN_LINES         3
#define PIPELINE_DEPTH         4
#define STACK_DEPTH         4
#define TOP_POS_TIME        5 
#define SMS_LEN_TYPE_SIZE 1
#define SMS_NV_LEN                    (SMS_MAX_LEN+NMBRDIGIT+SMS_LEN_TYPE_SIZE)

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
}MenuOper_t;

typedef struct
{
     uint8 len;
     uint8* p;
}buf_t;

typedef struct
{
	uint8   high_line;          
	uint8   sel_item;
	uint8   show_item;
}node_info_t;

typedef struct
{
     uint8 id;
     node_info_t node_info;
}pipeline_t;

typedef struct
{
     uint8 id;
     node_info_t node_info;
}stack_p_t;

typedef struct
{
     uint8           stack_i;
     uint8           stack_depth;
     stack_p_t*   stack_p;
}stack_t;

typedef struct
{
	uint8              num[NMBRDIGIT];
	uint8              time[TIME_LEN];
}Record;


typedef struct
{
	uint8              num[NMBRDIGIT];
	uint8              name[MAX_NAME_LEN];
}Contact_Node;

enum MenuID
{
	MENU_ID_ROOT = 0,

	/*independent ID in the root, parrelled with main menu*/

      MENU_ID_POWERON_ANIMATION,
      MENU_ID_POWEROFF_ANIMATION,
      MENU_ID_INITNWK,

    	MENU_ID_INPUTNAME,
	MENU_ID_INPUTNUMBER,

    	MENU_ID_SHOWMESSAGE,
	MENU_ID_SHOWQUESTION,
	MENU_ID_SHOWALERT,
	
	MENU_ID_SHOWSETTING,
	
      MENU_ID_MAIN,
    
	/*--------separetor  of orphan nodes and tree nodes------------------*/
	MENU_ID_SEPARATOR,

	/*the tree menu begins from functionlist*/
       MENU_ID_FUNCTIONLIST,


       MENU_ID_CARD_DETECT,
       MENU_ID_STATION_DETECT,
       //MENU_ID_PHONE_DETECT,


       MENU_ID_CARD_SETTING,
       MENU_ID_STATION_SETTING,
#ifdef MENU_RF_DEBUG
       MENU_ID_CHANNEL_SELECT,
#endif
       MENU_ID_LABEL_CALBRATE,
       
       /*CARD setting item*/
       MENU_ID_CARD_SLEEPTIME,
       MENU_ID_CARD_POLLINTERVAL,
       MENU_ID_CARD_POLLTIMEOUT,
};

/*-----------------global variable declares-----------------*/
extern  bool              shortcuts_flag;
extern  uint8             CurrentNodeID;   
extern  uint8             NearLastNodeID;
//extern  uint8             C_num [NMBRDIGIT];
//extern  uint8             num_id;
extern  uint8             missed_call_amount;

extern node_info_t     node_info;
extern buf_t              data_buf;
extern buf_t              num_buf;
extern bool               new_sms_flag;
extern bool               shortcuts_flag;
extern stack_t           global_stack;
extern app_Sleep_t    med_sleepcfg;
/* global buffer to send variables or strings when state jump, no detail defination, you can define its usage by spectial use*/
extern uint8             g_jump_buf[MAX_DATA_BUF]; 

/*-----------------Function declares-----------------*/
uint8 GetTypeFromID(uint8 ID);
uint8 GetIDFromIdx(uint8 node_type, uint8 idx);
uint8 GetIdxFromID(uint8 id);
void* Buffer_Init(buf_t* const, uint8);
void Buffer_Free(buf_t* const);

void Stack_Push(stack_t* const, uint8, const node_info_t*);
void Stack_Pop(stack_t* const, uint8*, node_info_t*);
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

void Menu_Stack_Init(void);
//void  Add_CallRecord(uint8, Record*);
void Clr_Num_Buf(void);
//void  menu_Dial(void);

#endif
