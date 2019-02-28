#include "GasMenuLib_global.h"
#include "GasMenuLib_orphan.h"
#include "GasMenuLib_tree.h"
#include "App_cfg.h"
#include "osal.h"
#include "GasMonitor_MenuLibChinese.h"
#include "string.h"
#include "TimeUtil.h"
//#include "MenuAdjustUtil.h"
#include "drivers.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchdogUtil.h"
#endif
/*----------------- variable-----------------*/
/*global */
uint8             CurrentNodeID = MENU_ID_ROOT;
uint8             NearLastNodeID = MENU_ID_MAIN;
uint8             g_jump_buf[MAX_DATA_BUF];
stack_t          global_stack;
node_info_t    node_info = {1,0,0};

/*static */
static node_info_t     node_info_jumpbackup;
static uint8               NodeID_jumpbackup;

/*-----------------Functions -----------------*/

void* Buffer_Init(buf_t* const buf, uint8 buf_len)
{
	buf->len = 0;

	if(buf->p == NULL)
		buf->p = (uint8*)osal_mem_alloc(buf_len);

	return buf->p;
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
void Buffer_Copy(buf_t*  dstbuf, const buf_t * srcbuf)
{
	if(srcbuf->len>0)
	{
		dstbuf->len = srcbuf->len;
		osal_memcpy(dstbuf->p, srcbuf->p, srcbuf->len+1);
		//strcpy((char *)dstbuf->p, (const char *)srcbuf->p);
	}
}
void Buffer_Clear(buf_t*  buf)
{
	buf->len = 0;
	if(buf->p != NULL)
	{
		*buf->p =  '\0';
	}
}

/*
static void Stack_Init(stack_t* const stack, stack_p_t* stack_p, uint8 stack_depth)
{
stack->stack_depth = stack_depth;
stack->stack_p       = stack_p;
stack->stack_i        = 0;
}*/

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

bool  Stack_Pop(stack_t* const stack, uint8 *NodeID, node_info_t* node_info)
{
	if(stack->stack_i > 0)
	{
		stack->stack_i--;
		if(NodeID != NULL)
			*NodeID = stack->stack_p[stack->stack_i].id;
		if(node_info != NULL)
			*node_info = stack->stack_p[stack->stack_i].node_info;
		return true;
	}
	else
	{
		return false;
	}
}
void Stack_Clear(stack_t* const stack)
{
	stack->stack_i = 0;
}

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
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
	FeedWatchDog();
#endif
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

	if(ID == MENU_ID_MAIN)
	{
		Stack_Clear(&global_stack);
		menu_tree_stack_clear();
	}
	//if(ID != CurrentNodeID)
	//{
	NearLastNodeID = CurrentNodeID;
	CurrentNodeID = ID;
	MENU_RESET_NODEINFO();
	menu_display();
	//}
}
void  menu_Mark(void)
{
		NodeID_jumpbackup = CurrentNodeID;
		node_info_jumpbackup = node_info;
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

void  menu_JumpandPush(uint8 ID)
{

	if(ID != CurrentNodeID)
	{
		Stack_Push(&global_stack, CurrentNodeID, &node_info);
		menu_JumptoMenu(ID);
	}      
}

void  menu_JumpandPop(void)
{
	NearLastNodeID = CurrentNodeID;
	Stack_Pop(&global_stack, &CurrentNodeID, &node_info);
	menu_display();
}

void menu_JumptoItself(void)
{  
    menu_display();
}

 char *  menu_itoa ( uint16 value, char * buffer)
{
	uint16 oct = 1;
	uint16 tmp;
        tmp = value;
	while(tmp = tmp /10) ++oct;
        tmp = value;
	uint16 i = 0;
	for(; i<oct; ++i)
	{
		buffer[oct-i-1] = tmp%10 +'0';
		tmp /= 10;
	}
	buffer[oct] = '\0';
	return(buffer);
}