#include "3g_log.h"

void mem_printf(const char* p_title, const char* p_mem, int size)
{
    int i = 0;

    if (!size) {
        return;
    }

    if ((int)p_mem < 0x20000000) {
        ERROR_LOG("0x%x is not memory addr.\n", p_mem);
        return;
    }

    if (p_title) {
        rt_kprintf("%s", p_title);
    }

    for (; i < size; i++) {
        rt_kprintf("%x ", p_mem[i]);
    }

    rt_kprintf("\n", p_title);
}

