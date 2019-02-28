#ifndef _MEM_H_
#define _MEM_H_

#include "types.h"

void rt_system_heap_init(void *begin_addr, void *end_addr);

void *rt_malloc(uint16 nbytes);
void rt_free(void *ptr);

#endif

