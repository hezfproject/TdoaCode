/***********************************************
    Filename:      DQUE.c 
    Revised:        $Date: 2009/06/02 00:32:57 $
    Revision:       $Revision: 1.2 $

    Description:    This file contains the interface of Dqueue.
************************************************/
/*********************************************************************
 **********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "DQUE.h"
#include "OSAL.h"


/*********************************************************************
 * @fn      Dque_Init
 *
 * @brief   Init the queue.
 *
 * @param  queue - the queue to be initialized.
 *		   bufsize - capability of the queue.
 *		   pBuf - the space be prepared to hold data.
 *		   itemSize - the element size of a queue item.
 *
 * @return  status

 *********************************************************************/
uint8 Dque_Init(DQUE queue, int16 bufsize, uint8* pBuf, int16 itemSize)
{
	if (!queue || !bufsize || ! pBuf || !itemSize || bufsize % itemSize)
		return DQUE_INVALID_DATA;
	
	queue->pBuffer = pBuf;
	queue->dataHead = queue->dataTail = queue->pBuffer;
	queue->maxBufSize = bufsize;
	queue->itemSize = itemSize;
	queue->dataSize = 0;
	return DQUE_SUCCESS;
}

/*********************************************************************
 * @fn      Dque_AddItemToTail
 *
 * @brief   Add an item to the tail of the queue.
 *
 * @param  queue - the queue to be operated.
 *		   pData - the input buffer hold data.
 *
 * @return  status

 *********************************************************************/
ROOTSEG uint8 Dque_AddItemToTail(DQUE queue, uint8* pData)
{
	if (!pData 
	|| queue->dataSize >= queue->maxBufSize)
		return DQUE_INVALID_DATA;

	uint8 *pbuf = queue->dataTail;
	osal_memcpy(pbuf , pData, queue->itemSize);
	queue->dataSize += queue->itemSize;
	queue->dataTail += queue->itemSize;
	
	if (queue->dataTail >= queue->pBuffer + queue->maxBufSize)
		queue->dataTail = queue->pBuffer;
	return DQUE_SUCCESS;
}

/*********************************************************************
 * @fn      Dque_AddItemToTail
 *
 * @brief   Add a piece of data to the tail of the queue, may be one item or several items.
 *
 * @param  queue - the queue to be operated.
 *		   pData - the input buffer hold data.
 *		   datasize - the input data size.
 *
 * @return  status

 *********************************************************************/
ROOTSEG uint8 Dque_AddDataToTail(DQUE queue, uint8* pData, int16 datasize)
{
	if (!pData 
	|| datasize <= 0
	|| datasize % queue->itemSize
	|| queue->dataSize >= queue->maxBufSize)
		return DQUE_INVALID_DATA;
	
	if (datasize + queue->dataSize > queue->maxBufSize)
		datasize = queue->maxBufSize - queue->dataSize;//cut the data.

	if (queue->dataTail + datasize <= queue->pBuffer + queue->maxBufSize)
	{
		osal_memcpy(queue->dataTail, pData, datasize);
		queue->dataTail += datasize;
	}
	else
	{
		int16 firstlen = (int16)(queue->pBuffer + queue->maxBufSize - queue->dataTail);
		int16 lastlen = datasize - firstlen;
		osal_memcpy(queue->dataTail, pData, firstlen);
		osal_memcpy(queue->pBuffer, pData+firstlen, lastlen);
		queue->dataTail = queue->pBuffer + lastlen;
	}

	queue->dataSize += datasize;
	if (queue->dataTail >= queue->pBuffer + queue->maxBufSize)
		queue->dataTail = queue->pBuffer;
	
	return DQUE_SUCCESS;
}

/*********************************************************************
 * @fn      Dque_GetDataFromHead
 *
 * @brief   Fetch a piece of data from the head of the queue, may be one item or several.
 *
 * @param  queue - the queue to be operated.
 *		   pData - the output buffer to hold data.
 *		   datasize - the output data size.
 *
 * @return  status

 *********************************************************************/
ROOTSEG uint8 Dque_GetDataFromHead(DQUE queue, uint8* pData, int16 datasize)
{
	if (!pData
		|| datasize <= 0
		|| queue->dataSize <= 0
		||datasize % queue->itemSize)
		return DQUE_INVALID_LEN;

	if (datasize > queue->dataSize)
		datasize = queue->dataSize; //cut the request length.
	
	if (queue->dataHead + datasize <= queue->pBuffer + queue->maxBufSize)
	{
		osal_memcpy(pData, queue->dataHead, datasize);
		queue->dataHead += datasize;
	}
	else
	{
		int16 firstlen = (int16)(queue->pBuffer + datasize - queue->dataHead);
		int16 lastlen = datasize - firstlen;
		osal_memcpy(pData, queue->dataHead, firstlen);
		osal_memcpy(pData + firstlen, queue->pBuffer, lastlen);
		queue->dataHead = queue->pBuffer + lastlen;
	}

	queue->dataSize -= datasize;
	if (queue->dataHead >= queue->pBuffer + queue->maxBufSize)
		queue->dataHead = queue->pBuffer;
        
        return DQUE_SUCCESS;
}

/*********************************************************************
 * @fn      Dque_GetItemFromHead
 *
 * @brief   Add an item to the tail of the queue.
 *
 * @param  queue - the queue to be operated.
 *		   pData - the output buffer to hold data.
 *
 * @return  status

 *********************************************************************/
ROOTSEG uint8 Dque_GetItemFromHead(DQUE queue, uint8* pData)
{
	if (!pData
	|| queue->dataSize <= 0)
		return DQUE_INVALID_DATA;

	osal_memcpy(pData, queue->dataHead, queue->itemSize);
	
	queue->dataSize -= queue->itemSize;
	if (queue->dataSize <= 0)
		queue->dataHead = queue->dataTail = queue->pBuffer;
	else
	{
		queue->dataHead += queue->itemSize;
		if (queue->dataHead >= queue->pBuffer + queue->maxBufSize)
			queue->dataHead = queue->pBuffer;
	}

	return DQUE_SUCCESS;
}

/*********************************************************************
 * @fn      Dque_IsFull
 *
 * @brief   queue is full or not.
 *
 * @param  queue - the queue to be operated.
 *
 * @return  status

 *********************************************************************/
ROOTSEG bool Dque_IsFull(DQUE queue)
{
	return (queue->dataSize >= queue->maxBufSize);
}

/*********************************************************************
 * @fn      Dque_IsEmpty
 *
 * @brief   queue is empty or not.
 *
 * @param  queue - the queue to be operated.
 *
 * @return  status

 *********************************************************************/
ROOTSEG bool Dque_IsEmpty(DQUE queue)
{
	return (queue->dataSize <= 0);
}

/*********************************************************************
 * @fn      Dque_GetDataSize
 *
 * @brief   get the data size of the queue.
 *
 * @param  queue - the queue to be operated.
 *
 * @return  data size in queue.

 *********************************************************************/
ROOTSEG int16 Dque_GetDataSize(DQUE queue)
{
	return (queue->dataSize);
}

