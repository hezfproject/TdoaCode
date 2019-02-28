#ifndef _MEM_H_
#define _MEM_H_

#include "cc_def.h"
#include "CommonTypes.h"

void rt_system_heap_init(void *begin_addr, void *end_addr);

void *rt_malloc(uint_32 nbytes);
void rt_free(void *ptr);

#endif

