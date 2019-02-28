#include "Comdef.h"
#include "Queue.h"
#include "osal_memory.h"
#include "osal.h"

/*********************************************************************
 * INCLUDES
 */
static struct sNode * _QueueAllocNode(uint8 elemSize);
static void		    _QuequFreeNode(struct sNode *p);

/*********************************************************************
 * INCLUDES
 */

/* 1.��ʼ������ */
void _QueueInit(struct queueLK *hq, uint8 elemSize, uint8 maxLen)
{
	hq->front = hq->rear = NULL;     
	hq->elemSize = elemSize;
	hq->maxLen = maxLen;
	hq->len = 0;
	return;
}

/* 2.�������в���һ��Ԫ��x */
bool  QueueInput(struct queueLK *hq, const void* pInElem)
{
	if(hq->len >= hq->maxLen)
	{
		return false;
	}

	/* �õ�һ����newPָ����ָ����½�� */
	struct sNode *newP;
	newP = _QueueAllocNode(hq->elemSize);
	osal_memcpy(newP->pElem, pInElem, hq->elemSize);
	newP->next = NULL;

	/* ������Ϊ�գ����½�㼴�Ƕ��׽�����Ƕ�β��� */
	if(hq->rear == NULL)
	{
		hq->front = hq->rear = newP;
	}else
	{    /* �����ӷǿգ��������޸Ķ�β����ָ����Ͷ�βָ�룬ʹָ֮���µĶ�β��� */
		hq->rear->next = newP; 
		hq->rear = newP;
	}

	hq->len++;
	return true;
}

/* 3.�Ӷ�����ɾ��һ��Ԫ�� */
bool  QueueOutput( struct queueLK *hq,  void* pOutElem)
{
	struct sNode *p;

	if(hq->front == NULL)
	{
		hq->len = 0;
		return false;
	}
        
        p = hq->front;
        osal_memcpy(pOutElem, p->pElem, hq->elemSize);  
	hq->front = p->next;        /* ʹ����ָ��ָ����һ����� */

	if(hq->front == NULL){
		hq->rear = NULL;
		hq->len = 0;
	}
	
	_QuequFreeNode(p); /* ����ԭ���׽�� */

	if(hq->len >0 )
	{
		hq->len--;
	}
	return true;    /* ���ر�ɾ���Ķ���Ԫ��ֵ */
}


/* 5.��������Ƿ�Ϊ�գ���Ϊ���򷵻�1, ���򷵻�0 */
bool  QueueIsempty(struct queueLK *hq)
{
	if(hq->len == 0){
		return true;
	}else{
		return false;
	}
}
 bool QueueIsFull(struct queueLK * hq)
 {
 	if(hq->len >= hq->maxLen)
	{
		return true;
	}
	else
	{
		return false;
	}
 }
 
/* 6.��������е�����Ԫ�� */
void QueueClear(struct queueLK *hq)
{
	struct sNode *p = hq->front;      
	/* ����ɾ�������е�ÿһ����㣬���ʹ����ָ��Ϊ�� */
	while(p != NULL){
		hq->front = hq->front->next;
		_QuequFreeNode(p);
		p = hq->front;
	}   
	hq->rear = NULL;       
	hq->len = 0;
	return;
}

uint8 QueueGetLen(struct queueLK *hq)
{
	return hq->len;
}

struct sNode * _QueueAllocNode(uint8 elemSize)
{
	struct sNode *p;
	p = osal_mem_alloc(sizeof(struct sNode));
	if(p == NULL)
	{
		return NULL;
	}
	p->pElem = osal_mem_alloc(elemSize);
	if(p->pElem == NULL)
	{
		osal_mem_free(p);
		return NULL;
	}
	return p;
}
void _QuequFreeNode(struct sNode *p)
{
	osal_mem_free(p->pElem);
	p->next = NULL;

	osal_mem_free(p);
}
