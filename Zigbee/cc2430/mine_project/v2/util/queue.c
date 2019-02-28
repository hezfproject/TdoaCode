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

/* 1.初始化链队 */
void _QueueInit(struct queueLK *hq, uint8 elemSize, uint8 maxLen)
{
	hq->front = hq->rear = NULL;     
	hq->elemSize = elemSize;
	hq->maxLen = maxLen;
	hq->len = 0;
	return;
}

/* 2.向链队中插入一个元素x */
bool  QueueInput(struct queueLK *hq, const void* pInElem)
{
	if(hq->len >= hq->maxLen)
	{
		return false;
	}

	/* 得到一个由newP指针所指向的新结点 */
	struct sNode *newP;
	newP = _QueueAllocNode(hq->elemSize);
	osal_memcpy(newP->pElem, pInElem, hq->elemSize);
	newP->next = NULL;

	/* 若链队为空，则新结点即是队首结点又是队尾结点 */
	if(hq->rear == NULL)
	{
		hq->front = hq->rear = newP;
	}else
	{    /* 若链队非空，则依次修改队尾结点的指针域和队尾指针，使之指向新的队尾结点 */
		hq->rear->next = newP; 
		hq->rear = newP;
	}

	hq->len++;
	return true;
}

/* 3.从队列中删除一个元素 */
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
	hq->front = p->next;        /* 使队首指针指向下一个结点 */

	if(hq->front == NULL){
		hq->rear = NULL;
		hq->len = 0;
	}
	
	_QuequFreeNode(p); /* 回收原队首结点 */

	if(hq->len >0 )
	{
		hq->len--;
	}
	return true;    /* 返回被删除的队首元素值 */
}


/* 5.检查链队是否为空，若为空则返回1, 否则返回0 */
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
 
/* 6.清除链队中的所有元素 */
void QueueClear(struct queueLK *hq)
{
	struct sNode *p = hq->front;      
	/* 依次删除队列中的每一个结点，最后使队首指针为空 */
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
