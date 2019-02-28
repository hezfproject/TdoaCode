#include "voicedqueue.h"
#include "osal_memory.h"
#include "string.h"

bool voicedqueue_init(voice_dqueue_ctrl_t *h, uint8 maxitem, uint8 itemsize)
{
	if(h == NULL)
	{
		return false;
	}
	
	h->pbuf = (uint8 *)osal_mem_alloc(maxitem*itemsize + 10); // guard 
	if(h->pbuf == NULL)
	{
		return false;
	}
	h->itemsize = itemsize;
	h->maxitem = maxitem;
	h->idx = 0;
	return true;
}
bool voicedqueue_destory( voice_dqueue_ctrl_t *h)
{
	if(h== NULL)
	{
		return false;
	}
	osal_mem_free(h->pbuf);
	
	h->pbuf =NULL;
	h->itemsize = 0;
	h->maxitem = 0;
	h->idx = 0;
	return true;
}

bool  voicedqueue_additem(voice_dqueue_ctrl_t *h, uint8 *p)
{
	if(h==NULL || p==NULL || h->idx >= h->maxitem)
	{
		return false;
	}
	memcpy(h->pbuf + h->idx*h->itemsize, p, h->itemsize);
	h->idx++;

	return true;
}
bool  voicedqueue_getitem(voice_dqueue_ctrl_t *h, uint8 *p)
{
	if(h==NULL || p==NULL || h->idx == 0)
	{
		return false;
	}

	//memcpy(p, h->pbuf + h->idx*h->itemsize, h->itemsize);
       memmove(p, h->pbuf, h->itemsize);

       uint8* p1 = h->pbuf;
       uint8* p2 = h->pbuf + h->itemsize;
       uint16 size = (h->idx - 1)*h->itemsize;
       
       memmove(p1, p2, size);
       h->idx--;
	return true;
}

 bool   voicedqueue_adddata(voice_dqueue_ctrl_t *h, uint8 *p, uint8 len)
{
	if(h==NULL || p==NULL || h->idx >= h->maxitem)
	{
		return false;
	}

	uint8 maxinItem= h->maxitem - h->idx;
	uint8 inItemLen = len/h->itemsize;

	for(uint8 i=0; i< MIN(maxinItem, inItemLen);i++)
	{
		voicedqueue_additem(h, p + i*h->itemsize);
	}

	return true;
}

 
 bool   voicedqueue_flush(voice_dqueue_ctrl_t *h)
{
	if(h==NULL)
	{
		return false;
	}
        h->idx = 0;
        return true;
}

/* get all datas in buff */
uint16   voicedqueue_getalldata(voice_dqueue_ctrl_t *h, uint8 *p)
{
	if(h==NULL || p==NULL || h->idx == 0)
	{
		return 0;
	}
	
	uint16 size =  h->idx * h->itemsize;
	memcpy(p, h->pbuf, size);
	h->idx = 0;
	return size;
}

uint16  voicedqueue_getsize (const voice_dqueue_ctrl_t *h)
{
	return ((uint16)h->idx * (uint16)h->itemsize);
}

uint16  voicedqueue_getmaxsize (const voice_dqueue_ctrl_t *h)
{
	return ((uint16)h->maxitem* (uint16)h->itemsize);
}

bool  voicedqueue_isfull (const voice_dqueue_ctrl_t *h)
{
	return h->idx>=h->maxitem ? true:false; 
}
bool  voicedqueue_isempty (const voice_dqueue_ctrl_t *h)
{
	return h->idx ==0 ? true:false;
}


