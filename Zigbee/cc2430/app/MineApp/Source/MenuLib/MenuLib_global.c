#include "Hal_key_cfg.h"
#include "MenuLib_global.h"
#include "MenuLib_orphan.h"
#include "MenuChineseInputUtil.h"
#include "MenuLib_tree.h"
#include "osal_NV.h"
#include "App_cfg.h"
#include "osal.h"
#include "MineApp_global.h"
#include "MineApp_MenuLibChinese.h"
#include "string.h"
#include "TimeUtil.h"
#include "hal_audio.h"
#include "lcd_serial.h"
#include "MenuAdjustUtil.h"
#include "Hal_drivers.h"
#include "MineApp_MP_Function.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchdogUtil.h"
#endif
/*----------------- variable-----------------*/
/*global */
bool              shortcuts_flag = FALSE;
uint8             CurrentNodeID = MENU_ID_ROOT;
uint8             NearLastNodeID = MENU_ID_MAIN;
uint8             missed_call_amount = 0;
uint8             g_jump_buf[MAX_DATA_BUF];
uint8             unread_sms = 0;
buf_t             num_buf = {0, NULL};
buf_t             incoming_buf = {0, NULL};
buf_t             data_buf = {0, NULL};
//bool              new_sms_flag = FALSE;
stack_t          global_stack;
node_info_t    node_info = {1,0,0};
/*static */
static stack_p_t        pipeline[PIPELINE_DEPTH];
static node_info_t     node_info_jumpbackup;
static uint8               NodeID_jumpbackup;

static bool MP_SettingInformation_IsValid(const set_info_t* p);

/*-----------------Functions -----------------*/

#ifdef NEW_DOUBLE_NVID_OP
void menu_doubleNVID_read(uint8 type, uint16 offset, uint8 len, void*buf)
{
	switch(type)
	{
	case SMS_NVID:
		{
			if(offset < SMS_NV_LEN* (MAX_SMS_NUM/2) + sizeof(uint8))
			{
				osal_nv_read(MINEAPP_NV_SMS1, offset, len, buf);
			}
			else if(offset <= SMS_NV_LEN * (MAX_SMS_NUM) + sizeof(uint8))
			{
				osal_nv_read(MINEAPP_NV_SMS2, offset-SMS_NV_LEN * (MAX_SMS_NUM/2) - sizeof(uint8), len, buf);
			}
			break;
		}
	default:
		break;
	}
}
uint8 menu_doubleNVID_write(uint8 type, uint16 offset, uint8 len, void*buf)
{
	uint8 rtn = NV_OPER_FAILED;

	switch(type)
	{
	case SMS_NVID:
		{
			if(offset < SMS_NV_LEN * (MAX_SMS_NUM/2) + sizeof(uint8))
			{
				rtn = osal_nv_write(MINEAPP_NV_SMS1, offset, len, buf);
			}
			else if(offset <= SMS_NV_LEN * (MAX_SMS_NUM) + sizeof(uint8))
			{
				rtn = osal_nv_write(MINEAPP_NV_SMS2, offset-SMS_NV_LEN* (MAX_SMS_NUM/2) - sizeof(uint8), len, buf);
			}
			break;
		}
	default:
		break;
	}

	return rtn;
}

void menu_doubleNVID_delete(uint8 type, uint8 index)
{
	uint8 count, tmp_len, max_num;
	uint16 firstNVID;
	void* buf;

	if(type == SMS_NVID)
	{
		firstNVID = MINEAPP_NV_SMS1;
		tmp_len = SMS_NV_LEN;
		max_num = MAX_SMS_NUM;
		buf = (uint8*)osal_mem_alloc(SMS_NV_LEN);
	}
	if (buf == NULL)
		return;
	osal_nv_read(firstNVID, 0, sizeof(uint8), &count);
	// index = 0 ~ count-1
	if(count <= max_num/2)
	{
		for(uint8 i=index; i<count; i++)
		{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
			FeedWatchDog();
#endif
			osal_nv_read(firstNVID, (i+1)*tmp_len+sizeof(uint8), tmp_len, buf);
			osal_nv_write(firstNVID, i*tmp_len+sizeof(uint8), tmp_len, buf);
		}
	}
	else if(count <= max_num)
	{
		if(index < max_num/2)
		{
			for(uint8 i=index; i<max_num/2; i++)
			{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
				FeedWatchDog();
#endif
				osal_nv_read(firstNVID, (i+1)*tmp_len+sizeof(uint8), tmp_len, buf);
				osal_nv_write(firstNVID, i*tmp_len+sizeof(uint8), tmp_len, buf);
			}
			osal_nv_read(firstNVID+1, 0, tmp_len, buf);
			osal_nv_write(firstNVID, (max_num/2-1)*tmp_len+sizeof(uint8), tmp_len, buf);
			for(uint8 i=1; i<count-(max_num/2); i++)
			{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
				FeedWatchDog();
#endif
				osal_nv_read(firstNVID+1, i*tmp_len+sizeof(uint8), tmp_len, buf);
				osal_nv_write(firstNVID+1, (i-1)*tmp_len+sizeof(uint8), tmp_len, buf);
			}
		}
		else if(index < max_num)
		{
			for(uint8 i= index-(max_num/2); i<count-(1+max_num/2); i++)
			{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
				FeedWatchDog();
#endif
				osal_nv_read(firstNVID+1, (i+1)*tmp_len+sizeof(uint8), tmp_len, buf);
				osal_nv_write(firstNVID+1, i*tmp_len+sizeof(uint8), tmp_len, buf);
			}
		}
	}
	count--;
	osal_nv_write(firstNVID, 0, sizeof(uint8), &count);

	if(buf != NULL)
	{
		osal_mem_free(buf);
		buf = NULL;
	}
}


#endif


void MP_SettingInformation_GetDefault(set_info_t* p)
{
	p->bell_gain = RING_GAIN_LEVEL_4;
	p->sound_gain = DEFAULT_OUTGAIN_LEVEL;
	p->bell_ring_t = RING_BELL_1-RING_BELL_1;
	p->sms_ring_t = RING_BELL_4-RING_BELL_1;
	p->padlock_ctl = TRUE;
	p->shake_ctl   = TRUE;
	p->timeformat_t = TIME_FORMAT_24;
	p->backlight_ctl  = BACKLIGHT_CTL_10S;
#ifdef 	MENU_TIMEUPDATE_CTL				   
	p->time_autoupdate = TRUE;				
#endif		
}

void MP_SettingInformation_Collect(set_info_t* set_info)
{
	HalRingGetGain(RING_TYPE_BELL, &set_info->bell_gain);
	set_info->sound_gain = AudioGetOutputGain();
	set_info->bell_ring_t = HalRingGetBellName() - RING_BELL_1;
	set_info->sms_ring_t = HalRingGetSMSBell() - RING_BELL_1;
	set_info->padlock_ctl = HalGetPadLockEnable();
	set_info->backlight_ctl = LCDGetBackLightCtl();
	set_info->shake_ctl = HalRingGetShakeCtl();

#ifdef MENU_CLOCKFORMAT
	set_info->timeformat_t = GetTimeFormat();
#else
	set_info->timeformat_t = TIME_FORMAT_24;
#endif

#ifdef 	MENU_TIMEUPDATE_CTL
	set_info->GetTimeAutoUpdate();
#endif

}

void MP_SettingInformation_Handout(const set_info_t* set_info)
{
	HalRingSetGain(RING_TYPE_BELL, set_info->bell_gain);
	AudioSetOutputGain(set_info->sound_gain);
	HalRingSetGain(RING_TYPE_SOUND, set_info->sound_gain);
	HalRingSetBellName((RingName)(set_info->bell_ring_t + RING_BELL_1));
	HalRingSetSMSBell((RingName)(set_info->sms_ring_t + RING_BELL_1));
	HalSetPadLockEnable(set_info->padlock_ctl);
	LCDSetBackLightCtl(set_info->backlight_ctl);
#ifdef MENU_CLOCKFORMAT
	SetTimeFormat(set_info->timeformat_t);
#endif
	HalRingSetShakeCtl(set_info->shake_ctl);
#ifdef 	MENU_TIMEUPDATE_CTL
	SetTimeAutoUpdate(set_info->time_autoupdate);
#endif
}
uint8 MP_SettingInformation_ReadFlash(set_info_t* p)
{
	uint8 flag = osal_nv_read(MINEAPP_NV_SET_INFORMATION, 0, sizeof(set_info_t),p);
	if(flag != ZSuccess)
	{
		return flag;
	}

	if(!MP_SettingInformation_IsValid(p))
	{
		MP_SettingInformation_GetDefault(p);
		MP_SettingInformation_WriteFlash(p);
		return ZInvalidParameter;
	}
	return ZSuccess;
}
uint8 MP_SettingInformation_WriteFlash(const set_info_t* p)
{
	return osal_nv_write(MINEAPP_NV_SET_INFORMATION, 0, sizeof(set_info_t),(void*)p);
}

bool MP_SettingInformation_IsValid(const set_info_t* p)
{
	if( p->bell_gain < MAX_GAIN_LEVEL
		&& p->sound_gain < MAX_GAIN_LEVEL
		&& p->bell_ring_t <= (uint8)(RING_BELL_4- RING_BELL_1)
		&& p->sms_ring_t <= (uint8)(RING_BELL_4- RING_BELL_1)
		&& p->backlight_ctl <= BACKLIGHT_CTL_30S
#ifdef MENU_CLOCKFORMAT
		&& p->timeformat_t <= TIME_FORMAT_12
#endif
		)
	{
		return true;
	}
	else
	{
		return false;
	}
}

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
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
				FeedWatchDog();
#endif
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

		for(i=1; i<MAX_CALL_NUM; i++)
		{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
			FeedWatchDog();
#endif
			osal_nv_read(nv_id, i*len+1, len, &record);
			osal_nv_write(nv_id, (i-1)*len+1, len, &record);
		}
		osal_nv_write(nv_id, (MAX_CALL_NUM-1)*len+1, len, new_record);
		pos = MAX_CALL_NUM;
	}

	osal_nv_write(nv_id, 0, 1, &pos);
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

/*
void Clr_Num_Buf(void)
{
osal_memset(num_buf.p, '\0', 1);
num_buf.len = 0;
}
*/
void  menu_Dial(const buf_t numbuff)
{
	Record new_record;
	uint8 nwk_stat = Menu_GetNwkStat();
	if(nwk_stat == NWK_STAT_NORMAL)
	{
		SET_ON_CALLING();
		menu_JumptoMenu(MENU_ID_DIALING);
		GetTimeChar(new_record.time);
		osal_memcpy(new_record.num, numbuff.p, numbuff.len+1);
		Add_CallRecord(MENU_ID_CALLRECORD_DIALEDCALL, &new_record);
	}
	else if(nwk_stat == NWK_STAT_SWITCH)
	{
		strcpy((char*)g_jump_buf, SWITCHING_CHINA);
		menu_JumpandMark(MENU_ID_SHOWMESSAGE);
		MineApp_StartMenuLibEvt(3800);
		HalRingOpen(RING_BUSY,OPENFLAG_NONE);
		osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 15);
	}
	else
	{
		strcpy((char*)g_jump_buf, NONWK_CHINA);
		menu_JumpandMark(MENU_ID_SHOWMESSAGE);
		MineApp_StartMenuLibEvt(3800);
		HalRingOpen(RING_BUSY,OPENFLAG_NONE);
		osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 15);

	}
}


