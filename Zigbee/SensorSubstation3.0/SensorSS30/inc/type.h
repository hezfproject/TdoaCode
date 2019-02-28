#ifndef _TYPE_H_
#define _TYPE_H_

typedef signed   char   INT8;
typedef unsigned char   UINT8;

typedef signed   short  INT16;
typedef unsigned short  UINT16;

typedef signed   long   INT32;
typedef unsigned long   UINT32;

typedef void            VOID;

typedef enum{
    false,
    true
}BOOL;

typedef struct spi_data
{
    INT32   s32Type;
    INT32   s32Value;
    UINT32  u32SeqNum;
}SPI_DATA;

#endif