#ifndef _QUEUE_H_
#define _QUEUE_H_
	
struct sNode{
    void *pElem;          	       /* 值域 */
    struct sNode *next;        /* 链接指针 */
};
typedef struct queueLK{
    struct sNode *front;    /* 队首指针 */
    struct sNode *rear;        /* 队尾指针 */
    uint8 		     elemSize;
    uint8		     maxLen;
    uint8   	     len; 
}queueLK_t;


#define QueueInit(hp,type,maxLen)  _QueueInit(hp,sizeof(type),maxLen)
bool  QueueInput(struct queueLK *hq, const void* pInElem);
bool  QueueOutput(struct queueLK *hq,  void* pOutElem);
bool  QueueIsempty(struct queueLK *hq);
 bool QueueIsFull(struct queueLK * hq);
void QueueClear(struct queueLK *hq);
uint8 QueueGetLen(struct queueLK *hq);


void _QueueInit(struct queueLK *hq, uint8 elemSize, uint8 maxLen);
#endif
