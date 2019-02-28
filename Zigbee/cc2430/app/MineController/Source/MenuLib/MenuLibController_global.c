#include "Hal_key_cfg.h"
#include "MenuLibController_global.h"
#include "MenuLibController_orphan.h"
//#include "MenuChineseInputUtil.h"
#include "MenuLibController_tree.h"
#include "osal_NV.h"
#include "App_cfg.h"
#include "osal.h"
//#include "MineController_global.h"
#include "MineController_MenuLibChinese.h"
#include "string.h"
#include "TimeUtil.h"

/*----------------- variable-----------------*/
/*global */
bool              shortcuts_flag = FALSE;
uint8             CurrentNodeID = MENU_ID_ROOT;
uint8             NearLastNodeID = MENU_ID_MAIN;
uint8             missed_call_amount = 0;
uint8             g_jump_buf[MAX_DATA_BUF];
buf_t             num_buf = {0, NULL};
buf_t             data_buf = {0, NULL};
bool              new_sms_flag = FALSE;
stack_t          global_stack;
node_info_t    node_info = {1,0,0};
app_Sleep_t   med_sleepcfg = {NODESLEEP, SLEEPTIME, 5000};

/*static */
static stack_p_t        pipeline[PIPELINE_DEPTH];
static node_info_t     node_info_jumpbackup;
static uint8               NodeID_jumpbackup;
/*-----------------Functions -----------------*/


void* Buffer_Init(buf_t* const buf, uint8 buf_len)
{
       buf->len = 0;
	return buf->p = (uint8*)osal_mem_alloc(buf_len);
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
void Stack_Pop(stack_t* const stack, uint8 *NodeID, node_info_t* node_info)
{
         if(stack->stack_i > 0)
         {
		  stack->stack_i--;
                if(NodeID != NULL)
                       *NodeID = stack->stack_p[stack->stack_i].id;
                if(node_info != NULL)
		          *node_info = stack->stack_p[stack->stack_i].node_info;
         }
}
void Stack_Clear(stack_t* const stack)
{
        stack->stack_i = 0;
}
#if 0
void Menu_Stack_Init(void)
{
      Stack_Init(&global_stack, pipeline, PIPELINE_DEPTH);
      menu_tree_stack_init();
}
/*********************************************************************
* @fn      Add_CallRecord
*
* @brief   Move the record from tail to head and add the new_reocrd to tail of list.
*
* @param   index   --the index of record list that will be operated
*               new_record--the reocrd that will be added  
*
* @return  none
*/
void Add_CallRecord(uint8 index, Record* new_record)
{
	uint8 i, j, len, pos;
	uint16 nv_id;
	Record record;


	if(index == MENU_ID_CALLRECORD_MISSEDCALL)
		nv_id = MINEAPP_NV_MISSED;
	else if(index == MENU_ID_CALLRECORD_ANSWEREDCALL)
		nv_id = MINEAPP_NV_ANSWERED;
	else if(index == MENU_ID_CALLRECORD_DIALEDCALL)
		nv_id = MINEAPP_NV_DIALED;
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
		osal_nv_read(nv_id, pos*len+1, len, &record);
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
#endif 
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

void Clr_Num_Buf(void)
{
/*	uint8 len = osal_strlen((char*)C_num);
	osal_memset(C_num, '\0', len);
	num_id = 0;
	*/
	//osal_memset(num_buf.p, '\0', num_buf.len);
	*num_buf.p = 0;
	num_buf.len = 0;
}
#if 0
void  menu_Dial()
{
	Record new_record;
	if(!Menu_IsNwkOn())//nwk_stat == FALSE)
	{
		strcpy((char*)g_jump_buf, NONWK_CHINA);
		menu_JumpandMark(MENU_ID_SHOWMESSAGE);
	}
	else
	{
		SET_ON_CALLING();
		menu_JumptoMenu(MENU_ID_DIALING);
		GetTimeChar(new_record.time);
		//num_buf.len = osal_strlen((char*)num_buf.p);
		//num_buf.p[num_buf.len] = '\0';
		osal_memcpy(new_record.num, num_buf.p, num_buf.len+1);
		Add_CallRecord(MENU_ID_CALLRECORD_DIALEDCALL, &new_record);
	}
}

#endif
