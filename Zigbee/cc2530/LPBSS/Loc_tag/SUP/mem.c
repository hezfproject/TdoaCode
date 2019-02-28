/*
 * File      : mem.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2008 - 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2008-7-12      Bernard      the first version
 * 2010-06-09     Bernard      fix the end stub of heap
 *                             fix memory check in rt_realloc function
 * 2010-07-13     Bernard      fix RT_ALIGN issue found by kuronca
 * 2010-10-14     Bernard      fix rt_realloc issue when realloc a NULL pointer.
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *         Simon Goldschmidt
 *
 */

#include "bsp.h"
#include "track.h"
#include "semaphore.h"
#include <string.h>
/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define RT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def RT_NULL
 * Similar as the \c NULL in C library.
 */
#define RT_NULL                         (NULL)

#define RT_ALIGN_SIZE                   1

/* #define RT_MEM_DEBUG */
#define RT_MEM_STATS

#define HEAP_MAGIC 0x1ea0
struct heap_mem
{
    /* magic and used flag */
    uint16 magic;
    uint16 used;

    uint16 next, prev;
};

/** pointer to the heap: for alignment, heap_ptr is now a pointer instead of an array */
static uint8 *heap_ptr;

/** the last entry, always unused! */
static struct heap_mem *heap_end;

#define MIN_SIZE 12
#define MIN_SIZE_ALIGNED     RT_ALIGN(MIN_SIZE, RT_ALIGN_SIZE)
#define SIZEOF_STRUCT_MEM    RT_ALIGN(sizeof(struct heap_mem), RT_ALIGN_SIZE)

#define RT_ASSERT(EX)   if (!(EX)) {volatile char dummy = 0;\
                            ERR_DBG("(%s) assert failed at %s:%d \n", \
                            #EX, __FUNCTION__, __LINE__); while (dummy == 0);}


static struct heap_mem *lfree;   /* pointer to the lowest free block */

static uint8 heap_sem;
static uint16 mem_size_aligned;

#ifdef RT_MEM_STATS
static uint16 used_mem, max_mem;
#endif

static void plug_holes(struct heap_mem *mem)
{
    struct heap_mem *nmem;
    struct heap_mem *pmem;

    RT_ASSERT((uint8 *)mem >= heap_ptr);
    RT_ASSERT((uint8 *)mem < (uint8 *)heap_end);
    RT_ASSERT(mem->used == 0);

    /* plug hole forward */
    nmem = (struct heap_mem *)&heap_ptr[mem->next];
    if (mem != nmem && nmem->used == 0 && (uint8 *)nmem != (uint8 *)heap_end)
    {
        /* if mem->next is unused and not end of heap_ptr, combine mem and mem->next */
        if (lfree == nmem)
        {
            lfree = mem;
        }
        mem->next = nmem->next;
        ((struct heap_mem *)&heap_ptr[nmem->next])->prev = (uint8 *)mem - heap_ptr;
    }

    /* plug hole backward */
    pmem = (struct heap_mem *)&heap_ptr[mem->prev];
    if (pmem != mem && pmem->used == 0)
    {
        /* if mem->prev is unused, combine mem and mem->prev */
        if (lfree == mem)
        {
            lfree = pmem;
        }
        pmem->next = mem->next;
        ((struct heap_mem *)&heap_ptr[mem->next])->prev = (uint8 *)pmem - heap_ptr;
    }
}

/**
 * @ingroup SystemInit
 *
 * This function will init system heap
 *
 * @param begin_addr the beginning address of system page
 * @param end_addr the end address of system page
 */
void rt_system_heap_init(void *begin_addr, void *end_addr)
{
    struct heap_mem *mem;
    uint16 begin_align = RT_ALIGN((uint16)begin_addr, RT_ALIGN_SIZE);
    uint16 end_align = RT_ALIGN_DOWN((uint16)end_addr, RT_ALIGN_SIZE);

    /* alignment addr */
    if ((end_align > (2 * SIZEOF_STRUCT_MEM)) &&
            ((end_align - 2 * SIZEOF_STRUCT_MEM) >= begin_align))
    {
        /* calculate the aligned memory size */
        mem_size_aligned = end_align - begin_align - 2 * SIZEOF_STRUCT_MEM;
    }
    else
    {
        ERR_DBG("mem init, error begin address 0x%x, and end address 0x%x\n", (uint16)begin_addr, (uint16)end_addr);

        return;
    }

    /* point to begin address of heap */
    heap_ptr = (uint8 *)begin_align;

    ERR_DBG("mem init, heap begin address 0x%x, size %d\n", (uint16)heap_ptr, mem_size_aligned);

    /* initialize the start of the heap */
    mem = (struct heap_mem *)heap_ptr;
    mem->magic = HEAP_MAGIC;
    mem->next = mem_size_aligned + SIZEOF_STRUCT_MEM;
    mem->prev = 0;
    mem->used = 0;

    /* initialize the end of the heap */
    heap_end = (struct heap_mem *)&heap_ptr[mem->next];
    heap_end->magic = HEAP_MAGIC;
    heap_end->used = 1;
    heap_end->next = mem_size_aligned + SIZEOF_STRUCT_MEM;
    heap_end->prev = mem_size_aligned + SIZEOF_STRUCT_MEM;

    /* initialize the lowest-free pointer to the start of the heap */
    lfree = (struct heap_mem *)heap_ptr;

    _sem_release(heap_sem);
}

/**
 * @addtogroup MM
 */

/*@{*/

/**
 * Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param size is the minimum size of the requested block in bytes.
 *
 * @return pointer to allocated memory or NULL if no free memory was found.
 */
void *rt_malloc(uint16 size)
{
    uint16 ptr, ptr2;
    struct heap_mem *mem, *mem2;

    if (size == 0)
        return RT_NULL;

    if (size != RT_ALIGN(size, RT_ALIGN_SIZE))
        ERR_DBG("malloc size %d, but align to %d\n", size, RT_ALIGN(size, RT_ALIGN_SIZE));
    else
        ERR_DBG("malloc size %d\n", size);

    /* alignment size */
    size = RT_ALIGN(size, RT_ALIGN_SIZE);

    if (size > mem_size_aligned)
    {
        ERR_DBG("no memory\n");

        return RT_NULL;
    }

    /* every data block must be at least MIN_SIZE_ALIGNED long */
    if (size < MIN_SIZE_ALIGNED)
        size = MIN_SIZE_ALIGNED;

    _sem_take_retv(heap_sem, RT_NULL);

    for (ptr = (uint8 *)lfree - heap_ptr; ptr < mem_size_aligned - size;
            ptr = ((struct heap_mem *)&heap_ptr[ptr])->next)
    {
        mem = (struct heap_mem *)&heap_ptr[ptr];

        if ((!mem->used) && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size)
        {
            /* mem is not used and at least perfect fit is possible:
             * mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem */

            if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED))
            {
                /* (in addition to the above, we test if another struct heap_mem (SIZEOF_STRUCT_MEM) containing
                 * at least MIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'mem')
                 * -> split large block, create empty remainder,
                 * remainder must be large enough to contain MIN_SIZE_ALIGNED data: if
                 * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
                 * struct heap_mem would fit in but no data between mem2 and mem2->next
                 * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
                 *       region that couldn't hold data, but when mem->next gets freed,
                 *       the 2 regions would be combined, resulting in more free memory
                 */
                ptr2 = ptr + SIZEOF_STRUCT_MEM + size;

                /* create mem2 struct */
                mem2 = (struct heap_mem *)&heap_ptr[ptr2];
                mem2->used = 0;
                mem2->next = mem->next;
                mem2->prev = ptr;

                /* and insert it between mem and mem->next */
                mem->next = ptr2;
                mem->used = 1;

                if (mem2->next != mem_size_aligned + SIZEOF_STRUCT_MEM)
                {
                    ((struct heap_mem *)&heap_ptr[mem2->next])->prev = ptr2;
                }
#ifdef RT_MEM_STATS
                used_mem += (size + SIZEOF_STRUCT_MEM);
                if (max_mem < used_mem)
                    max_mem = used_mem;
#endif
            }
            else
            {
                /* (a mem2 struct does no fit into the user data space of mem and mem->next will always
                 * be used at this point: if not we have 2 unused structs in a row, plug_holes should have
                 * take care of this).
                 * -> near fit or excact fit: do not split, no mem2 creation
                 * also can't move mem->next directly behind mem, since mem->next
                 * will always be used at this point!
                 */
                mem->used = 1;
#ifdef RT_MEM_STATS
                used_mem += mem->next - ((uint8 *)mem - heap_ptr);
                if (max_mem < used_mem)
                    max_mem = used_mem;
#endif
            }
            /* set memory block magic */
            mem->magic = HEAP_MAGIC;

            if (mem == lfree)
            {
                /* Find next free block after mem and update lowest free pointer */
                while (lfree->used && lfree != heap_end)
                    lfree = (struct heap_mem *)&heap_ptr[lfree->next];

                RT_ASSERT(((lfree == heap_end) || (!lfree->used)));
            }

            _sem_release(heap_sem);
            RT_ASSERT((uint16)mem + SIZEOF_STRUCT_MEM + size <= (uint16)heap_end);
            RT_ASSERT((uint16)((uint8 *)mem + SIZEOF_STRUCT_MEM) % RT_ALIGN_SIZE == 0);
            RT_ASSERT((((uint16)mem) & (RT_ALIGN_SIZE - 1)) == 0);

            ERR_DBG("allocate memory at 0x%x, size: %d\n",
                    (uint16)((uint8 *)mem + SIZEOF_STRUCT_MEM),
                    (uint16)(mem->next - ((uint8 *)mem - heap_ptr)));

            /* return the memory data except mem struct */
            return (uint8 *)mem + SIZEOF_STRUCT_MEM;
        }
    }

    _sem_release(heap_sem);

    return RT_NULL;
}

/**
 * This function will release the previously allocated memory block by rt_malloc.
 * The released memory block is taken back to system heap.
 *
 * @param rmem the address of memory which will be released
 */
void rt_free(void *rmem)
{
    struct heap_mem *mem;

    if (rmem == RT_NULL)
        return;
    RT_ASSERT((((uint16)rmem) & (RT_ALIGN_SIZE - 1)) == 0);
    RT_ASSERT((uint8 *)rmem >= (uint8 *)heap_ptr &&
              (uint8 *)rmem < (uint8 *)heap_end);

    if ((uint8 *)rmem < (uint8 *)heap_ptr || (uint8 *)rmem >= (uint8 *)heap_end)
    {
        ERR_DBG("illegal memory\n");

        return;
    }

    /* Get the corresponding struct heap_mem ... */
    mem = (struct heap_mem *)((uint8 *)rmem - SIZEOF_STRUCT_MEM);

    ERR_DBG("release memory 0x%x, size: %d\n",
            (uint16)rmem,
            (uint16)(mem->next - ((uint8 *)mem - heap_ptr)));

    _sem_take_ret(heap_sem);
    /* ... which has to be in a used state ... */
    RT_ASSERT(mem->used);
    RT_ASSERT(mem->magic == HEAP_MAGIC);
    /* ... and is now unused. */
    mem->used = 0;
    mem->magic = 0;

    if (mem < lfree)
    {
        /* the newly freed struct is now the lowest */
        lfree = mem;
    }

#ifdef RT_MEM_STATS
    used_mem -= (mem->next - ((uint8 *)mem - heap_ptr));
#endif

    /* finally, see if prev or next are free also */
    plug_holes(mem);
    _sem_release(heap_sem);
}

#ifdef RT_MEM_STATS
void rt_memory_info(uint16 *total, uint16 *used, uint16 *max_used)
{
    if (total != RT_NULL) *total = mem_size_aligned;
    if (used  != RT_NULL) *used = used_mem;
    if (max_used != RT_NULL) *max_used = max_mem;
}
#endif

/*@}*/

