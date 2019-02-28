
#include <jendefs.h>
#include "mem_util.h"

#ifndef __PACKED 
#define __PACKED __attribute__((__packed__))
#endif

typedef struct sBlock_s
{
    struct sBlock_s* psNextFreeBlock;
    uint16 size;
    bool_t    bFree;
}sBlock;


PRIVATE uint8 au8Heap[MAX_HEAP_SIZE];
PRIVATE uint8* pu8HeapEnd = (uint8*)(&(au8Heap[MAX_HEAP_SIZE-1]));
PRIVATE sBlock sFreeBlockHead;
PRIVATE sBlock* psFreeBlockHead = &sFreeBlockHead;
PRIVATE sBlock* psFreeBlockTail;
PRIVATE uint8 u8HeaderSize = sizeof(sBlock);


PRIVATE sBlock* fetchFreeBlock(uint32 size);

/****************************************************************************
 *
 * NAME: MemUtil_vInit
 *
 * DESCRIPTION:
 * MUST be called before all memUtil functions
 * the MAX_HEAP_SIZE defines the heap size
 *
 ****************************************************************************/
PUBLIC void MemUtil_vInit()
{
    psFreeBlockHead->psNextFreeBlock = (sBlock*)au8Heap;
    psFreeBlockHead->psNextFreeBlock->size = MAX_HEAP_SIZE -u8HeaderSize;
    psFreeBlockHead->psNextFreeBlock->psNextFreeBlock = NULL;
    psFreeBlockHead->psNextFreeBlock->bFree = TRUE;

    psFreeBlockTail = psFreeBlockHead->psNextFreeBlock;
}

/****************************************************************************
 *
 * NAME: MemUtil_pvAlloc
 *
 * DESCRIPTION:
 * alloc memory
 *
 *
 * RETURNS:
 * NULL if fails
 * 
 ****************************************************************************/
PUBLIC void* MemUtil_pvAlloc(uint32 size)
{
    /*align to 4 bytes boundary*/
    if(size & 0x3) size = (size& 0xFFFFFFFC) + 0x4;

    uint8* p =(uint8*)fetchFreeBlock(size);
    if(p) p += u8HeaderSize;
    
    return p;
}


/****************************************************************************
 *
 * NAME: MemUtil_vFree
 *
 * DESCRIPTION:
 * free memory
 *
 * 
 ****************************************************************************/
PUBLIC void MemUtil_vFree(void* p)
{
    sBlock* psBlock;
    if(p)
    {
        psBlock = (sBlock*)((uint8*)p-u8HeaderSize);        

        psBlock->bFree = TRUE;
        psBlock->psNextFreeBlock = NULL;

        psFreeBlockTail->psNextFreeBlock = psBlock;
        psFreeBlockTail = psBlock;
    }
}

/****************************************************************************
 *
 * NAME: MemUtil_vPurge
 *
 * DESCRIPTION:
 * this function can clean the memory fragments
 * if alloc fails, user can call this function and try again
 * 
 ****************************************************************************/
PUBLIC void MemUtil_vPurge()
{
    sBlock* p = (sBlock*) au8Heap;
    sBlock* pNext, *pPrev;
    uint32 size;

    psFreeBlockHead ->psNextFreeBlock = NULL;
    pPrev = psFreeBlockHead;

    while((uint32)p < (uint32)(pu8HeapEnd))
    {
        if(!p->bFree) 
        {
            p = (sBlock*)((uint8*)p + p->size + u8HeaderSize);
            continue;
        }

        pNext = (sBlock*)((uint8*)p + p->size + u8HeaderSize);
        size = p->size;
        while((uint32)pNext < (uint32)(pu8HeapEnd))
        {
            if(pNext->bFree)
            {
                size += pNext->size + u8HeaderSize;           
                pNext = (sBlock*)((uint8*)pNext + pNext->size + u8HeaderSize);
            }
            else break;
        }

        p->size = size;
        p->psNextFreeBlock = NULL;
        
        pPrev->psNextFreeBlock = p;
        pPrev = p;
        
        if((uint32)pNext < (uint32)(pu8HeapEnd)) 
        {
            p = (sBlock*)((uint8*)pNext + pNext->size + u8HeaderSize);
        }
        else
        {
            break;
        }

    }

    psFreeBlockTail = pPrev;
    
}

PRIVATE sBlock* fetchFreeBlock(uint32 size)
{
    sBlock* p = psFreeBlockHead->psNextFreeBlock;
    sBlock* prev = psFreeBlockHead;
    while(p)
    {
        if(p->size >= size) break;
        prev = p;
        p = p->psNextFreeBlock;        
    }

    if(p == NULL) return NULL;
    
    //memory left is not big enough for split
    if(p->size - size <= u8HeaderSize)
    {
        prev->psNextFreeBlock = p->psNextFreeBlock;
        if(prev->psNextFreeBlock == NULL) psFreeBlockTail = prev;
    }
    else //split the memory and create a new block
    {
        prev->psNextFreeBlock = (sBlock*)((uint8*)p+size+u8HeaderSize);
        prev->psNextFreeBlock->psNextFreeBlock = p->psNextFreeBlock;
        prev->psNextFreeBlock->size = p->size - size - u8HeaderSize;
        prev->psNextFreeBlock->bFree = TRUE;
        p->size = size;

        if(prev->psNextFreeBlock->psNextFreeBlock == NULL) psFreeBlockTail = prev->psNextFreeBlock;
    }

    p->psNextFreeBlock = NULL;
    p->bFree = FALSE;
    return p;
}


