#ifndef MEM_UTIL_H
#define MEM_UTIL_H

#ifndef MAX_HEAP_SIZE
#define MAX_HEAP_SIZE 4096
#endif


PUBLIC void MemUtil_vInit();
PUBLIC void* MemUtil_pvAlloc(uint32 size);
PUBLIC void MemUtil_vFree(void* p);
PUBLIC void MemUtil_vPurge();

#endif


