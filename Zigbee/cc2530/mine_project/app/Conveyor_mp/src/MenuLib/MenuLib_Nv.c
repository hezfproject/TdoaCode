#include "MenuLib_Nv.h"
#include "App_cfg.h"
#include "Osal_Nv.h"
#include "string.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchDogUtil.h"
#endif
#include "ZComdef.h"

#if 1
/****************************************************************************/
/***                                              Functions                                              ***/
/**************************about  Contact   ************************************/
static uint8 Contact_GetContact(Contact_Node *pContactNode, uint16 item_contact);
static uint8 Contact_SetContact( Contact_Node *pContactNode, uint16 item_contact);

static uint8 Contact_List_CutUnvalued(uint16 item_contact);
static uint8 Contact_List_toFind_Idx(Contact_Node *pContactNode,uint16 *head_item, uint8 idx);
static uint8 Contact_List_Search_First_Unvalued(uint16 *real_item);
static uint8 Contact_List_Search_First_Valid(Contact_Node *pContactNode,uint16 *head_item);
static uint8 Contact_List_Search_End_Valid(Contact_Node *pContactNode,uint16 *head_item,uint8 *pidx);
static void reset_contact_item_L(void);
static uint8 int_contact_item_L_Next(uint16 real_item);


uint8 int_contact_item_L(void);
uint16 Get_contact_item(uint8 idx);
uint8 menu_Contact_nv_init(void);
uint8 menu_Contact_ReadContactNum(uint8 * pContactNum);
uint8 menu_Contact_ReadContact(Contact_Node *pContactNode, uint8 idx);
uint8 Record_SMS_When_AddContact(Contact_Node *pContactNode,uint16 contact_item);
uint8 menu_Contact_AddContact(Contact_Node *pContactNode);
uint8 menu_Contact_DeleteContact(uint8 idx);
uint8 menu_Contact_SearchContactByNum(Contact_Node *pContactNode, uint8* pidx, const uint8* pNum);
bool Is_inContact_SearchContactByNum_and_item(Contact_Node *pContactNode,uint16 contact_item, const uint8* pNum);

uint8 contact_item_L[MAX_CONTACT_NUM];
uint8 sms_item_L[MAX_SMS_NUM];
uint8 Conveyor_item_L[MAX_CONVEYOR_INFO_NUM];
/****************************************************************************/
/***                                              Functions                                              ***/
/**************************about  Record   ************************************/

static uint8 Record_GetRecord(Record *pRecord, uint16 item_contact);
static uint8 Record_SetRecord( Record *pRecord, uint16 item_contact);

static uint8 Record_List_Get_Item_base_and_end(uint16 *item_base,uint16 *item_end,Record_type recordtype);
static uint8 Record_List_Search_First_Unvalued(uint16 *unuse_item,uint16 item_base,uint16 item_end);
static uint8 Record_List_Search_First_Valid(Record *pRecord,uint16 *head_item,uint16 item_base,uint16 item_end);
static uint8 Record_List_CutUnvalued(uint16 item_contact,uint16 item_base,uint16 item_end);
static uint8 Record_List_toFind_Idx(Record *pRecord,uint16 *head_item, uint8 idx,Record_type recordtype );
static uint8 Record_List_Search_End_Valid(Record *pRecord,uint16 *head_item,uint8 *pidx,Record_type recordtype );
static uint8 Record_List_init (Record_type recordtype );

void  menu_Record_nv_init(void);
uint8 menu_Record_AddRecord(Record *pRecord,Record_type recordtype);
uint8 menu_Record_DeleteRecord(uint8 idx,Record_type recordtype);
uint8 menu_Record_Read_Num(uint8 *pNum,Record_type recordtype);
uint8 menu_Record_DeleteAll(Record_type recordtype);
uint8 menu_Record_ReadRecord(Record *pRecord, uint8 idx,Record_type recordtype);


/****************************************************************************/
/***                                              Functions                                              ***/
/**************************about  SMS   ************************************/


static uint8 SMS_GetSMS(sms_saved_t *pSMS, uint16 item_SMS);
static uint8 SMS_SetSMS( sms_saved_t *pSMS, uint16 item_SMS);

static uint8 SMS_List_CutUnvalued(uint16 item_SMS);
static uint8 SMS_List_Search_First_Unvalued(uint16 *real_item,uint16 item_base,uint16 item_end);
static uint8 SMS_List_Search_First_Valid(sms_saved_t *pSMS,uint16 *head_item,uint16 item_base,uint16 item_end);
static uint8 SMS_List_Search_End_Valid(sms_saved_t *pSMS,uint16 *head_item,uint8 *pidx,sms_type smstype);
static uint8 SMS_List_Get_Item_base_and_end(uint16 *item_base, uint16 *item_end,uint8 *SMS_max,sms_type smstype);
static uint8 menu_SMS_Search_Idx(sms_saved_t *pSMS,uint16 *head_item, uint8 idx,sms_type smstype);

//uint8 int_SMS_item_L(void);
//uint8 int_SMS_item_L_Next(uint16 real_item);
//void reset_SMS_item_L(void);
//uint16 Get_SMS_item(uint8 idx);
uint8 SMS_List_init ( sms_type  smstype);
uint8 menu_SMS_Read_unread(uint8* pNum);
uint8 menu_SMS_Read_Num(uint8 *pNum,sms_type smstype);
uint8  menu_SMS_Search_Inbox(app_SMS_t *pSMS);
uint8 menu_SMS_Add(sms_saved_t *pSMS,sms_type smstype);
uint8 menu_SMS_Delete(uint8 idx,sms_type smstype);
uint8 menu_SMS_DeleteAll(sms_type smstype);
uint8 menu_SMS_Read_SMS(sms_saved_t *pSMS,uint8 idx,sms_type smstype);


uint8 Conveyor_GetConveyor(Conveyor_sms_t *pConveyor, uint16 item_Conveyor);
uint8 Conveyor_SetConveyor(Conveyor_sms_t *pConveyor, uint16 item_Conveyor);
uint8 Conveyor_List_Get_Item_base_and_end(uint16 *item_base, uint16 *item_end,uint8 *conveyor_max,conveyor_type conveyortype);
uint8 Conveyor_List_CutUnvalued(uint16 item_Conveyor);
uint8 Conveyor_List_Search_First_Unvalued(uint16 *real_item,uint16 item_base,uint16 item_end);
uint8 Conveyor_List_Search_End_Valid(Conveyor_sms_t *pConveyor,uint16 *head_item,uint8 *pidx,conveyor_type conveyortype);
uint8 Conveyor_List_Search_First_Valid(Conveyor_sms_t *pConveyor,uint16 *head_item,uint16 item_base,uint16 item_end);
void reset_Conveyor_item_L(void);
uint8 int_Conveyor_item_L(void);
uint16 Get_Conveyor_item(uint8 idx);
uint8 int_Conveyor_item_L_Next(uint16 real_item);
void menu_Conveyor_nv_init(void);
uint8 list_Conveyortype_init(conveyor_type conveyortype);
uint8 menu_Conveyor_Search_Idx(Conveyor_sms_t *pConveyor,uint16 *head_item, uint8 idx,conveyor_type conveyortype);
uint8 menu_Conveor_Add(Conveyor_sms_t *pConveyor,conveyor_type conveyortype);
uint8 menu_Conveyor_Read_Num(uint8 *pNum,conveyor_type conveyortype);
uint8 menu_Conveyor_Read_Conveyor(Conveyor_sms_t *pConveyor,uint8 idx,conveyor_type conveyortype);
uint8 Conveyor_List_Get_FirstValid_Item(uint16 *head_item,conveyor_type conveyortype);
bool menu_Conveyor_Search_Inbox(app_Conveyor_info_t *pConveyor);


/****************************************************************************/

uint8 Contact_GetContact(Contact_Node *pContactNode, uint16 item_contact)
{
    uint8 flag;
    if(pContactNode==NULL||item_contact>MP_NV_CONTACT_END||item_contact<MP_NV_CONTACT_BASE)
    {
        return ZInvalidParameter;
    }
    flag = osal_nv_read(item_contact, 0, sizeof(Contact_Node), pContactNode);
    return flag;
}
uint8 Contact_SetContact( Contact_Node *pContactNode, uint16 item_contact)
{
    uint8 flag;
    if(pContactNode==NULL||item_contact>MP_NV_CONTACT_END||item_contact<MP_NV_CONTACT_BASE)
    {
        return ZInvalidParameter;
    }
    flag = osal_nv_write(item_contact, 0, sizeof(Contact_Node), pContactNode);
    return flag;
}

uint8 Contact_List_CutUnvalued(uint16 item_contact)
{
    Contact_Node node;
    uint8 flag;
    if(item_contact>MP_NV_CONTACT_END||item_contact<MP_NV_CONTACT_BASE)
    {
        return ZInvalidParameter;
    }
    flag=Contact_GetContact(&node, item_contact);
    if(flag==ZSuccess)
    {
        node.item_head.item_next=LIST_ITEM_END;
        flag = Contact_SetContact(&node,item_contact);
    }
    return flag;
}

uint8 menu_Contact_nv_init(void)
{
    uint8 flag;
    uint8 rtn = ZSuccess;

    for ( uint8 i=0; i<=(MAX_CONTACT_NUM-1); i++ )
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_item_init ( MP_NV_CONTACT_BASE + i, sizeof ( Contact_Node ), NULL );
        if ( flag == NV_ITEM_UNINIT )
        {
            Contact_Node  Contact;
            Contact.item_head.isvalid=  false;
            if(i==0)
            {
                Contact.item_head.item_self=LIST_ITEM_START;
                Contact.item_head.item_next=MP_NV_CONTACT_BASE+1;
            }
            else
                Contact.item_head.item_self=MP_NV_CONTACT_BASE+i;
            if(i==(MAX_CONTACT_NUM-1))
                Contact.item_head.item_next=LIST_ITEM_END;
            else if(i!=0)
                Contact.item_head.item_next=Contact.item_head.item_self+1;
            if ( ZSuccess!=osal_nv_write (MP_NV_CONTACT_BASE + i,0, sizeof ( Contact_Node ),  &Contact ) )
                rtn = MP_STATUS_OPER_FAILED;
        }
        else if ( flag == NV_OPER_FAILED )
        {
            rtn = MP_STATUS_OPER_FAILED;
        }
    }
    return rtn;

}
uint8 int_contact_item_L()
{
    Contact_Node node;
    uint8 flag,idx=0;
    uint16 head_item=MP_NV_CONTACT_BASE;
    flag=Contact_List_Search_End_Valid(&node,&head_item,&idx);
    if(flag!=ZSuccess)return FAILURE;
    if(idx&&idx<=MAX_CONTACT_NUM)
    {
        return ZSuccess;
    }
    else
        return  ZFailure;
}

uint8 int_contact_item_L_Next(uint16 real_item)
{
    Contact_Node node;
    uint8 flag,idx=0;
    //uint16 head_item=MP_NV_CONTACT_BASE;
    for(uint8 i=0; i<MAX_CONTACT_NUM; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        if(contact_item_L[i]==0x00)
        {
            idx=i;
            break;
        }
    }
    if(!idx)
    {
        contact_item_L[idx]=LO_UINT16(real_item);
        return ZSuccess;
    }
    flag= Contact_GetContact(&node, (Get_contact_item(idx-1)));
    if(flag!=ZSuccess)return flag;
    contact_item_L[idx]=LO_UINT16(real_item);
    if(idx&&idx<=MAX_CONTACT_NUM)
    {
        return ZSuccess;
    }
    else
        return  ZFailure;
}

void reset_contact_item_L(void)
{

    for(uint8 i=0; i<MAX_CONTACT_NUM; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        contact_item_L[i]=0x00;
    }
}
uint16 Get_contact_item(uint8 idx)
{

    uint16 real_item=0;
    real_item= BUILD_UINT16(contact_item_L[idx], (uint8)0x04);
    //real_item |= contact_item_L[idx];
    //real_item &=0x00ff;
    //real_item|=0x0400;
    return real_item;
}
uint8 menu_Contact_ReadContactNum(uint8 * pContactNum)
{
    uint8  idx;
    *pContactNum=0;
    for(idx=0; idx<MAX_CONTACT_NUM; idx++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif

        if((Get_contact_item(idx)>=MP_NV_CONTACT_BASE)&&(Get_contact_item(idx)<=MP_NV_CONTACT_END))continue;
        if(idx&&idx<=MAX_CONTACT_NUM)
        {
            *pContactNum=idx;
            return ZSuccess;
        }
        else return ZFailure;
    }

    * pContactNum=idx;
    return ZSuccess;


}
uint8 menu_Contact_ReadContact(Contact_Node *pContactNode,uint8 idx)
{
    uint8 flag=FAILURE;
    if(idx>MAX_CONTACT_NUM-1)return ZInvalidParameter;
    if((Get_contact_item(idx)>=MP_NV_CONTACT_BASE)&&(Get_contact_item(idx)<=MP_NV_CONTACT_END))
        flag= Contact_GetContact(pContactNode, (Get_contact_item(idx)));
    return flag;
}
uint8 Record_SMS_When_AddContact(Contact_Node *pContactNode,uint16 contact_item)
{
    Record record;
    sms_saved_t sms;
    uint8 flag=ZSuccess;
    uint16 start_item;
    for(uint16 i=MP_NV_DIALED_BASE; i<=MP_NV_ANSWERED_END; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=Record_GetRecord(&record,i);
        if(flag!=ZSuccess)return flag;
        if(!record.item_head.isvalid)continue;
        if((record.Contect_item_L==LIST_ITEM_NULL)&&(strcmp((char *)record.num.Nmbr, (char *)pContactNode->num.Nmbr) == 0))
        {
            record.Contect_item_L=LO_UINT16(contact_item);
            flag=Record_SetRecord(&record,i);
            if(flag!=ZSuccess)return flag;
        }

    }
#ifdef SMS_SENDBOX
    start_item=MP_NV_SMS_SEND_BASE;
#else
    start_item=MP_NV_SMS_INBOX_BASE;
#endif
    for(uint16 i=start_item; i<=MP_NV_SMS_INBOX_END; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=SMS_GetSMS(&sms,i);
        if(flag!=ZSuccess)return flag;
        if(!sms.head.item_head.isvalid)continue;
        if((sms.head.Contect_item_L==LIST_ITEM_NULL)&&(strcmp((char *)sms.head.nmbr.Nmbr, (char *)pContactNode->num.Nmbr) == 0))
        {
            sms.head.Contect_item_L=LO_UINT16(contact_item);
            flag=SMS_SetSMS(&sms,i);
            if(flag!=ZSuccess)return flag;
        }
    }
    return flag;

}
uint8 menu_Contact_AddContact(Contact_Node *pContactNode)
{
    Contact_Node node;
    uint8 flag,idx;
    uint16 unuse_item,head_item=MP_NV_CONTACT_BASE;
    if(pContactNode==NULL)
    {
        return ZInvalidParameter;
    }

    flag=Contact_List_Search_First_Unvalued(&unuse_item);
    if(flag!=ZSuccess)return ZBufferFull;
    //flag=Contact_List_Search_End_Valid(&node,&head_item,&idx);
    for(idx=0; idx<MAX_CONTACT_NUM; idx++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        if((Get_contact_item(idx)>=MP_NV_CONTACT_BASE)&&(Get_contact_item(idx)<=MP_NV_CONTACT_END))
        {
            if(!idx)head_item=Get_contact_item(idx);
            continue;
        }
        if(!idx)
        {
            flag=ZFailure;
            break;
        }
        flag=Contact_GetContact(&node, (Get_contact_item(idx-1)));
        break;
    }
    if(flag!=ZSuccess)
        pContactNode->item_head.item_self=LIST_ITEM_START;
    else
    {
        pContactNode->item_head.item_self=unuse_item;
        node.item_head.item_next=unuse_item;
        if(idx==1)
        {
            flag=Contact_SetContact(&node,head_item);
        }
        else
        {
            flag=Contact_SetContact(&node,node.item_head.item_self);
        }
        if(flag!=ZSuccess)return flag;
    }
    Record_SMS_When_AddContact(pContactNode,unuse_item);
    pContactNode->item_head.isvalid=true;
    pContactNode->item_head.item_next=LIST_ITEM_END;
    flag=Contact_SetContact(pContactNode,unuse_item);
    if(flag==ZSuccess)
        //{
        //if(contact_item_L[0]==0x00)
        //	{
        // 	contact_item_L[0]=LO_UINT16(unuse_item);
        //	return ZSuccess;
        //	}
        flag= int_contact_item_L_Next(unuse_item);
    //}
    return flag;
}

uint8 menu_Contact_DeleteContact(uint8 idx)
{
    Contact_Node node;
    uint8 flag;
    uint16 save_next,head_item;


    flag=Contact_List_toFind_Idx(&node,&head_item,idx);
    if(flag!=ZSuccess)return flag;
    save_next=node.item_head.item_next;
    node.item_head.isvalid=false;
    if(!idx)
    {
        flag=Contact_SetContact(&node,head_item);
        if(flag!=ZSuccess)return flag;
        if(save_next==LIST_ITEM_END)
        {
            return int_contact_item_L();
        }
        flag=Contact_GetContact(&node,save_next);
        if(flag!=ZSuccess)return flag;
        node.item_head.item_self=LIST_ITEM_START;
        flag= Contact_SetContact(&node,save_next);
    }
    else
    {
        flag=Contact_SetContact(&node,node.item_head.item_self);
        if(flag!=ZSuccess)return flag;
        flag=Contact_List_toFind_Idx(&node,&head_item,idx-1);
        if(flag!=ZSuccess)return flag;
        node.item_head.item_next=save_next;
        if((idx-1)==0)
            flag= Contact_SetContact(&node,head_item);
        else 	flag= Contact_SetContact(&node,node.item_head.item_self);
    }
    if(flag==ZSuccess)return int_contact_item_L();
    else return flag;
}
bool Is_inContact_SearchContactByNum_and_item(Contact_Node *pContactNode,uint16 contact_item, const uint8* pNum)
{
    Contact_Node c_node;
    uint8 flag;

    if((contact_item>MP_NV_CONTACT_END)||(contact_item<MP_NV_CONTACT_BASE))
        return FALSE;
    flag=Contact_GetContact(&c_node,contact_item);
    if(flag!=ZSuccess)return flag;
    if(!c_node.item_head.isvalid)return FALSE;
    if(strcmp((char *)c_node.num.Nmbr, (char *)pNum) == 0)
    {
        *pContactNode = c_node;
        return TRUE;
    }
    else return FALSE;
}

uint8 menu_Contact_SearchContactByNum(Contact_Node *pContactNode, uint8* pidx, const uint8* pNum)
{
    Contact_Node c_node;
    uint8 i,flag;

    if(pNum==NULL)
    {
        return ZFailure;
    }
    for(i=0; i<=(MAX_CONTACT_NUM-1); i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        //flag=Contact_List_toFind_Idx(&c_node,&head_item,i);
        flag=menu_Contact_ReadContact(&c_node,i);
        if(flag!=ZSuccess)return ZFailure;
        if(!c_node.item_head.isvalid)continue;
        if(strcmp((char *)c_node.num.Nmbr, (char *)pNum) == 0)
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
    /* read the sleep timer; ST0 must be read first */
    return ZFailure;

}
uint8 Contact_List_toFind_Idx(Contact_Node *pContactNode,uint16 *head_item, uint8 idx )
{
    Contact_Node c_node;
    uint8 flag,j=0;
    uint16 save_last_ture_item;
    if((idx+1)>MAX_CONTACT_NUM)return ZInvalidParameter;
    flag=Contact_List_Search_First_Valid(&c_node,head_item);
    if(flag!=ZSuccess)return flag;
    if((idx+1)==1)
    {
        *pContactNode=c_node;
        return ZSuccess;
    }
    save_last_ture_item=*head_item;
    for(uint16 i=c_node.item_head.item_next; i<=MP_NV_CONTACT_END;)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=Contact_GetContact(&c_node, i);
        if(flag!=ZSuccess)return flag;
        if(c_node.item_head.isvalid&&(c_node.item_head.item_self==i))
        {
            save_last_ture_item=i;
            ++j;
            if(j==idx)
            {
                *pContactNode=c_node;
                return SUCCESS;
            }
            if((c_node.item_head.item_next>=MP_NV_CONTACT_BASE)&&(c_node.item_head.item_next<=MP_NV_CONTACT_END))i=c_node.item_head.item_next;
            else  return FAILURE;
        }
        else
        {
            return Contact_List_CutUnvalued(save_last_ture_item);
        }
    }
    return FAILURE;
}
uint8 Contact_List_Search_First_Unvalued(uint16 *real_item)
{
    uint8 flag;
    Contact_Node  c_node;
    for(uint16 i=MP_NV_CONTACT_BASE; i<=MP_NV_CONTACT_END; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_read(i, 0, sizeof(Contact_Node), &c_node);
        if(flag!=ZSuccess)return flag;
        if(!c_node.item_head.isvalid)
        {
            *real_item=i;
            return SUCCESS;
        }
        else if((c_node.item_head.item_self!=i)&&(c_node.item_head.item_self!=LIST_ITEM_START))
        {
            *real_item=i;
            flag=Contact_GetContact(&c_node, i);
            if(flag!=ZSuccess)return flag;
            c_node.item_head.isvalid=false;
            flag= Contact_SetContact(&c_node, i);
            i=*real_item;
        }
    }
    return flag;
}

uint8 Contact_List_Search_First_Valid(Contact_Node *pContactNode,uint16 *head_item)
{

    uint8 flag;
    Contact_Node  node;

    *head_item=MP_NV_CONTACT_BASE;
    for(uint16 i=MP_NV_CONTACT_BASE; i<=MP_NV_CONTACT_END; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_read(i, 0, sizeof(Contact_Node), &node);
        if(flag!=ZSuccess)return flag;
        if(node.item_head.isvalid&&(node.item_head.item_self==LIST_ITEM_START))
        {
            *head_item=i;
            *pContactNode=node;
            return ZSuccess;
        }
    }
    return ZFailure;
}

uint8 Contact_List_Search_End_Valid(Contact_Node *pContactNode,uint16 *head_item,uint8 *pidx)
{

    Contact_Node c_node;
    uint8 flag,j=0;
    uint16 save_last_ture_item;
    uint16 i;

    *pidx=0;
    reset_contact_item_L();
    flag=Contact_List_Search_First_Valid(&c_node,head_item);
    if(flag!=ZSuccess)return flag;
    *pidx=1;
    contact_item_L[0]=*head_item;
    *pContactNode=c_node;
    save_last_ture_item=*head_item;
    if(c_node.item_head.item_next==LIST_ITEM_END)return ZSuccess;
    for( i=c_node.item_head.item_next; i<=MP_NV_CONTACT_END;)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=Contact_GetContact(&c_node, i);
        if(flag!=ZSuccess)return Contact_List_CutUnvalued(save_last_ture_item);
        if(c_node.item_head.isvalid&&(c_node.item_head.item_self==i))
        {
            save_last_ture_item=i;
            ++j;
            contact_item_L[j]=i;
            if((c_node.item_head.item_next>=MP_NV_CONTACT_BASE)&&(c_node.item_head.item_next<=MP_NV_CONTACT_END))
            {
                i=c_node.item_head.item_next;
            }
            else
            {
                if(c_node.item_head.item_next==LIST_ITEM_END)
                {
                    *pContactNode=c_node;
                    *pidx+=j;
                    return SUCCESS;
                }
                *pidx+=j;
                flag=Contact_List_CutUnvalued(save_last_ture_item);
                return flag;
            }
        }
        else
        {
            return Contact_List_CutUnvalued(save_last_ture_item);
        }
    }
    return FAILURE;
}
/*****************************************************************************/
/***        Include files        				     Record                            		           ***/
/****************************************************************************/
uint8 Record_List_Get_Item_base_and_end(uint16 *item_base,uint16 *item_end,Record_type recordtype)
{
    uint8 rtn = ZSuccess;
    switch(recordtype)
    {
    case  Record_type_DIALED:
        *item_base=MP_NV_DIALED_BASE;
        *item_end=MP_NV_DIALED_END;
        break;
    case  Record_type_MISSED:
        *item_base=MP_NV_MISSED_BASE;
        *item_end=MP_NV_MISSED_END;
        break;
    case  Record_type_ANSWERED:
        *item_base=MP_NV_ANSWERED_BASE;
        *item_end=MP_NV_ANSWERED_END;
        break;

    default:
        return FAILURE;
    }
    return rtn;

}
uint8 Record_List_Search_First_Unvalued(uint16 *unuse_item,uint16 item_base,uint16 item_end)
{
    uint8 flag;
    Record  record;
    uint16 i;
    for( i=item_base; i<=item_end; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif

        flag = Record_GetRecord(&record,i);
        if(flag!=ZSuccess)return flag;
        if(!record.item_head.isvalid)
        {
            *unuse_item=i;
            return ZSuccess;
        }
        else if((record.item_head.item_self!=i)&&(record.item_head.item_self!=LIST_ITEM_START))
        {
            *unuse_item=i;
            flag=Record_GetRecord(&record, i);
            if(flag!=ZSuccess)return flag;
            record.item_head.isvalid=false;
            flag= Record_SetRecord(&record, i);
            if(flag!=ZSuccess)return flag;
            i=item_base;
        }
    }
    return ZFailure;
}
uint8 Record_List_Search_First_Valid(Record *pRecord,uint16 *head_item,uint16 item_base,uint16 item_end)
{

    uint8 flag;
    Record  record;

    for(uint16 i=item_base; i<=item_end; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = Record_GetRecord(&record,i);
        if(flag!=ZSuccess)return flag;
        if(record.item_head.isvalid&&(record.item_head.item_self==LIST_ITEM_START))
        {
            *head_item=i;
            *pRecord=record;
            return ZSuccess;
        }
    }
    return ZFailure;
}
uint8 Record_GetRecord(Record *pRecord, uint16 item_contact)
{
    uint8 flag;
    if(pRecord==NULL)
    {
        return ZInvalidParameter;
    }
    flag = osal_nv_read(item_contact, 0, sizeof(Record), pRecord);
    return flag;
}
uint8 Record_SetRecord( Record *pRecord, uint16 item_contact)
{
    uint8 flag;
    if(pRecord==NULL)
    {
        return ZInvalidParameter;
    }
    flag = osal_nv_write(item_contact, 0, sizeof(Record), pRecord);
    return flag;
}
uint8 Record_List_CutUnvalued(uint16 item_contact,uint16 item_base,uint16 item_end)
{
    Record record;
    uint8 flag;
    if(item_contact>item_base||item_contact<item_end)
    {
        return ZInvalidParameter;
    }
    flag=Record_GetRecord(&record, item_contact);
    if(flag==ZSuccess)
    {
        record.item_head.item_next=LIST_ITEM_END;
        flag = Record_SetRecord(&record,item_contact);
    }
    return flag;
}

uint8 Record_List_toFind_Idx(Record *pRecord,uint16 *head_item, uint8 idx,Record_type recordtype )
{
    Record record;
    uint8 flag,j=0;;
    uint16 item_base,item_end;
    uint16 save_last_ture_item;

    if((idx+1)>MAX_CONTACT_NUM)return ZInvalidParameter;

    flag=Record_List_Get_Item_base_and_end(&item_base,&item_end,recordtype);
    if(flag!=ZSuccess)return flag ;

    flag=Record_List_Search_First_Valid(&record,head_item,item_base,item_end);
    if(flag!=ZSuccess)return flag;
    if((idx+1)==1)
    {
        *pRecord=record;
        return ZSuccess;
    }
    save_last_ture_item=*head_item;
    for(uint16 i=record.item_head.item_next; i<item_end;)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=Record_GetRecord(&record, i);
        if(flag!=ZSuccess)return flag;
        if(record.item_head.isvalid&&(record.item_head.item_self==i))
        {
            save_last_ture_item=i;
            ++j;
            if(j==idx)
            {
                *pRecord=record;
                return SUCCESS;
            }
            if((record.item_head.item_next>=item_base)&&(record.item_head.item_next<=item_end))i=record.item_head.item_next;
            else  return FAILURE;
        }
        else
        {
            flag= Record_List_CutUnvalued(save_last_ture_item,item_base,item_end);
            return flag;
        }
    }
    return FAILURE;
}
uint8 Record_List_Search_End_Valid(Record *pRecord,uint16 *head_item,uint8 *pidx,Record_type recordtype )
{
    Record record;
    uint8 flag,j=0;
    uint16 save_last_ture_item;
    uint16 i;
    uint16 item_base=0,item_end=0;

    flag= Record_List_Get_Item_base_and_end(&item_base,&item_end,recordtype);
    if(flag!=ZSuccess)return flag;
    *pidx=0;
    flag=Record_List_Search_First_Valid(&record,head_item,item_base,item_end);
    if(flag!=ZSuccess)return flag;
    *pidx=1;
    *pRecord=record;
    save_last_ture_item=*head_item;
    if(record.item_head.item_next==LIST_ITEM_END)  return  ZSUCCESS;//return Record_List_Make_nouse_False(save_last_ture_item,item_base,item_end);
    for( i=record.item_head.item_next; i<=MP_NV_CONTACT_END;)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=Record_GetRecord(&record, i);
        if(flag!=ZSuccess)return Record_List_CutUnvalued(save_last_ture_item,item_base,item_end);
        //if(record.item_head.item_self==LIST_ITEM_START)
        //{
        //record.item_head.item_self=i;
        //flag=Record_SetRecord(&record, i);
        //if(flag!=ZSuccess)return flag;
        //}
        if(record.item_head.isvalid)
        {
            save_last_ture_item=i;
            ++j;
            if((record.item_head.item_next>=item_base)&&(record.item_head.item_next<=item_end))
            {
                i=record.item_head.item_next;
            }
            else
            {
                if(record.item_head.item_next==LIST_ITEM_END)
                {
                    *pRecord=record;
                    *pidx+=j;
                    return SUCCESS;
                }
            }
        }
        else
        {
            return Record_List_CutUnvalued(save_last_ture_item,item_base,item_end);
        }
    }
    return FAILURE;
}
uint8 menu_Record_Read_Num(uint8 *pNum,Record_type recordtype)
{
    Record record;
    uint16 head_item;
    return  Record_List_Search_End_Valid(&record,&head_item,pNum,recordtype);
}
uint8  Record_List_init (Record_type recordtype )
{
    uint8 flag;
    uint8 rtn = ZSuccess;
    uint16 item_base=0,item_end=0;

    flag= Record_List_Get_Item_base_and_end(&item_base,&item_end,recordtype);
    if(flag!=ZSuccess)return flag;
    for ( uint8 i=0; i< MAX_CALL_NUM; i++ )
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_item_init ( item_base + i, sizeof ( sms_saved_t ), NULL );
        if ( flag == NV_ITEM_UNINIT )
        {
            Record  record;
            record.item_head.isvalid=false;
            record.Contect_item_L=LIST_ITEM_NULL;
            if(i==0)
            {
                record.item_head.item_self=LIST_ITEM_START;
                record.item_head.item_next=item_base+1;
            }
            else
                record.item_head.item_self=item_base + i;
            if(i==(MAX_CALL_NUM-1))
                record.item_head.item_next=LIST_ITEM_END;
            else if(i!=0)
                record.item_head.item_next=record.item_head.item_self+1;
            if ( ZSuccess!=osal_nv_write ( item_base + i,0, sizeof ( Record ),  &record ) )
                rtn = MP_STATUS_OPER_FAILED;
            osal_nv_read(item_base + i, 0, sizeof(Record), &record);
        }
        else if ( flag == NV_OPER_FAILED )
        {
            rtn = MP_STATUS_OPER_FAILED;
        }
    }
    return rtn;

}
void menu_Record_nv_init ()
{
    Record_List_init(Record_type_DIALED);
    Record_List_init(Record_type_MISSED);
    Record_List_init(Record_type_ANSWERED);
}

uint8 menu_Record_AddRecord(Record *pRecord,Record_type recordtype)
{
    Record record;
    uint8 flag,idx;
    uint16 unuse_item,head_item,item_save;
    uint16 item_base,item_end,save_next;
    if(pRecord==NULL)
    {
        return ZInvalidParameter;
    }
    flag= Record_List_Get_Item_base_and_end(&item_base,&item_end,recordtype);
    if(flag!=ZSuccess)return flag;
    flag= Record_List_Search_First_Valid(&record,&head_item,item_base,item_end);
    item_save=head_item;
    save_next=record.item_head.item_next;
    if(flag!=ZSuccess)
    {
        pRecord->item_head.item_self=LIST_ITEM_START;
        unuse_item=item_base;
    }
    else
    {
        flag=Record_List_Search_First_Unvalued(&unuse_item,item_base,item_end);
        if(flag!=ZSuccess)
        {
            if(save_next!=LIST_ITEM_END)
            	{
			flag=Record_GetRecord(&record,save_next); 
            if(flag!=ZSuccess)return flag;
            head_item=record.item_head.item_self;
            record.item_head.item_self=LIST_ITEM_START;
            flag=Record_SetRecord(&record,head_item);
            if(flag!=ZSuccess)return flag;
            flag=Record_GetRecord(&record,item_save);
            record.item_head.item_self=item_save;
	     record.item_head.isvalid=false;
            flag=Record_SetRecord(&record,item_save);
            if(flag!=ZSuccess)return flag;
	     unuse_item=item_save;				
            	}			
              else 
              {
              	for(uint16 i=item_base;i<=item_end;i++)
				{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        			FeedWatchDog();
#endif				
				if(i!=head_item)
					{
					record.item_head.isvalid=false;
					record.item_head.item_self=i;
					record.item_head.item_next=i;
					flag=Record_SetRecord(&record,i);
					if(flag!=ZSuccess)return flag;				
					}
				}
			if((head_item+1)<=item_end)
				{
				unuse_item=head_item+1;
				}
				else
				{
				unuse_item=item_base;
				}
              }				
        }
        flag=Record_List_Search_End_Valid(&record,&head_item,&idx,recordtype);
	 if(flag!=ZSuccess)	
	 	{
              	for(uint16 i=item_base;i<=item_end;i++)
				{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        			FeedWatchDog();
#endif				
				if(i!=head_item)
					{
					record.item_head.isvalid=false;
					record.item_head.item_self=i;
					record.item_head.item_next=i;
					flag=Record_SetRecord(&record,i);
					if(flag!=ZSuccess)return flag;				
					}
				}
        		pRecord->item_head.item_self=LIST_ITEM_START;
        		unuse_item=item_base;
	 	}
	 	else
	 	{
        pRecord->item_head.item_self=unuse_item;
        record.item_head.item_next=unuse_item;
        		if(idx==1)
			{
				flag=Record_SetRecord(&record,head_item);
        		}
        		else
			{
				flag=Record_SetRecord(&record,record.item_head.item_self);
        		}
        if(flag!=ZSuccess)return flag;
	 	}
    }
    pRecord->item_head.isvalid=true;
    pRecord->item_head.item_next=LIST_ITEM_END;
    flag= Record_SetRecord(pRecord,unuse_item);
    return flag;
}

uint8 menu_Record_DeleteRecord(uint8 idx,Record_type recordtype)
{
    Record record;
    uint8 flag;
    uint16 save_next,head_item;

    flag=Record_List_toFind_Idx(&record,&head_item,idx,recordtype);
    if(flag!=ZSuccess)return flag;
    save_next=record.item_head.item_next;
    record.item_head.isvalid=false;
    if(!idx)
    {
        flag=Record_SetRecord(&record,head_item);
        if(flag!=ZSuccess)return flag;
        if(save_next==LIST_ITEM_END)return 	ZSuccess;
        flag=Record_GetRecord(&record,save_next);
        if(flag!=ZSuccess)return flag;
        record.item_head.item_self=LIST_ITEM_START;
        return Record_SetRecord(&record,save_next);
    }
    else
    {
        flag=Record_SetRecord(&record,record.item_head.item_self);
        if(flag!=ZSuccess)return flag;
        flag=Record_List_toFind_Idx(&record,&head_item,idx-1,recordtype);
        if(flag!=ZSuccess)return flag;
        record.item_head.item_next=save_next;
        if((idx-1)==0)
            return Record_SetRecord(&record,head_item);
        return Record_SetRecord(&record,record.item_head.item_self);
    }

}
uint8 menu_Record_DeleteAll(Record_type recordtype)
{
    Record record;
    uint8 flag;
    uint16 item_base,item_end;

    flag=Record_List_Get_Item_base_and_end(&item_base,&item_end,recordtype);
    if(flag!=ZSuccess)return flag;

    for(uint16 i=item_base; i<=item_end; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=Record_GetRecord(&record, i);
        if(flag!=ZSuccess)return flag;
        record.item_head.isvalid=false;
        flag= Record_SetRecord(&record, i);
        if(flag!=ZSuccess)return flag;
    }
    return ZSuccess;
}
uint8 menu_Record_ReadRecord(Record *pRecord, uint8 idx,Record_type recordtype)
{

    Record record;
    uint8 flag,j=0;
    uint16 save_last_ture_item,head_item;
    uint16 item_base,item_end;

    if(idx+1>MAX_CALL_NUM)return ZInvalidParameter;
    flag= Record_List_Get_Item_base_and_end(&item_base,&item_end,recordtype);
    if(flag!=ZSuccess)return flag;
    flag=Record_List_Search_First_Valid(&record,&head_item,item_base,item_end);
    if(flag!=ZSuccess)return flag;
    if((idx+1)==1)
    {
        *pRecord=record;
        return ZSuccess;
    }
    save_last_ture_item=head_item;
    for(uint16 i=record.item_head.item_next; i<=item_end;)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=Record_GetRecord(&record, i);
        if(flag!=ZSuccess)return flag;
        if(record.item_head.isvalid&&((record.item_head.item_self==i)||(record.item_head.item_self==LIST_ITEM_END)))
        {
            save_last_ture_item=i;
            ++j;
            if(j==idx)
            {
                *pRecord=record;
                return SUCCESS;
            }
            if((record.item_head.item_next>=item_base)&&(record.item_head.item_next<=item_end))i=record.item_head.item_next;
            else  return FAILURE;
        }
        else
        {
            Record_List_CutUnvalued(save_last_ture_item,item_base,item_end);
            return FAILURE;
        }
    }
    return FAILURE;

}
void Add_CallRecord(uint8 index, Record* new_record)
{
    Record_type recordtype;
    if(index == MENU_ID_CALLRECORD_MISSEDCALL)
    {
        recordtype = Record_type_MISSED;
    }
    else if(index == MENU_ID_CALLRECORD_ANSWEREDCALL)
    {
        recordtype = Record_type_ANSWERED;
    }
    else if(index == MENU_ID_CALLRECORD_DIALEDCALL)
    {
        recordtype = Record_type_DIALED;
    }
    else
    {
        return;
    }
    menu_Record_AddRecord(new_record,recordtype);
}
/****************************************************************************/
/***        Include files             SMS                                    ***/
/****************************************************************************/
uint8 SMS_GetSMS(sms_saved_t *pSMS, uint16 item_SMS)
{
    uint8 flag;
    if(pSMS==NULL)
    {
        return ZInvalidParameter;
    }
    flag = osal_nv_read(item_SMS, 0, sizeof(sms_saved_t), pSMS);
    return flag;
}
uint8 SMS_SetSMS( sms_saved_t *pSMS, uint16 item_SMS)
{
    uint8 flag;
    if(pSMS==NULL)
    {
        return ZInvalidParameter;
    }
    flag = osal_nv_write(item_SMS, 0, sizeof(sms_saved_t), pSMS);
    return flag;
}

uint8 SMS_List_CutUnvalued(uint16 item_SMS)
{
    sms_saved_t sms;
    uint8 flag;
    flag=SMS_GetSMS(&sms, item_SMS);
    if(flag)
    {
        sms.head.item_head.item_next=LIST_ITEM_END;
        flag = SMS_SetSMS(&sms,item_SMS);
    }
    return flag;
}

uint8 menu_SMS_Search_Inbox(app_SMS_t *pSMS)//sms_saved_t *pSMS)//only user for incept
{
    sms_saved_t sms;
    uint8 flag;


    if(pSMS==NULL)
    {
        return ZInvalidParameter;
    }
    for(uint16 i=MP_NV_SMS_INBOX_BASE; i<=MP_NV_SMS_INBOX_END; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=SMS_GetSMS(&sms,i);
        if(flag==ZSuccess)
        {
            if(sms.head.item_head.isvalid)
            {
                if((!strcmp((char *)sms.head.nmbr.Nmbr, (char *)pSMS->nmbr.Nmbr))
                        &&(sms.head.seqnum==pSMS->blk))
                {
                    return ZSuccess;
                }
            }
        }
    }
    return ZFailure;
}

uint8 SMS_List_init (sms_type  smstype)
{
    uint8 flag;
    uint8 rtn = ZSuccess;
    uint8 SMS_max;
    uint16 item_base,item_end;

    flag=SMS_List_Get_Item_base_and_end(&item_base,&item_end,&SMS_max,smstype);
    if(flag!=ZSuccess)return flag;
    for ( uint8 i=0; i< SMS_max; i++ )
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_item_init ( item_base + i, sizeof ( sms_saved_t ), NULL );
        if ( flag == NV_ITEM_UNINIT )
        {
            sms_saved_t  sms;
            sms.head.Contect_item_L=LIST_ITEM_NULL;
            sms.head.item_head.isvalid=false;
            if(smstype==SMSTYPE_INBOX)sms.head.isReaded=false;
            else     sms.head.isReaded=TRUE;
            if(i==0)
            {
                sms.head.item_head.item_self=LIST_ITEM_START;
                sms.head.item_head.item_next=item_base+i;
            }
            else
                sms.head.item_head.item_self=item_base + i;
            if(i==(SMS_max-1))
                sms.head.item_head.item_next=LIST_ITEM_END;
            else if(i!=0)
                sms.head.item_head.item_next=sms.head.item_head.item_self+1;
            if ( ZSuccess!=osal_nv_write ( item_base + i,0, sizeof ( sms_saved_t ),  &sms ) )
                rtn = MP_STATUS_OPER_FAILED;
        }
        else if ( flag == NV_OPER_FAILED )
        {
            rtn = MP_STATUS_OPER_FAILED;
        }
    }
    return rtn;

}

uint8 SMS_List_Get_Item_base_and_end(uint16 *item_base, uint16 *item_end,uint8 *SMS_max,sms_type smstype)
{
    uint8 rtn = ZSuccess;
    switch(smstype)
    {
#ifdef SMS_SENDBOX
    case  SMSTYPE_SEND:
        *item_base=MP_NV_SMS_SEND_BASE;
        *item_end=MP_NV_SMS_SEND_END;
        *SMS_max= MAX_SMS_NUM_SENDBOX;
        break;
#endif
#ifdef SMS_TEMPLATE
    case  SMSTYPE_TEMPLATE:
        *item_base=MP_NV_SMS_TEMPLATE_BASE;
        *item_end=MP_NV_SMS_TEMPLATE_END;
        *SMS_max=MAX_SMS_NUM_TEMPLATE;
        break;
#endif
    case  SMSTYPE_INBOX:
        *item_base=MP_NV_SMS_INBOX_BASE;
        *item_end=MP_NV_SMS_INBOX_END;
        *SMS_max=MAX_SMS_NUM;
        break;

    default:
        return ZFailure;

    }
    return rtn;
}

uint8 menu_SMS_Read_SMS(sms_saved_t *pSMS,uint8 idx,sms_type smstype)
{
    uint16 head_item;
    //uint8 flag=FAILURE;
    //if(SMSTYPE_SEND==smstype)
    return menu_SMS_Search_Idx(pSMS,&head_item,idx,smstype);
#if 0
    else if(SMSTYPE_INBOX==smstype)
    {
        if(idx>MAX_SMS_NUM-1)return ZInvalidParameter;
        if((Get_SMS_item(idx)>=MP_NV_SMS_INBOX_BASE)&&(Get_SMS_item(idx)<=MP_NV_SMS_INBOX_END))
            flag= SMS_GetSMS(pSMS, (Get_SMS_item(idx)));
        return flag;
    }
    else return flag;
#endif
}
uint8 menu_SMS_Search_Idx(sms_saved_t *pSMS,uint16 *head_item, uint8 idx,sms_type smstype)
{
    sms_saved_t sms;
    uint8 flag,SMS_max,j=0;
    uint16 item_base,item_end;
    uint16 save_last_ture_item;

    flag=SMS_List_Get_Item_base_and_end(&item_base,&item_end,&SMS_max,smstype);
    if(flag!=ZSuccess)return flag;
    if(idx>(SMS_max-1))return ZInvalidParameter;
    flag=SMS_List_Search_First_Valid(&sms,head_item,item_base,item_end);
    if(flag!=ZSuccess)return flag;
    if(idx==0)
    {
        *pSMS=sms;
        return ZSuccess;
    }
    save_last_ture_item=*head_item;
    for(uint16 i=sms.head.item_head.item_next; i<=item_end;)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=SMS_GetSMS(&sms, i);
        if(flag!=ZSuccess)return flag;
        if(sms.head.item_head.isvalid&&(sms.head.item_head.item_self==i))
        {
            save_last_ture_item=i;
            ++j;
            if(j==idx)
            {
                *pSMS=sms;
                return SUCCESS;
            }
            if((sms.head.item_head.item_next>=item_base)&&(sms.head.item_head.item_next<=item_end))i=sms.head.item_head.item_next;
            else  return FAILURE;
        }
        else
        {
            return SMS_List_CutUnvalued(save_last_ture_item);
        }
    }
    return FAILURE;
}
uint8 SMS_List_Search_First_Unvalued(uint16 *real_item,uint16 item_base,uint16 item_end)
{
    uint8 flag;
    sms_saved_t  sms;

    for(uint16 i=item_base; i<=item_end; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_read(i, 0, sizeof(sms_saved_t), &sms);
        if(flag!=ZSuccess)return flag;
        if(!sms.head.item_head.isvalid)
        {
            *real_item=i;
            return SUCCESS;
        }
        else if((sms.head.item_head.item_self!=i)&&(sms.head.item_head.item_self!=LIST_ITEM_START))
        {
            *real_item=i;
            flag=SMS_GetSMS(&sms, i);
            if(flag!=ZSuccess)return flag;
            sms.head.item_head.isvalid=false;
            flag= SMS_SetSMS(&sms, i);
            i=*real_item;
        }
    }
    return FAILURE;
}
uint8 SMS_List_Get_FirstValid_Item(uint16 *head_item,sms_type smstype)
{
    uint8 SMS_max;
    sms_saved_t  sms;
    uint16 item_base,item_end;

    SMS_List_Get_Item_base_and_end(&item_base,&item_end,&SMS_max,smstype);
    return SMS_List_Search_First_Valid(&sms,head_item,item_base,item_end);
}
uint8 SMS_List_Search_First_Valid(sms_saved_t *pSMS,uint16 *head_item,uint16 item_base,uint16 item_end)
{
    uint8 flag;
    sms_saved_t  sms;

    for(uint16 i=item_base; i<=item_end; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_read(i, 0, sizeof(sms_saved_t), &sms);
        if(flag!=ZSuccess)return flag;
        if(sms.head.item_head.isvalid&&(sms.head.item_head.item_self==LIST_ITEM_START))
        {
            *pSMS=sms;
            *head_item=i;
            return ZSuccess;
        }
    }
    return ZFailure;
}


#if 0
uint8 int_SMS_item_L()
{
    sms_saved_t  sms;
    uint8 flag,idx=0;
    uint16 head_item=MP_NV_SMS_INBOX_BASE;
    flag=SMS_List_Search_End_Valid(&sms,&head_item,&idx,SMSTYPE_INBOX);
    if(flag!=ZSuccess)return FAILURE;
    if(idx&&idx<=MAX_SMS_NUM)
    {
        return ZSuccess;
    }
    else
        return  ZFailure;
}
uint8 int_SMS_item_L_Next(uint16 real_item)
{
    sms_saved_t  sms;
    uint8 flag,idx=0;
    for(uint8 i=0; i<MAX_SMS_NUM; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        if(sms_item_L[i]==0x00)
        {
            idx=i;
            break;
        }
    }
    if(!idx)
    {
        sms_item_L[idx]=LO_UINT16(real_item);
        return ZSuccess;
    }
    flag= SMS_GetSMS(&sms, (Get_SMS_item(idx-1)));
    if(flag!=ZSuccess)return flag;
    sms_item_L[idx]=LO_UINT16(real_item);
    if(idx&&idx<=MAX_SMS_NUM)
        return ZSuccess;
    else
        return  ZFailure;
}
void reset_SMS_item_L(void)
{
    for(uint8 i=0; i<MAX_SMS_NUM; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        sms_item_L[i]=0x00;
    }
}
uint16 Get_SMS_item(uint8 idx)
{
    uint16 real_item=0;
    real_item= BUILD_UINT16(sms_item_L[idx], (uint8)0x04);
    return real_item;
}
#endif
uint8 SMS_List_Search_End_Valid(sms_saved_t *pSMS,uint16 *head_item,uint8 *pidx,sms_type smstype)
{
    sms_saved_t  sms;
    uint8 flag,SMS_max,j=0;
    uint16 save_last_ture_item;
    uint16 i;
    uint16 item_base,item_end;


    //if(smstype==SMSTYPE_INBOX) reset_SMS_item_L();
    flag=SMS_List_Get_Item_base_and_end(&item_base,&item_end,&SMS_max,smstype);
    *pidx=0;
    flag=SMS_List_Search_First_Valid(&sms,head_item,item_base,item_end);
    if(flag!=ZSuccess)return flag;
    *pidx=1;
    *pSMS=sms;
    // if(smstype==SMSTYPE_INBOX)sms_item_L[0]=*head_item;
    save_last_ture_item=*head_item;
    if(sms.head.item_head.item_next==LIST_ITEM_END)return ZSuccess;
    for( i=sms.head.item_head.item_next; i<=item_end;)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=SMS_GetSMS(&sms, i);
        if(flag!=ZSuccess)return SMS_List_CutUnvalued(save_last_ture_item);
        if(sms.head.item_head.isvalid&&(sms.head.item_head.item_self==i))
        {
            save_last_ture_item=i;
            ++j;
            // if(smstype==SMSTYPE_INBOX) sms_item_L[j]=i;
            if((sms.head.item_head.item_next>=item_base)&&(sms.head.item_head.item_next<=item_end))
            {
                i=sms.head.item_head.item_next;
            }
            else
            {
                if(sms.head.item_head.item_next==LIST_ITEM_END)
                {
                    *pSMS=sms;
                    *pidx+=j;
                    return SUCCESS;
                }
            }
        }
        else
        {
            return SMS_List_CutUnvalued(save_last_ture_item);
        }
    }
    return FAILURE;

}

uint8 menu_SMS_Read_Num(uint8 *pNum,sms_type smstype)
{
    sms_saved_t sms;
    uint16 head_item;
    //if(SMSTYPE_INBOX!=smstype)
    return SMS_List_Search_End_Valid(&sms,&head_item,pNum,smstype);
#if 0
    else if(SMSTYPE_INBOX==smstype)
    {
        *pNum=0;
        for(uint8 idx=0; idx<MAX_SMS_NUM; idx++)
        {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
            FeedWatchDog();
#endif
            if((Get_SMS_item(idx)>=MP_NV_SMS_INBOX_BASE)&&(Get_SMS_item(idx)<=MP_NV_SMS_INBOX_END))continue;
            if(idx&&idx<=MAX_SMS_NUM)
            {
                *pNum=idx;
                return ZSuccess;
            }
            else return ZFailure;
        }
        return ZSuccess;
    }
    else  return ZFailure;
#endif
}
uint8 menu_SMS_Read_unread(uint8* pNum)
{
    sms_saved_t sms;
    uint8  idx=0;
    uint8  flag=ZFailure;
    *pNum=0;
#if 1
    for(uint8 i=0; i<MAX_SMS_NUM-1; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        // if((Get_SMS_item(i)>=MP_NV_SMS_INBOX_BASE)&&(Get_SMS_item(i)<=MP_NV_SMS_INBOX_END))
        //  	{
        flag=SMS_GetSMS(&sms, (MP_NV_SMS_INBOX_BASE+i));
        if(flag==ZSUCCESS)
        {
            if(!sms.head.item_head.isvalid)
            {
                continue;
            }
            if(!sms.head.isReaded)
            {
                idx++;
            }
        }
    }

#else
    for(uint8 i=0; i<MAX_SMS_NUM; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        if((Get_SMS_item(i)>=MP_NV_SMS_INBOX_BASE)&&(Get_SMS_item(i)<=MP_NV_SMS_INBOX_END))
        {
            flag=SMS_GetSMS(&sms, (Get_SMS_item(i)));
            if(flag==ZSUCCESS)
            {
                if(!sms.head.item_head.isvalid)
                {
                    break;
                }
                if(!sms.head.isReaded)
                {
                    idx++;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
#endif
    *pNum=idx;
    return ZSuccess;
}

uint8 menu_SMS_Add(sms_saved_t *pSMS,sms_type smstype)
{
    sms_saved_t sms;
    uint8 flag,idx,SMS_max;
    uint16 unuse_item,item_base,item_end,head_item;

    if(pSMS==NULL)
    {
        return ZInvalidParameter;
    }
    flag=SMS_List_Get_Item_base_and_end(&item_base,&item_end,&SMS_max,smstype);
    if(flag!=ZSuccess)return flag;
    flag=SMS_List_Search_First_Unvalued(&unuse_item,item_base,item_end);
    if(flag!=ZSuccess)return ZBufferFull;
#if 0
    if(SMSTYPE_INBOX==smstype)
    {
        for(idx=0; idx<MAX_SMS_NUM; idx++)
        {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
            FeedWatchDog();
#endif
            if((Get_SMS_item(idx)>=MP_NV_SMS_INBOX_BASE)&&(Get_SMS_item(idx)<=MP_NV_SMS_INBOX_END))
            {
                if(!idx)head_item=Get_SMS_item(idx);
                continue;
            }
            if(!idx)
            {
                flag=ZFailure;
                break;
            }
            flag=SMS_GetSMS(&sms, (Get_SMS_item(idx-1)));
            break;
        }
        if(flag!=ZSuccess)
            pSMS->head.item_head.item_self=LIST_ITEM_START;
        else
        {
            pSMS->head.item_head.item_self=unuse_item;
            sms.head.item_head.item_next=unuse_item;
            if(idx==1)
            {
                flag=SMS_SetSMS(&sms,head_item);
            }
            else
            {
                flag=SMS_SetSMS(&sms,sms.head.item_head.item_self);
            }
            if(flag!=ZSuccess)return flag;
        }
        pSMS->head.item_head.isvalid=true;
        pSMS->head.item_head.item_next=LIST_ITEM_END;
        flag=SMS_SetSMS(pSMS,unuse_item);
        if(flag==ZSuccess)
            flag= int_SMS_item_L_Next(unuse_item);
        return flag;
    }
    else
#endif
    {
        flag=SMS_List_Search_End_Valid(&sms,&head_item,&idx,smstype);
        if(flag!=ZSuccess)
            pSMS->head.item_head.item_self=LIST_ITEM_START;
        else
        {
            pSMS->head.item_head.item_self=unuse_item;
            sms.head.item_head.item_next=unuse_item;
            if(idx==1)flag=SMS_SetSMS(&sms,head_item);
            else  flag=SMS_SetSMS(&sms,sms.head.item_head.item_self);
            if(flag!=ZSuccess)return flag;
        }
        pSMS->head.item_head.isvalid=true;
        pSMS->head.item_head.item_next=LIST_ITEM_END;
        flag= SMS_SetSMS(pSMS,unuse_item);
        return flag;
    }

}

uint8 menu_SMS_Delete(uint8 idx,sms_type smstype)
{
    sms_saved_t sms;
    uint8 flag;
    uint16 save_next,head_item;

    flag=menu_SMS_Search_Idx(&sms,&head_item,idx,smstype);
    if(flag!=ZSuccess)return flag;
    save_next=sms.head.item_head.item_next;
    sms.head.item_head.isvalid=false;
    if(!idx)
    {
        flag=SMS_SetSMS(&sms,head_item);
        if(flag!=ZSuccess)return flag;
        if(save_next==LIST_ITEM_END)
        {
            //if(SMSTYPE_INBOX==smstype)
            //	{
            //		return int_SMS_item_L();
            //	}
            //	else
            //	{
            return ZSuccess;
            //	}
        }
        flag=SMS_GetSMS(&sms,save_next);
        if(flag!=ZSuccess)return flag;
        sms.head.item_head.item_self=LIST_ITEM_START;
        flag= SMS_SetSMS(&sms,save_next);
    }
    else
    {
        flag=SMS_SetSMS(&sms,sms.head.item_head.item_self);
        if(flag!=ZSuccess)return flag;
        flag=menu_SMS_Search_Idx(&sms,&head_item,idx-1,smstype);
        if(flag!=ZSuccess)return flag;
        sms.head.item_head.item_next=save_next;
        if((idx-1)==0)
            flag= SMS_SetSMS(&sms,head_item);
        flag= SMS_SetSMS(&sms,sms.head.item_head.item_self);
    }
    // if((flag==ZSuccess)&&(SMSTYPE_INBOX==smstype))return int_SMS_item_L();
    return flag;
}

uint8 menu_SMS_DeleteAll(sms_type smstype)
{
    sms_saved_t sms;
    uint8 flag,SMS_max;
    uint16 item_base,item_end;

    flag=SMS_List_Get_Item_base_and_end(&item_base,&item_end,&SMS_max,smstype);
    if(flag!=ZSuccess)return flag;
    //reset_SMS_item_L();
    for(uint16 i=item_base; i<=item_end; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=SMS_GetSMS(&sms, i);
        if(flag!=ZSuccess)return flag;
        sms.head.item_head.isvalid=false;
        flag= SMS_SetSMS(&sms, i);
        if(flag!=ZSuccess)return flag;
    }
    return ZSuccess;
}

/*******************************************************************************************************/
//
//                                     Conveyor
//
/******************************************************************************************************/

uint8 Conveyor_GetConveyor(Conveyor_sms_t *pConveyor, uint16 item_Conveyor)
{
    uint8 flag;
    if(pConveyor==NULL)
    {
        return ZInvalidParameter;
    }
    flag = osal_nv_read(item_Conveyor, 0, sizeof(Conveyor_sms_t), pConveyor);
    return flag;
}
uint8 Conveyor_SetConveyor(Conveyor_sms_t *pConveyor, uint16 item_Conveyor)
{
    uint8 flag;
    if(pConveyor==NULL)
    {
        return ZInvalidParameter;
    }
    flag = osal_nv_write(item_Conveyor, 0, sizeof(Conveyor_sms_t), pConveyor);
    return flag;
}

uint8 Conveyor_List_Get_Item_base_and_end(uint16 *item_base, uint16 *item_end,uint8 *conveyor_max,conveyor_type conveyortype)
{
    uint8 rtn = ZSuccess;
    switch(conveyortype)
    {
    case  CTYPE_CMD:
        *item_base=MP_CONVEYOR_CMD_BASE;
        *item_end=MP_CONVEYOR_CMD_END;
        *conveyor_max=MAX_CONVEYOR_CMD_NUM;
        break;

    case  CTYPE_INFO:
        *item_base=MP_CONVEYOR_INFO_BASE;
        *item_end=MP_CONVEYOR_INFO_END;
        *conveyor_max=MAX_CONVEYOR_INFO_NUM;
        break;

    default:
        return ZFailure;

    }
    return rtn;
}

uint8 Conveyor_List_CutUnvalued(uint16 item_Conveyor)
{
    Conveyor_sms_t  conveyor_s;
    uint8 flag;
    flag=Conveyor_GetConveyor(&conveyor_s, item_Conveyor);
    if(flag)
    {
        conveyor_s.head.item_head.item_next=LIST_ITEM_END;
        flag = Conveyor_SetConveyor(&conveyor_s,item_Conveyor);
    }
    return flag;
}

uint8 Conveyor_List_Search_First_Unvalued(uint16 *real_item,uint16 item_base,uint16 item_end)
{
    uint8 flag;
    uint16 head_item=item_base;
    uint16 next_item;
    Conveyor_sms_t  conveyor_s;

    for(uint16 i=item_base; i<=item_end; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_read(i, 0, sizeof(Conveyor_sms_t), &conveyor_s);
        if(flag!=ZSuccess)return flag;
        if(!conveyor_s.head.item_head.isvalid)
        {
            *real_item=i;    
            return ZSuccess;
        }
        else if((conveyor_s.head.item_head.item_self!=i)&&(conveyor_s.head.item_head.item_self!=LIST_ITEM_START))
        {
            *real_item=i;
            flag=Conveyor_GetConveyor(&conveyor_s, i);
            if(flag!=ZSuccess)return flag;
            conveyor_s.head.item_head.isvalid=false;
            flag= Conveyor_SetConveyor(&conveyor_s, i);
            i=*real_item;
        }
        else if(conveyor_s.head.item_head.isvalid&&(conveyor_s.head.item_head.item_self==LIST_ITEM_START))
        {
            head_item=i;
            *real_item=head_item;
        }
    }    	
    flag=Conveyor_GetConveyor(&conveyor_s,head_item);
    if(flag!=ZSuccess)return flag;
    if((conveyor_s.head.item_head.item_next>=item_base)&&(conveyor_s.head.item_head.item_next<=item_end))
    {
        next_item=conveyor_s.head.item_head.item_next;
        conveyor_s.head.item_head.isvalid=FALSE;
        flag= Conveyor_SetConveyor(&conveyor_s, head_item);
    }
    else
    {
        return ZFailure;
    }
    flag=Conveyor_GetConveyor(&conveyor_s,next_item);
    if(flag!=ZSuccess)return flag;
    conveyor_s.head.item_head.item_self=LIST_ITEM_START;
    return Conveyor_SetConveyor(&conveyor_s, next_item);
    //return FAILURE;
}

uint8 Conveyor_List_Search_End_Valid(Conveyor_sms_t *pConveyor,uint16 *head_item,uint8 *pidx,conveyor_type conveyortype)
{
    Conveyor_sms_t  conveyor_s;
    uint8 flag,SMS_max,j=0;
    uint16 save_last_ture_item;
    uint16 i;
    uint16 item_base,item_end;


    if(conveyortype==CTYPE_INFO) reset_Conveyor_item_L();
    flag=Conveyor_List_Get_Item_base_and_end(&item_base,&item_end,&SMS_max,conveyortype);
    *pidx=0;
    flag=Conveyor_List_Search_First_Valid(&conveyor_s,head_item,item_base,item_end);
    if(flag!=ZSuccess)return flag;
    *pidx=1;
    *pConveyor=conveyor_s;
    if(conveyortype==CTYPE_INFO)Conveyor_item_L[0]=*head_item;
    save_last_ture_item=*head_item;
    if(conveyor_s.head.item_head.item_next==LIST_ITEM_END)return ZSuccess;
    for( i=conveyor_s.head.item_head.item_next; i<=item_end;)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=Conveyor_GetConveyor(&conveyor_s, i);
        if(flag!=ZSuccess)return Conveyor_List_CutUnvalued(save_last_ture_item);
        if(conveyor_s.head.item_head.isvalid&&(conveyor_s.head.item_head.item_self==i))
        {
            save_last_ture_item=i;
            ++j;
            if(conveyortype==CTYPE_INFO) Conveyor_item_L[j]=i;
            if((conveyor_s.head.item_head.item_next>=item_base)&&(conveyor_s.head.item_head.item_next<=item_end))
            {
                i=conveyor_s.head.item_head.item_next;
            }
            else
            {
                if(conveyor_s.head.item_head.item_next==LIST_ITEM_END)
                {
                    *pConveyor=conveyor_s;
                    *pidx+=j;
                    return SUCCESS;
                }
                else return Conveyor_List_CutUnvalued(save_last_ture_item);
            }
        }
        else
        {
            return Conveyor_List_CutUnvalued(save_last_ture_item);
        }
    }
    return Conveyor_List_CutUnvalued(save_last_ture_item);

}

uint8 Conveyor_List_Search_First_Valid(Conveyor_sms_t *pConveyor,uint16 *head_item,uint16 item_base,uint16 item_end)
{
    uint8 flag;
    Conveyor_sms_t  conveyor_s;

    for(uint16 i=item_base; i<=item_end; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_read(i, 0, sizeof(Conveyor_sms_t), &conveyor_s);
        if(flag!=ZSuccess)return flag;
        if(conveyor_s.head.item_head.isvalid&&(conveyor_s.head.item_head.item_self==LIST_ITEM_START))
        {
            *pConveyor=conveyor_s;
            *head_item=i;
            return ZSuccess;
        }
    }
    return ZFailure;
}

void reset_Conveyor_item_L(void)
{
    for(uint8 i=0; i<MAX_CONVEYOR_INFO_NUM; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        Conveyor_item_L[i]=0x00;
    }
}
uint8 int_Conveyor_item_L(void)
{
    Conveyor_sms_t  conveyor_s;
    uint8 flag,idx=0;
    uint16 head_item=MP_CONVEYOR_INFO_BASE;
    flag=Conveyor_List_Search_End_Valid(&conveyor_s,&head_item,&idx,CTYPE_INFO);
    if(flag!=ZSuccess)return FAILURE;
    if(idx&&idx<=MAX_CONVEYOR_INFO_NUM)
    {
        return ZSuccess;
    }
    else
        return  ZFailure;
}
uint16 Get_Conveyor_item(uint8 idx)
{
    uint16 real_item=0;
    real_item= BUILD_UINT16(Conveyor_item_L[idx], (uint8)0x04);
    return real_item;
}

uint8 int_Conveyor_item_L_Next(uint16 real_item)
{
    Conveyor_sms_t  conveyor_s;
    uint8 flag,idx=0;
    for(uint8 i=0; i<MAX_CONVEYOR_INFO_NUM; i++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        if(Conveyor_item_L[i]==0x00)
        {
            idx=i;
            break;
        }
    }
    if(!idx)
    {
        Conveyor_item_L[idx]=LO_UINT16(real_item);
        return ZSuccess;
    }
    flag= Conveyor_GetConveyor(&conveyor_s, (Get_Conveyor_item(idx-1)));
    if(flag!=ZSuccess)return flag;
    Conveyor_item_L[idx]=LO_UINT16(real_item);
    if(idx&&idx<=MAX_CONVEYOR_INFO_NUM)
        return ZSuccess;
    else
        return  ZFailure;
}


void menu_Conveyor_nv_init(void)
{
    list_Conveyortype_init(CTYPE_INFO);    
    list_Conveyortype_init(CTYPE_CMD);    
}

uint8 list_Conveyortype_init(conveyor_type conveyortype)
{
    uint8 flag = ZSuccess;
    uint8 conveyor_max;
    uint16 item_base,item_end;

    flag=Conveyor_List_Get_Item_base_and_end(&item_base,&item_end,&conveyor_max,conveyortype);
    if(flag!=ZSuccess)return flag;

    for ( uint8 i=0; i<conveyor_max; i++ )
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag = osal_nv_item_init ( item_base + i, sizeof ( Conveyor_sms_t ), NULL );
        if ( flag == NV_ITEM_UNINIT )
        {
            Conveyor_sms_t  conveyor_s;
            conveyor_s.head.item_head.isvalid=  false;
            if(i==0)
            {
                conveyor_s.head.item_head.item_self=LIST_ITEM_START;
                conveyor_s.head.item_head.item_next=item_base+1;
            }
            else
                conveyor_s.head.item_head.item_self=item_base+i;
            if(i==(conveyor_max-1))
                conveyor_s.head.item_head.item_next=LIST_ITEM_END;
            else if(i!=0)
                conveyor_s.head.item_head.item_next=conveyor_s.head.item_head.item_self+1;
            if ( ZSuccess!=osal_nv_write (item_base + i,0, sizeof ( Conveyor_sms_t ),  &conveyor_s ) )
                flag = MP_STATUS_OPER_FAILED;
        }
        else if ( flag == NV_OPER_FAILED )
        {
            flag = MP_STATUS_OPER_FAILED;
        }
    }
    return flag;

}

uint8 menu_Conveyor_Search_Idx(Conveyor_sms_t *pConveyor,uint16 *head_item, uint8 idx,conveyor_type conveyortype)
{
    Conveyor_sms_t conveyor_s;
    uint8 flag,conveyor_max,j=0;
    uint16 item_base,item_end;
    uint16 save_last_ture_item;

    flag=Conveyor_List_Get_Item_base_and_end(&item_base,&item_end,&conveyor_max,conveyortype);
    if(flag!=ZSuccess)return flag;
    if(idx>(conveyor_max-1))return ZInvalidParameter;
    flag=Conveyor_List_Search_First_Valid(&conveyor_s,head_item,item_base,item_end);
    if(flag!=ZSuccess)return flag;
    if(idx==0)
    {
        *pConveyor=conveyor_s;
        return ZSuccess;
    }
    save_last_ture_item=*head_item;
    for(uint16 i=conveyor_s.head.item_head.item_next; i<=item_end;)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        flag=Conveyor_GetConveyor(&conveyor_s, i);
        if(flag!=ZSuccess)return flag;
        if(conveyor_s.head.item_head.isvalid&&(conveyor_s.head.item_head.item_self==i))
        {
            save_last_ture_item=i;
            ++j;
            if(j==idx)
            {
                *pConveyor=conveyor_s;
                return SUCCESS;
            }
            if((conveyor_s.head.item_head.item_next>=item_base)&&(conveyor_s.head.item_head.item_next<=item_end))i=conveyor_s.head.item_head.item_next;
            else  return FAILURE;
        }
        else
        {
            return Conveyor_List_CutUnvalued(save_last_ture_item);
        }
    }
    return FAILURE;
}

uint8 menu_Conveor_Add(Conveyor_sms_t *pConveyor,conveyor_type conveyortype)
{
    Conveyor_sms_t conveyor_s;
    uint8 flag,idx,conveyor_max;
    uint16 unuse_item,item_base,item_end,head_item;

    if(pConveyor==NULL)
    {        
        return ZInvalidParameter;
    }
    flag=Conveyor_List_Get_Item_base_and_end(&item_base,&item_end,&conveyor_max,conveyortype);
    if(flag!=ZSuccess)return flag;
    flag=Conveyor_List_Search_First_Unvalued(&unuse_item,item_base,item_end);
    if(flag!=ZSuccess) return ZBufferFull;
        flag=Conveyor_List_Search_End_Valid(&conveyor_s,&head_item,&idx,conveyortype);
        if(flag!=ZSuccess)
        {
            pConveyor->head.item_head.item_self=LIST_ITEM_START;
        }
        else
        {
            pConveyor->head.item_head.item_self=unuse_item;
            conveyor_s.head.item_head.item_next=unuse_item;
            if(idx==1)
            {
                flag=Conveyor_SetConveyor(&conveyor_s,head_item);
            }
            else
            {
                flag=Conveyor_SetConveyor(&conveyor_s,conveyor_s.head.item_head.item_self);
            }
            if(flag!=ZSuccess)return flag;
        }
        pConveyor->head.item_head.isvalid=true;
        pConveyor->head.item_head.item_next=LIST_ITEM_END;
        flag= Conveyor_SetConveyor(pConveyor,unuse_item);
        if((flag==ZSuccess)&&(CTYPE_INFO==conveyortype))
        {
            flag= int_Conveyor_item_L_Next(unuse_item);
        }
        return flag;
}

uint8 menu_Conveyor_Read_Num(uint8 *pNum,conveyor_type conveyortype)
{
    Conveyor_sms_t conveyor_s;
    uint8 idx,flag;
    uint16 head_item;
    if(CTYPE_CMD==conveyortype)
    {    
        flag= Conveyor_List_Search_End_Valid(&conveyor_s,&head_item,pNum,conveyortype);
        return flag;
    }
    else if(CTYPE_INFO==conveyortype)
    {
        *pNum=0;
        for( idx=0; idx<MAX_CONVEYOR_INFO_NUM; idx++)
        {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
            FeedWatchDog();
#endif
            if((Get_Conveyor_item(idx)>=MP_CONVEYOR_INFO_BASE)&&(Get_Conveyor_item(idx)<=MP_CONVEYOR_INFO_END))continue;
            if(idx<=MAX_CONVEYOR_INFO_NUM)
            {
                *pNum=idx;
                return ZSuccess;
            }
            else return ZFailure;
        }
        *pNum=idx;
        return ZSuccess;
    }
    else  return ZFailure;
}

//Conveyor_sms_t conveyor_s;
uint8 menu_Conveyor_Read_Conveyor(Conveyor_sms_t *pConveyor,uint8 idx,conveyor_type conveyortype)
{
    uint16 head_item;
    uint8   flag=FAILURE;
    if(CTYPE_CMD==conveyortype)
        return menu_Conveyor_Search_Idx(pConveyor,&head_item,idx,conveyortype);
    else if(CTYPE_INFO==conveyortype)
    {
        if(idx>MAX_CONVEYOR_INFO_NUM-1)return ZInvalidParameter;
        if((Get_Conveyor_item(idx)>=MP_CONVEYOR_INFO_BASE)&&(Get_Conveyor_item(idx)<=MP_CONVEYOR_INFO_END))
            flag= Conveyor_GetConveyor(pConveyor, (Get_Conveyor_item(idx)));
        return flag;
    }
    else return flag;
}

uint8 Conveyor_List_Get_FirstValid_Item(uint16 *head_item,conveyor_type conveyortype)
{
    uint8 conveyor_max;
    Conveyor_sms_t conveyor_s;
    uint16 item_base,item_end;

    Conveyor_List_Get_Item_base_and_end(&item_base,&item_end,&conveyor_max,conveyortype);
    return Conveyor_List_Search_First_Valid(&conveyor_s,head_item,item_base,item_end);
}

bool menu_Conveyor_Search_Inbox(app_Conveyor_info_t *pConveyor)
{
    Conveyor_sms_t conveyor_s;
    uint8 flag,idx;


    if(pConveyor==NULL)
    {
        return ZInvalidParameter;
    }
    for( idx=0; idx<MAX_CONVEYOR_INFO_NUM; idx++)
    {
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
        FeedWatchDog();
#endif
        if((Get_Conveyor_item(idx)>=MP_CONVEYOR_INFO_BASE)&&(Get_Conveyor_item(idx)<=MP_CONVEYOR_INFO_END))
        {
            flag=Conveyor_GetConveyor(&conveyor_s,Get_Conveyor_item(idx));
            if(flag==ZSuccess)
            {
                if(conveyor_s.head.item_head.isvalid
                        &&((conveyor_s.head.item_head.item_self==Get_Conveyor_item(idx))||(conveyor_s.head.item_head.item_self==LIST_ITEM_START)))
                {
                    if((conveyor_s.num==pConveyor->num)
                            //(!strcmp((char *)conveyor_s.head.nmbr.Nmbr, (char *)pConveyor->num))
                            &&(conveyor_s.head.seqnum==pConveyor->blk))
                    {
                        return TRUE;
                    }
                }
            }
        }
        else
        {
            break;
        }
    }
    return false;
}


#endif
