#include <jendefs.h>

//convert 2 once
uint8 StringUtil_BCD2uint8(uint8 u8BCD)
{
    uint8 rval=0;
    uint8 tmp;

    rval = tmp = u8BCD & 0xF;
    if (tmp >= 10) return 0xFF;

    tmp = (u8BCD>>4) & 0xF;
    if (tmp >= 10) return 0xFF;

    rval += tmp*10;

    return rval;
}


uint32 StringUtil_BCD2uint32(uint32 u32BCD)
{
    uint32 rval=0, index=1;
    uint8 tmp, i;

    for(i=0; i < 4; i++)
    {
        tmp = StringUtil_BCD2uint8((uint8)((u32BCD >> (i*8))&0xFF));
        if(tmp == 0xFF) return 0xFFFFFFFF;
        rval += tmp* index;
        index *= 100;
    }

    return rval;
}

