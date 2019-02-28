#include "MenuLib_Nv.h"
#include "App_cfg.h"
#include "Osal_Nv.h"
#include "string.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchDogUtil.h"
#endif


/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

static uint8 Contact_GetContactNum(uint8* pContactNum);
static uint8 Contact_SetContactNum(uint8 ContactNum);
static uint8 Contact_GetContact(Contact_Node *pContactNode, uint8 idx);
static uint8 Contact_SetContact(const Contact_Node *pContactNode, uint8 idx);

/****************************************************************************/
/***       Functions                                                 ***/
/****************************************************************************/
uint8 Contact_GetContactNum(uint8* pContactNum)
{
	if(pContactNum==NULL)
	{
		return ZInvalidParameter;
	}
	return osal_nv_read(MINEAPP_NV_CONTACT1, 0, sizeof(uint8), pContactNum);
}
uint8 Contact_SetContactNum(uint8 ContactNum)
{
	if(ContactNum>MAX_CONTACT_NUM)
	{
		return ZInvalidParameter;
	}
	return osal_nv_write(MINEAPP_NV_CONTACT1, 0, sizeof(uint8), &ContactNum);
}

uint8 Contact_GetContact(Contact_Node *pContactNode, uint8 idx)
{
	uint8 flag;
	if(pContactNode==NULL||idx>=MAX_CONTACT_NUM)
	{
		return ZInvalidParameter;
	}
	
	if(idx < MAX_CONTACT_NUM/2)
	{
		flag = osal_nv_read(MINEAPP_NV_CONTACT1, sizeof(uint8)+sizeof(Contact_Node)*idx, sizeof(Contact_Node), pContactNode);
	}
	else if(idx < MAX_CONTACT_NUM)
	{
		flag = osal_nv_read(MINEAPP_NV_CONTACT2, sizeof(Contact_Node)*(idx-MAX_CONTACT_NUM/2), sizeof(Contact_Node), pContactNode);
	}
	return flag;
}

uint8 Contact_SetContact(const Contact_Node *pContactNode, uint8 idx)
{
	uint8 flag;
	
	if(pContactNode==NULL||idx>=MAX_CONTACT_NUM)
	{
		return ZInvalidParameter;
	}
	
	if(idx < MAX_CONTACT_NUM/2)
	{
		flag = osal_nv_write(MINEAPP_NV_CONTACT1,  sizeof(uint8)+sizeof(Contact_Node)*idx, sizeof(Contact_Node), (uint8*)pContactNode);
	}
	else if(idx < MAX_CONTACT_NUM)
	{
		flag = osal_nv_write(MINEAPP_NV_CONTACT2, sizeof(Contact_Node)*(idx-MAX_CONTACT_NUM/2), sizeof(Contact_Node), (uint8*)pContactNode);
	}
	return flag;
}

uint8 menu_Contact_ReadContactNum(uint8 * pContactNum)
{
	if(pContactNum==NULL)
	{
		return ZInvalidParameter;
	}
	return Contact_GetContactNum(pContactNum);
}
uint8 menu_Contact_ReadContact(Contact_Node *pContactNode, uint8 idx)
{
	uint8 flag;
	uint8 ContactNum;
	if(pContactNode==NULL || idx>= MAX_CONTACT_NUM)
	{
		return ZInvalidParameter;
	}
	
	flag = Contact_GetContactNum(&ContactNum);

	if(idx < ContactNum)
	{
		flag = Contact_GetContact(pContactNode, idx);
	}
	else
	{
		return ZFailure;
	}
	return flag;
}

uint8 menu_Contact_AddContact(const Contact_Node *pContactNode)
{
	uint8 flag;
	uint8 ContactNum;

	if(pContactNode==NULL)
	{
		return ZInvalidParameter;
	}
	
	flag = Contact_GetContactNum(&ContactNum);

	if(ContactNum < MAX_CONTACT_NUM)
	{
		flag = Contact_SetContact(pContactNode, ContactNum);
		if(flag == ZSuccess)
		{
			flag = Contact_SetContactNum(ContactNum+1);
		}

	}
	else
	{
		return ZFailure;
	}
	return flag;
}

uint8 menu_Contact_DeleteContact(uint8 idx)
{
	uint8 flag;
	uint8 ContactNum;

	if(idx >= MAX_CONTACT_NUM)
	{
		return ZInvalidParameter;
	}
	
	flag = Contact_GetContactNum(&ContactNum);

	if(idx < ContactNum && idx <MAX_CONTACT_NUM)
	{
		Contact_Node Node;
		for(uint8 j=idx;j<ContactNum-1;j++)
		{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
			FEEDWATCHDOG();
#endif
			Contact_GetContact(&Node, j+1);
			Contact_SetContact(&Node, j);
		}
		flag = Contact_SetContactNum(--ContactNum);
	}
	else
	{
		return ZFailure;
	}
	return flag;
}
uint8 menu_Contact_SearchContactByNum(Contact_Node *pContactNode, uint8* pidx, const uint8* pNum)
{
	uint8 flag;
	uint8 ContactNum;
	Contact_Node c_node;

	
	if(pNum==NULL)
	{
		return ZFailure;
	}
	
	flag = Contact_GetContactNum(&ContactNum);
	
	if(ContactNum==0 || ContactNum > MAX_CONTACT_NUM)
	{
		return ZFailure;
	}
	else
	{
		for(uint8 i=0;i<ContactNum;i++)
		{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
			FEEDWATCHDOG();
#endif
			Contact_GetContact(&c_node,i);
			if(strcmp((char *)c_node.num, (char *)pNum) == 0)
			{
				if(pContactNode!=NULL)
				{
					*pContactNode = c_node;
				}
				if(pidx !=NULL)
				{
					*pidx = i;
				}
				return ZSuccess;
			}
		}	
	}	
	return ZFailure;
}

